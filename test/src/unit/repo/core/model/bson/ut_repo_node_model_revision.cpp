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

#include <repo/core/model/bson/repo_node_model_revision.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;
/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(ModelRevisionNodeTest, Constructor)
{
	RevisionNode empty;

	EXPECT_TRUE(empty.isEmpty());
	EXPECT_EQ(NodeType::REVISION, empty.getTypeAsEnum());

	auto repoBson = RepoBSON(BSON("test" << "blah" << "test2" << 2));

	auto fromRepoBSON = RevisionNode(repoBson);
	EXPECT_EQ(NodeType::REVISION, fromRepoBSON.getTypeAsEnum());
	EXPECT_EQ(fromRepoBSON.nFields(), repoBson.nFields());
	EXPECT_EQ(0, fromRepoBSON.getFileList().size());
}

TEST(ModelRevisionNodeTest, TypeTest)
{
	RevisionNode node;

	EXPECT_EQ(REPO_NODE_TYPE_REVISION, node.getType());
	EXPECT_EQ(NodeType::REVISION, node.getTypeAsEnum());
}

TEST(ModelRevisionNodeTest, PositionDependantTest)
{
	RevisionNode node;
	EXPECT_FALSE(node.positionDependant());
}

TEST(ModelRevisionNodeTest, CloneAndUpdateStatusTest)
{
	ModelRevisionNode empty;
	auto updatedEmpty = empty.cloneAndUpdateStatus(ModelRevisionNode::UploadStatus::GEN_DEFAULT);
	EXPECT_EQ(updatedEmpty.getUploadStatus(), ModelRevisionNode::UploadStatus::GEN_DEFAULT);
	auto updatedEmpty2 = updatedEmpty.cloneAndUpdateStatus(ModelRevisionNode::UploadStatus::GEN_SEL_TREE);
	EXPECT_EQ(updatedEmpty2.getUploadStatus(), ModelRevisionNode::UploadStatus::GEN_SEL_TREE);
}

TEST(ModelRevisionNodeTest, GetterTest)
{
	ModelRevisionNode empty;
	EXPECT_TRUE(empty.getAuthor().empty());
	auto offset = empty.getCoordOffset();
	EXPECT_EQ(3, offset.size());
	for (const auto &v : offset)
		EXPECT_EQ(0, v);

	EXPECT_TRUE(empty.getMessage().empty());
	EXPECT_TRUE(empty.getTag().empty());
	EXPECT_EQ(ModelRevisionNode::UploadStatus::COMPLETE, empty.getUploadStatus());

	EXPECT_EQ(0, empty.getOrgFiles().size());
	EXPECT_EQ(-1, empty.getTimestampInt64());

	auto user = getRandomString(rand() % 10 + 1);
	auto branch = repo::lib::RepoUUID::createUUID();
	std::vector<repo::lib::RepoUUID> parents;

	for (int i = 0; i < rand() % 10 + 1; ++i)
	{
		parents.push_back(repo::lib::RepoUUID::createUUID());
	}

	std::vector<std::string> files = { getRandomString(rand() % 10 + 1), getRandomString(rand() % 10 + 1) };
	std::vector<double> offsetIn = { rand() / 100.f, rand() / 100.f, rand() / 100.f };

	auto message = getRandomString(rand() % 10 + 1);
	auto tag = getRandomString(rand() % 10 + 1);
	auto rId = repo::lib::RepoUUID::createUUID();

	auto revisionNode = RepoBSONFactory::makeRevisionNode(user, branch, rId, files, parents, offsetIn, message, tag);

	EXPECT_EQ(user, revisionNode.getAuthor());
	auto offset2 = revisionNode.getCoordOffset();
	EXPECT_EQ(offsetIn.size(), offset2.size());
	for (uint32_t i = 0; i < offset2.size(); ++i)
		EXPECT_EQ(offsetIn[i], offset2[i]);

	EXPECT_EQ(message, revisionNode.getMessage());
	EXPECT_EQ(tag, revisionNode.getTag());
	EXPECT_EQ(ModelRevisionNode::UploadStatus::COMPLETE, revisionNode.getUploadStatus());

	auto filesOut = revisionNode.getOrgFiles();
	EXPECT_EQ(files.size(), filesOut.size());

	EXPECT_NE(-1, revisionNode.getTimestampInt64());
}