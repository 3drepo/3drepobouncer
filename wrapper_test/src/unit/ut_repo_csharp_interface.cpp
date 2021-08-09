/**
*  Copyright (C) 2021 3D Repo Ltd
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
#include <iostream>
#include "repo_csharp_interface.h"
#include <boost/filesystem.hpp>

static std::string getDataPath(
	const std::string& file)
{
	char* pathChr = getenv("REPO_MODEL_PATH");
	std::string returnPath = "";

	if (pathChr)
	{
		std::string path = pathChr;
		path.erase(std::remove(path.begin(), path.end(), '"'), path.end());
		boost::filesystem::path fileDir(path);
		auto fullPath = fileDir / boost::filesystem::path(file);
		returnPath = fullPath.string();
	}
	return returnPath;
}

TEST(RepoCsharpInterface, LoadScene)
{
	// using the readly ony database so the project should be in 
	// the test db, even if a round of tests has already been run on it
	std::string database = "sampleDataReadOnly";
	std::string project = "3drepoBIM";
	std::string revisionID = "5be1aca9-e4d0-4cec-987d-80d2fde3dade";
	std::string configPath = getDataPath("config/config.json");
	bool connected = repoConnect(&configPath[0]);
	// check connection
	ASSERT_TRUE(connected);
	bool loadedScene = repoLoadSceneForAssetBundleGeneration(
		&database[0],
		&project[0],
		&revisionID[0]);
	// check scene loaded
	ASSERT_TRUE(loadedScene);
	// vr enabled check
	ASSERT_FALSE(repoIsVREnabled());
	// super mesh count
	int numSuperMeshes = repoGetNumSuperMeshes();
	ASSERT_EQ(4, numSuperMeshes);
	// federation check
	bool isFederation = repoIsFederation(
		&database[0],
		&project[0]);
	ASSERT_FALSE(isFederation);
}

TEST(RepoCsharpInterface, AssetBundleSave)
{
	// using the read/write database
	std::string database = "stUpload";
	std::string project = "cube";
	std::string revisionID = "82272074-74d1-4e1a-bdaf-a6fa39af227f";
	std::string configPath = getDataPath("config/config.json");
	bool connected = repoConnect(&configPath[0]);
	// check connection
	ASSERT_TRUE(connected);
	// load scene
	bool loadedScene = repoLoadSceneForAssetBundleGeneration(
		&database[0],
		&project[0],
		&revisionID[0]);
	// check scene loaded
	ASSERT_TRUE(loadedScene);
	// asset bundle save check
	std::string dummyAssetBundlePath = getDataPath(
		"textures/brick_non_uniform_running_burgundy.png");
	char* dummyAssetBundlePathPtr = &dummyAssetBundlePath[0];
	bool assetsSavedCorrectly = repoSaveAssetBundles(&dummyAssetBundlePathPtr, 1);
	ASSERT_TRUE(assetsSavedCorrectly);
}