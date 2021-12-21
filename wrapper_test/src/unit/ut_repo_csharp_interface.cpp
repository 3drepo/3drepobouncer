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

// Only one test for the whole wrapper, as the current implementation is done
// via the singleton pattern, and the core repo controller state may 
// be carried over between tests.
// This test covers all the wrapper functions that are run by the asset bundle 
// creator via the `BuildAllAssetBundles` entry point
TEST(RepoCsharpInterface, FullWrapperTest)
{
	// using the read/write section of the test database
	std::string database = "stUpload";
	std::string project = "cube";
	std::string revisionID = "";
	std::string configPath = getDataPath("config/config.json");
	bool connected = repoConnect(&configPath[0]);
	ASSERT_TRUE(connected);
	bool loadedScene = repoLoadSceneForAssetBundleGeneration(
		&database[0],
		&project[0],
		&revisionID[0]);
	ASSERT_TRUE(loadedScene);
	ASSERT_FALSE(repoIsVREnabled());
	bool isFederation = repoIsFederation(
		&database[0],
		&project[0]);
	ASSERT_FALSE(isFederation);
	// main loop to access supermesh data
	int numSuperMeshes = repoGetNumSuperMeshes();
	ASSERT_EQ(numSuperMeshes, 1);
	repoInfo << "looping over supermeshes";
	for (int superMeshIdx = 0; superMeshIdx < numSuperMeshes; superMeshIdx++)
	{
		// get info regarding the current supermesh
		long numSubSupermeshes{0};
		long numVertices{0};
		long numFaces{0};
		bool hasNormals{false};
		bool hasUV{false};
		int primitiveType{0};
		int numMappings{0};
		std::string superMeshId = repoGetSuperMeshInfo(
			superMeshIdx,
			&numSubSupermeshes,
			&numVertices,
			&numFaces,
			&hasNormals,
			&hasUV,
			&primitiveType,
			&numMappings);
		ASSERT_EQ(numSubSupermeshes, 1);
		ASSERT_EQ(numVertices, 24);
		ASSERT_EQ(numFaces, 12);
		ASSERT_EQ(hasNormals, true);
		ASSERT_EQ(hasUV, false);
		ASSERT_EQ(primitiveType, 3);
		repoInfo << "supermesh id: " << superMeshId;
		// initialize and fill buffers to hold supermesh data
		std::vector<float> vertices(numVertices * 3);
		std::vector<float> normals(numVertices * 3);
		std::vector<uint16_t> faces(numFaces * primitiveType);
		std::vector<int> numSubmeshesPerSubSupermesh(numSubSupermeshes);
		std::vector<float> uvs(numVertices * 2);
		repoGetSuperMeshBuffers(
			superMeshIdx, 
			&vertices[0], 
			&normals[0], 
			&faces[0], 
			&uvs[0], 
			&numSubmeshesPerSubSupermesh[0]);
		// sub supermesh loop
		repoInfo << "looping over sub supermeshes";
		for (int subSuperMeshIdx = 0; subSuperMeshIdx < numSubSupermeshes; ++subSuperMeshIdx)
		{
			repoInfo << "sub supermesh id: " << superMeshId << "_" << subSuperMeshIdx;
			// get info regarding the sub supermesh
			int vFrom{0};
			int vTo{0};
			int fFrom{0};
			int fTo{0};
			repoGetSubMeshInfo(superMeshIdx, subSuperMeshIdx, &vFrom, &vTo, &fFrom, &fTo);
			// initialize and fill buffers to hold sub supermesh data
			int subSuerMeshNumVertices = vTo - vFrom;
			std::vector<float> buffersIds(subSuerMeshNumVertices);
			repoGetIdMapBuffer(superMeshIdx, subSuperMeshIdx, &buffersIds[0]);
			// loop over sub meshes in sub subsupermesh
			repoInfo << "looping over submeshes";
			for (int subMeshIdx = 0; subMeshIdx < numSubmeshesPerSubSupermesh[subSuperMeshIdx]; subMeshIdx++)
			{
				repoInfo << "submesh id: " << superMeshId << "_" << subSuperMeshIdx << "_" << subMeshIdx;
				int supermeshFaceFrom{0};
				int supermeshFaceTo{0};
				std::string subMeshId = repoGetOrgMeshInfoFromSubMesh(
					superMeshIdx, 
					subSuperMeshIdx, 
					subMeshIdx, 
					&supermeshFaceFrom, 
					&supermeshFaceTo);
			}
		}
		repoFreeSuperMesh(superMeshIdx);
	}
	// asset bundle save check with dummy file
	std::string dummyAssetBundlePath = getDataPath(
		"textures/brick_non_uniform_running_burgundy.png");
	char* dummyAssetBundlePathPtr = &dummyAssetBundlePath[0];
	bool assetsSavedCorrectly = repoSaveAssetBundles(&dummyAssetBundlePathPtr, 1);
	ASSERT_TRUE(assetsSavedCorrectly);
}

