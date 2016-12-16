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

#include <repo/core/model/bson/repo_node_reference.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(RefNodeTest, Constructor)
{
	ReferenceNode empty;

	EXPECT_TRUE(empty.isEmpty());
	EXPECT_EQ(NodeType::REFERENCE, empty.getTypeAsEnum());

	auto repoBson = RepoBSON(BSON("test" << "blah" << "test2" << 2));

	auto fromRepoBSON = ReferenceNode(repoBson);
	EXPECT_EQ(NodeType::REFERENCE, fromRepoBSON.getTypeAsEnum());
	EXPECT_EQ(fromRepoBSON.nFields(), repoBson.nFields());
	EXPECT_EQ(0, fromRepoBSON.getFileList().size());
}

TEST(RefNodeTest, TypeTest)
{
	ReferenceNode node;

	EXPECT_EQ(REPO_NODE_TYPE_REFERENCE, node.getType());
	EXPECT_EQ(NodeType::REFERENCE, node.getTypeAsEnum());
}

TEST(RefNodeTest, PositionDependantTest)
{
	ReferenceNode node;
	EXPECT_FALSE(node.positionDependant());
}

TEST(RefNodeTest, GetterTest)
{
	ReferenceNode empty;
	EXPECT_EQ(repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), empty.getRevisionID());
	EXPECT_TRUE(empty.getDatabaseName().empty());
	EXPECT_TRUE(empty.getProjectName().empty());
	EXPECT_FALSE(empty.useSpecificRevision());

	auto dbName = getRandomString(rand() % 10 + 1);
	auto projName = getRandomString(rand() % 10 + 1);
	auto revId = repo::lib::RepoUUID::createUUID();
	auto refNode1 = RepoBSONFactory::makeReferenceNode(dbName, projName, revId, true);

	auto dbName2 = getRandomString(rand() % 10 + 1);
	auto projName2 = getRandomString(rand() % 10 + 1);
	auto refNode2 = RepoBSONFactory::makeReferenceNode(dbName2, projName2);

	EXPECT_EQ(dbName, refNode1.getDatabaseName());
	EXPECT_EQ(projName, refNode1.getProjectName());
	EXPECT_EQ(revId, refNode1.getRevisionID());
	EXPECT_TRUE(refNode1.useSpecificRevision());

	EXPECT_EQ(dbName2, refNode2.getDatabaseName());
	EXPECT_EQ(projName2, refNode2.getProjectName());
	EXPECT_EQ(repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), refNode2.getRevisionID());
	EXPECT_FALSE(refNode2.useSpecificRevision());
}