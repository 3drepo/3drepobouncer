/**
*  Copyright (C) 2016 3D Repo Ltd
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

#include <cstdlib>
#include <sstream>
#include <repo/lib/datastructure/repo_vector.h>
#include <gtest/gtest.h>

#include "../../repo_test_utils.h"

using namespace repo::lib;

TEST(RepoVector2DTest, constructorTest)
{
	RepoVector2D defaultV;
	EXPECT_EQ(0, defaultV.x);
	EXPECT_EQ(0, defaultV.y);

	float x = (rand()%1000) / 1000.f;
	float y = (rand() % 1000) / 1000.f;
	RepoVector2D randomV(x, y);
	EXPECT_EQ(x, randomV.x);
	EXPECT_EQ(y, randomV.y);

	std::vector<float> arr = {x, y};
	RepoVector2D randomV2(arr);
	EXPECT_EQ(x, randomV2.x);
	EXPECT_EQ(y, randomV2.y);

	std::vector<float> arr1 = { x};
	RepoVector2D randomV3(arr1);
	EXPECT_EQ(x, randomV3.x);
	EXPECT_EQ(0, randomV3.y);

	std::vector<float> arr2;
	RepoVector2D randomV4(arr2);
	EXPECT_EQ(0, randomV4.x);
	EXPECT_EQ(0, randomV4.y);
}

TEST(RepoVector2DTest, assignmentOpTest)
{
	RepoVector2D defaultV;

	float x = (rand() % 1000) / 1000.f;
	float y = (rand() % 1000) / 1000.f;
	RepoVector2D randomV(x, y);
	
	defaultV = randomV;
	EXPECT_EQ(randomV.x, defaultV.x);
	EXPECT_EQ(randomV.y, defaultV.y);
}

TEST(RepoVector2DTest, toStringTest)
{
	RepoVector2D defaultV, defaultV2;

	float x = (rand() % 1000) / 1000.f;
	float y = (rand() % 1000) / 1000.f;
	RepoVector2D randomV(x, y);

	EXPECT_EQ(defaultV.toString(), defaultV.toString());
	EXPECT_EQ(defaultV2.toString(), defaultV2.toString());
	EXPECT_EQ(randomV.toString(), randomV.toString());
	EXPECT_NE(defaultV2.toString(), randomV.toString());
}

TEST(RepoVector2DTest, toStdVecTest)
{
	RepoVector2D defaultV, defaultV2;

	float x = (rand() % 1000) / 1000.f;
	float y = (rand() % 1000) / 1000.f;
	RepoVector2D randomV(x, y);
	std::vector<float> defaultVArr = { 0, 0 };
	std::vector<float> randomVArr = { x, y };

	EXPECT_TRUE(compareStdVectors(defaultVArr, defaultV.toStdVector()));
	EXPECT_TRUE(compareStdVectors(randomVArr, randomV.toStdVector()));
	
}


TEST(RepoVector2DTest, eqOperatorTest)
{
	RepoVector2D defaultV, defaultV2;

	float x = (rand() % 1000) / 1000.f;
	float y = (rand() % 1000) / 1000.f;
	RepoVector2D randomV(x, y);

	EXPECT_EQ(defaultV, defaultV);
	EXPECT_EQ(defaultV2, defaultV);
	EXPECT_EQ(randomV, randomV);
	EXPECT_NE(defaultV2, randomV);
}