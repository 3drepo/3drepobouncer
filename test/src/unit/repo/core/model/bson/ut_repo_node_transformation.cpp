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

#include <gtest/gtest.h>

#include <repo/core/model/bson/repo_node_transformation.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include "../../../../repo_test_utils.h"

using namespace repo::core::model;

std::vector<float> identity =
{ 1, 0, 0, 0,
0, 1, 0, 0,
0, 0, 1, 0,
0, 0, 0, 1 };

std::vector<float> notId =
{ 1, 2, 3, 4,
5, 6, 7, 8,
9, 0.3f, 10, 11,
5342, 31, 0.6f, 12 };

std::vector<float> idInBoundary =
{ 1, 0, 0, 0,
0, 1, 0, (float)1e-6,
0, 0, 1, 0,
0, (float)1e-6, (float)1e-6, 1 };

std::vector<float> notIdInBoundary = { 1, 0, 0, 0,
0, 1, 0, (float)2e-5,
0, 0, 2, 0,
0, (float)2e-5, (float)2e-5, 1 };

TransformationNode makeTransformationNode(
	const std::vector<float> &matrix)
{
	RepoBSONBuilder bsonBuilder;
	RepoBSONBuilder rows;
	for (uint32_t i = 0; i < 4; ++i)
	{
		RepoBSONBuilder columns;
		for (uint32_t j = 0; j < 4; ++j){
			columns << std::to_string(j) << matrix[i * 4 + j];
		}
		rows.appendArray(std::to_string(i), columns.obj());
	}
	bsonBuilder.appendArray(REPO_NODE_LABEL_MATRIX, rows.obj());

	return bsonBuilder.obj();
}

TEST(RepoTransformationNodeTest, Constructor)
{
	auto empty = TransformationNode();

	EXPECT_TRUE(empty.isEmpty());
	EXPECT_EQ(NodeType::TRANSFORMATION, empty.getTypeAsEnum());

	auto repoBson = RepoBSON(BSON("test" << "blah" << "test2" << 2));

	auto fromRepoBSON = TransformationNode(repoBson);
	EXPECT_EQ(NodeType::TRANSFORMATION, fromRepoBSON.getTypeAsEnum());
	EXPECT_EQ(fromRepoBSON.nFields(), repoBson.nFields());
	EXPECT_EQ(0, fromRepoBSON.getFileList().size());
}

TEST(RepoTransformationNodeTest, IdentityTest)
{
	auto empty = TransformationNode();
	EXPECT_TRUE(empty.isIdentity());

	EXPECT_TRUE(makeTransformationNode(identity).isIdentity());
	EXPECT_TRUE(makeTransformationNode(idInBoundary).isIdentity());
	EXPECT_FALSE(makeTransformationNode(notId).isIdentity());
	EXPECT_FALSE(makeTransformationNode(notIdInBoundary).isIdentity());
}

TEST(RepoTransformationNodeTest, IdentityTest2)
{
	auto identity = TransformationNode::identityMat();

	ASSERT_EQ(4, identity.size());
	for (int i = 0; i < 4; ++i)
	{
		ASSERT_EQ(4, identity[i].size());
		for (int j = 0; j < 4; ++j)
		{
			float expectedOutcome = i % 4 == j ? 1 : 0;
			EXPECT_EQ(expectedOutcome, identity[i][j]);
		}
	}
}

TEST(RepoTransformationNodeTest, TypeTest)
{
	TransformationNode node = TransformationNode();

	EXPECT_EQ(REPO_NODE_TYPE_TRANSFORMATION, node.getType());
	EXPECT_EQ(NodeType::TRANSFORMATION, node.getTypeAsEnum());
}

TEST(RepoTransformationNodeTest, PositionDependantTest)
{
	TransformationNode node = TransformationNode();
	//transformation node should always be position dependant
	EXPECT_TRUE(node.positionDependant());
}

TEST(RepoTransformationNodeTest, SEqualTest)
{
	auto empty1 = TransformationNode();
	auto empty2 = TransformationNode();

	auto notEmpty1 = makeTransformationNode(notId);
	auto notEmpty2 = makeTransformationNode(notId);
	auto notEmpty3 = makeTransformationNode(identity);

	EXPECT_TRUE(empty1.sEqual(empty2));
	EXPECT_TRUE(empty2.sEqual(empty1));
	EXPECT_TRUE(empty1.sEqual(empty1));
	EXPECT_TRUE(notEmpty1.sEqual(notEmpty2));
	EXPECT_TRUE(notEmpty2.sEqual(notEmpty1));
	EXPECT_FALSE(notEmpty1.sEqual(empty2));
	EXPECT_TRUE(notEmpty3.sEqual(notEmpty3));
	EXPECT_FALSE(notEmpty3.sEqual(notEmpty2));
	EXPECT_FALSE(empty1.sEqual(notEmpty2));
}

TEST(RepoTransformationNodeTest, CloneAndApplyTransformationTest)
{
	auto empty = TransformationNode();

	TransformationNode modifiedEmpty = empty.cloneAndApplyTransformation(notId);

	EXPECT_EQ(empty.getTransMatrix(false), identity);
	EXPECT_EQ(modifiedEmpty.getTransMatrix(false), notId);

	auto filled = makeTransformationNode(notId);
	TransformationNode modifiedFilled = filled.cloneAndApplyTransformation(std::vector<float>());

	EXPECT_EQ(modifiedFilled.getTransMatrix(false), notId);
}

TEST(RepoTransformationNodeTest, GetTransMatrixTest)
{
	TransformationNode empty = TransformationNode();
	EXPECT_EQ(identity, empty.getTransMatrix(false));

	TransformationNode notEmpty = makeTransformationNode(notId);
	EXPECT_EQ(notId, notEmpty.getTransMatrix(false));

	//check transpose is done correctly
	auto notIdTransposed = notEmpty.getTransMatrix(true);

	auto notIdTransData = notIdTransposed.getData();
	ASSERT_EQ(notId.size(), notIdTransData.size());
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			int index = i * 4 + j;
			int transIndex = j * 4 + i;
			EXPECT_EQ(notId[index], notIdTransData[transIndex]);
		}
	}
}