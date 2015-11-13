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


	//ensure extref files are retained
	std::vector<uint8_t> file1;
	std::vector<uint8_t> file2;

	size_t fileSize = 32876;
	file1.resize(fileSize);
	file2.resize(fileSize);

	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>>> files;
	files["field1"] = std::pair<std::string, std::vector<uint8_t>>("file1", file1);
	files["field2"] = std::pair<std::string, std::vector<uint8_t>>("file2", file2);

	RepoNode nodeWithFiles = RepoNode(node, files);
	RepoNode clonedNodeWithFiles = nodeWithFiles.cloneAndAddParent(parent);

	EXPECT_TRUE(clonedNodeWithFiles.hasOversizeFiles());
	auto mappingOut = clonedNodeWithFiles.getFilesMapping();
	EXPECT_EQ(2, mappingOut.size());

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

	//ensure extref files are retained
	std::vector<uint8_t> file1;
	std::vector<uint8_t> file2;

	size_t fileSize = 32876;
	file1.resize(fileSize);
	file2.resize(fileSize);

	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>>> files;
	files["field1"] = std::pair<std::string, std::vector<uint8_t>>("file1", file1);
	files["field2"] = std::pair<std::string, std::vector<uint8_t>>("file2", file2);

	RepoNode nodeWithFiles = RepoNode(node, files);
	RepoNode clonedNodeWithFiles = nodeWithFiles.cloneAndAddParent(parent);

	EXPECT_TRUE(clonedNodeWithFiles.hasOversizeFiles());
	auto mappingOut = clonedNodeWithFiles.getFilesMapping();
	EXPECT_EQ(2, mappingOut.size());
}


TEST(RepoNodeTest, CloneAndApplyTransformationTest)
{
	std::vector<float> mat;
	RepoNode transedEmpty = emptyNode.cloneAndApplyTransformation(mat);
	RepoNode transedTest = testNode.cloneAndApplyTransformation(mat);
	EXPECT_EQ(emptyNode.toString(), transedEmpty.toString());
	EXPECT_EQ(testNode.toString(), transedTest.toString());
	EXPECT_NE(transedEmpty.toString(), transedTest.toString());

	//ensure extref files are retained
	std::vector<uint8_t> file1;
	std::vector<uint8_t> file2;

	size_t fileSize = 32876;
	file1.resize(fileSize);
	file2.resize(fileSize);

	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>>> files;
	files["field1"] = std::pair<std::string, std::vector<uint8_t>>("file1", file1);
	files["field2"] = std::pair<std::string, std::vector<uint8_t>>("file2", file2);

	RepoNode nodeWithFiles = RepoNode(emptyNode, files);
	RepoNode clonedNodeWithFiles = nodeWithFiles.cloneAndApplyTransformation(mat);

	EXPECT_TRUE(clonedNodeWithFiles.hasOversizeFiles());
	auto mappingOut = clonedNodeWithFiles.getFilesMapping();
	EXPECT_EQ(2, mappingOut.size());
}

TEST(RepoNodeTest, CloneAndChangeNameTest)
{
	std::string name = "3dRepo";
	std::string name2 = "Open Source";
	RepoNode emptyNodeWithName = emptyNode.cloneAndChangeName(name);

	EXPECT_FALSE(emptyNodeWithName.isEmpty());
	EXPECT_EQ(name, emptyNodeWithName.getStringField(REPO_NODE_LABEL_NAME));
	//ensure it is a clone, there is no change
	EXPECT_TRUE(emptyNode.isEmpty());

	RepoNode anotherNameNode = emptyNodeWithName.cloneAndChangeName(name2);
	EXPECT_EQ(name, emptyNodeWithName.getStringField(REPO_NODE_LABEL_NAME));
	EXPECT_EQ(name2, anotherNameNode.getStringField(REPO_NODE_LABEL_NAME));
	EXPECT_NE(emptyNodeWithName.getUniqueID(), anotherNameNode.getUniqueID());
	EXPECT_EQ(emptyNodeWithName.getUniqueID(), emptyNodeWithName.cloneAndChangeName(name2, false).getUniqueID());


	//ensure extref files are retained
	std::vector<uint8_t> file1;
	std::vector<uint8_t> file2;

	size_t fileSize = 32876;
	file1.resize(fileSize);
	file2.resize(fileSize);

	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>>> files;
	files["field1"] = std::pair<std::string, std::vector<uint8_t>>("file1", file1);
	files["field2"] = std::pair<std::string, std::vector<uint8_t>>("file2", file2);

	RepoNode nodeWithFiles = RepoNode(emptyNode, files);
	RepoNode clonedNodeWithFiles = nodeWithFiles.cloneAndChangeName(name2);

	EXPECT_TRUE(clonedNodeWithFiles.hasOversizeFiles());
	auto mappingOut = clonedNodeWithFiles.getFilesMapping();
	EXPECT_EQ(2, mappingOut.size());
}

TEST(RepoNodeTest, CloneAndRemoveParentTest)
{

	RepoNode stillEmpty = emptyNode.cloneAndRemoveParent(generateUUID());
	EXPECT_TRUE(stillEmpty.isEmpty());

	RepoBSONBuilder builder;
	builder.append("_id", generateUUID());
	RepoNode startNode = RepoNode(builder.obj());

	repoUUID singleParent = generateUUID();
	RepoNode oneParentNode = startNode.cloneAndAddParent(singleParent);
	//Ensure the field is removed all together if there is no parent left
	EXPECT_FALSE(oneParentNode.cloneAndRemoveParent(singleParent).hasField(REPO_NODE_LABEL_PARENTS));

	std::vector<repoUUID> parents;
	size_t nParents = 10;
	for (int i = 0; i < nParents; ++i)
	{
		parents.push_back(generateUUID());
	}

	RepoNode manyParentsNode = startNode.cloneAndAddParent(parents);

	int indToRemove = std::rand() % nParents;

	RepoNode parentRemovedNode = manyParentsNode.cloneAndRemoveParent(parents[indToRemove]);

	std::vector<repoUUID> parentsOut = parentRemovedNode.getParentIDs();
	EXPECT_EQ(parentsOut.end(), std::find(parentsOut.begin(), parentsOut.end(), parents[indToRemove]));

	EXPECT_NE(manyParentsNode.getUniqueID(), parentRemovedNode.getUniqueID());
	EXPECT_EQ(manyParentsNode.getUniqueID(), manyParentsNode.cloneAndRemoveParent(parents[indToRemove], false).getUniqueID());


	//ensure extref files are retained
	std::vector<uint8_t> file1;
	std::vector<uint8_t> file2;

	size_t fileSize = 32876;
	file1.resize(fileSize);
	file2.resize(fileSize);

	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>>> files;
	files["field1"] = std::pair<std::string, std::vector<uint8_t>>("file1", file1);
	files["field2"] = std::pair<std::string, std::vector<uint8_t>>("file2", file2);

	RepoNode nodeWithFiles = RepoNode(emptyNode, files);
	RepoNode clonedNodeWithFiles = nodeWithFiles.cloneAndRemoveParent(singleParent);

	EXPECT_TRUE(clonedNodeWithFiles.hasOversizeFiles());
	auto mappingOut = clonedNodeWithFiles.getFilesMapping();
	EXPECT_EQ(2, mappingOut.size());
}

TEST(RepoNodeTest, CloneAndAddFieldTest)
{
	std::string field = "3dRepo";
	std::string name = "Open Source";

	std::string field2 = "field2";
	std::string name2 = "Open Source too";

	RepoBSON *changeBson = new RepoBSON(BSON(field << name));
	RepoBSON *changeBson2 = new RepoBSON(BSON(field2 << name2));
	RepoBSON *changeBson3 = new RepoBSON(BSON(field << name2));

	RepoNode nodeFieldAdded = emptyNode.cloneAndAddFields(changeBson);

	EXPECT_FALSE(nodeFieldAdded.isEmpty());
	EXPECT_EQ(name, nodeFieldAdded.getStringField(field));
	//ensure it is a clone, there is no change
	EXPECT_TRUE(emptyNode.isEmpty());

	RepoNode anotherNameNode = nodeFieldAdded.cloneAndAddFields(changeBson2);
	EXPECT_EQ(name, nodeFieldAdded.getStringField(field));
	EXPECT_EQ(name2, anotherNameNode.getStringField(field2));
	EXPECT_NE(nodeFieldAdded.getUniqueID(), anotherNameNode.getUniqueID());
	EXPECT_EQ(nodeFieldAdded.getUniqueID(), nodeFieldAdded.cloneAndAddFields(changeBson2, false).getUniqueID());

	RepoNode replaceField = nodeFieldAdded.cloneAndAddFields(changeBson3);
	EXPECT_EQ(name2, replaceField.getStringField(field));


	//ensure extref files are retained
	std::vector<uint8_t> file1;
	std::vector<uint8_t> file2;

	size_t fileSize = 32876;
	file1.resize(fileSize);
	file2.resize(fileSize);

	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>>> files;
	files["field1"] = std::pair<std::string, std::vector<uint8_t>>("file1", file1);
	files["field2"] = std::pair<std::string, std::vector<uint8_t>>("file2", file2);

	RepoNode nodeWithFiles = RepoNode(emptyNode, files);
	RepoNode clonedNodeWithFiles = nodeWithFiles.cloneAndAddFields(changeBson);

	EXPECT_TRUE(clonedNodeWithFiles.hasOversizeFiles());
	auto mappingOut = clonedNodeWithFiles.getFilesMapping();
	EXPECT_EQ(2, mappingOut.size());

	delete changeBson;
	delete changeBson2;
	delete changeBson3;
}


TEST(RepoNodeTest, CloneAndAddMergedNodesTest)
{
	std::vector<repoUUID> mergeMap;

	size_t size = 2123;
	mergeMap.resize(size);

	RepoNode node = emptyNode.cloneAndAddMergedNodes(mergeMap);

	EXPECT_FALSE(node.isEmpty());
	EXPECT_TRUE(node.hasField(REPO_LABEL_MERGED_NODES));

	std::vector<repoUUID> mergeMapOut = node.getUUIDFieldArray(REPO_LABEL_MERGED_NODES);

	ASSERT_EQ(mergeMapOut.size(), mergeMap.size());
	for (size_t i = 0; i < mergeMapOut.size(); ++i)
	{
		EXPECT_EQ(mergeMapOut[i], mergeMap[i]);
	}


	//ensure extref files are retained
	std::vector<uint8_t> file1;
	std::vector<uint8_t> file2;

	size_t fileSize = 32876;
	file1.resize(fileSize);
	file2.resize(fileSize);

	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>>> files;
	files["field1"] = std::pair<std::string, std::vector<uint8_t>>("file1", file1);
	files["field2"] = std::pair<std::string, std::vector<uint8_t>>("file2", file2);

	RepoNode nodeWithFiles = RepoNode(emptyNode, files);
	RepoNode clonedNodeWithFiles = nodeWithFiles.cloneAndAddMergedNodes(mergeMap);

	EXPECT_TRUE(clonedNodeWithFiles.hasOversizeFiles());
	auto mappingOut = clonedNodeWithFiles.getFilesMapping();
	EXPECT_EQ(2, mappingOut.size());
}