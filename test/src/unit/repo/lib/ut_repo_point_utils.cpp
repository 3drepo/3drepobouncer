/**
*  Copyright (C) 2026 3D Repo Ltd
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

#include "repo/lib/point_cloud/repo_point_cloud_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include "../test/src/unit/repo_test_matchers.h"
#include "../test/src/unit/repo_test_utils.h"

using namespace repo::lib::pointcloud;
using namespace testing;

TEST(PointCloudUtilsTest, getCellCoordinateFromTreePosition)
{
	srand(time(NULL));

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Calculate number of cells per side for that split
	int noCellsPerSide = std::pow(2, splits);

	// Test 1: Check minimum corner. Should be [4,4, ... ,4]
	auto minCornerExp = std::vector<uint8_t>(splits, 4);
	auto minCornerAct = PointCloudUtils::getTreePositionFromCellCoordinates(0, 0, 0, splits);
	EXPECT_THAT(minCornerAct, minCornerExp);

	// Test 2: Check maximum corner. Should be [2,2, ... ,2]
	int maxIndex = noCellsPerSide - 1;
	auto maxCornerExp = std::vector<uint8_t>(splits, 2);
	auto maxCornerAct = PointCloudUtils::getTreePositionFromCellCoordinates(maxIndex, maxIndex, maxIndex, splits);
	EXPECT_THAT(maxCornerAct, maxCornerExp);

	// Test 3: Check a random cell

	// pick random cell on each axis
	int randX = rand() % noCellsPerSide;
	int randY = rand() % noCellsPerSide;
	int randZ = rand() % noCellsPerSide;

	// Calculate expected position
	auto randCellExp = std::vector<uint8_t>();
	int noCellsPerSideSplit = noCellsPerSide / 2.0;
	int midpointX = noCellsPerSideSplit;
	int midpointY = noCellsPerSideSplit;
	int midpointZ = noCellsPerSideSplit;
	for (int i = 0; i < splits; i++)
	{
		noCellsPerSideSplit = noCellsPerSideSplit / 2.0;

		if (randX < midpointX)
		{
			// Lower half of the x-axis
			midpointX -= noCellsPerSideSplit;
			
			if (randY < midpointY)
			{
				// Lower half of the y-axis
				midpointY -= noCellsPerSideSplit;

				if (randZ < midpointZ)
				{
					// Lower half of the z-axis
					randCellExp.push_back(4);
					midpointZ -= noCellsPerSideSplit;
				}
				else
				{
					// Upper half of the z-axis
					midpointZ += noCellsPerSideSplit;
					randCellExp.push_back(0);
				}
			}
			else
			{
				// Upper half of the y-axis
				midpointY += noCellsPerSideSplit;

				if (randZ < midpointZ)
				{
					// Lower half of the z-axis
					midpointZ -= noCellsPerSideSplit;
					randCellExp.push_back(7);
				}
				else
				{
					// Upper half of the z-axis
					midpointZ += noCellsPerSideSplit;
					randCellExp.push_back(3);
				}
			}
		}
		else
		{
			// Upper half of the x-axis
			midpointX += noCellsPerSideSplit;

			if (randY < midpointY)
			{
				// Lower half of the y-axis
				midpointY -= noCellsPerSideSplit;

				if (randZ < midpointZ)
				{
					// Lower half of the z-axis
					midpointZ -= noCellsPerSideSplit;
					randCellExp.push_back(5);
				}
				else
				{
					// Upper half of the z-axis
					midpointZ += noCellsPerSideSplit;
					randCellExp.push_back(1);
				}
			}
			else
			{
				// Upper half of the y-axis
				midpointY += noCellsPerSideSplit;

				if (randZ < midpointZ)
				{
					// Lower half of the z-axis
					midpointZ -= noCellsPerSideSplit;
					randCellExp.push_back(6);
				}
				else
				{
					// Upper half of the z-axis
					midpointZ += noCellsPerSideSplit;
					randCellExp.push_back(2);
				}
			}
		}
	}

	auto randCellAct = PointCloudUtils::getTreePositionFromCellCoordinates(randX, randY, randZ, splits);
	EXPECT_THAT(randCellAct, randCellExp);
}

TEST(PointCloudUtilsTest, GetIndexFromCellCoordinates)
{
	srand(time(NULL));

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Calculate number of cells per side for that split
	int noCellsPerSide = std::pow(2, splits);

	// Test 1: Check minimum corner. Should be 0
	auto minCornerExp = 0;
	auto minCornerAct = PointCloudUtils::getIndexFromCellCoordinates(0, 0, 0, splits);
	EXPECT_THAT(minCornerAct, minCornerExp);

	// Test 2: Check maximum corner. Should be noCellsPerSide^3
	int maxIndex = noCellsPerSide - 1;
	auto maxCornerExp = std::pow(noCellsPerSide, 3) - 1;
	auto maxCornerAct = PointCloudUtils::getIndexFromCellCoordinates(maxIndex, maxIndex, maxIndex, splits);
	EXPECT_THAT(maxCornerAct, maxCornerExp);

	// Test 3: Check a random cell

	// pick random cell on each axis
	int randX = rand() % noCellsPerSide;
	int randY = rand() % noCellsPerSide;
	int randZ = rand() % noCellsPerSide;

	// Calculate expected position
	auto randCellExp = (randX * std::pow(noCellsPerSide, 2)) + (randY * noCellsPerSide) + randZ;

	auto randCellAct = PointCloudUtils::getIndexFromCellCoordinates(randX, randY, randZ, splits);
	EXPECT_THAT(randCellAct, randCellExp);
}

TEST(PointCloudUtilsTest, GetIndexFromTreePosition)
{
	srand(time(NULL));

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Calculate number of cells per side for that split
	int noCellsPerSide = std::pow(2, splits);

	// Test 1: Check minimum corner
	auto minPos = std::vector<uint8_t>(splits, 4);
	int minCornerExp = 0;
	auto minCornerAct = PointCloudUtils::getIndexFromTreePosition(minPos, splits);
	EXPECT_THAT(minCornerAct, Eq(minCornerExp));

	// Test 2: Check maximum corner
	auto maxPos = std::vector<uint8_t>(splits, 2);
	auto maxCornerExp = std::pow(noCellsPerSide, 3) - 1;
	auto maxCornerAct = PointCloudUtils::getIndexFromTreePosition(maxPos, splits);
	EXPECT_THAT(maxCornerAct, Eq(maxCornerExp));

	// Test 3: Check a random tree position

	// Create random tree pos
	auto randPos = std::vector<uint8_t>();
	for (int i = 0; i < splits; i++)
	{
		int randChildIndex = rand() % 8;
		randPos.push_back(randChildIndex);
	}

	// Calculate expected index
	int xPos = 0;
	int yPos = 0;
	int zPos = 0;
	int noCellsPerSideSplit = noCellsPerSide;
	for (int i = 0; i < splits; i++)
	{
		noCellsPerSideSplit = noCellsPerSideSplit / 2;

		switch (randPos[i])
		{
		case 0:
			// xPos = xPos;
			// yPos = yPos;
			zPos = zPos + noCellsPerSideSplit;
			break;
		case 1:
			xPos = xPos + noCellsPerSideSplit;
			// yPos = yPos;
			zPos = zPos + noCellsPerSideSplit;
			break;
		case 2:
			xPos = xPos + noCellsPerSideSplit;
			yPos = yPos + noCellsPerSideSplit;
			zPos = zPos + noCellsPerSideSplit;
			break;
		case 3:
			// xPos = xPos;
			yPos = yPos + noCellsPerSideSplit;
			zPos = zPos + noCellsPerSideSplit;
			break;
		case 4:
			//xPos = xPos;
			//yPos = yPos;
			//zPos = zPos;
			break;
		case 5:
			xPos = xPos + noCellsPerSideSplit;
			//yPos = yPos;
			//zPos = zPos;
			break;
		case 6:
			xPos = xPos + noCellsPerSideSplit;
			yPos = yPos + noCellsPerSideSplit;
			//zPos = zPos;
			break;
		case 7:
			xPos = xPos;
			yPos = yPos + noCellsPerSideSplit;
			//zPos = zPos;
			break;
		}
	}
	int randIndexExp = xPos * (noCellsPerSide * noCellsPerSide) + yPos * noCellsPerSide + zPos;
	int randIndexAct = PointCloudUtils::getIndexFromTreePosition(randPos, splits);
	EXPECT_THAT(randIndexAct, Eq(randIndexExp));
}

TEST(PointCloudUtilsTest, GetBoundsFromTreePosition)
{
	srand(time(NULL));

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto randomBounds = repo::lib::RepoBounds(randV1, randV2);

	// Calculate length of the edge of the bounds at the lowest level
	double boundsLength = (dims.x / std::pow(2, splits));
	repoError << boundsLength << " | " << splits;

	// Test 1: Check minimum corner
	auto minPos = std::vector<uint8_t>(splits, 4);
	repo::lib::RepoVector3D64 minCornerMin = randV1;
	repo::lib::RepoVector3D64 minCornerMax = minCornerMin + repo::lib::RepoVector3D64(boundsLength, boundsLength, boundsLength);
	auto minCornerExp = repo::lib::RepoBounds(minCornerMin, minCornerMax);
	auto minCornerAct = PointCloudUtils::getBoundsFromTreePosition(minPos, randomBounds);
	EXPECT_THAT(minCornerAct, Eq(minCornerExp));


	// Test 2: Check maximum corner
	auto maxPos = std::vector<uint8_t>(splits, 2);
	repo::lib::RepoVector3D64 maxCornerMax = randV2;
	repo::lib::RepoVector3D64 maxCornerMin = maxCornerMax - repo::lib::RepoVector3D64(boundsLength, boundsLength, boundsLength);
	auto maxCornerExp = repo::lib::RepoBounds(maxCornerMin, maxCornerMax);
	auto maxCornerAct = PointCloudUtils::getBoundsFromTreePosition(maxPos, randomBounds);
	EXPECT_THAT(maxCornerAct, Eq(maxCornerExp));

	// Test 3: Check a random tree position
	
	// Create random tree pos
	auto randPos = std::vector<uint8_t>();
	for (int i = 0; i < splits; i++)
	{
		int randChildIndex =rand() % 8;
		randPos.push_back(randChildIndex);
	}

	// Calculated expected bounds
	repo::lib::RepoVector3D64 randMin = randV1;
	repo::lib::RepoVector3D64 randMax = randV2;
	repo::lib::RepoVector3D64 localDims = dims;
	for (int i = 0; i < splits; i++)
	{
		localDims = localDims / 2;

		switch (randPos[i])
		{
		case 0:
			randMin = randMin + (localDims * repo::lib::RepoVector3D64(0, 0, 1));
			randMax = randMax - (localDims * repo::lib::RepoVector3D64(1, 1, 0));
			break;
		case 1:
			randMin = randMin + (localDims * repo::lib::RepoVector3D64(1, 0, 1));
			randMax = randMax - (localDims * repo::lib::RepoVector3D64(0, 1, 0));
			break;
		case 2:
			randMin = randMin + (localDims * repo::lib::RepoVector3D64(1, 1, 1));
			randMax = randMax - (localDims * repo::lib::RepoVector3D64(0, 0, 0));
			break;
		case 3:
			randMin = randMin + (localDims * repo::lib::RepoVector3D64(0, 1, 1));
			randMax = randMax - (localDims * repo::lib::RepoVector3D64(1, 0, 0));
			break;
		case 4:
			randMin = randMin + (localDims * repo::lib::RepoVector3D64(0, 0, 0));
			randMax = randMax - (localDims * repo::lib::RepoVector3D64(1, 1, 1));
			break;
		case 5:
			randMin = randMin + (localDims * repo::lib::RepoVector3D64(1, 0, 0));
			randMax = randMax - (localDims * repo::lib::RepoVector3D64(0, 1, 1));
			break;
		case 6:
			randMin = randMin + (localDims * repo::lib::RepoVector3D64(1, 1, 0));
			randMax = randMax - (localDims * repo::lib::RepoVector3D64(0, 0, 1));
			break;
		case 7:
			randMin = randMin + (localDims * repo::lib::RepoVector3D64(0, 1, 0));
			randMax = randMax - (localDims * repo::lib::RepoVector3D64(1, 0, 1));
			break;
		}
	}
	auto randBoundsExp = repo::lib::RepoBounds(randMin, randMax);
	auto randBoundsAct = PointCloudUtils::getBoundsFromTreePosition(randPos, randomBounds);
	EXPECT_THAT(randBoundsAct, Eq(randBoundsExp));
}

TEST(PointCloudUtilsTest, Project3DPositionToCellCoordinates)
{
	srand(time(NULL));

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Calculate number of cells per side for that split
	int noCellsPerSide = std::pow(2, splits);

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto randomBounds = repo::lib::RepoBounds(randV1, randV2);

	// Test 1: Check minimum corner
	int minCornerActX = -1;
	int minCornerActY = -1;
	int minCornerActZ = -1;
	PointCloudUtils::project3DPositionToCellCoordinates(
		randV1,
		randomBounds,
		splits,
		minCornerActX,
		minCornerActY,
		minCornerActZ);
	EXPECT_THAT(minCornerActX, Eq(0));
	EXPECT_THAT(minCornerActY, Eq(0));
	EXPECT_THAT(minCornerActZ, Eq(0));


	// Test 2: Check maximum corner
	int maxCornerActX = -1;
	int maxCornerActY = -1;
	int maxCornerActZ = -1;
	int maxIndex = noCellsPerSide - 1;
	PointCloudUtils::project3DPositionToCellCoordinates(
		randV2,
		randomBounds,
		splits,
		maxCornerActX,
		maxCornerActY,
		maxCornerActZ);
	EXPECT_THAT(maxCornerActX, Eq(maxIndex));
	EXPECT_THAT(maxCornerActY, Eq(maxIndex));
	EXPECT_THAT(maxCornerActZ, Eq(maxIndex));


	// Test 3: Check a random position

	// Create random position
	repo::lib::RepoVector3D64 randPos = makeRandomRepoVector(randomBounds);

	// Calculate expected position
	double cellSize = randLength / noCellsPerSide;
	int randPosExpX = floor((randPos.x - randV1.x) / cellSize);
	randPosExpX = std::min(randPosExpX, noCellsPerSide);
	int randPosExpY = floor((randPos.y - randV1.y) / cellSize);
	randPosExpY = std::min(randPosExpY, noCellsPerSide);
	int randPosExpZ = floor((randPos.z - randV1.z) / cellSize);
	randPosExpZ = std::min(randPosExpZ, noCellsPerSide);

	// Calculate actual position
	int randPosActX = -1;
	int randPosActY = -1;
	int randPosActZ = -1;
	PointCloudUtils::project3DPositionToCellCoordinates(
		randPos,
		randomBounds,
		splits,
		randPosActX,
		randPosActY,
		randPosActZ);

	// Compare
	EXPECT_THAT(randPosActX, Eq(randPosExpX));
	EXPECT_THAT(randPosActY, Eq(randPosExpY));
	EXPECT_THAT(randPosActZ, Eq(randPosExpZ));
}

TEST(PointCloudUtilsTest, Project3DPositionToCellIndex)
{
	srand(time(NULL));

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Calculate number of cells per side for that split
	int noCellsPerSide = std::pow(2, splits);

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto randomBounds = repo::lib::RepoBounds(randV1, randV2);

	// Test 1: Check minimum corner
	int minCornerAct = PointCloudUtils::project3DPositionToCellIndex(
		randV1,
		randomBounds,
		splits);
	EXPECT_THAT(minCornerAct, Eq(0));


	// Test 2: Check maximum corner
	int maxCornerAct = PointCloudUtils::project3DPositionToCellIndex(
		randV2,
		randomBounds,
		splits);
	EXPECT_THAT(maxCornerAct, Eq(std::pow(noCellsPerSide, 3) - 1));


	// Test 3: Check a random position

	// Create random position
	repo::lib::RepoVector3D64 randPos = makeRandomRepoVector(randomBounds);

	// Calculate expected position
	double cellSize = randLength / noCellsPerSide;
	int randPosExpX = floor((randPos.x - randV1.x) / cellSize);
	randPosExpX = std::min(randPosExpX, noCellsPerSide);
	int randPosExpY = floor((randPos.y - randV1.y) / cellSize);
	randPosExpY = std::min(randPosExpY, noCellsPerSide);
	int randPosExpZ = floor((randPos.z - randV1.z) / cellSize);
	randPosExpZ = std::min(randPosExpZ, noCellsPerSide);
	int randPosExp = randPosExpX * (noCellsPerSide * noCellsPerSide) + randPosExpY * noCellsPerSide + randPosExpZ;

	// Calculate actual position
	int randPosAct = PointCloudUtils::project3DPositionToCellIndex(
		randPos,
		randomBounds,
		splits);

	// Compare
	EXPECT_THAT(randPosAct, Eq(randPosExp));
}

TEST(PointCloudUtilsTest, Project3DPositionToTreePosition)
{
	srand(time(NULL));

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Calculate number of cells per side for that split
	int noCellsPerSide = std::pow(2, splits);

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto randomBounds = repo::lib::RepoBounds(randV1, randV2);

	// Test 1: Check minimum corner
	auto minCornerExp = std::vector<uint8_t>(splits, 4);
	auto minCornerAct = PointCloudUtils::project3DPositionToTreePosition(
		randV1,
		randomBounds,
		splits);
	EXPECT_THAT(minCornerAct, Eq(minCornerExp));


	// Test 2: Check maximum corner
	auto maxCornerExp = std::vector<uint8_t>(splits, 2);
	auto maxCornerAct = PointCloudUtils::project3DPositionToTreePosition(
		randV2,
		randomBounds,
		splits);
	EXPECT_THAT(maxCornerAct, Eq(maxCornerExp));


	// Test 3: Check a random position

	// Create random position
	repo::lib::RepoVector3D64 randPos = makeRandomRepoVector(randomBounds);

	// Calculate expected position
	double cellSize = randLength / noCellsPerSide;
	int randPosExpX = floor((randPos.x - randV1.x) / cellSize);
	randPosExpX = std::min(randPosExpX, noCellsPerSide);
	int randPosExpY = floor((randPos.y - randV1.y) / cellSize);
	randPosExpY = std::min(randPosExpY, noCellsPerSide);
	int randPosExpZ = floor((randPos.z - randV1.z) / cellSize);
	randPosExpZ = std::min(randPosExpZ, noCellsPerSide);
	
	auto randPosExp = std::vector<uint8_t>();
	int noCellsPerSideSplit = noCellsPerSide / 2.0;
	int midpointX = noCellsPerSideSplit;
	int midpointY = noCellsPerSideSplit;
	int midpointZ = noCellsPerSideSplit;
	for (int i = 0; i < splits; i++)
	{
		noCellsPerSideSplit = noCellsPerSideSplit / 2.0;

		if (randPosExpX < midpointX)
		{
			// Lower half of the x-axis
			midpointX -= noCellsPerSideSplit;

			if (randPosExpY < midpointY)
			{
				// Lower half of the y-axis
				midpointY -= noCellsPerSideSplit;

				if (randPosExpZ < midpointZ)
				{
					// Lower half of the z-axis
					randPosExp.push_back(4);
					midpointZ -= noCellsPerSideSplit;
				}
				else
				{
					// Upper half of the z-axis
					midpointZ += noCellsPerSideSplit;
					randPosExp.push_back(0);
				}
			}
			else
			{
				// Upper half of the y-axis
				midpointY += noCellsPerSideSplit;

				if (randPosExpZ < midpointZ)
				{
					// Lower half of the z-axis
					midpointZ -= noCellsPerSideSplit;
					randPosExp.push_back(7);
				}
				else
				{
					// Upper half of the z-axis
					midpointZ += noCellsPerSideSplit;
					randPosExp.push_back(3);
				}
			}
		}
		else
		{
			// Upper half of the x-axis
			midpointX += noCellsPerSideSplit;

			if (randPosExpY < midpointY)
			{
				// Lower half of the y-axis
				midpointY -= noCellsPerSideSplit;

				if (randPosExpZ < midpointZ)
				{
					// Lower half of the z-axis
					midpointZ -= noCellsPerSideSplit;
					randPosExp.push_back(5);
				}
				else
				{
					// Upper half of the z-axis
					midpointZ += noCellsPerSideSplit;
					randPosExp.push_back(1);
				}
			}
			else
			{
				// Upper half of the y-axis
				midpointY += noCellsPerSideSplit;

				if (randPosExpZ < midpointZ)
				{
					// Lower half of the z-axis
					midpointZ -= noCellsPerSideSplit;
					randPosExp.push_back(6);
				}
				else
				{
					// Upper half of the z-axis
					midpointZ += noCellsPerSideSplit;
					randPosExp.push_back(2);
				}
			}
		}
	}

	// Calculate actual position
	auto randPosAct = PointCloudUtils::project3DPositionToTreePosition(
		randPos,
		randomBounds,
		splits);

	// Compare
	EXPECT_THAT(randPosAct, Eq(randPosExp));
}

TEST(PointCloudUtilsTest, GetChildBoundsByIndex)
{
	srand(time(NULL));

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto randomBounds = repo::lib::RepoBounds(randV1, randV2);
	
	// Split is numbered as such:
	// ------------------------------ x
	// |							|
	// |	0/4		|		1/5		|
	// |							|
	// |----------------------------|
	// |							|
	// |	3/7		|		2/6		|
	// |							|
	// ------------------------------
	// y
	// 
	// 0 to 3 are the top level, 4 to 7 are the bottom level

	// Create children bounds according to this scheme
	dims = dims / 2.0;

	// Child 0 bounds:
	auto min0 = randV1 + (dims * repo::lib::RepoVector3D64(0, 0, 1));
	auto max0 = randV2 - (dims * repo::lib::RepoVector3D64(1, 1, 0));
	auto boundsExp0 = repo::lib::RepoBounds(min0, max0);

	// Child 1 bounds:
	auto min1 = randV1 + (dims * repo::lib::RepoVector3D64(1, 0, 1));
	auto max1 = randV2 - (dims * repo::lib::RepoVector3D64(0, 1, 0));
	auto boundsExp1 = repo::lib::RepoBounds(min1, max1);

	// Child 2 bounds:
	auto min2 = randV1 + (dims * repo::lib::RepoVector3D64(1, 1, 1));
	auto max2 = randV2 - (dims * repo::lib::RepoVector3D64(0, 0, 0));
	auto boundsExp2 = repo::lib::RepoBounds(min2, max2);

	// Child 3 bounds:
	auto min3 = randV1 + (dims * repo::lib::RepoVector3D64(0, 1, 1));
	auto max3 = randV2 - (dims * repo::lib::RepoVector3D64(1, 0, 0));
	auto boundsExp3 = repo::lib::RepoBounds(min3, max3);

	// Child 4 bounds:
	auto min4 = randV1 + (dims * repo::lib::RepoVector3D64(0, 0, 0));
	auto max4 = randV2 - (dims * repo::lib::RepoVector3D64(1, 1, 1));
	auto boundsExp4 = repo::lib::RepoBounds(min4, max4);

	// Child 5 bounds:
	auto min5 = randV1 + (dims * repo::lib::RepoVector3D64(1, 0, 0));
	auto max5 = randV2 - (dims * repo::lib::RepoVector3D64(0, 1, 1));
	auto boundsExp5 = repo::lib::RepoBounds(min5, max5);

	// Child 6 bounds:
	auto min6 = randV1 + (dims * repo::lib::RepoVector3D64(1, 1, 0));
	auto max6 = randV2 - (dims * repo::lib::RepoVector3D64(0, 0, 1));
	auto boundsExp6 = repo::lib::RepoBounds(min6, max6);

	// Child 7 bounds:
	auto min7 = randV1 + (dims * repo::lib::RepoVector3D64(0, 1, 0));
	auto max7 = randV2 - (dims * repo::lib::RepoVector3D64(1, 0, 1));
	auto boundsExp7 = repo::lib::RepoBounds(min7, max7);

	// Get actual bounds
	auto boundsAct0 = PointCloudUtils::getChildBoundsByIndex(randomBounds, 0);
	auto boundsAct1 = PointCloudUtils::getChildBoundsByIndex(randomBounds, 1);
	auto boundsAct2 = PointCloudUtils::getChildBoundsByIndex(randomBounds, 2);
	auto boundsAct3 = PointCloudUtils::getChildBoundsByIndex(randomBounds, 3);
	auto boundsAct4 = PointCloudUtils::getChildBoundsByIndex(randomBounds, 4);
	auto boundsAct5 = PointCloudUtils::getChildBoundsByIndex(randomBounds, 5);
	auto boundsAct6 = PointCloudUtils::getChildBoundsByIndex(randomBounds, 6);
	auto boundsAct7 = PointCloudUtils::getChildBoundsByIndex(randomBounds, 7);

	// Compare expected and actual
	EXPECT_THAT(boundsExp0, Eq(boundsAct0));
	EXPECT_THAT(boundsExp1, Eq(boundsAct1));
	EXPECT_THAT(boundsExp2, Eq(boundsAct2));
	EXPECT_THAT(boundsExp3, Eq(boundsAct3));
	EXPECT_THAT(boundsExp4, Eq(boundsAct4));
	EXPECT_THAT(boundsExp5, Eq(boundsAct5));
	EXPECT_THAT(boundsExp6, Eq(boundsAct6));
	EXPECT_THAT(boundsExp7, Eq(boundsAct7));
}

TEST(PointCloudUtilsTest, ConsistencyTest)
{
	// This test does not test for the correctness of the individual methods, but will
	// test for their consistency with each other.

	srand(time(NULL));

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Calculate number of cells per side for that split
	int noCellsPerSide = std::pow(2, splits);

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto randomBounds = repo::lib::RepoBounds(randV1, randV2);

	// Create random tree pos
	auto randPos = std::vector<uint8_t>();
	for (int i = 0; i < splits; i++)
	{
		int randChildIndex = rand() % 8;
		randPos.push_back(randChildIndex);
	}

	// Tree pos -> child bounds -> 3D Pos project -> Tree pos
	{
		auto childBounds = PointCloudUtils::getBoundsFromTreePosition(randPos, randomBounds);
		auto minPos = childBounds.min();
		auto maxPos = childBounds.max();
		auto centrePos = childBounds.center();

		// min pos needs to be slightly altered since the bounds are not actually always inclusive of the lower bound
		// to avoid unclear behaviour when a point falls on the mathematical boundary
		// (For more detail see description in repo_point_cloud_utils.h)
		minPos.x = std::nextafter(minPos.x, std::numeric_limits<double>::infinity());
		minPos.y = std::nextafter(minPos.y, std::numeric_limits<double>::infinity());
		minPos.z = std::nextafter(minPos.z, std::numeric_limits<double>::infinity());

		EXPECT_THAT(PointCloudUtils::project3DPositionToTreePosition(minPos, randomBounds, splits), Eq(randPos));
		EXPECT_THAT(PointCloudUtils::project3DPositionToTreePosition(maxPos, randomBounds, splits), Eq(randPos));
		EXPECT_THAT(PointCloudUtils::project3DPositionToTreePosition(centrePos, randomBounds, splits), Eq(randPos));
	}

	// getBoundsFromTreePosition vs. getChildBoundsByIndex
	for (int i = 0; i < 8; i++)
	{
		auto childTreePos = std::vector<uint8_t>(1, i);
		auto childBounds1 = PointCloudUtils::getBoundsFromTreePosition(childTreePos, randomBounds);
		auto childBounds2 = PointCloudUtils::getChildBoundsByIndex(randomBounds, i);
		EXPECT_THAT(childBounds1, Eq(childBounds2));
	}
}

TEST(PointCloudUtilsTest, CombineTreePositionsTest)
{
	{
		auto a = std::vector<uint8_t>();
		auto b = std::vector<uint8_t>();

		auto c = PointCloudUtils::combineTreePositions(a, b);
		
		EXPECT_THAT(c.size(), Eq(0));
	}

	{
		auto a = std::vector<uint8_t>();
		auto b = std::vector<uint8_t>();

		a.push_back(1);
		a.push_back(2);

		b.push_back(3);

		auto c = PointCloudUtils::combineTreePositions(a, b);

		EXPECT_THAT(c.size(), Eq(3));
		EXPECT_THAT(c[0], Eq(1));
		EXPECT_THAT(c[1], Eq(2));
		EXPECT_THAT(c[2], Eq(3));
	}

	{
		auto a = std::vector<uint8_t>();
		auto b = std::vector<uint8_t>();

		b.push_back(1);
		b.push_back(2);

		a.push_back(3);

		auto c = PointCloudUtils::combineTreePositions(a, b);

		EXPECT_THAT(c.size(), Eq(3));
		EXPECT_THAT(c[0], Eq(3));
		EXPECT_THAT(c[1], Eq(1));
		EXPECT_THAT(c[2], Eq(2));
	}

	{
		auto a = std::vector<uint8_t>();
		auto b = std::vector<uint8_t>();

		a.push_back(1);
		a.push_back(2);

		auto c = PointCloudUtils::combineTreePositions(a, b);

		EXPECT_THAT(c.size(), Eq(2));
		EXPECT_THAT(c[0], Eq(1));
		EXPECT_THAT(c[1], Eq(2));
	}

	{
		auto a = std::vector<uint8_t>();
		auto b = std::vector<uint8_t>();

		b.push_back(3);

		auto c = PointCloudUtils::combineTreePositions(a, b);

		EXPECT_THAT(c.size(), Eq(1));
		EXPECT_THAT(c[0], Eq(3));
	}
}