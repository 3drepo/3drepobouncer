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
#include <repo/core/model/bson/repo_bson_collection_stats.h>
#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;

TEST(CollectionStatsTest, Constructor)
{
	auto cs = CollectionStats();
	EXPECT_TRUE(cs.isEmpty());
	auto cs2 = CollectionStats(RepoBSON());
	EXPECT_TRUE(cs2.isEmpty());

	RepoBSON bson = BSON(getRandomString(rand() % 20 + 1) << getRandomString(rand() % 20 + 1) << getRandomString(rand() % 20 + 1) << getRandomString(rand() % 20 + 1));
	auto cs3 = CollectionStats(bson);

	EXPECT_EQ(cs3.toString(), bson.toString());
}

TEST(CollectionStatsTest, getStats)
{
	auto emptyStats = CollectionStats();
	EXPECT_EQ(0, emptyStats.getActualSizeOnDisk());
	EXPECT_EQ(0, emptyStats.getCount());
	EXPECT_EQ(0, emptyStats.getSize());
	EXPECT_EQ(0, emptyStats.getStorageSize());
	EXPECT_EQ(0, emptyStats.getTotalIndexSize());
	EXPECT_TRUE(emptyStats.getCollection().empty());
	EXPECT_TRUE(emptyStats.getDatabase().empty());
	EXPECT_TRUE(emptyStats.getNs().empty());

	auto handler = getHandler();
	std::string errMsg;
	ASSERT_TRUE(handler);

	std::string collection = REPO_GTEST_DBNAME1_PROJ + ".scene";
	auto stats = handler->getCollectionStats(REPO_GTEST_DBNAME1, collection, errMsg);
	ASSERT_FALSE(stats.isEmpty());
	//EXPECT_EQ(18926576, stats.getActualSizeOnDisk());
	EXPECT_EQ(14, stats.getCount());
	/*EXPECT_EQ(18918176, stats.getSize());
	EXPECT_EQ(67248128, stats.getStorageSize());
	EXPECT_EQ(8176, stats.getTotalIndexSize());*/
	EXPECT_EQ(collection, stats.getCollection());
	EXPECT_EQ(REPO_GTEST_DBNAME1, stats.getDatabase());
	EXPECT_EQ(REPO_GTEST_DBNAME1 + "." + collection, stats.getNs());
}