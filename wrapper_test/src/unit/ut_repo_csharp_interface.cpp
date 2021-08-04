/**
*  Copyright (C) 2015 3D Repo Ltd
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

// Assumes that the test database has been reset to its original 
// state. It will change the state of the database (adds a dummy file)
TEST(RepoCsharpInterface, ConfirmingProjectEntryInDatabase)
{
	std::string database = "sampleDataRW";
	std::string project = "cube";
	std::string revisionID = "";
	std::string configPath = getDataPath("config/config.json");
	bool connected = repoConnect(&configPath[0]);
	// check connection
	EXPECT_TRUE(connected);
	if (!connected) return;
	bool loadedScene = repoLoadSceneForAssetBundleGeneration(
		&database[0],
		&project[0],
		&revisionID[0]);
	// check scene loaded
	EXPECT_TRUE(loadedScene);
	// vr enabled check
	EXPECT_FALSE(repoIsVREnabled());
	// super mesh count
	int numSuperMeshes = repoGetNumSuperMeshes();
	EXPECT_EQ(numSuperMeshes, 1);
	// federation check
	bool isFederation = repoIsFederation(
		&database[0],
		&project[0]);
	EXPECT_FALSE(isFederation);
	// asset bundle save check
	std::string dummyAssetBundlePath = getDataPath(
		"textures/brick_non_uniform_running_burgundy.png");
	char* dummyAssetBundlePathPtr = &dummyAssetBundlePath[0];
	bool assetsSavedCorrectly = repoSaveAssetBundles(&dummyAssetBundlePathPtr, 1);
}