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
#include <repo/manipulator/modelconvertor/import/repo_model_import_oda.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_manager.h>
#include <repo_log.h>
#include <repo/error_codes.h>
#include "../../../../../repo_test_utils.h"
#include "../../../../../repo_test_database_info.h"
#include "../../../../../repo_test_scene_utils.h"

using namespace repo::manipulator::modelconvertor;
using namespace repo::core::model;
using namespace testing;

#define TESTDB "ODAModelImportTest"

#pragma optimize("", off)

namespace ODAModelImportUtils
{
	repo::core::model::RepoScene* ModelImportManagerImport(std::string collection, std::string filename)
	{
		ModelImportConfig config(
			true,
			ModelUnits::MILLIMETRES,
			"",
			0,
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			collection);

		auto handler = getHandler();

		uint8_t err;
		std::string msg;

		ModelImportManager manager;
		auto scene = manager.ImportFromFile(filename, config, handler, err);
		scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());
		scene->loadScene(handler.get(), msg);

		return scene;
	}
}

TEST(ODAModelImport, Sample2025NWDTree)
{
	auto scene = ODAModelImportUtils::ModelImportManagerImport("Sample2025NWD", getDataPath(nwdModel2025));

	// For NWDs, Layers/Levels & Collections always end up as branch nodes. Composite
	// Objects and Groups are leaf nodes if they contain only Geometric Items &
	// Instances as children.

	// See this page for the meaning of the icons in Navis: 
	// https://help.autodesk.com/view/NAV/2024/ENU/?guid=GUID-BC657B3A-5104-45B7-93A9-C6F4A10ED0D4,
	// but keep in mind the terminology on that page doesn't match exactly ODAs.

	// This snippet tests whether geometry is grouped successfully

	SceneUtils utils(scene);

	auto nodes = utils.findNodesByMetadata("Element::Id", "309347");
	EXPECT_THAT(nodes.size(), Eq(1));
	EXPECT_THAT(nodes[0].isLeaf(), IsTrue());
	EXPECT_THAT(nodes[0].hasGeometry(), IsTrue());
	EXPECT_THAT(nodes[0].getPath(), Eq("rootNode->sample2025.nwd->Level 0->Planting->Planting_RPC_Tree_Deciduous->Hawthorn-7.4_Meters->Planting_RPC_Tree_Deciduous"));

	nodes = utils.findTransformationNodesByName("Wall-Ext_102Bwk-75Ins-100LBlk-12P");
	EXPECT_THAT(nodes.size(), Eq(1));

	auto children = nodes[0].getChildren();
	EXPECT_THAT(children.size(), Eq(6));

	for (auto n : children) {
		EXPECT_THAT(n.isLeaf(), IsTrue());
		EXPECT_THAT(n.hasGeometry(), IsTrue());
		EXPECT_THAT(n.name(), Eq("Basic Wall"));
	}

	// These two nodes correspond to hidden items. In Navis, hidden elements should
	// be imported unaffected.

	EXPECT_THAT(utils.findNodesByMetadata("Element::Id", "694").size(), Gt(0));
	EXPECT_THAT(utils.findNodesByMetadata("Element::Id", "311").size(), Gt(0));
}

TEST(ODAModelImport, ColouredBoxesDWG)
{
	auto scene = ODAModelImportUtils::ModelImportManagerImport("ColouredBoxesDWG", getDataPath("colouredBoxes.dwg"));
	SceneUtils utils(scene);

	// The coloured boxes file contains 3d geometry, with subobject colours assigned
	// in different ways.

	// These two elements are set to byLayer, and the layers have two different
	// colours

	auto byLayer1 = utils.findNodeByMetadata("Entity Handle::Value", "[6FC5]");
	EXPECT_THAT(byLayer1.getColours(), ElementsAre(repo::lib::repo_color4d_t(1, 1, 1, 1)));

	auto byLayer2 = utils.findNodeByMetadata("Entity Handle::Value", "[6FC9]");
	EXPECT_THAT(byLayer2.getColours(), ElementsAre(repo::lib::repo_color4d_t(1, 0, 0, 1)));

	// This element has a fixed colour

	auto cyan = utils.findNodeByMetadata("Entity Handle::Value", "[702C]");
	EXPECT_THAT(cyan.getColours(), ElementsAre(repo::lib::repo_color4d_t(0, 1, 1, 1)));

	// This is a 3d solid that has one face with a fixed colour and the others by
	// layer

	auto mixed = utils.findNodeByMetadata("Entity Handle::Value", "[6FCD]");
	EXPECT_THAT(mixed.getColours(), UnorderedElementsAre(
		repo::lib::repo_color4d_t(1, 1, 1, 1),
		repo::lib::repo_color4d_t(0, 1, 0, 1)
	));

	// This block reference contains the above solid, with a face assiged to a
	// specific colour, and a face assigned to the block colour. The remaining
	// faces are per-layer, but the layer has changed again.

	auto block = utils.findNodeByMetadata("Entity Handle::Value", "[7063]");
	EXPECT_THAT(block.getColours(), UnorderedElementsAre(
		repo::lib::repo_color4d_t(1, 0, 1, 1),
		repo::lib::repo_color4d_t(0, 1, 0, 1),
		repo::lib::repo_color4d_t(1, 0, 0, 1),
		repo::lib::repo_color4d_t(1, 0, 0, 1)
	));
}

MATCHER_P(Paths, matcher, "") {
	std::vector<std::string> paths;
	for (auto& n : arg) {
		paths.push_back(n.getPath());
	}
	return ExplainMatchResult(matcher, paths, result_listener);
}

TEST(ODAModelImport, NestedBlocksDWG)
{
	auto scene = ODAModelImportUtils::ModelImportManagerImport("NestedBlocksDWG", getDataPath("nestedBlocks.dwg"));
	SceneUtils utils(scene);

	// This dwg contains a number of nested blocks. The DWG importer should put
	// block items under the block reference for the given layer (by block, or
	// explicit) they items are in.

	// The outer most block touches three layers: it itself is on layer 0, it contains a
	// nested block reference on layer 2, and an entity on layer 3. The nested reference
	// itself explicitly has an item on layer 1 (the other layer 2 references show on 
	// layer 0, by convention as only the first block is considered).

	EXPECT_THAT(utils.findNodesByMetadata("Entity Handle::Value", "[4FA]"), Paths(UnorderedElementsAre(
		"rootNode->0->My Outer Block",
		"rootNode->Layer1->My Outer Block",
		"rootNode->Layer3->My Outer Block"
	)));

	EXPECT_THAT(utils.findNodesByMetadata("Entity Handle::Value", "[50D]"), Paths(UnorderedElementsAre(
		"rootNode->0->My Block", 
		"rootNode->Layer1->My Block"
	)));

	EXPECT_THAT(utils.findNodesByMetadata("Entity Handle::Value", "[423]"), Paths(UnorderedElementsAre(
		"rootNode->0->Block Text"
	)));

	// Even though this handle exists, it should be compressed in the tree.
	EXPECT_THAT(utils.findNodesByMetadata("Entity Handle::Value", "[534]"), IsEmpty());

	EXPECT_THAT(utils.getRootNode().getChildNames(), UnorderedElementsAre("0", "Layer1", "Layer3"));
}

TEST(ODAModelImport, RevitMEPSystems)
{
	auto scene = ODAModelImportUtils::ModelImportManagerImport("RevitMeta3", getDataPath(rvtMeta3));
	SceneUtils utils(scene);

	// In rvtMeta3, some of these elements belong to systems, and others do not.
	// They should all exist as tree leaf nodes however, with geometry.

	std::vector<std::string> elementsWithMetadata = {
		"702167",
		"702041",
		"706118",
		"706347",
		"703971",
		"704116"
	};

	for (auto e : elementsWithMetadata) {
		auto nodes = utils.findNodesByMetadata("Element ID", e);
		EXPECT_THAT(nodes.size(), Eq(1));
		EXPECT_THAT(nodes[0].isLeaf(), IsTrue());
		EXPECT_THAT(nodes[0].hasGeometry(), IsTrue());
	}
}