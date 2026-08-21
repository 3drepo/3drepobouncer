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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include <repo/core/model/bson/repo_node_point.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core//model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_point_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::core::model;
using namespace repo::test::utils::point;
using namespace testing;

TEST(PointNodeTest, EmptyConstructor)
{
	PointNode empty;

	EXPECT_FALSE(empty.getNumPoints());
	EXPECT_EQ(NodeType::POINT, empty.getTypeAsEnum());
}

TEST(PointNodeTest, TypeTest)
{
	PointNode node;

	EXPECT_EQ(REPO_NODE_TYPE_POINT, node.getType());
	EXPECT_EQ(NodeType::POINT, node.getTypeAsEnum());
}

TEST(PointNodeTest, PositionDependentTest)
{
	PointNode node;
	// Point Node should always be position dependent
	EXPECT_TRUE(node.positionDependant());
}

static void PointNodeTestDeserialise(point_data data)
{
	comparePointNode(data, PointNode(pointNodeTestBSONFactory(data)));
}

TEST(PointNodeTest, Deserialise)
{
	PointNodeTestDeserialise(point_data(false, false, 0, 100, 1));
	PointNodeTestDeserialise(point_data(false, false, 0, 100, 2));
	PointNodeTestDeserialise(point_data(false, false, 0, 100, 3));

	PointNodeTestDeserialise(point_data(false, false, 0, 100, 1));
	PointNodeTestDeserialise(point_data(false, false, 2, 100, 2));
	PointNodeTestDeserialise(point_data(false, false, 3, 100, 3));

	PointNodeTestDeserialise(point_data(true, false, 0, 100, 1));
	PointNodeTestDeserialise(point_data(false, true, 2, 100, 2));
	PointNodeTestDeserialise(point_data(true, true, 3, 100, 3));
}

TEST(PointNodeTest, Serialise)
{
	PointNode node;

	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_LABEL_ID), Eq(node.getUniqueID()));
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_SHARED_ID), IsFalse());
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_TYPE), Eq("point"));
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_PARENTS), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_NAME), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_POINT_LABEL_BOUNDING_BOX), IsTrue());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_POINT_LABEL_POINTS_COUNT), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_POINT_LABEL_POINTS), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_POINT_LABEL_COLOURS), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_POINT_LABEL_TREE_POSITION), IsFalse());

	node.setSharedID(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_SHARED_ID), Eq(node.getSharedID()));

	node.addParent(repo::lib::RepoUUID::createUUID());
	node.addParent(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDFieldArray(REPO_NODE_LABEL_PARENTS), node.getParentIDs());

	node.changeName("MyName");
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_NAME), Eq(node.getName()));

	// We consider colour a mandatory attribute, so refuse serialisation without it
	node.setPoints(makePoints(100));
	EXPECT_THROW(
		{
			((RepoBSON)node).getIntField(REPO_NODE_POINT_LABEL_POINTS_COUNT);
		},
		repo::lib::RepoException
	);

	// In the same way, colours without points cannot be serialised
	node.setPoints(std::vector<repo::lib::RepoVector3D>());
	node.setColourAttributes(makeColourAttributes(100));
	EXPECT_THROW(
		{
			((RepoBSON)node).getIntField(REPO_NODE_POINT_LABEL_POINTS_COUNT);
		},
		repo::lib::RepoException
	);

	// We also refuse serialisation if the number of points and colour attributes diverge
	node.setPoints(makePoints(100));
	node.setColourAttributes(makeColourAttributes(101));
	EXPECT_THROW(
		{
			((RepoBSON)node).getIntField(REPO_NODE_POINT_LABEL_POINTS_COUNT);
		},
		repo::lib::RepoException
	);

	node.setPoints(makePoints(100));
	node.setColourAttributes(makeColourAttributes(100));
	((RepoBSON)node).getIntField(REPO_NODE_POINT_LABEL_POINTS_COUNT), Eq(node.getNumPoints());
	std::vector<repo::lib::RepoVector3D> points;
	((RepoBSON)node).getBinaryFieldAsVector(REPO_NODE_POINT_LABEL_POINTS, points);
	std::vector<repo::lib::repo_color4d_t> colourAttributes;
	((RepoBSON)node).getBinaryFieldAsVector(REPO_NODE_POINT_LABEL_COLOURS, colourAttributes);
	EXPECT_THAT(colourAttributes, ElementsAreArray(node.getColourAttributes()));

	auto min = makeRandomRepoVector();
	auto max = makeRandomRepoVector();
	repo::lib::RepoBounds bounds = repo::lib::RepoBounds(min, max);
	node.setBoundingBox(bounds);
	EXPECT_THAT(((RepoBSON)node).getBoundsField(REPO_NODE_POINT_LABEL_BOUNDING_BOX), Eq(node.getBoundingBox()));

	node.setTreePosition(makeTreePosition(3));
	EXPECT_THAT(((RepoBSON)node).getByteArray(REPO_NODE_POINT_LABEL_TREE_POSITION), ElementsAreArray(node.getTreePosition()));
}

TEST(PointNodeTest, SEqualTest)
{
	PointNode point;
	RepoNode node;

	EXPECT_FALSE(point.sEqual(node));
	EXPECT_TRUE(point.sEqual(point));

	// Point nodes with different combinations of attributes should not
	// return equal. Different objects with the same configurations however
	// should.

	// Create two lists, with intra-list point nodes being separate but equivalent,
	// and inter-list point nodes being different.

	std::vector<repo::lib::RepoVector3D> emptyP, p;
	std::vector<std::vector<float>> bbox;
	std::vector<repo::lib::repo_color4d_t> emptyCol, cols;
	p.resize(10);
	cols.resize(10);
	bbox.resize(2);
	bbox[0].resize(3);
	bbox[1].resize(3);

	std::vector< std::vector<repo::core::model::PointNode>> pointNodesLists;
	pointNodesLists.resize(2);

	for (int i = 0; i < 2; i++) {

		auto& pointNodes = pointNodesLists[i];

		// Create a set of point nodes with all possible variations, which should result
		// in different formats

		pointNodes.push_back(PointNode()); // empty

		// Different tree levels

		pointNodes.push_back(makeDeterministicPointNode(0));
		pointNodes.push_back(makeDeterministicPointNode(1));
		pointNodes.push_back(makeDeterministicPointNode(2));
		pointNodes.push_back(makeDeterministicPointNode(3));


		// (Different buffer contents)

		// Every time makeDeterministicPointhNode is called it resets the random
		// seed, so the following should create identical copies to the above and
		// each entry will differ only by the calls to setVertices, etc, which
		// continue the random sequence.

		pointNodes.push_back(makeDeterministicPointNode(3));
		pointNodes[pointNodes.size() - 1].setPoints(makePoints(100));

		pointNodes.push_back(makeDeterministicPointNode(3));
		pointNodes[pointNodes.size() - 1].setColourAttributes(makeColourAttributes(100));
	}

	for (size_t i = 0; i < pointNodesLists[0].size(); i++)
	{
		auto& pointNode1 = pointNodesLists[0][i];
		auto& pointNode2 = pointNodesLists[1][i];

		// The meshes between the two lists should be the same

		EXPECT_TRUE(pointNode1.sEqual(pointNode2));
	}

	auto& pointNodes = pointNodesLists[0];
	for (size_t i = 0; i < pointNodes.size(); i++)
	{
		auto& outer = pointNodes[i];
		for (size_t j = 0; j < pointNodes.size(); j++)
		{
			if (i != j)
			{
				auto& inner = pointNodes[j];
				EXPECT_FALSE(inner.sEqual(outer));
			}
		}
	}
}