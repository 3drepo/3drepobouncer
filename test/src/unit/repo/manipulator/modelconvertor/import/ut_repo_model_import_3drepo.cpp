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
#include "../../bouncer/src/repo/error_codes.h"

using namespace repo::manipulator::modelconvertor;

namespace RepoModelImportUtils
{
	static std::unique_ptr<AbstractModelImport> ImportBIMFile(
		std::string bimFilePath, 
		uint8_t& impModelErrCode)
	{
		ModelImportConfig config;
		auto modelConvertor = std::unique_ptr<AbstractModelImport>(new RepoModelImport(config));
		repoWarning << "ImportBIMFile before bimFilePath:" << bimFilePath << ", impModelErrCode : " << impModelErrCode;
		modelConvertor->importModel(bimFilePath, impModelErrCode);
		repoWarning << "ImportBIMFile after bimFilePath:" << bimFilePath << ", impModelErrCode : " << impModelErrCode;
		return modelConvertor;
	}

	static bool CheckUVs(const repo::core::model::RepoNodeSet& meshes)
	{
		for (auto const mesh : meshes)
		{
			auto meshNode = static_cast<repo::core::model::MeshNode*>(mesh);
			std::vector<repo::lib::RepoVector2D> uvs = meshNode->getUVChannels();
			std::vector<repo::lib::RepoVector3D> vertices = meshNode->getVertices();
			if (uvs.size() != vertices.size())
			{
				repoError << "UV count mesh is incorrect. Expected " << vertices.size() << ", found : " << uvs.size();
				return false;
			}
		}
		return true;
	}

	static void PrintDebugMesh(std::string debugMeshDataFilePath, const repo::core::model::RepoNodeSet& meshes)
	{
		if (debugMeshDataFilePath != "")
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
						if (uvIt == (uvs.end() - 1)) { lastsep = '\n'; }
						stream << uvIt->x << ",";
						stream << uvIt->y << lastsep;
					}
				}
			}
			stream.close();
		}
	}

	static void DumpTextureFileImage(
		std::string textureFilesDumpPath, 
		const repo::core::model::RepoNodeSet& textures)
	{
		for (auto const img : textures)
		{
			auto textureNode = static_cast<repo::core::model::TextureNode*>(img);
			std::vector<char> imgData = textureNode->getRawData();
			boost::filesystem::path dirPath(textureFilesDumpPath);
			boost::filesystem::path fileName("DUMP_" + textureNode->getName());
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
}

TEST(RepoModelImport, ValidBIM001Import)
{
	uint8_t errCode = 0;
	RepoModelImportUtils::ImportBIMFile(
		getDataPath("RepoModelImport/cube_bim1_spoofed.bim"), errCode);
	EXPECT_EQ(REPOERR_UNSUPPORTED_BIM_VERSION, errCode);
}

TEST(RepoModelImport, ValidBIM002Import)
{
	uint8_t errCode = 0;
	RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/cube_bim2_navis_2021_repo_4.6.1.bim"), errCode);
	EXPECT_EQ(REPOERR_OK, errCode);
}

TEST(RepoModelImport, ValidBIM003Import)
{
	uint8_t errCode = 0;
	RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/BrickWalls_bim3.bim"), errCode);
	EXPECT_EQ(REPOERR_OK, errCode);
}

TEST(RepoModelImport, ValidBIM004Import)
{
	uint8_t errCode = 0;
	RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/wall_section_bim4.bim"), errCode);
	EXPECT_EQ(REPOERR_OK, errCode);
}

TEST(RepoModelImport, MissingTextureFields)
{
	uint8_t errCode = 0;
	RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/BrickWalls_bim3_CorruptedTextureField.bim"), errCode);
	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE, errCode);
}

TEST(RepoModelImport, EmptyTextureByteCount)
{
	uint8_t errCode = 0;
	RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/missingTextureByteCount.bim"), errCode);
	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE, errCode);
}

TEST(RepoModelImport, UnreferencedTextures)
{
	uint8_t errCode = 0;
	RepoModelImportUtils::ImportBIMFile(getDataPath("RepoModelImport/BrickWalls_bim3_MissingTexture.bim"), errCode);
	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE, errCode);
}

