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

#include <repo/core/model/bson/repo_node_reference.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;
using namespace testing;

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(RefNodeTest, Constructor)
{
	ReferenceNode node;
	EXPECT_THAT(node.getType(), Eq(REPO_NODE_TYPE_REFERENCE));
	EXPECT_THAT(node.getTypeAsEnum(), Eq(NodeType::REFERENCE));
	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsTrue());
	EXPECT_THAT(node.getName(), IsEmpty());
	EXPECT_THAT(node.getProjectRevision().isDefaultValue(), IsTrue());
	EXPECT_THAT(node.getDatabaseName().empty(), IsTrue());
	EXPECT_THAT(node.getProjectId().empty(), IsTrue());
	EXPECT_THAT(node.useSpecificRevision(), IsFalse());
}

TEST(RefNodeTest, Deserialise)
{
	auto project = repo::lib::RepoUUID::createUUID().toString(); // For legacy reasons the project reference is stored as a string
	auto teamspace = "myTeamspace";
	auto uniqueId = repo::lib::RepoUUID::createUUID();
	auto sharedId = repo::lib::RepoUUID::createUUID();
	auto name = "myName";
	auto revisionId = repo::lib::RepoUUID::createUUID();
	auto isUnique = true;
	auto revId = repo::lib::RepoUUID::createUUID();

	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_ID, uniqueId);
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId);
	builder.append(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_REFERENCE);
	builder.append(REPO_NODE_LABEL_NAME, name);
	builder.append(REPO_NODE_REVISION_ID, revId);
	builder.append(REPO_NODE_REFERENCE_LABEL_OWNER, teamspace);
	builder.append(REPO_NODE_REFERENCE_LABEL_PROJECT, project);
	builder.append(REPO_NODE_REFERENCE_LABEL_REVISION_ID, revisionId);
	builder.append(REPO_NODE_REFERENCE_LABEL_UNIQUE, isUnique);

	auto node = ReferenceNode(builder.obj());

	EXPECT_THAT(node.getUniqueID(), Eq(uniqueId));
	EXPECT_THAT(node.getSharedID(), Eq(sharedId));
	EXPECT_THAT(node.getRevision(), Eq(revId));
	EXPECT_THAT(node.getName(), Eq(name));
	EXPECT_THAT(node.getProjectRevision(), Eq(revisionId));
	EXPECT_THAT(node.getDatabaseName(), Eq(teamspace));
	EXPECT_THAT(node.getProjectId(), Eq(project));
	EXPECT_THAT(node.useSpecificRevision(), Eq(isUnique));
}

TEST(RefNodeTest, Serialise)
{
	auto node = ReferenceNode();

	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID).isDefaultValue(), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_SHARED_ID), IsFalse());
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_TYPE), Eq(REPO_NODE_TYPE_REFERENCE));
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_REFERENCE_LABEL_OWNER), IsEmpty());
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_REFERENCE_LABEL_PROJECT), IsEmpty());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REFERENCE_LABEL_REVISION_ID), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REFERENCE_LABEL_UNIQUE), IsFalse());

	node.setUniqueId(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID), node.getUniqueID());

	node.setSharedID(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_SHARED_ID), node.getSharedID());

	node.setRevision(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_REVISION_ID), node.getRevision());

	node.setDatabaseName("database");
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_REFERENCE_LABEL_OWNER), node.getDatabaseName());

	node.setProjectId("project");
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_REFERENCE_LABEL_PROJECT), node.getProjectId());

	node.setProjectRevision(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_REFERENCE_LABEL_REVISION_ID), node.getProjectRevision());

	node.setUseSpecificRevision(true);
	EXPECT_THAT(((RepoBSON)node).getBoolField(REPO_NODE_REFERENCE_LABEL_UNIQUE), node.useSpecificRevision());

	node.setUseSpecificRevision(false);
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REFERENCE_LABEL_UNIQUE), IsFalse());
}

TEST(RefNodeTest, Factory)
{
	auto project = repo::lib::RepoUUID::createUUID().toString(); // For legacy reasons the project reference is stored as a string
	auto database = "myTeamspace";
	auto uniqueId = repo::lib::RepoUUID::createUUID();
	auto name = "myName";
	auto revisionId = repo::lib::RepoUUID::createUUID();
	auto isUnique = true;
	auto revId = repo::lib::RepoUUID::createUUID();

	auto node = RepoBSONFactory::makeReferenceNode(
		database,
		project,
		revisionId,
		false,
		name
	);

	EXPECT_THAT(node.getDatabaseName(), Eq(database));
	EXPECT_THAT(node.getProjectId(), Eq(project));
	EXPECT_THAT(node.getProjectRevision(), Eq(revisionId));
	EXPECT_THAT(node.getRevision(), Eq(repo::lib::RepoUUID::defaultValue));
	EXPECT_THAT(node.useSpecificRevision(), IsFalse());
	EXPECT_THAT(node.getName(), name);

	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsFalse()); // By convention, factory produced ReferenceNodes have SharedIds

	// That the name is automatically produced if name is omitted

	node = RepoBSONFactory::makeReferenceNode(
		database,
		project,
		revisionId,
		false
	);

	EXPECT_THAT(node.getName(), Eq(database + std::string(".") + project));

}

TEST(RefNodeTest, Methods)
{
	auto node = ReferenceNode();

	auto projectRevision = repo::lib::RepoUUID::createUUID();
	auto projectId = repo::lib::RepoUUID::createUUID().toString();
	auto sharedId = repo::lib::RepoUUID::createUUID();
	auto uniqueId = repo::lib::RepoUUID::createUUID();
	auto name = "myName";
	auto database = "database";

	node.setProjectId(projectId);
	EXPECT_THAT(node.getProjectId(), Eq(projectId));

	node.setProjectRevision(projectRevision);
	EXPECT_THAT(node.getProjectRevision(), Eq(projectRevision));

	node.setSharedID(sharedId);
	EXPECT_THAT(node.getSharedID(), Eq(sharedId));

	node.changeName(name);
	EXPECT_THAT(node.getName(), Eq(name));

	node.setDatabaseName(database);
	EXPECT_THAT(node.getDatabaseName(), Eq(database));

	node.setUseSpecificRevision(true);
	EXPECT_THAT(node.useSpecificRevision(), IsTrue());

	node.setUseSpecificRevision(false);
	EXPECT_THAT(node.useSpecificRevision(), IsFalse());
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

// This function is used by the CopyConstructor Test to return a stack-allocated
// copy of a MetadataNode on the stack.
ReferenceNode makeRefNode()
{
	auto a = ReferenceNode();
	a.setSharedID(repo::lib::RepoUUID("e624aab0-f983-49fb-9263-1991288cb449"));
	a.setProjectRevision(repo::lib::RepoUUID("debd2287-36ca-4584-bce0-56487a1882c0"));
	a.setProjectId("764c3c91-1c30-4ad7-ae5f-406c05d1f96a");
	a.setDatabaseName("database");
	return a;
}

// This function is used by the CopyConstructor Test to return a heap-allocated
// copy of a MetadataNode originally allocated on the stack.
ReferenceNode* makeNewNode()
{
	auto a = makeRefNode();
	return new ReferenceNode(a);
}

TEST(RefNodeTest, CopyConstructor)
{
	auto a = makeRefNode();

	auto b = a;
	EXPECT_THAT(a.sEqual(b), IsTrue());

	b.setProjectRevision(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(a.sEqual(b), IsFalse());

	auto c = new ReferenceNode(a);
	EXPECT_THAT(a.sEqual(*c), IsTrue());

	c->setProjectRevision(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(a.sEqual(*c), IsFalse());

	delete c;

	auto d = makeNewNode();
	EXPECT_THAT(a.sEqual(*d), IsTrue());

	d->setProjectRevision(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(a.sEqual(*d), IsFalse());

	delete d;

	auto e = makeRefNode();
	EXPECT_THAT(a.sEqual(e), IsTrue());

	e.setProjectRevision(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(a.sEqual(e), IsFalse());
}