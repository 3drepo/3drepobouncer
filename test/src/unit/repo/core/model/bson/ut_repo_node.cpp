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
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include <repo/core/model/bson/repo_node.h>
#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;
using namespace testing;

static const std::string typicalName = "3drepo";
static const repo::lib::RepoUUID typicalUniqueID = repo::lib::RepoUUID::createUUID();
static const repo::lib::RepoUUID typicalSharedID = repo::lib::RepoUUID::createUUID();

RepoNode makeTypicalNode()
{
	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_NAME, typicalName);
	builder.append(REPO_NODE_LABEL_ID, typicalUniqueID);
	builder.append(REPO_NODE_LABEL_SHARED_ID, typicalSharedID);

	return RepoNode(builder.obj());
}

RepoNode makeRandomNode()
{
	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_ID, repo::lib::RepoUUID::createUUID());
	builder.append(REPO_NODE_LABEL_SHARED_ID, repo::lib::RepoUUID::createUUID());

	return RepoNode(builder.obj());
}

RepoNode makeNode(const repo::lib::RepoUUID &unqiueID, const repo::lib::RepoUUID &sharedID, const std::string &name = "")
{
	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_ID, unqiueID);
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedID);

	if (!name.empty())
	{
		builder.append(REPO_NODE_LABEL_NAME, name);
	}

	return RepoNode(builder.obj());
}

TEST(RepoNodeTest, Constructor)
{
	auto node = RepoNode();
	EXPECT_THAT(node.getParentIDs(), IsEmpty());
	EXPECT_THAT(node.getName(), IsEmpty());
	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsTrue());
	EXPECT_THAT(node.getRevision().isDefaultValue(), IsTrue());
	EXPECT_THAT(node.getType(), IsEmpty());
}

// The following few tests are concerned with converting to and from RepoBSON.

TEST(RepoNodeTest, Deserialise)
{
	// Test the RepoNode constructor works for an existing BSON created independently
	// of RepoNode.

	auto name = "MyNode";
	auto uniqueId = repo::lib::RepoUUID::createUUID();
	auto sharedId = repo::lib::RepoUUID::createUUID();
	auto parentIds = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
	});
	auto revisionId = repo::lib::RepoUUID::createUUID();

	RepoBSONBuilder builder;
	builder.append(REPO_NODE_LABEL_NAME, name);
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId);
	builder.append(REPO_NODE_LABEL_ID, uniqueId);
	builder.append(REPO_NODE_REVISION_ID, revisionId);
	builder.appendArray(REPO_NODE_LABEL_PARENTS, parentIds);

	auto node = RepoNode(builder.obj());

	EXPECT_EQ(node.getName(), name);
	EXPECT_EQ(node.getSharedID(), sharedId);
	EXPECT_EQ(node.getUniqueID(), uniqueId);
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds));
	EXPECT_THAT(node.getRevision(), Eq(revisionId));

}

TEST(RepoNodeTest, DeserialiseNoSharedIDOrParents)
{
	// Test the RepoNode constructor works for an existing BSON created independently
	// of RepoNode.

	auto name = "MyNode";
	auto uniqueId = repo::lib::RepoUUID::createUUID();

	RepoBSONBuilder builder;
	builder.append(REPO_NODE_LABEL_NAME, name);
	builder.append(REPO_NODE_LABEL_ID, uniqueId);

	auto node = RepoNode(builder.obj());

	EXPECT_EQ(node.getName(), name);
	EXPECT_EQ(node.getUniqueID(), uniqueId);
	EXPECT_TRUE(node.getSharedID().isDefaultValue());
	EXPECT_TRUE(node.getRevision().isDefaultValue());
	EXPECT_FALSE(node.getParentIDs().size());
}

TEST(RepoNodeTest, DeserialiseNoName)
{
	// Test the RepoNode constructor works for an existing BSON created independently
	// of RepoNode.

	auto uniqueId = repo::lib::RepoUUID::createUUID();

	RepoBSONBuilder builder;
	builder.append(REPO_NODE_LABEL_ID, uniqueId);

	auto node = RepoNode(builder.obj());

	EXPECT_TRUE(node.getName().empty());
	EXPECT_EQ(node.getUniqueID(), uniqueId);
}

TEST(RepoNodeTest, Serialise)
{
	RepoNode node;

	// An empty node should not store anything other than the ID, and the ID should
	// always be initialised.

	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_NAME), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_TYPE), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_SHARED_ID), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_PARENTS), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_ID), IsTrue());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID).isDefaultValue(), IsFalse());

	// Setting each other property should set the appropraite field in the RepoBSON

	auto parentIds = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		});
	node.addParent(parentIds[0]);
	node.addParent(parentIds[1]);
	node.addParent(parentIds[2]);
	EXPECT_THAT(((RepoBSON)node).getUUIDFieldArray(REPO_NODE_LABEL_PARENTS), UnorderedElementsAreArray(parentIds));

	auto name = "name";
	node.changeName(name, false);
	EXPECT_EQ(((RepoBSON)node).getStringField(REPO_NODE_LABEL_NAME), name);

	auto sharedId = repo::lib::RepoUUID::createUUID();
	node.setSharedID(sharedId);
	EXPECT_EQ(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_SHARED_ID), sharedId);

	auto uniqueId = repo::lib::RepoUUID::createUUID();
	node.setUniqueId(uniqueId);
	EXPECT_EQ(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID), uniqueId);
}

TEST(RepoNodeTest, PositionDependantTest)
{
	RepoNode node;
	EXPECT_FALSE(node.positionDependant());

	//To make sure this is a virtual function
	RepoNode* mesh = new MeshNode();
	EXPECT_TRUE(mesh->positionDependant());
}

TEST(RepoNodeTest, ChangeName)
{
	RepoNode node;
	EXPECT_TRUE(node.getName().empty());
	
	auto name = "name";
	node.changeName(name);
	EXPECT_EQ(node.getName(), name);
}

TEST(RepoNodeTest, AddParentTest)
{
	RepoNode node;

	auto parentIds = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID(),
	});

	node.addParent(parentIds[0]);
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds));

	auto parentId = repo::lib::RepoUUID::createUUID();

	parentIds.push_back(parentId);
	node.addParent(parentId);
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds));

	// Make sure we don't add the same parent multiple times

	node.addParent(parentId);

	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds));
}

TEST(RepoNodeTest, AddParentsTest)
{
	RepoNode node;

	auto parentIds = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		});

	node.addParents(parentIds);
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds));

	auto parentId1 = repo::lib::RepoUUID::createUUID();
	auto parentId2 = repo::lib::RepoUUID::createUUID();

	parentIds.push_back(parentId1);
	parentIds.push_back(parentId2);

	node.addParents({ parentId1, parentId2 });
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds));

	// Make sure we don't add the same parent multiple times

	node.addParent({ parentId1 });
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds));

	node.addParents({ parentId1, parentId2 });
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds));

	// Combination of new and existing Ids

	auto parentId3 = repo::lib::RepoUUID::createUUID();
	parentIds.push_back(parentId3);

	node.addParents({ parentId1, parentId2, parentId3 });
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds));
}

TEST(RepoNodeTest, SetParentsTest)
{
	RepoNode node;

	auto parentIds1 = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		});

	node.addParents(parentIds1);
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds1));

	auto parentIds2 = std::vector<repo::lib::RepoUUID>({
	repo::lib::RepoUUID::createUUID(),
	repo::lib::RepoUUID::createUUID(),
		});

	node.setParents(parentIds2);
	 
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parentIds2));
}

TEST(RepoNodeTest, SetSharedIDTest)
{
	RepoNode node;
	EXPECT_TRUE(node.getSharedID().isDefaultValue());

	auto sharedId = repo::lib::RepoUUID::createUUID();
	node.setSharedID(sharedId);
	EXPECT_EQ(node.getSharedID(), sharedId);
}

TEST(RepoNodeTest, GetUniqueIDTest)
{
	RepoNode node;
	EXPECT_FALSE(node.getUniqueID().isDefaultValue());
}

TEST(RepoNodeTest, GetTypeTest)
{
	RepoNode node1;
	EXPECT_EQ(node1.getType(), "");
	EXPECT_EQ(node1.getTypeAsEnum(), NodeType::UNKNOWN);
}

TEST(RepoNodeTest, GetParentsIDTest)
{
	std::vector<repo::lib::RepoUUID> parent;
	size_t nParents = 10;

	for (size_t i = 0; i < nParents; ++i)
	{
		parent.push_back(repo::lib::RepoUUID::createUUID());
	}

	RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, repo::lib::RepoUUID::createUUID());
	builder.appendArray(REPO_NODE_LABEL_PARENTS, parent);
	RepoNode node = builder.obj();

	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parent));
}

TEST(RepoNodeTest, OperatorEqualTest)
{
	RepoNode typicalNode = makeTypicalNode();
	EXPECT_NE(makeRandomNode(), typicalNode);
	EXPECT_NE(makeRandomNode(), makeRandomNode());
	EXPECT_EQ(typicalNode, typicalNode);

	repo::lib::RepoUUID sharedID = repo::lib::RepoUUID::createUUID();
	EXPECT_NE(makeNode(repo::lib::RepoUUID::createUUID(), sharedID), makeNode(repo::lib::RepoUUID::createUUID(), sharedID));
	EXPECT_NE(makeNode(sharedID, repo::lib::RepoUUID::createUUID()), makeNode(sharedID, repo::lib::RepoUUID::createUUID()));
}

TEST(RepoNodeTest, OperatorCompareTest)
{
	RepoNode typicalNode = makeTypicalNode();
	EXPECT_FALSE(typicalNode > typicalNode);

	repo::lib::RepoUUID sharedID = repo::lib::RepoUUID::createUUID();
	repo::lib::RepoUUID uniqueID = repo::lib::RepoUUID::createUUID();
	repo::lib::RepoUUID sharedID2 = repo::lib::RepoUUID::createUUID();
	repo::lib::RepoUUID uniqueID2 = repo::lib::RepoUUID::createUUID();

	EXPECT_EQ(uniqueID > uniqueID, makeNode(uniqueID, sharedID, "1") > makeNode(uniqueID, sharedID, "1"));
	EXPECT_EQ(sharedID > sharedID2, makeNode(uniqueID, sharedID, "2") > makeNode(uniqueID, sharedID2, "2"));
	EXPECT_EQ(sharedID > sharedID2, makeNode(uniqueID, sharedID, "3") > makeNode(uniqueID2, sharedID2, "3"));
	EXPECT_EQ(sharedID > sharedID2, makeNode(uniqueID2, sharedID, "4") > makeNode(uniqueID, sharedID2, "4"));

	EXPECT_EQ(uniqueID2 < uniqueID, makeNode(uniqueID2, sharedID, "5") < makeNode(uniqueID, sharedID, "5"));
	EXPECT_EQ(sharedID < sharedID2, makeNode(uniqueID2, sharedID, "6") < makeNode(uniqueID, sharedID2, "6"));
	EXPECT_EQ(sharedID < sharedID2, makeNode(uniqueID, sharedID, "7") < makeNode(uniqueID, sharedID2, "7"));
	EXPECT_EQ(sharedID < sharedID2, makeNode(uniqueID, sharedID, "8") < makeNode(uniqueID2, sharedID2, "8"));

	EXPECT_EQ(uniqueID < uniqueID, makeNode(uniqueID, sharedID, "9") < makeNode(uniqueID, sharedID, "9"));
	EXPECT_EQ(uniqueID < uniqueID2, makeNode(uniqueID, sharedID, "10") < makeNode(uniqueID2, sharedID, "10"));
	EXPECT_EQ(uniqueID < uniqueID2, makeNode(uniqueID, sharedID, "11") < makeNode(uniqueID2, sharedID, "11"));
	EXPECT_EQ(uniqueID < uniqueID2, makeNode(uniqueID, sharedID, "12") < makeNode(uniqueID2, sharedID, "12"));

	EXPECT_EQ(uniqueID < uniqueID, makeNode(uniqueID, sharedID, "13") < makeNode(uniqueID, sharedID, "13"));
	EXPECT_EQ(uniqueID < uniqueID2, makeNode(uniqueID, sharedID, "14") < makeNode(uniqueID2, sharedID, "14"));
	EXPECT_EQ(uniqueID < uniqueID2, makeNode(uniqueID, sharedID, "15") < makeNode(uniqueID2, sharedID, "15"));
	EXPECT_EQ(uniqueID < uniqueID2, makeNode(uniqueID, sharedID, "16") < makeNode(uniqueID2, sharedID, "16"));
}

TEST(RepoNodeTest, RepoNodeSetTest)
{
	// Does the MaterialNode behave correctly when added to a RepoNodeSet?

	auto node1 = new RepoNode();
	auto node2 = new RepoNode();

	node2->setUniqueId(node1->getUniqueID());

	// Nodes have identical unique and sharedIds

	repo::core::model::RepoNodeSet nodes;
	nodes.insert(node1);
	nodes.insert(node2);

	EXPECT_THAT(nodes.size(), Eq(1));

	// Changing the unique Id of node2 makes it different from node1

	node2->setUniqueId(repo::lib::RepoUUID::createUUID());
	nodes.insert(node2);
	EXPECT_THAT(nodes.size(), Eq(2));

	// A new node that is identical to node1 and should not be added

	auto node3 = new RepoNode();
	node3->setUniqueId(node1->getUniqueID());
	nodes.insert(node3);
	EXPECT_THAT(nodes.size(), Eq(2));

	// But giving it a sharedId will allow it to be added
	node3->setSharedID(repo::lib::RepoUUID::createUUID());
	nodes.insert(node3);
	EXPECT_THAT(nodes.size(), Eq(3));

	// The current version of RepoNodeSet only considers the shared and uniqueIds,
	// not the sEqual response.
}