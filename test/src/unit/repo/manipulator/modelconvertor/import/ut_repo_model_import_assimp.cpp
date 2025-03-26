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
#include <repo/manipulator/modelconvertor/import/repo_model_import_assimp.h>
#include <repo_log.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_database_info.h"
#include "../../../../repo_test_scene_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::manipulator::modelconvertor;
using namespace testing;

namespace RepoModelImportUtils
{
	static std::unique_ptr<AssimpModelImport> CreateModelConvertor(
		std::string filePath,
		uint8_t& impModelErrCode)
	{
		ModelImportConfig config;
		auto modelConvertor = std::unique_ptr<AssimpModelImport>(new AssimpModelImport(config));
		modelConvertor->importModel(filePath, impModelErrCode);
		return modelConvertor;
	}

	static repo::core::model::RepoScene* ImportAssimpFile(
		std::string filePath)
	{
		uint8_t impModelErrCode;
		ModelImportConfig config;
		auto modelConvertor = std::unique_ptr<AssimpModelImport>(new AssimpModelImport(config));
		modelConvertor->importModel(filePath, impModelErrCode);
		return modelConvertor->generateRepoScene(impModelErrCode);
	}
}

TEST(AssimpModelImport, MainTest)
{
	uint8_t errCode = 0;
	// non fbx file that does not have metadata
	auto importer = RepoModelImportUtils::CreateModelConvertor(getDataPath("3DrepoBIM.obj"), errCode);
	EXPECT_FALSE(importer->requireReorientation());
	EXPECT_EQ(0, errCode);

	// file with enough metadata to reorientate
	importer = RepoModelImportUtils::CreateModelConvertor(getDataPath("AssimpModelImport/AIM4G.fbx"), errCode);
	EXPECT_FALSE(importer->requireReorientation());
	EXPECT_EQ(0, errCode);
}

TEST(AssimpModelImport, SupportExtenions)
{
	EXPECT_TRUE(AssimpModelImport::isSupportedExts(".fbx"));
	EXPECT_TRUE(AssimpModelImport::isSupportedExts(".ifc"));
	EXPECT_TRUE(AssimpModelImport::isSupportedExts(".obj"));
}


TEST(AssimpModelImport, Unreal3D)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("box_d.3d")));
	EXPECT_TRUE(scene.isPopulated());

	EXPECT_THAT(scene.getRootNode().name(), Eq(std::string("<UnrealRoot>")));

	auto meshes = scene.getRootNode().getMeshes();
	EXPECT_THAT(meshes.size(), Eq(1));
	EXPECT_THAT(meshes[0].name(), IsEmpty());
}

TEST(AssimpModelImport, BlendMirrored)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("mirrored.blend")));
	EXPECT_TRUE(scene.isPopulated());

	/*
	* The figure model is mirrored around the x-axis
	*/

	auto node = scene.findTransformationNodeByName("Cube");

	EXPECT_THAT(node.numMeshes, Eq(2));
	EXPECT_THAT(node.isLeaf(), IsTrue());
}

TEST(AssimpModelImport, BlendHierarchy)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("cubeHierarchy.blend")));
	EXPECT_TRUE(scene.isPopulated());

	/*
	* The hierarchy model contains a number of branch nodes with meshes. We need
	* to make sure in these cases a seperate node has been created for the meshes
	* as otherwise it will not be possible to select just that node in the tree.
	*/

	EXPECT_THAT(scene.findLeafNode("Cube").getPath(), Eq("<BlenderRoot>->Cube1->Cube"));
	EXPECT_THAT(scene.findLeafNode("Cube.001").getPath(), Eq("<BlenderRoot>->Cube1->Cube.001->Cube.001"));
	EXPECT_THAT(scene.findLeafNode("Cube.003").getPath(), Eq("<BlenderRoot>->Cube1->Cube.001->Cube.002->Cube.003"));
	EXPECT_THAT(scene.findLeafNode("Cube.007").getPath(), Eq("<BlenderRoot>->Cube1->Cube.001->Cube.002->Cube.007"));
	EXPECT_THAT(scene.findLeafNode("Cube.008").getPath(), Eq("<BlenderRoot>->Cube1->Cube.001->Cube.002->Cube.008"));
	EXPECT_THAT(scene.findLeafNode("Cube.004").getPath(), Eq("<BlenderRoot>->Cube1->Cube.001->Cube.003->Cube.004"));
	EXPECT_THAT(scene.findLeafNode("Cube.006").getPath(), Eq("<BlenderRoot>->Cube1->Cube.001->Cube.003->Cube.006"));
}

TEST(AssimpModelImport, Cob)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("molecule.cob")));
	EXPECT_TRUE(scene.isPopulated());

	EXPECT_THAT(scene.findLeafNode("Sphere_0").getPath(), Eq("->Nitrogen_0->Sphere_0"));
	EXPECT_THAT(scene.findLeafNode("Sphere_1").getPath(), Eq("->Nitrogen_0->Sphere_1"));
	EXPECT_THAT(scene.findLeafNode("Sphere_2").getPath(), Eq("->Nitrogen_0->Sphere_2"));
	EXPECT_THAT(scene.findLeafNode("Sphere_3").getPath(), Eq("->Nitrogen_0->Sphere_3"));

	EXPECT_THAT(scene.findLeafNode("Sphere_0").getColours()[0], Eq(repo::lib::repo_color4d_t(0.345098048, 0.435294151, 0.909803987, 1)));
	EXPECT_THAT(scene.findLeafNode("Sphere_1").getColours()[0], Eq(repo::lib::repo_color4d_t(1, 1, 1, 1)));
	EXPECT_THAT(scene.findLeafNode("Sphere_2").getColours()[0], Eq(repo::lib::repo_color4d_t(1, 1, 1, 1)));
	EXPECT_THAT(scene.findLeafNode("Sphere_3").getColours()[0], Eq(repo::lib::repo_color4d_t(1, 1, 1, 1)));
}

TEST(AssimpModelImport, Irr)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("box.irr")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_THAT(scene.findLeafNode("testCube").getPath(), Eq("<IRRSceneRoot>->testCube"));
}

TEST(AssimpModelImport, IrrMesh)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("cellar.irrmesh")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_THAT(scene.getRootNode().getMeshes().size(), Eq(2));
}

TEST(AssimpModelImport, MDL5)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("molecule.mdl")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, XML)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("mesh.mesh.xml"))); // When working with xml files, meshes must be called [x].mesh.xml
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, ASE)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("cubes.ase")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_THAT(scene.findLeafNode("Quader01").getPath(), Eq("<ASERoot>->->Quader01"));
	EXPECT_THAT(scene.findLeafNode("Quader02").getPath(), Eq("<ASERoot>->->Quader02"));
	EXPECT_THAT(scene.findLeafNode("Quader03").getPath(), Eq("<ASERoot>->->Quader03"));
}

TEST(AssimpModelImport, STL1)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("3dsmax.stl")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, STL2)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("ext.stl")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, B3D)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("bison.b3d")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, HMP)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("terrain.hmp")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, LWO)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("cube.lwo")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_THAT(scene.findLeafNode("Layer_0").getPath(), Eq("Pivot-Layer_0->Layer_0"));
	EXPECT_TRUE(scene.findLeafNode("Layer_0").getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, MD2)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("sydney.md2")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, MD3)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("winchester.MD3")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, MS3D)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("twospheres.ms3d")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, OFF)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("cube.off")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, PK3)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("SGDTT3.pk3")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, PLY)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("bison.ply")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, Q3S)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("bison.q3s")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_THAT(scene.findLeafNode("Unnamed Mesh").getMeshes().size(), Eq(1));
	EXPECT_THAT(scene.findLeafNode("Unnamed Mesh").getPath(), Eq("->Unnamed Mesh"));
}

TEST(AssimpModelImport, SMD)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("triangle.smd")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_THAT(scene.findLeafNode("<SMD_root>").getPath(), Eq("<SMD_root>-><SMD_root>"));
	EXPECT_THAT(scene.findLeafNode("<SMD_root>").getMeshes().size(), Eq(1));
	EXPECT_THAT(scene.findLeafNode("TriangleTest0").getPath(), Eq("<SMD_root>->TriangleTest0"));
	EXPECT_THAT(scene.findLeafNode("TriangleTest0").getMeshes().size(), Eq(0));
	EXPECT_TRUE(scene.scene->isMissingTexture());
}

TEST(AssimpModelImport, TER)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("terrain.ter")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, XGL)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("sample_official.xgl")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}

TEST(AssimpModelImport, ZGL)
{
	SceneUtils scene(RepoModelImportUtils::ImportAssimpFile(getDataPath("cubes_with_alpha.zgl")));
	EXPECT_TRUE(scene.isPopulated());
	EXPECT_TRUE(scene.getRootNode().isLeaf());
	EXPECT_TRUE(scene.getRootNode().getMeshes().size(), Eq(1));
}
