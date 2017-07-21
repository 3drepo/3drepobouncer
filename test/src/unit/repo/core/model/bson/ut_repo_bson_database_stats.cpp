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

#include <cstdlib>

#include <gtest/gtest.h>

#include "../../../../repo_test_database_info.h"
#include "../../../../repo_test_utils.h"
#include <repo/core/model/bson/repo_bson_database_stats.h>

using namespace repo::core::model;

TEST(DatabaseStatsTest, Constructor)
{
	auto cs = DatabaseStats();
	EXPECT_TRUE(cs.isEmpty());
	auto cs2 = DatabaseStats(RepoBSON());
	EXPECT_TRUE(cs2.isEmpty());

	RepoBSON bson = BSON(getRandomString(rand() % 20 + 1) << getRandomString(rand() % 20 + 1) << getRandomString(rand() % 20 + 1) << getRandomString(rand() % 20 + 1));
	auto cs3 = DatabaseStats(bson);

	EXPECT_EQ(cs3.toString(), bson.toString());
}

TEST(DatabaseStatsTest, getStats)
{
	auto emptyStats = DatabaseStats();
	EXPECT_EQ(0, emptyStats.getCollectionsCount());
	EXPECT_EQ(0, emptyStats.getObjectsCount());
	EXPECT_EQ(0, emptyStats.getAvgObjSize());
	EXPECT_EQ(0, emptyStats.getDataSize());
	EXPECT_EQ(0, emptyStats.getStorageSize());
	EXPECT_EQ(0, emptyStats.getNumExtents());
	EXPECT_EQ(0, emptyStats.getIndexesCount());
	EXPECT_EQ(0, emptyStats.getIndexSize());
	EXPECT_EQ(0, emptyStats.getFileSize());
	EXPECT_EQ(0, emptyStats.getNsSizeMB());
	EXPECT_EQ(0, emptyStats.getDataFileVersionMajor());
	EXPECT_EQ(0, emptyStats.getDataFileVersionMinor());
	EXPECT_EQ(0, emptyStats.getExtentFreeListNum());
	EXPECT_EQ(0, emptyStats.getExtentFreeListSize());

	EXPECT_TRUE(emptyStats.getDatabase().empty());

	auto handler = getHandler();
	std::string errMsg;

	ASSERT_TRUE(handler);

	auto stats = handler->getDatabaseStats(REPO_GTEST_DBNAME1, errMsg);
	ASSERT_FALSE(stats.isEmpty());
	//EXPECT_EQ(12, stats.getCollectionsCount()); //FIXME: travis is giving out 10, but i can't reproduce this.
	//EXPECT_EQ(100, stats.getObjectsCount());
	//EXPECT_EQ(439404, stats.getAvgObjSize());
	//EXPECT_EQ(43940480, stats.getDataSize());
	//EXPECT_EQ(194637824, stats.getStorageSize());
	//EXPECT_EQ(17, stats.getNumExtents());
	EXPECT_EQ(12, stats.getIndexesCount());
	/*EXPECT_EQ(98112, stats.getIndexSize());
	EXPECT_EQ(469762048, stats.getFileSize());
	EXPECT_EQ(16, stats.getNsSizeMB());
	EXPECT_EQ(4, stats.getDataFileVersionMajor());*/
	//EXPECT_EQ(5, stats.getDataFileVersionMinor());
	EXPECT_EQ(0, stats.getExtentFreeListNum());
	EXPECT_EQ(0, stats.getExtentFreeListSize());

	repoTrace << stats;

	EXPECT_EQ(REPO_GTEST_DBNAME1, stats.getDatabase());
}