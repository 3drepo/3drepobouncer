/**
*  Copyright (C) 2025 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include <repo/manipulator/modelconvertor/import/repo_model_import_manager.h>
#include <repo/manipulator/modelutility/repo_scene_manager.h>
#include <repo/manipulator/modelutility/repo_maker_selection_tree.h>

// The ODA NWD importer also uses rapidjson, so make sure to import our
// copy into a different namespace to avoid interference between two
// possibly different versions.

#define RAPIDJSON_NAMESPACE repo::rapidjson
#define RAPIDJSON_NAMESPACE_BEGIN namespace repo { namespace rapidjson {
#define RAPIDJSON_NAMESPACE_END } }

#include <repo/manipulator/modelutility/rapidjson/rapidjson.h>
#include <repo/manipulator/modelutility/rapidjson/document.h>

#include "../../../repo_test_utils.h"
#include "../../../repo_test_mesh_utils.h"
#include "../../../repo_test_database_info.h"
#include "../../../repo_test_fileservice_info.h"
#include "../../../repo_test_scene_utils.h"

using namespace repo::core::model;
using namespace testing;
using namespace repo::manipulator::modelconvertor;
using namespace repo::manipulator::modelutility;
using namespace repo;

#define TESTDB "SelectionTreeTest"

repo::core::model::RepoScene* ModelImportManagerImport(std::string filename, const ModelImportConfig& config)
{
	auto handler = getHandler();

	uint8_t err;
	std::string msg;

	ModelImportManager manager;
	auto scene = manager.ImportFromFile(filename, config, handler, err);
	scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());

	// The Selection Tree Maker no longer needs a populated scene, but SceneUtils
	// still does in order to validate it...
	if (!scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT).size()) {
		scene->loadScene(handler.get(), msg);
	}

	SceneManager manager2;	
	manager2.generateAndCommitSelectionTree(scene, handler.get());

	return scene;
}

/**
* The following types are concerned with checking the full tree response. The
* response hierarchy is checked against the scene collection for accuracy,
* and also for *internal* consistency, by making sure the tree obeys its own
* conventions (such as the dependencies between visibility states).
* 
* Outside the hierarchy, these tests don't check the settings expressed for
* the nodes - these should be checked explicitly against the tree for known
* models in separate tests.
*/

class TreeTestUtilities
{
	// TreeTestUtilities directly parses the Json using rapidjsons document
	// API. The jsons are never meant to be read back into bouncer (outside
	// the tests).

	enum Visibility {
		Visible,
		Invisible,
		Mixed
	};

	Visibility parseToggleState(const char* s) {
		auto v = std::string(s);
		if (v == "visible") {
			return Visibility::Visible;
		}
		else if (v == "invisible") {
			return Visibility::Invisible;
		}
		else if (v == "parentOfInvisible") {
			return Visibility::Mixed;
		}
		throw "Invalid state exception";
	}

	// This snippet serialises the path in the same way as the exporter.

	std::string getPathAsString(const SceneUtils::NodeInfo& node) {
		std::string result;
		auto n = node;
		while (true) {
			result = n.getUniqueId().toString() + result;
			auto p = n.getParents({});
			if (p.size()) {
				n = p[0];
				result = "__" + result;
			}
			else
			{
				break;
			}
		}
		return result;
	}

	struct CheckTreeContext
	{
		repo::lib::RepoUUID parentUniqueId;
	};

	void checkTree(
		const rapidjson::GenericObject<true, rapidjson::Value>& value,
		CheckTreeContext& ctx,
		SceneUtils& scene)
	{
		// (The node must exist - if the node can't be found the following will throw...)

		auto node = scene.findNodeByUniqueId(repo::lib::RepoUUID(std::string(value["_id"].GetString())));

		EXPECT_THAT(value["account"].GetString(), Eq(scene.getTeamspaceName()));
		EXPECT_THAT(value["project"].GetString(), Eq(scene.getContainerName()));
		EXPECT_THAT(value["type"].GetString(), Eq(node.node->getType()));

		if (!node.name().empty()) {
			EXPECT_THAT(value["name"].GetString(), Eq(node.name()));
		}
		else {
			EXPECT_THAT(value.HasMember("name"), IsFalse());
		}

		EXPECT_THAT(value["path"].GetString(), Eq(getPathAsString(node)));

		EXPECT_THAT(value["_id"].GetString(), Eq(node.getUniqueId().toString()));
		EXPECT_THAT(value["shared_id"].GetString(), Eq(node.getSharedId().toString()));

		if (!ctx.parentUniqueId.isDefaultValue()) {
			EXPECT_THAT(ctx.parentUniqueId, Eq(node.getParent().getUniqueId()));
		}

		if (value.HasMember("meta")) {
			const auto& metas = value["meta"];
			EXPECT_THAT(metas.IsArray(), IsTrue());

			std::vector<std::string> expected;
			for (auto& n : node.getMetadataNodes()) {
				expected.push_back(n.getUniqueId().toString());
			}

			std::vector<std::string> actual;
			for (auto& n : metas.GetArray()) {
				actual.push_back(n.GetString());
			}

			EXPECT_THAT(actual, UnorderedElementsAreArray(expected));
		}

		int numVisibleChildren = false;
		int numInvisibleChildren = false;
		int numMixedChildren = false;

		if (value.HasMember("children"))
		{
			const auto& children = value["children"];
			EXPECT_THAT(children.IsArray(), IsTrue());

			CheckTreeContext c;
			c.parentUniqueId = node.getUniqueId();

			for (auto& v : children.GetArray()) {
				const auto& vv = v.GetObject();
				checkTree(vv, c, scene);

				// In the current version of the tree, toggleState must be explicitly set
				switch (parseToggleState(vv["toggleState"].GetString())) {
				case Visibility::Visible:
					numVisibleChildren++;
					break;
				case Visibility::Invisible:
					numInvisibleChildren++;
					break;
				case Visibility::Mixed:
					numMixedChildren++;
					break;
				}
			}
		}

		// toggleState should be visible by default, unless one or more descendents are
		// invisible, in which case the parents *must* be either invisible *only* if
		// all children are invisible, or mixed otherwise.

		auto toggleState = parseToggleState(value["toggleState"].GetString());

		switch (toggleState) {
		case Visibility::Visible:
		{
			EXPECT_THAT(numInvisibleChildren, Eq(0));
			EXPECT_THAT(numMixedChildren, Eq(0));
		}
		break;
		case Visibility::Invisible:
		{
			EXPECT_THAT(numMixedChildren, Eq(0));
			EXPECT_THAT(numVisibleChildren, Eq(0));

			invisibleNodeIds.push_back(repo::lib::RepoUUID(std::string(value["_id"].GetString())));
		}
		break;
		case Visibility::Mixed:
		{
			if (!numMixedChildren) {
				EXPECT_THAT(numInvisibleChildren, Gt(0));
			}
		}
		break;
		}
	}

	void checkIdToName(const rapidjson::GenericObject<true, rapidjson::Value>& value,
		SceneUtils& scene)
	{
		std::unordered_map<std::string, std::string> actual;
		for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr) {
			actual[std::string(itr->name.GetString())] = std::string(itr->value.GetString());
		}

		std::unordered_map<std::string, std::string> expected;
		for (auto& n : scene.getTransformations()) {
			expected[n.getUniqueId().toString()] = n.name();
		}
		for (auto& n : scene.getMeshes()) {
			expected[n.getUniqueId().toString()] = n.name();
		}

		EXPECT_THAT(actual, Eq(expected));
	}

	void checkIdToPath(const rapidjson::GenericObject<true, rapidjson::Value>& value,
		SceneUtils& scene)
	{
		std::unordered_map<std::string, std::string> actual;
		for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr) {
			actual[std::string(itr->name.GetString())] = std::string(itr->value.GetString());
		}

		std::unordered_map<std::string, std::string> expected;
		for (auto& n : scene.getTransformations()) {
			expected[n.getUniqueId().toString()] = getPathAsString(n);
		}
		for (auto& n : scene.getMeshes()) {
			expected[n.getUniqueId().toString()] = getPathAsString(n);
		}

		EXPECT_THAT(actual, Eq(expected));
	}

	void checkIdToMeshes(const rapidjson::GenericObject<true, rapidjson::Value>& value,
		SceneUtils& scene)
	{
		for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr) {
			auto uuid = repo::lib::RepoUUID(std::string(itr->name.GetString()));

			std::vector<repo::lib::RepoUUID> actual;
			for (const auto& m : itr->value.GetArray()) {
				actual.push_back(repo::lib::RepoUUID(std::string(m.GetString())));
			}

			std::vector<repo::lib::RepoUUID> expected;
			auto node = scene.findNodeByUniqueId(uuid);
			for (auto& m : node.getMeshesRecursive()) {
				expected.push_back(m.getUniqueId());
			}

			EXPECT_THAT(actual, UnorderedElementsAreArray(expected));
		}
	}

	void checkHiddenNodes(const rapidjson::GenericArray<true, rapidjson::Value>& value,
		SceneUtils& scene)
	{
		std::vector<repo::lib::RepoUUID> uuids;
		for (auto& id : value) {
			uuids.push_back(std::string(id.GetString()));
		}
		EXPECT_THAT(uuids, UnorderedElementsAreArray(invisibleNodeIds));
	}

	rapidjson::Document getJsonFile(const RepoScene* scene, std::string file)
	{
		auto handler = getHandler();

		auto fullTree = handler->getFileManager()->getFile(
			scene->getDatabaseName(),
			scene->getProjectName() + std::string(".stash.json_mpc"),
			scene->getRevisionID().toString() + std::string("/") + file
		);

		rapidjson::Document document;
		document.Parse(reinterpret_cast<const char*>(fullTree.data()), fullTree.size());
	
		EXPECT_FALSE(document.HasParseError());
		EXPECT_TRUE(document.IsObject());

		return document;
	}

	void checkAllForScene(const RepoScene* scene)
	{
		SceneUtils utils(scene);

		const auto fulltree = getJsonFile(scene, "fulltree.json");

		const auto& nodes = fulltree["nodes"]; // nodes is actually an object - the root of the tree.
		CheckTreeContext ctx;
		checkTree(nodes.GetObject(), ctx, utils);

		const auto& idToName = fulltree["idToName"];
		checkIdToName(idToName.GetObject(), utils);

		const auto treePath = getJsonFile(scene, "tree_path.json");
		const auto& idToPath = treePath["idToPath"];
		checkIdToPath(idToPath.GetObject(), utils);

		const auto idToMeshes = getJsonFile(scene, "idToMeshes.json");
		checkIdToMeshes(idToMeshes.GetObject(), utils);

		// The invisibleNodeIds vector used by checkHiddenNodes will be populaed by
		// the checkTree method above, which must always be called before this one.

		// If there are no invisible nodes, then the modelProperties may not be
		// written at all.

		if (invisibleNodeIds.size()) {
			const auto modelProperties = getJsonFile(scene, "modelProperties.json");
			const auto& hiddenNodes = modelProperties["hiddenNodes"];
			checkHiddenNodes(hiddenNodes.GetArray(), utils);
		}
	}

public:
	static TreeTestUtilities CheckAllForScene(const RepoScene* scene)
	{
		TreeTestUtilities utils;
		utils.checkAllForScene(scene);
		return utils;
	}

	std::vector<repo::lib::RepoUUID> invisibleNodeIds;
};

TEST(RepoSelectionTreeTest, dwgModel)
{
	auto collection = "dwgModel";

	ModelImportConfig config(
		repo::lib::RepoUUID::createUUID(),
		TESTDB,
		collection
	);
	config.targetUnits = ModelUnits::MILLIMETRES;

	auto scene = ModelImportManagerImport(getDataPath(dwgModel), config);

	TreeTestUtilities::CheckAllForScene(scene);
}

TEST(RepoSelectionTreeTest, HiddenIfc)
{
	// This model contains IFC Spaces - i.e. nodes that should be hidden by default

	auto collection = "duplex";

	ModelImportConfig config(
		repo::lib::RepoUUID::createUUID(),
		TESTDB,
		collection
	);
	config.targetUnits = ModelUnits::MILLIMETRES;

	auto scene = ModelImportManagerImport(getDataPath(ifcModel), config);

	auto tree = TreeTestUtilities::CheckAllForScene(scene);

	// Check that the right nodes have been marked as hidden by default, and
	// that these show up in the model settings correctly.

	// (We don't need to check all the parent states or modelProperties, as the
	// consistency of these is checked by TreeTestUtilities)

	std::vector<std::string> names = {
		"A101 (IFC Space)",
		"A102 (IFC Space)",
		"A103 (IFC Space)",
		"A104 (IFC Space)",
		"A105 (IFC Space)",
		"A201 (IFC Space)",
		"A202 (IFC Space)",
		"A203 (IFC Space)",
		"A204 (IFC Space)",
		"A205 (IFC Space)",
		"B101 (IFC Space)",
		"B102 (IFC Space)",
		"B103 (IFC Space)",
		"B104 (IFC Space)",
		"B105 (IFC Space)",
		"B201 (IFC Space)",
		"B202 (IFC Space)",
		"B203 (IFC Space)",
		"B204 (IFC Space)",
		"B205 (IFC Space)",
		"R301 (IFC Space)"
	};
	
	SceneUtils utils(scene);

	EXPECT_THAT(tree.invisibleNodeIds.size(), Eq(names.size()));

	std::vector<std::string> actual;
	for (auto& id : tree.invisibleNodeIds) {
		auto n = utils.findNodeByUniqueId(id);
		actual.push_back(n.name());
	}

	EXPECT_THAT(actual, UnorderedElementsAreArray(names));
}

TEST(RepoSelectionTreeTest, HiddenSynchro)
{
	// This model contains 4D elements, some of which will be hidden by default

	auto collection = "synchro6_3";

	ModelImportConfig config(
		repo::lib::RepoUUID::createUUID(),
		TESTDB,
		collection
	);
	config.targetUnits = ModelUnits::MILLIMETRES;

	auto scene = ModelImportManagerImport(getDataPath("synchro6_3.spm"), config);

	auto tree = TreeTestUtilities::CheckAllForScene(scene);

	EXPECT_THAT(tree.invisibleNodeIds.size(), Eq(52));

	SceneUtils utils(scene);

	std::vector<repo::lib::RepoUUID> expected;
	for (const auto& name : {
			"dzwig4.dwf",
			"Mobile Crane 90T 14m-18T.dwf",
			"Site Cabins Double Stack Group.dwf"
		}) {
		for (auto& m : utils.findTransformationNodeByName(name).getMeshesRecursive()) {
			expected.push_back(m.getUniqueId());
		}
	}

	EXPECT_THAT(tree.invisibleNodeIds, UnorderedElementsAreArray(expected));
}

// Todo..

// 1. Check naming of reference nodes
// 2. Check ordering of IFC spaces