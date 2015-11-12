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

#include <repo/core/model/bson/repo_node.h>
#include <repo/core/model/bson/repo_node_mesh.h>

using namespace repo::core::model;

static const RepoNode testNode = RepoNode(BSON("ice" << "lolly" << "amount" << 100));
static const RepoNode emptyNode;

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(RepoNodeTest, Constructor)
{
	RepoNode node;

	EXPECT_TRUE(node.isEmpty());

	const RepoBSON testBson = RepoBSON(BSON("ice" << "lolly" << "amount" << 100));

	RepoNode node2(testBson);

	EXPECT_EQ(node2.toString(), testBson.toString());
}

TEST(RepoNodeTest, PositionDependantTest)
{
	RepoNode node;
	EXPECT_FALSE(node.positionDependant());

	//To make sure this is a virtual function
	RepoNode *mesh = new MeshNode();
	EXPECT_TRUE(mesh->positionDependant());
}

TEST(RepoNodeTest, CloneAndAddParentTest)
{
	RepoNode node;

	EXPECT_FALSE(node.hasField(REPO_NODE_LABEL_PARENTS));

	repoUUID parent = generateUUID();
	RepoNode nodeWithParent = node.cloneAndAddParent(parent);

	ASSERT_TRUE(nodeWithParent.hasField(REPO_NODE_LABEL_PARENTS));
	EXPECT_NE(node, nodeWithParent);
	EXPECT_NE(node.toString(), nodeWithParent.toString());


	RepoBSONElement parentField = nodeWithParent.getField(REPO_NODE_LABEL_PARENTS);
	ASSERT_EQ(ElementType::ARRAY, parentField.type());

	std::vector<repoUUID> parentsOut = nodeWithParent.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);

	EXPECT_EQ(1, parentsOut.size());
	EXPECT_EQ(parent, parentsOut[0]);

	//Ensure same parent isn't added
	RepoNode sameParentTest = nodeWithParent.cloneAndAddParent(parent);

	parentsOut = sameParentTest.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);

	ASSERT_EQ(1, parentsOut.size());
	EXPECT_EQ(parent, parentsOut[0]);

	//Try to add a parent when there's already a vector
	repoUUID parent2 = generateUUID();

	RepoNode secondParentNode = nodeWithParent.cloneAndAddParent(parent2);
	parentsOut = secondParentNode.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);

	ASSERT_EQ(2, parentsOut.size());
	EXPECT_EQ(parent, parentsOut[0]);
	EXPECT_EQ(parent2, parentsOut[1]);

}

TEST(RepoNodeTest, CloneAndAddParentTest_MultipleParents)
{
	RepoNode node;

	EXPECT_FALSE(node.hasField(REPO_NODE_LABEL_PARENTS));

	std::vector<repoUUID> parent;
	size_t nParents = 10;

	for (size_t i = 0; i < nParents; ++i)
	{
		parent.push_back(generateUUID());
	}


	RepoNode nodeWithParent = node.cloneAndAddParent(parent);

	ASSERT_TRUE(nodeWithParent.hasField(REPO_NODE_LABEL_PARENTS));
	EXPECT_NE(node, nodeWithParent);
	EXPECT_NE(node.toString(), nodeWithParent.toString());


	RepoBSONElement parentField = nodeWithParent.getField(REPO_NODE_LABEL_PARENTS);
	ASSERT_EQ(ElementType::ARRAY, parentField.type());

	std::vector<repoUUID> parentsOut = nodeWithParent.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);

	EXPECT_EQ(nParents, parentsOut.size());

	for (size_t i = 0; i < nParents; ++i)
	{
		//EXPECT_EQ(parent[i], parentsOut[i]);
		auto it = std::find(parentsOut.begin(), parentsOut.end(), parent[i]);
		EXPECT_NE(parentsOut.end(), it);
		parentsOut.erase(it);
	}


	EXPECT_EQ(0, parentsOut.size());

	//Ensure same parent isn't added

	RepoNode sameParentTest = nodeWithParent.cloneAndAddParent(parent);

	parentsOut = sameParentTest.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);

	EXPECT_EQ(nParents, parentsOut.size());

	for (size_t i = 0; i < nParents; ++i)
	{
		//EXPECT_EQ(parent[i], parentsOut[i]);
		auto it = std::find(parentsOut.begin(), parentsOut.end(), parent[i]);
		EXPECT_NE(parentsOut.end(), it);
		parentsOut.erase(it);
	}

	EXPECT_EQ(0, parentsOut.size());

	//Try to add a parent when there's already a vector
	std::vector<repoUUID> parent2;

	for (size_t i = 0; i < nParents; ++i)
	{
		parent2.push_back(generateUUID());
	}


	RepoNode secondParentNode = nodeWithParent.cloneAndAddParent(parent2);
	parentsOut = secondParentNode.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);

	ASSERT_EQ(2*nParents, parentsOut.size());

	std::vector<repoUUID> fullParents = parent;	
	fullParents.insert(fullParents.end(),parent2.begin(), parent2.end());
	for (size_t i = 0; i < nParents*2; ++i)
	{
		//EXPECT_EQ(parent[i], parentsOut[i]);
		auto it = std::find(parentsOut.begin(), parentsOut.end(), fullParents[i]);
		EXPECT_NE(parentsOut.end(), it);
		parentsOut.erase(it);
	}

}


TEST(RepoNodeTest, CloneAndApplyTransformationTest)
{
	std::vector<float> mat;
	RepoNode transedEmpty = emptyNode.cloneAndApplyTransformation(mat);
	RepoNode transedTest = testNode.cloneAndApplyTransformation(mat);
	EXPECT_EQ(emptyNode.toString(), transedEmpty.toString());
	EXPECT_EQ(testNode.toString(), transedTest.toString());
	EXPECT_NE(transedEmpty.toString(), transedTest.toString());
}

