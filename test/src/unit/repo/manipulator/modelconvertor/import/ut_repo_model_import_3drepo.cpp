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
#include <repo_log.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_database_info.h"
#include "boost/filesystem.hpp"
#include "../../bouncer/src/repo/error_codes.h"

using namespace repo::manipulator::modelconvertor;
using namespace testing;

namespace RepoModelImportUtils
{
	static std::unique_ptr<AbstractModelImport> ImportBIMFile(
		std::string bimFilePath,
		uint8_t& impModelErrCode)
	{
		ModelImportConfig config;
		auto modelConvertor = std::unique_ptr<AbstractModelImport>(new RepoModelImport(config));
		modelConvertor->importModel(bimFilePath, impModelErrCode);
		return modelConvertor;
	}

	static bool CheckUVs(const repo::core::model::RepoNodeSet& meshes)
	{
		for (auto const mesh : meshes)
		{
			auto meshNode = static_cast<repo::core::model::MeshNode*>(mesh);
			std::vector<repo::lib::RepoVector2D> uvs = meshNode->getUVChannelsSerialised();
			std::vector<repo::lib::RepoVector3D> vertices = meshNode->getVertices();
			if (uvs.size() != vertices.size())
			{
				repoError << "UV count mesh is incorrect. Expected " << vertices.size() << ", found : " << uvs.size();
				return false;
			}
		}
		return true;
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

