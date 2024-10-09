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
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include <repo/core/model/bson/repo_node_transformation.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_mesh_utils.h"

using namespace repo::core::model;
using namespace testing;

std::vector<float> identity = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1 
};

std::vector<float> notId = { 
	1, 2, 3, 4,
	5, 6, 7, 8,
	9, 0.3f, 10, 11,
	5342, 31, 0.6f, 12 
};

std::vector<float> idInBoundary ={
	1, 0, 0, 0,
	0, 1, 0, (float)1e-6,
	0, 0, 1, 0,
	0, (float)1e-6, (float)1e-6, 1 
};

std::vector<float> notIdInBoundary = {
	1, 0, 0, 0,
	0, 1, 0, (float)2e-5,
	0, 0, 2, 0,
	0, (float)2e-5, (float)2e-5, 1 
};

// This function is used by the CopyConstructor Test to return a stack-allocated
// copy of a TransformationNode on the stack.
TransformationNode makeRefNode()
{
	auto a = TransformationNode();
	a.setSharedID(repo::lib::RepoUUID::createUUID());
	std::srand(0);
	a.setTransformation(repo::test::utils::mesh::makeTransform(true, true));
	return a;
}

// This function is used by the CopyConstructor Test to return a heap-allocated
// copy of a TransformationNode originally allocated on the stack.
TransformationNode* makeNewNode()
{
	auto a = makeRefNode();
	return new TransformationNode(a);
}

TEST(RepoTransformationNodeTest, Constructor)
{
	auto empty = TransformationNode();
	EXPECT_THAT(empty.getTypeAsEnum(), Eq(NodeType::TRANSFORMATION));
	EXPECT_THAT(empty.getType(), Eq(REPO_NODE_TYPE_TRANSFORMATION));
	EXPECT_THAT(empty.getTransMatrix().isIdentity(), IsTrue());
	EXPECT_THAT(empty.isIdentity(), IsTrue());
	EXPECT_THAT(empty.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(empty.getSharedID().isDefaultValue(), IsTrue());
}

TEST(RepoTransformationNodeTest, IdentityTest)
{
	auto node = TransformationNode();
	EXPECT_THAT(node.isIdentity(), IsTrue());

	node.setTransformation(repo::lib::RepoMatrix(idInBoundary));
	EXPECT_THAT(node.isIdentity(), IsTrue());

	node.setTransformation(repo::lib::RepoMatrix(notId));
	EXPECT_THAT(node.isIdentity(), IsFalse());

	node.setTransformation(repo::lib::RepoMatrix(notIdInBoundary));
	EXPECT_THAT(node.isIdentity(), IsFalse());
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
	EXPECT_TRUE(node.positionDependant());
}

TEST(RepoTransformationNodeTest, SEqualTest)
{
	auto empty1 = TransformationNode();
	auto empty2 = TransformationNode();

	TransformationNode notEmpty1;
	notEmpty1.setTransformation(notId);

	TransformationNode notEmpty2;
	notEmpty2.setTransformation(notId);

	TransformationNode notEmpty3;
	notEmpty3.setTransformation(identity);

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

TEST(RepoTransformationNodeTest, Serialise)
{
	TransformationNode node;

	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID).isDefaultValue(), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_SHARED_ID), IsFalse());
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_TYPE), Eq(REPO_NODE_TYPE_TRANSFORMATION));
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_NAME), IsFalse());

	node.setUniqueId(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID), node.getUniqueID());

	node.setSharedID(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_SHARED_ID), node.getSharedID());

	node.setRevision(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_REVISION_ID), node.getRevision());

	node.changeName("name");
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_NAME), node.getName());

	EXPECT_THAT(((RepoBSON)node).getMatrixField(REPO_NODE_LABEL_MATRIX), Eq(repo::lib::RepoMatrix()));

	node.setTransformation(repo::test::utils::mesh::makeTransform(true, true));
	EXPECT_THAT(((RepoBSON)node).getMatrixField(REPO_NODE_LABEL_MATRIX), Eq(node.getTransMatrix()));
}

TEST(RepoTransformationNodeTest, Deserialise)
{
	auto uniqueId = repo::lib::RepoUUID::createUUID();
	auto sharedId = repo::lib::RepoUUID::createUUID();
	auto name = "myName";
	auto revisionId = repo::lib::RepoUUID::createUUID();
	auto m = repo::test::utils::mesh::makeTransform(true, true);
	auto parents = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
	});

	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_ID, uniqueId);
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId);
	builder.append(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_TRANSFORMATION);
	builder.appendArray(REPO_NODE_LABEL_PARENTS, parents);
	builder.append(REPO_NODE_LABEL_NAME, name);
	builder.append(REPO_NODE_REVISION_ID, revisionId);
	builder.append(REPO_NODE_LABEL_MATRIX, m);

	auto node = TransformationNode(builder.obj());

	EXPECT_THAT(node.getUniqueID(), Eq(uniqueId));
	EXPECT_THAT(node.getSharedID(), Eq(sharedId));
	EXPECT_THAT(node.getRevision(), Eq(revisionId));
	EXPECT_THAT(node.getName(), Eq(name));
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parents));
	EXPECT_THAT(node.getTransMatrix(), Eq(m));
}

TEST(RepoTransformationNodeTest, Factory)
{
	auto node = RepoBSONFactory::makeTransformationNode();

	EXPECT_THAT(node.getName(), Eq("<transformation>"));
	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getTransMatrix(), Eq(repo::lib::RepoMatrix()));

	auto m = repo::test::utils::mesh::makeTransform(true, true);

	node = RepoBSONFactory::makeTransformationNode(m);

	EXPECT_THAT(node.getName(), Eq("<transformation>"));
	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getTransMatrix(), Eq(m));

	node = RepoBSONFactory::makeTransformationNode(m, "myName");

	EXPECT_THAT(node.getName(), Eq("myName"));
	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getTransMatrix(), Eq(m));

	auto parents = std::vector < repo::lib::RepoUUID>({
			repo::lib::RepoUUID::createUUID()
		});

	node = RepoBSONFactory::makeTransformationNode(m, "myName", parents);

	EXPECT_THAT(node.getName(), Eq("myName"));
	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getTransMatrix(), Eq(m));
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parents));
}

TEST(RepoTransformationNodeTest, GetTransMatrixTest)
{
	TransformationNode node;
	EXPECT_THAT(node.getTransMatrix(), Eq(repo::lib::RepoMatrix()));

	auto m = repo::test::utils::mesh::makeTransform(true, true);
	node.setTransformation(m);
	EXPECT_THAT(node.getTransMatrix(), Eq(m));

	m = repo::test::utils::mesh::makeTransform(true, true);
	node.setTransformation(m);
	EXPECT_THAT(node.getTransMatrix(), Eq(m));

	node = TransformationNode();
	m = repo::test::utils::mesh::makeTransform(true, true);
	node.applyTransformation(m); // Applying a matrix to an Identity transform should result in that same matrix
	EXPECT_THAT(node.getTransMatrix(), Eq(m));
}

TEST(RepoTransformationNodeTest, CopyConstructor)
{
	auto a = makeRefNode();

	auto b = a;
	EXPECT_THAT(a.sEqual(b), IsTrue());

	b.setTransformation(repo::test::utils::mesh::makeTransform(true,true));
	EXPECT_THAT(a.sEqual(b), IsFalse());

	auto c = new TransformationNode(a);
	EXPECT_THAT(a.sEqual(*c), IsTrue());

	c->setTransformation(repo::test::utils::mesh::makeTransform(true, true));
	EXPECT_THAT(a.sEqual(*c), IsFalse());

	delete c;

	auto d = makeNewNode();
	EXPECT_THAT(a.sEqual(*d), IsTrue());

	d->setTransformation(repo::test::utils::mesh::makeTransform(true, true));
	EXPECT_THAT(a.sEqual(*d), IsFalse());

	delete d;

	auto e = makeRefNode();
	EXPECT_THAT(a.sEqual(e), IsTrue());

	e.setTransformation(repo::test::utils::mesh::makeTransform(true, true));
	EXPECT_THAT(a.sEqual(e), IsFalse());
}