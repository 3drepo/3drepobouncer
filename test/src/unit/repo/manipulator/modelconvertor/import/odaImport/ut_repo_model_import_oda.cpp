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
#include "../../../../../repo_test_matchers.h"
#include "../../../../../repo_test_common_tests.h"
#include "repo/manipulator/modelconvertor/import/odaHelper/file_processor_nwd.h"

using namespace repo::manipulator::modelconvertor;
using namespace repo::core::model;
using namespace testing;

#define TESTDB "ODAModelImportTest"

namespace ODAModelImportUtils
{
	repo::core::model::RepoScene* ModelImportManagerImport(std::string filename, const ModelImportConfig& config)
	{
		auto handler = getHandler();

		uint8_t err;
		std::string msg;

		ModelImportManager manager;
		auto scene = manager.ImportFromFile(filename, config, handler, err);
		scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());
		scene->loadScene(handler.get(), msg);

		return scene;
	}

	repo::core::model::RepoScene* ModelImportManagerImport(std::string collection, std::string filename)
	{
		ModelImportConfig config(
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			collection
		);
		config.targetUnits = ModelUnits::MILLIMETRES;

		return ModelImportManagerImport(filename, config);
	}

	bool allNormalsAre(const repo::core::model::MeshNode& mesh, repo::lib::RepoVector3D normal, float tolerance, size_t count = ULONG_MAX)
	{
		auto normals = mesh.getNormals();
		if (mesh.getNormals().size() <= 0) {
			return false;
		}
		for (size_t i = 0; i < std::min(count, normals.size()); i++)
		{
			if ((normals[i] - normal).norm() > tolerance) {
				return false;
			}
		}
		return true;
	}

	repo::lib::RepoBounds getBounds(const std::vector<repo::core::model::MeshNode>& meshes) {
		repo::lib::RepoBounds bounds;
		for (auto& m : meshes) {
			bounds.encapsulate(m.getBoundingBox());
		}
		return bounds;
	}
}

/*
* Some ODA functionality uses global states and so must only be initialised/
* deinitialised once per-process. This test suite manages those resources
* for the NwdFileProcessor, for which there is a known issue on Linux.
* See this page for how the Test class works:
* https://google.github.io/googletest/advanced.html#sharing-resources-between-tests-in-the-same-test-suite
*/
class NwdTestSuite : public testing::Test
{
protected:
	static void SetUpTestSuite() {
		::odaHelper::FileProcessorNwd::createSharedSystemServices();
	}

	static void TearDownTestSuite() {
		::odaHelper::FileProcessorNwd::destorySharedSystemServices();
	}
};

TEST_F(NwdTestSuite, Sample2025NWDTree)
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

	auto children = nodes[0].getChildren({repo::core::model::NodeType::MESH, repo::core::model::NodeType::TRANSFORMATION});
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

TEST_F(NwdTestSuite, NwdDwgText1)
{
	auto scene = ODAModelImportUtils::ModelImportManagerImport("NwdDwgText1", getDataPath("groupsAndReferences.nwc"));
	SceneUtils utils(scene);

	// Check that the text objects are transformed correctly regardless of how
	// they're nested in the Dwg.

	{
		auto n = utils.findNodeByMetadata("Text::Contents", "Elements");
		auto mesh = n.getMeshesInProjectCoordinates()[0];
		mesh.updateBoundingBox();
		EXPECT_THAT(mesh.getBoundingBox(), BoundsAre(repo::lib::RepoBounds(repo::lib::RepoVector3D64(73.5607, 0, -150.424), repo::lib::RepoVector3D64(559.571, 0, -63.1341)), 1));
	}

	{
		auto n = utils.findNodeByMetadata("Text::Contents", "Groups");
		auto mesh = n.getMeshesInProjectCoordinates()[0];
		mesh.updateBoundingBox();
		EXPECT_THAT(mesh.getBoundingBox(), BoundsAre(repo::lib::RepoBounds(repo::lib::RepoVector3D64(1403.44, 0, -150.563), repo::lib::RepoVector3D64(1785.87, 0, -39.3713)), 1));
	}

	{
		auto n = utils.findNodeByMetadata("Text::Contents", "Block");
		auto mesh = n.getMeshesInProjectCoordinates()[0];
		mesh.updateBoundingBox();
		EXPECT_THAT(mesh.getBoundingBox(), BoundsAre(repo::lib::RepoBounds(repo::lib::RepoVector3D64(2858.4, 0, -149.527), repo::lib::RepoVector3D64(3142.53, 0, -62.2378)), 1));
	}

	{
		auto n = utils.findNodeByMetadata("Text::Contents", "Block References");
		auto mesh = n.getMeshesInProjectCoordinates()[0];
		mesh.updateBoundingBox();
		EXPECT_THAT(mesh.getBoundingBox(), BoundsAre(repo::lib::RepoBounds(repo::lib::RepoVector3D64(4072.89, 0, -148.987), repo::lib::RepoVector3D64(4999.74, 0, -60.2323)), 1));
	}

	{
		auto n = utils.findNodeByMetadata("Text::Contents", "Group of References");
		auto mesh = n.getMeshesInProjectCoordinates()[0];
		mesh.updateBoundingBox();
		EXPECT_THAT(mesh.getBoundingBox(), BoundsAre(repo::lib::RepoBounds(repo::lib::RepoVector3D64(6011.03, 0, -119.712), repo::lib::RepoVector3D64(6585.21, 0, -61.8148)), 1));
	}
}

TEST_F(NwdTestSuite, NwdDwgText2)
{
	auto scene = ODAModelImportUtils::ModelImportManagerImport("NwdDwgText2", getDataPath("dwgTextB.nwd"));
	SceneUtils utils(scene);

	// Check that the text objects are transformed correctly if they are rotated
	// in 3D, including normals.

	{
		auto n = utils.findNodeByMetadata("Text::Contents", "TextA");
		auto mesh = n.getMeshesInProjectCoordinates()[0];
		mesh.updateBoundingBox();
		EXPECT_THAT(mesh.getBoundingBox(), BoundsAre(repo::lib::RepoBounds(repo::lib::RepoVector3D64(-1820.88, 0, -7951.47), repo::lib::RepoVector3D64(339.193, 0, -262.202)), 120));
		EXPECT_TRUE(ODAModelImportUtils::allNormalsAre(mesh, repo::lib::RepoVector3D(0, 1, 0), 0.01, 10));
		EXPECT_THAT(n.getColours(), ElementsAre(repo::lib::repo_color3d_t(0.760784328, 0.807843149, 0.839215696)));
	}

	{
		auto n = utils.findNodeByMetadata("Text::Contents", "TextB");
		auto mesh = n.getMeshesInProjectCoordinates()[0];
		mesh.updateBoundingBox();
		EXPECT_THAT(mesh.getBoundingBox(), BoundsAre(repo::lib::RepoBounds(repo::lib::RepoVector3D64(-30309.4, -10499.1, -57602.3), repo::lib::RepoVector3D64(8771.25, 11210.5, -33405.4)), 120));
		EXPECT_TRUE(ODAModelImportUtils::allNormalsAre(mesh, repo::lib::RepoVector3D(0.531980395, 0.310758412, 0.787671328), 0.01, 10));
		EXPECT_THAT(n.getColours(), ElementsAre(repo::lib::repo_color3d_t(0.803921580, 0.125490203, 0.152941182)));
	}
}

TEST_F(NwdTestSuite, NwdWorldOrientation)
{
	auto scene = ODAModelImportUtils::ModelImportManagerImport("NwdWorldOrientation", getDataPath("orientedColumns.nwd"));
	SceneUtils utils(scene);

	// The orientedColumns file contains two colums. One modelled Y-Up and imported
	// into Navis with the World Orientation set to Y-Up, and one modelled Z-Up,
	// imported into Navis with a Transform of 90 degrees around the X-Axis applied
	// using the Units and Transforms Dialog (which was baked when Navis saved the
	// NWD).

	// Both columns should be oriented directly (+Z), and the higher column should
	// sit further North (i.e. along the 3DR +Y axis) than the other.

	auto column1 = ODAModelImportUtils::getBounds(utils.findLeafNode("Column1.obj").getMeshesInProjectCoordinates());
	auto column2 = ODAModelImportUtils::getBounds(utils.findLeafNode("Column2.obj").getMeshesInProjectCoordinates());

	// (These comparisons also consider the direct-x reorientation, because this is
	// applied by the ModelImportManager to all ODA imports at the root of the scene
	// graph. This may change in the future if we move it to the multipart optimiser
	// /repobundle export).

	EXPECT_THAT(column1, BoundsAre(repo::lib::RepoBounds(repo::lib::RepoVector3D64(-34, 0, -34), repo::lib::RepoVector3D64(34, 267, 34)), 2.0));
	EXPECT_THAT(column2, BoundsAre(repo::lib::RepoBounds(repo::lib::RepoVector3D64(-34, -102, 84), repo::lib::RepoVector3D64(34, 165, 152)), 2.0));
	EXPECT_THAT(repo::lib::RepoVector3D64(scene->getWorldOffset()), VectorNear(repo::lib::RepoVector3D64(-34, -102, 152), 1.0));
}

TEST(ODAModelImport, RevitHideCategories)
{
	auto scene = ODAModelImportUtils::ModelImportManagerImport("RevitHideCategories", getDataPath("annotationsExample.rvt"));
	SceneUtils utils(scene);

	EXPECT_THAT(utils.findNodesByMetadata("Category", "Sun Path"), IsEmpty());
	EXPECT_THAT(utils.findNodesByMetadata("Category", "Work Plane Grid"), IsEmpty());
	EXPECT_THAT(utils.findNodesByMetadata("Category", "Scope Boxes"), IsEmpty());
	EXPECT_THAT(utils.findNodesByMetadata("Category", "Levels"), IsEmpty());
	EXPECT_THAT(utils.findNodesByMetadata("Category", "Grids"), IsEmpty());
	EXPECT_THAT(utils.findNodesByMetadata("Spot Coordinates", "Grids"), IsEmpty());
}

TEST(ODAModelImport, DefaultViewVisibility)
{
	// If there is no named view provided for Revit files, all geometry should
	// be imported.

	::setupTextures();

	auto scene = ODAModelImportUtils::ModelImportManagerImport("RevitHideCategories", getDataPath("partiallyHiddenModel.rvt"));
	SceneUtils utils(scene);

	// All model categories, even those hidden in the default view
	EXPECT_THAT(utils.findNodesByMetadata("Category", "Stairs"), Not(IsEmpty()));

	// Cropping and sectioning should also be ignored
	auto bounds = scene->getSceneBoundingBox();
	EXPECT_THAT(bounds.size().x, Gt(197091));
	EXPECT_THAT(bounds.size().y, Gt(100899));
	EXPECT_THAT(bounds.size().z, Gt(106445));

	// Geometry from all phases should be included
	EXPECT_THAT(utils.findNodesByMetadata("Element ID", "2927126"), Not(IsEmpty())); // New Construction
	EXPECT_THAT(utils.findNodesByMetadata("Element ID", "2926032"), Not(IsEmpty())); // Outfitting (after current phase)
	EXPECT_THAT(utils.findNodesByMetadata("Element ID", "2926070"), Not(IsEmpty())); // Demolished

	// All details (Fine) should be included.
	// (The Handles for the Doors are only present when rendering in Fine mode)
	auto door = utils.findNodeByMetadata("Element ID", "2861117");
	auto doorMeshes = door.getMeshes(repo::core::model::MeshNode::Primitive::TRIANGLES);
	EXPECT_THAT(doorMeshes.size(), Eq(3));

	// Textures should be supported
	EXPECT_THAT(utils.findNodeByMetadata("Element ID", "2637395").hasTextures(), IsTrue());
	EXPECT_THAT(utils.findNodeByMetadata("Element ID", "2928303").hasTextures(), IsTrue());
	EXPECT_THAT(utils.findNodeByMetadata("Element ID", "2285130").hasTextures(), IsTrue());
	EXPECT_THAT(utils.findNodeByMetadata("Element ID", "2926032").hasTextures(), IsTrue());

	// Transparency should work as well
	EXPECT_THAT(door.hasTransparency(), IsTrue());

	// Should include 3d geometry without edges
	auto floorNode = utils.findNodeByMetadata("Element ID", "2928847");
	EXPECT_THAT(floorNode.getMeshes(repo::core::model::MeshNode::Primitive::TRIANGLES), Not(IsEmpty()));
	EXPECT_THAT(floorNode.getMeshes(repo::core::model::MeshNode::Primitive::LINES), IsEmpty());

	// The Lines model category should be treated as an annotation, and not
	// processed
	EXPECT_THAT(utils.findNodesByMetadata("Category", "Lines"), IsEmpty());
}

TEST(ODAModelImport, NamedView)
{
	// If a named view is provided, the view should adopt the visibility settings
	// of that view.

	::setupTextures();

	ModelImportConfig config(
		repo::lib::RepoUUID::createUUID(),
		TESTDB,
		"RevitNamedView"
	);
	config.targetUnits = ModelUnits::MILLIMETRES;
	config.viewName = "3D Repo";

	auto scene = ODAModelImportUtils::ModelImportManagerImport(getDataPath("partiallyHiddenModel.rvt"), config);
	SceneUtils utils(scene);

	// The saved/named view hides the Stairs category
	EXPECT_THAT(utils.findNodesByMetadata("Category", "Stairs"), IsEmpty());

	// The named view should be level Coarse (so hide door handles)
	auto door = utils.findNodeByMetadata("Element ID", "2861117");
	auto doorMeshes = door.getMeshes(repo::core::model::MeshNode::Primitive::TRIANGLES);
	EXPECT_THAT(doorMeshes.size(), Eq(1));

	// The named view has a sectioning box applied
	auto bounds = scene->getSceneBoundingBox();
	EXPECT_THAT(bounds.size().x, Lt(126729));
	EXPECT_THAT(bounds.size().y, Lt(52193));
	EXPECT_THAT(bounds.size().z, Lt(63451));

	// The named view should only show the Phase up to New Construction
	EXPECT_THAT(utils.findNodesByMetadata("Element ID", "2926032"), IsEmpty()); // Outfitting (after current phase)
	EXPECT_THAT(utils.findNodesByMetadata("Element ID", "2926070"), Not(IsEmpty())); // Demolished

	// The named view is in Hidden Lines mode, so won't include any textures
	EXPECT_THAT(utils.findNodeByMetadata("Element ID", "2928303").hasTextures(), IsFalse());
	EXPECT_THAT(utils.findNodeByMetadata("Element ID", "2285130").hasTextures(), IsFalse());
}

TEST(ODAModelImport, InvalidNamedView1)
{
	ModelImportConfig config(
		repo::lib::RepoUUID::createUUID(),
		TESTDB,
		"RevitNamedView"
	);
	config.targetUnits = ModelUnits::MILLIMETRES;
	config.viewName = "Level 0";

	try {
		ODAModelImportUtils::ModelImportManagerImport(getDataPath("partiallyHiddenModel.rvt"), config);
	}
	catch (repo::lib::RepoException& e){
		EXPECT_THAT(e.repoCode(), Eq(REPOERR_VIEW_NOT_3D));
	}
}

TEST(ODAModelImport, InvalidNamedView2)
{
	ModelImportConfig config(
		repo::lib::RepoUUID::createUUID(),
		TESTDB,
		"RevitNamedView"
	);
	config.targetUnits = ModelUnits::MILLIMETRES;
	config.viewName = "Not A View";

	try {
		ODAModelImportUtils::ModelImportManagerImport(getDataPath("partiallyHiddenModel.rvt"), config);
	}
	catch (repo::lib::RepoException& e) {
		EXPECT_THAT(e.repoCode(), Eq(REPOERR_VIEW_NOT_FOUND));
	}
}

TEST(ODAModelImport, DefaultViewDisplayOptions)
{
	::setupTextures();

	{
		ModelImportConfig config(
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			"RevitNamedView"
		);
		config.targetUnits = ModelUnits::MILLIMETRES;
		config.viewStyle = "shadedwithedges";

		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport(getDataPath("sample2025.rvt"), config));

		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").getMeshes(repo::core::model::MeshNode::Primitive::TRIANGLES).size(), Eq(3)); // Two textures and a flat surface
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").getMeshes(repo::core::model::MeshNode::Primitive::LINES).size(), Gt(0));
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").hasTextures(), IsTrue());
	}

	{
		ModelImportConfig config(
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			"RevitNamedView"
		);
		config.targetUnits = ModelUnits::MILLIMETRES;

		// Default (empty string) is shaded

		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport(getDataPath("sample2025.rvt"), config));

		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").getMeshes(repo::core::model::MeshNode::Primitive::TRIANGLES).size(), Gt(0)); // Two textures and a flat surface
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").getMeshes(repo::core::model::MeshNode::Primitive::LINES).size(), Eq(0));
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").hasTextures(), IsTrue());
	}

	{
		ModelImportConfig config(
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			"RevitNamedView"
		);
		config.targetUnits = ModelUnits::MILLIMETRES;
		config.viewStyle = "notsupported";

		EXPECT_THROW({
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport(getDataPath("sample2025.rvt"), config));
		},
		repo::lib::RepoSceneProcessingException);
	}

	{
		ModelImportConfig config(
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			"RevitNamedView"
		);
		config.targetUnits = ModelUnits::MILLIMETRES;
		config.viewStyle = "hiddenline";

		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport(getDataPath("sample2025.rvt"), config));

		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").getMeshes(repo::core::model::MeshNode::Primitive::TRIANGLES).size(), Gt(0)); // No textures means meshes are combined
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").getMeshes(repo::core::model::MeshNode::Primitive::LINES).size(), Gt(0));
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").hasTextures(), IsFalse());
	}

	{
		ModelImportConfig config(
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			"RevitNamedView"
		);
		config.targetUnits = ModelUnits::MILLIMETRES;
		config.viewStyle = "wireframe";

		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport(getDataPath("sample2025.rvt"), config));

		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").getMeshes(repo::core::model::MeshNode::Primitive::TRIANGLES).size(), Eq(0)); // No textures means meshes are combined
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").getMeshes(repo::core::model::MeshNode::Primitive::LINES).size(), Gt(0));
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "307098").hasTextures(), IsFalse());
	}
}

TEST(ODAModelImport, RvtUnits)
{
	// In mm
	auto expectedBounds = repo::lib::RepoBounds(
		repo::lib::RepoVector3D64(-708.47, -2500.0, -62.05),
		repo::lib::RepoVector3D64(-403.17, 0.0, 245.84)
	);

	// Regardless of what the project units are, the geometry should always be
	// imported in the target units of the import config (and correctly scaled
	// to those units).

	{
		ModelImportConfig config(repo::lib::RepoUUID::createUUID(), TESTDB, "RevitUnitsMillimeters");
		config.targetUnits = ModelUnits::MILLIMETRES;
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport(getDataPath("rvt_mm.rvt"), config));
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "330859").getProjectBounds(), BoundsAre(expectedBounds, 0.5));
	}

	{
		ModelImportConfig config(repo::lib::RepoUUID::createUUID(), TESTDB, "RevitUnitsFeet");
		config.targetUnits = ModelUnits::MILLIMETRES;
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport(getDataPath("rvt_ft.rvt"), config));
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "330859").getProjectBounds(), BoundsAre(expectedBounds, 0.5));
	}

	{
		ModelImportConfig config(repo::lib::RepoUUID::createUUID(), TESTDB, "RevitUnitsFeetandFractionalInches");
		config.targetUnits = ModelUnits::MILLIMETRES;
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport(getDataPath("rvt_ft-in.rvt"), config));
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "330859").getProjectBounds(), BoundsAre(expectedBounds, 0.5));
	}

	{
		ModelImportConfig config(repo::lib::RepoUUID::createUUID(), TESTDB, "RevitUnitsShaku");
		config.targetUnits = ModelUnits::MILLIMETRES;
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport(getDataPath("rvt_shaku.rvt"), config));
		EXPECT_THAT(scene.findNodeByMetadata("Element ID", "330859").getProjectBounds(), BoundsAre(expectedBounds, 0.5));
	}
}

TEST_F(NwdTestSuite, MetadataParentsNWD)
{
	// All metadata nodes must also have their sibling meshnodes as parents,
	// in order to resolve ids by mesh node in the frontend

	{
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport("MetadataParentsNWD", getDataPath(nwdModel2025)));
		common::checkMetadataInheritence(scene);
	}

	{
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport("MetadataParentsNWD", getDataPath("groupsAndReferences.nwc")));
		common::checkMetadataInheritence(scene);
	}

	{
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport("MetadataParentsNWD", getDataPath("orientedColumns.nwd")));
		common::checkMetadataInheritence(scene);
	}
}

TEST(ODAModelImport, MetadataParents)
{
	{
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport("MetadataParentsRVT", getDataPath("sample2025.rvt")));
		common::checkMetadataInheritence(scene);
	}

	{
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport("MetadataParentsRVT", getDataPath("MetaTest2.rvt")));
		common::checkMetadataInheritence(scene);
	}

	{
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport("MetadataParentsDWG", getDataPath("nestedBlocks.dwg")));
		common::checkMetadataInheritence(scene);
	}

	{
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport("MetadataParentsDWG", getDataPath("colouredBoxes.dwg")));
		common::checkMetadataInheritence(scene);
	}

	{
		SceneUtils scene(ODAModelImportUtils::ModelImportManagerImport("MetadataParentsDGN", getDataPath("sample.dgn")));
		common::checkMetadataInheritence(scene);
	}
}