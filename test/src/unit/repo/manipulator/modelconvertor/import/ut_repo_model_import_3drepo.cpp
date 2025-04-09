/**
*  Copyright (C) 2020 3D Repo Ltd
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
#include "repo/error_codes.h"
#include <repo/manipulator/modelconvertor/import/repo_model_import_3drepo.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_manager.h>
#include <repo_log.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_database_info.h"
#include "../../../../repo_test_scene_utils.h"
#include "../../../../repo_test_mesh_utils.h"

using namespace repo::manipulator::modelconvertor;
using namespace testing;
using namespace repo::test::utils;

namespace RepoModelImportUtils
{
	repo::core::model::RepoScene* ImportBIMFile(
		std::string bimFilePath,
		uint8_t& impModelErrCode)
	{
		ModelImportConfig config(
			true,
			ModelUnits::MILLIMETRES,
			"",
			0,
			repo::lib::RepoUUID::createUUID(),
			"BIMImportTest",
			"BIMImportCollection"
		);

		auto handler = getHandler();

		ModelImportManager manager;
		auto scene = manager.ImportFromFile(bimFilePath, config, handler, impModelErrCode);

		if (scene) {
			std::string msg;
			scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());
			scene->loadScene(handler.get(), msg);
		}

		return scene;
	}
}

TEST(RepoModelImport, ValidBIM001Import)
{
	uint8_t errCode = 0;
	RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/cube_bim1_spoofed.bim"), errCode);
	EXPECT_THAT(errCode, Eq(REPOERR_UNSUPPORTED_BIM_VERSION));
}

TEST(RepoModelImport, ValidBIM002Import)
{
	uint8_t errCode = 0;
	SceneUtils scene(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/cube_bim2_navis_2021_repo_4.6.1.bim"), errCode));

	EXPECT_EQ(REPOERR_OK, errCode);

	EXPECT_THAT(scene.findNodesByMetadata("Element ID:Value", "313363").size(), Eq(5));

	{
		auto node = scene.findLeafNode("Concrete, Sand/Cement Screed");
		EXPECT_THAT(node.isLeaf(), IsTrue());
		EXPECT_THAT(node.hasGeometry(), IsTrue());
		EXPECT_THAT(node.getColours(), UnorderedElementsAre(repo::lib::repo_color4d_t(0.498039215803146, 0.498039215803146, 0.498039215803146)));
	}
	{
		auto node = scene.findLeafNode("Copper");
		EXPECT_THAT(node.isLeaf(), IsTrue());
		EXPECT_THAT(node.hasGeometry(), IsTrue());
		EXPECT_THAT(node.getColours(), UnorderedElementsAre(repo::lib::repo_color4d_t(0, 0, 0)));
	}
	{
		auto node = scene.findLeafNode("Softwood, Lumber");
		EXPECT_THAT(node.isLeaf(), IsTrue());
		EXPECT_THAT(node.hasGeometry(), IsTrue());
		EXPECT_THAT(node.getColours(), UnorderedElementsAre(repo::lib::repo_color4d_t(0.874509811401367, 0.752941191196442, 0.52549022436142)));
	}
	{
		auto node = scene.findLeafNode("Wood Sheathing, Chipboard");
		EXPECT_THAT(node.isLeaf(), IsTrue());
		EXPECT_THAT(node.hasGeometry(), IsTrue());
		EXPECT_THAT(node.getColours(), UnorderedElementsAre(repo::lib::repo_color4d_t(0.498039215803146, 0.498039215803146, 0.498039215803146)));
	}
}

TEST(RepoModelImport, ValidBIM003Import)
{
	uint8_t errCode = 0;
	SceneUtils scene(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/BrickWalls_bim3.bim"), errCode));

	EXPECT_EQ(REPOERR_OK, errCode);

	std::vector<std::string> names({
		"Wall-Ext_102Bwk-75Ins-100LBlk-12P",
		"Wall-Ext_102Bwk-75Ins-100LBlk-12P_2"
		});

	for(auto name : names)
	{
		auto node = scene.findLeafNode(name);
		int numMeshes = 0;
		int numTextures = 0;

		for (auto m : node.getMeshes())
		{
			numMeshes++;
			numTextures += m.getTextures().size();

			if (m.getTextures().size()) {
				EXPECT_THAT(dynamic_cast<repo::core::model::MeshNode*>(m.node)->getNumUVChannels(), Eq(1));
			}
		}

		EXPECT_THAT(numMeshes, Eq(3));
		EXPECT_THAT(numTextures, Eq(2));
	}
}

TEST(RepoModelImport, ValidBIM004Import)
{
	uint8_t errCode = 0;
	SceneUtils scene(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/wall_section_bim4.bim"), errCode));

	EXPECT_EQ(REPOERR_OK, errCode);

	auto node = scene.findLeafNode("Wall-Ext_102Bwk-75Ins-100LBlk-12P");
	EXPECT_TRUE(node.hasGeometry());

	int i = 0;
	for (auto m : node.getMeshes()) {
		i += m.getTextures().size();
	}
	EXPECT_THAT(i, Eq(2));

	for (auto m : node.getMeshes()) {
		EXPECT_THAT(dynamic_cast<repo::core::model::MeshNode*>(m.node)->getPrimitive(), Eq(repo::core::model::MeshNode::Primitive::TRIANGLES));
	}

	EXPECT_THAT(mesh::shortestDistance(node.getMeshesInProjectCoordinates(), repo::lib::RepoVector3D(1.79, 8.00, 1.38)), Lt(0.1));

	node = scene.findLeafNode("Model Lines");
	EXPECT_THAT(node.getMeshes().size(), Gt(0));
	for (auto m : node.getMeshes()) {
		EXPECT_THAT(dynamic_cast<repo::core::model::MeshNode*>(m.node)->getPrimitive(), Eq(repo::core::model::MeshNode::Primitive::LINES));
	}

	EXPECT_THAT(mesh::shortestDistance(node.getMeshesInProjectCoordinates(), repo::lib::RepoVector3D(-14.28, 0, -2.28)), Lt(0.1));
	EXPECT_THAT(mesh::shortestDistance(node.getMeshesInProjectCoordinates(), repo::lib::RepoVector3D(-9.77, 0, 4.40)), Lt(0.1));
}

TEST(RepoModelImport, SharedCoordinates)
{
	/*
	* The three files contain a box of equivalent dimensions, offset and rotated
	* in 3D space by the same transforms. The geometry when transformed into
	* project coordinates should be identical, regardless of which tool exported
	* the bim file.
	*/

	uint8_t errCode = 0;

	std::vector<SceneUtils> scenes = {
		SceneUtils(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/rotatedbox.autocad.bim004.bim"), errCode)),
		SceneUtils(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/rotatedbox.navis.bim004.bim"), errCode)),
		SceneUtils(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/rotatedbox.revit.bim004.bim"), errCode))
	};

	std::vector<repo::core::model::MeshNode> meshes;
	for (auto& s : scenes) {
		for (auto& m : s.getMeshes()) {
			meshes.push_back(m.getMeshInProjectCoordinates());
		}
	}

	// For the above files, the geometry has been recreated in each tool (not imported)
	// so the tolerance accounts for the slight differences between tools (e.g. Revit
	// only specifies lengths to one decimal place, etc).
	// The original models are saved alongside the .bims in order to produce additional
	// exports for future .bim versions.

	EXPECT_THAT(mesh::hausdorffDistance(meshes), Lt(0.1));

	// At least one coordinate should be in a place we have previously measured (e.g.
	// with the Locate Point Measure Tool).

	for (auto& m : meshes) {
		EXPECT_THAT(mesh::shortestDistance(m, repo::lib::RepoVector3D(120811.3, 2443.6, -79440.4)), Lt(0.5));
	}

	// Finally, the scene offset should be set to a sensible value

	for (auto& s : scenes) {
		EXPECT_THAT(repo::lib::RepoVector3D64(s.scene->getWorldOffset()).norm(), Gt(100000));
	}
}

TEST(RepoModelImport, Normals)
{
	uint8_t errCode = 0;
	SceneUtils scene(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/rotatedbox.revit.bim004.bim"), errCode));
	auto mesh = scene.getMeshes()[0].getMeshInProjectCoordinates();

	// Check against known good values that should be in the normals list
	std::vector<repo::lib::RepoVector3D> normals = {
		{0.0000, 1.0000, 0.0000},
		{0.0000, -1.0000, 0.0000},
		{0.8440, 0.0000, 0.5362},
		{0.5362, 0.0000, -0.8440},
		{-0.8440, 0.0000, -0.5362},
		{-0.5362, 0.0000, 0.8440}
	};

	for (auto& n : normals) {
		EXPECT_THAT(mesh::shortestDistance(mesh.getNormals(), n), Lt(0.1));
	}
}

TEST(RepoModelImport, Metadata)
{
	uint8_t errCode = 0;
	SceneUtils scene(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/metadata.bim004.bim"), errCode));
	auto node = scene.findNodeByMetadata("Element ID", "329486");
	auto metadata = node.getMetadata();

	EXPECT_THAT(metadata["MyBool"].which(), Eq(0));
	EXPECT_THAT(metadata["MyInteger"].which(), Eq(1));
	EXPECT_THAT(metadata["Geometry::MyDouble (mm)"].which(), Eq(3));
	EXPECT_THAT(metadata["Text::MyDate"].which(), Eq(4));
	EXPECT_THAT(metadata["Text::MyString"].which(), Eq(4));
}

TEST(RepoModelImport, EmptyTransforms)
{
	/*
	* The following files should have identical geometry, but differ in how empty
	* transforms are arranged.
	* (If inspecting the imports, be aware the metadata keys have been adjusted to
	* keep the Json sections the same length, so may not always look sensible.)
	*/

	uint8_t errCode = 0;

	std::vector<SceneUtils> scenes = {
		SceneUtils(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/emptyTransforms1.bim004.bim"), errCode)),
		SceneUtils(RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/emptyTransforms2.bim004.bim"), errCode))
	};

	for (auto s : scenes)
	{
		EXPECT_THAT(s.getMeshes().size(), Eq(4));
		auto node = s.findLeafNode("450x450mm");
		auto mesh = node.getMeshesInProjectCoordinates()[0];
		EXPECT_THAT(mesh::shortestDistance(mesh.getVertices(), { -13623.82985447024, 4000, -9214.240784344169 }), Lt(0.1));
	}
}