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

using namespace repo::manipulator::modelconvertor;
using namespace repo::test;

static bool testBIMFileImport(
	std::string bimFilePath,
	int expMaterialsCount,
	int expTexturesCount,
	int expMeshesCount)
{
	ModelImportConfig config;
	uint8_t errCode = 0;
	auto modelConvertor = std::unique_ptr<AbstractModelImport>(new RepoModelImport(config));
	modelConvertor->importModel(bimFilePath, errCode);
	repoInfo << "Error code from importModel(): " << (int)errCode;
	auto repoScene = modelConvertor->generateRepoScene(errCode);
	repoInfo << "Error code from generateRepoScene(): " << (int)errCode;

	auto materials = repoScene->getAllMaterials(repo::core::model::RepoScene::GraphType::DEFAULT);
	auto textures = repoScene->getAllTextures(repo::core::model::RepoScene::GraphType::DEFAULT);
	auto meshes = repoScene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT);

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
		"CHecking to see if the right errors get thrown");

	TestLogging::printSubTestTitleString("BIM001 file - testing for unsupported error");
	EXPECT_TRUE(testBIMFileImport(
		getDataPath("RepoModelImport\\cube_bim1_spoofed.bim"), 3, 0, 4));
}

TEST(RepoModelImport, MissingTextureFields)
{
	TestLogging::printTestTitleString(
		"BIM file format - standard flow",
		"Happy path tests to check for compatability with all of the BIM \
		file format versions");
}

TEST(RepoModelImport, CorruptTextureData)
{
	TestLogging::printTestTitleString(
		"BIM file format - standard flow",
		"Happy path tests to check for compatability with all of the BIM \
		file format versions");
}

TEST(RepoModelImport, MissingReferencedTexture)
{
	TestLogging::printTestTitleString(
		"BIM file format - standard flow",
		"Happy path tests to check for compatability with all of the BIM \
		file format versions");
}

