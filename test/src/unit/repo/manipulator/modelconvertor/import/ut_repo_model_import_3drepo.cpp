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
#include <repo/manipulator/modelconvertor/import/repo_model_import_3drepo.h>
#include <repo/lib/repo_log.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_database_info.h"
#include "boost/filesystem.hpp"

using namespace repo::manipulator::modelconvertor;
using namespace repo::test;

static bool testBIMFileImport(
	std::string bimFilePath,
	int expMaterialsCount,
	int expTexturesCount,
	int expMeshesCount,
	int expImpModelErrCode = REPOERR_OK,
	int expGenSceneErrCode = REPOERR_OK,
	bool generateScene = true,
	std::string debugMeshDataFilePath = "",
	std::string textureFilesDumpPath = "")
{
	ModelImportConfig config;
	uint8_t impModelErrCode = 0;
	auto modelConvertor = std::unique_ptr<AbstractModelImport>(new RepoModelImport(config));
	modelConvertor->importModel(bimFilePath, impModelErrCode);
	if (impModelErrCode != expImpModelErrCode)
	{ 
		repoInfo << "Error from importModel(): " << TestLogging::getStringFromRepoErrorCode((int)impModelErrCode);
		return false; 
	}

	if(generateScene)
	{
		uint8_t genSceneErrCode = 0;
		auto repoScene = modelConvertor->generateRepoScene(genSceneErrCode);
		if (genSceneErrCode != expGenSceneErrCode)
		{
			repoInfo << "Error from generateRepoScene(): " << TestLogging::getStringFromRepoErrorCode((int)genSceneErrCode);
			return false;
		}

		auto materials = repoScene->getAllMaterials(repo::core::model::RepoScene::GraphType::DEFAULT);
		auto textures = repoScene->getAllTextures(repo::core::model::RepoScene::GraphType::DEFAULT);
		auto meshes = repoScene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT);

		if(debugMeshDataFilePath != "")
		{
			// Print out the mesh debug text file
			std::ofstream stream;
			stream.open(debugMeshDataFilePath);
			if (stream)
			{
				for (auto const mesh : meshes)
				{
					auto meshNode = static_cast<repo::core::model::MeshNode*>(mesh);
					std::vector<repo_face_t> triangularFaces = meshNode->getFaces();
					std::vector<repo::lib::RepoVector3D> vertices = meshNode->getVertices();
					std::vector<repo::lib::RepoVector3D> normals = meshNode->getNormals();
					std::vector<repo::lib::RepoVector2D> uvs = meshNode->getUVChannels();
					char lastsep = ',';
					stream << "------mesh------" << std::endl;
					// Set this to the parent node id for an easier debugging life
					stream << mesh->getName() << std::endl;
					for (auto vertIt = vertices.begin(); vertIt != vertices.end(); ++vertIt)
					{
						if (vertIt == (vertices.end() - 1)) { lastsep = '\n'; }
						stream << vertIt->x << ",";
						stream << vertIt->y << ",";
						stream << vertIt->z << lastsep;
					}
					lastsep = ',';
					for (auto normIt = normals.begin(); normIt != normals.end(); ++normIt)
					{
						if (normIt == (normals.end() - 1)) { lastsep = '\n'; }
						stream << normIt->x << ",";
						stream << normIt->y << ",";
						stream << normIt->z << lastsep;
					}
					lastsep = ',';
					for (auto faceIt = triangularFaces.begin(); faceIt != triangularFaces.end(); ++faceIt)
					{
						if (faceIt == (triangularFaces.end() - 1)) { lastsep = '\n'; }
						stream << faceIt->at(0) << ",";
						stream << faceIt->at(1) << ",";
						stream << faceIt->at(2) << lastsep;
					}
					lastsep = ',';
					for (auto uvIt = uvs.begin(); uvIt != uvs.end(); ++uvIt)
					{
						if (uvIt== (uvs.end() - 1)) { lastsep = '\n'; }
						stream << uvIt->x << ",";
						stream << uvIt->y << lastsep;
					}
				}
			}
			stream.close();
		}

		if(textureFilesDumpPath != "")
		{
			for (auto const img : textures)
			{
				auto textureNode = static_cast<repo::core::model::TextureNode*>(img);
				std::vector<char> imgData = textureNode->getRawData();
				boost::filesystem::path dirPath(textureFilesDumpPath);
				boost::filesystem::path fileName( "DUMP_" + textureNode->getName());
				auto fullPath = dirPath / fileName;
				std::ofstream stream;
				stream.open(fullPath.string(), std::ios::binary);
				if (stream)
				{
					stream.write(&imgData[0], imgData.size());
				}
				stream.close();
			}
		}

		bool materialsOk = materials.size() == expMaterialsCount;
		if (!materialsOk) { repoInfo << "Expected " << expMaterialsCount << " materials, found " << materials.size(); }
		bool texturesOk = textures.size() == expTexturesCount;
		if (!materialsOk) { repoInfo << "Expected " << expTexturesCount << " textures, found " << textures.size(); }
		bool meshesOk = meshes.size() == expMeshesCount;
		if (!materialsOk) { repoInfo << "Expected " << expMeshesCount << " meshes, found " << meshes.size(); }
		if (!repoScene->isOK()) { repoInfo << "Scene is not healthy"; }

		bool scenePassed = materialsOk && texturesOk && meshesOk && repoScene->isOK();
		repoInfo << "Generated scene passed: " << std::boolalpha << scenePassed;
		return scenePassed;
	}

	return true;
};

TEST(RepoModelImport, SupportedFormats)
{
	TestLogging::printTestTitleString(
		"Supported BIM format tests",
		"Happy path tests to check for compatability with all of the supported BIM \
		file format versions");

	TestLogging::printSubTestTitleString("BIM003 file - with textures");
	EXPECT_TRUE(testBIMFileImport(
		getDataPath("RepoModelImport\\cube_bim3_revit_2021_repo_17de4e0.bim"), 4, 3, 4));

	TestLogging::printSubTestTitleString("BIM002 file");
	EXPECT_TRUE(testBIMFileImport(
		getDataPath("RepoModelImport\\cube_bim2_navis_2021_repo_4.6.1.bim"), 3, 0, 4));
}

TEST(RepoModelImport, UnsupportedFormats)
{
	TestLogging::printTestTitleString(
		"Unsupported BIM file format",
		"Checking to see if the right errors get thrown");

	TestLogging::printSubTestTitleString("BIM001 file - testing for unsupported error");
	EXPECT_TRUE(testBIMFileImport(
		getDataPath("RepoModelImport\\cube_bim1_spoofed.bim"), 0, 0, 0, REPOERR_UNSUPPORTED_BIM_VERSION, REPOERR_OK, false));
}

TEST(RepoModelImport, MissingTextureFields)
{
	TestLogging::printTestTitleString(
		"Texture fields missing in the BIM JSON",
		"Check to see if the correct error is generated");

	TestLogging::printSubTestTitleString("Missing \"numImageBytes\" field");
	EXPECT_FALSE(testBIMFileImport(
		getDataPath("RepoModelImport\\cube_bim3_corrupted_texture_field.bim"), 4, 2, 4, REPOERR_OK, REPOERR_LOAD_SCENE_MISSING_TEXTURE));
}

TEST(RepoModelImport, MissingReferencedTexture)
{
	TestLogging::printTestTitleString(
		"Materials references a missing texture",
		"Check to see if the correct error is generated");

	TestLogging::printSubTestTitleString("Material references a texture id not included in file");
	EXPECT_FALSE(testBIMFileImport(
		getDataPath("RepoModelImport\\cube_bim3_missing_texture.bim"), 4, 3, 4, REPOERR_OK, REPOERR_LOAD_SCENE_MISSING_TEXTURE));
}

