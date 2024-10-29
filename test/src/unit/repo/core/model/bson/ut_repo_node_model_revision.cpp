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

#include <repo/core/model/bson/repo_node_model_revision.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_matchers.h"


using namespace repo::core::model;
using namespace testing;

TEST(ModelRevisionNodeTest, Constructor)
{
	ModelRevisionNode node;
	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsTrue());
	EXPECT_THAT(node.getMessage(), IsEmpty());
	EXPECT_THAT(node.getType(), Eq(REPO_NODE_TYPE_REVISION));
	EXPECT_THAT(node.getTypeAsEnum(), Eq(NodeType::REVISION));
	EXPECT_THAT((int)node.getUploadStatus(), Eq((int)RevisionNode::UploadStatus::COMPLETE));
	EXPECT_THAT(node.getAuthor(), IsEmpty());
	EXPECT_THAT(node.getTimestamp(), Eq(0));
	EXPECT_THAT(node.getCoordOffset(), ElementsAre(0,0,0));
	EXPECT_THAT(node.getTag(), IsEmpty());
	EXPECT_THAT(node.getOrgFiles(), IsEmpty());
}

/*
 * Tests the construction of a MetadataNode from BSON. This is the reference
 * database schema and should be effectively idential to the serialise method.
 */
TEST(ModelRevisionNodeTest, Deserialise)
{
	RepoBSONBuilder builder;

	auto id = repo::lib::RepoUUID::createUUID();
	auto branch = repo::lib::RepoUUID::createUUID();
	auto user = "testUser";
	auto message = "this is a message";
	auto tag = "myTagV1.1";
	auto offset = repo::lib::RepoVector3D64(1000, -5000, 12390).toStdVector();
	auto files = std::vector<std::string>({ "/efs/hello/my/file/directory/and.file" });

	builder.append(REPO_NODE_LABEL_ID, id);
	builder.append(REPO_NODE_LABEL_SHARED_ID, branch);
	builder.append(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_REVISION);
	builder.append(REPO_NODE_REVISION_LABEL_AUTHOR, user);
	builder.append(REPO_NODE_REVISION_LABEL_MESSAGE, message);
	builder.append(REPO_NODE_REVISION_LABEL_TAG, tag);
	builder.appendTimeStamp(REPO_NODE_REVISION_LABEL_TIMESTAMP);
	builder.appendArray(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT, offset);
	builder.appendArray(REPO_NODE_REVISION_LABEL_REF_FILE, files);
	builder.append(REPO_NODE_REVISION_LABEL_INCOMPLETE, (uint32_t)RevisionNode::UploadStatus::GEN_REPO_STASH);

	auto node = ModelRevisionNode(builder.obj());

	EXPECT_THAT(node.getUniqueID(), Eq(id));
	EXPECT_THAT(node.getSharedID(), Eq(branch));
	EXPECT_THAT(node.getAuthor(), Eq(user));
	EXPECT_THAT(node.getMessage(), Eq(message));
	EXPECT_THAT(node.getTag(), Eq(tag));
	EXPECT_THAT(node.getCoordOffset(), Eq(offset));
	EXPECT_THAT(node.getOrgFiles(), Eq(files));
	EXPECT_THAT(node.getTimestamp(), IsNow()); // Within a second or so of the current time...
	EXPECT_THAT((uint32_t)node.getUploadStatus(), Eq((uint32_t)RevisionNode::UploadStatus::GEN_REPO_STASH));
}

TEST(ModelRevisionNodeTest, DeserialiseEmpty)
{
	// Make sure any default value fields are initialised correctly

	RepoBSONBuilder builder;

	auto id = repo::lib::RepoUUID::createUUID();
	auto branch = repo::lib::RepoUUID::createUUID();

	builder.append(REPO_NODE_LABEL_ID, id);
	builder.append(REPO_NODE_LABEL_SHARED_ID, branch);
	builder.append(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_REVISION);
	builder.appendTimeStamp(REPO_NODE_REVISION_LABEL_TIMESTAMP); // Timestamp for revision nodes is not optional

	auto node = ModelRevisionNode(builder.obj());

	EXPECT_THAT(node.getUniqueID(), Eq(id));
	EXPECT_THAT(node.getSharedID(), Eq(branch));
	EXPECT_THAT(node.getAuthor(), IsEmpty());
	EXPECT_THAT(node.getMessage(), IsEmpty());
	EXPECT_THAT(node.getTag(), IsEmpty());
	EXPECT_THAT(node.getCoordOffset(), ElementsAre(0,0,0));
	EXPECT_THAT(node.getOrgFiles(), IsEmpty());
	EXPECT_THAT(node.getTimestamp(), IsNow()); // Within a second or so of the current time...
	EXPECT_THAT((uint32_t)node.getUploadStatus(), Eq((uint32_t)0));
}


TEST(ModelRevisionNodeTest, Serialise)
{
	auto user = "testUser";
	auto message = "this is a message";
	auto tag = "myTagV1.1";
	auto offset = repo::lib::RepoVector3D64(1000, -5000, 12390).toStdVector();
	auto files = std::vector<std::string>({ "/efs/hello/my/file/directory/and.file" });

	auto node = ModelRevisionNode();

	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID).isDefaultValue(), IsFalse());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_SHARED_ID).isDefaultValue(), IsTrue()); // By convention the ModelRevisionNode always writes the sharedId, even if it is 0
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_TYPE), Eq(REPO_NODE_TYPE_REVISION));
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REVISION_LABEL_AUTHOR), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REVISION_LABEL_MESSAGE), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REVISION_LABEL_TAG), IsFalse());
	EXPECT_THAT(((RepoBSON)node).getTimeStampField(REPO_NODE_REVISION_LABEL_TIMESTAMP), Eq(0));
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT), IsTrue());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REVISION_LABEL_REF_FILE), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REVISION_LABEL_INCOMPLETE), IsFalse());

	node.setUniqueID(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID), Eq(node.getUniqueID()));

	node.setSharedID(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_SHARED_ID), Eq(node.getSharedID()));

	node.addParents({
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
	});
	EXPECT_THAT(((RepoBSON)node).getUUIDFieldArray(REPO_NODE_LABEL_PARENTS), UnorderedElementsAreArray(node.getParentIDs()));

	node.setAuthor(user);
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_REVISION_LABEL_AUTHOR), Eq(node.getAuthor()));

	node.setMessage(message);
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_REVISION_LABEL_MESSAGE), Eq(node.getMessage()));

	node.setTag(tag);
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_REVISION_LABEL_TAG), Eq(node.getTag()));

	node.setTimestamp(1000);
	EXPECT_THAT(((RepoBSON)node).getTimeStampField(REPO_NODE_REVISION_LABEL_TIMESTAMP), Eq(node.getTimestamp()));

	node.setCoordOffset(offset);
	EXPECT_THAT(((RepoBSON)node).getDoubleVectorField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT), Eq(node.getCoordOffset()));

	node.setFiles(files);
	EXPECT_THAT(((RepoBSON)node).getStringArray(REPO_NODE_REVISION_LABEL_REF_FILE), Eq(node.getOrgFiles()));

	node.updateStatus(RevisionNode::UploadStatus::COMPLETE);
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REVISION_LABEL_INCOMPLETE), IsFalse());

	node.updateStatus(RevisionNode::UploadStatus::GEN_SEL_TREE);
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_REVISION_LABEL_INCOMPLETE), Eq((int)RevisionNode::UploadStatus::GEN_SEL_TREE));

	node.updateStatus(RevisionNode::UploadStatus::GEN_WEB_STASH);
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_REVISION_LABEL_INCOMPLETE), Eq((int)RevisionNode::UploadStatus::GEN_WEB_STASH));

	// When setting back to complete, the field should go away again
	node.updateStatus(RevisionNode::UploadStatus::COMPLETE);
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_REVISION_LABEL_INCOMPLETE), IsFalse());
}

TEST(ModelRevisionNodeTest, Factory)
{
	auto user = "testUser";
	auto branch = repo::lib::RepoUUID::createUUID();
	auto revId = repo::lib::RepoUUID::createUUID();
	auto message = "this is a message";
	auto tag = "myTagV1.1";
	auto offset = repo::lib::RepoVector3D64(1000, -5000, 12390).toStdVector();
	auto files = std::vector<std::string>({ "/efs/hello/my/file/directory/and.file" });
	auto parents = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID()
	});

	auto node = RepoBSONFactory::makeRevisionNode(
		user,
		branch,
		revId,
		files,
		parents,
		offset,
		message,
		tag
	);

	EXPECT_THAT(node.getAuthor(), Eq(user));
	EXPECT_THAT(node.getUniqueID(), Eq(revId));
	EXPECT_THAT(node.getSharedID(), Eq(branch));
	EXPECT_THAT(node.getOrgFiles(), Eq(files));
	EXPECT_THAT(node.getCoordOffset(), Eq(offset));
	EXPECT_THAT(node.getParentIDs(), Eq(parents));
	EXPECT_THAT(node.getMessage(), Eq(message));
	EXPECT_THAT(node.getTag(), Eq(tag));
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

// This function is used by the CopyConstructor Test to return a stack-allocated
// copy of a MetadataNode on the stack.
static ModelRevisionNode makeRefNode()
{
	auto a = ModelRevisionNode();
	a.setSharedID(repo::lib::RepoUUID("e624aab0-f983-49fb-9263-1991288cb449"));
	a.setAuthor("author");
	a.setCoordOffset({ 0, 1000, 10000 });
	a.setMessage("message");
	return a;
}

// This function is used by the CopyConstructor Test to return a heap-allocated
// copy of a MetadataNode originally allocated on the stack.
static ModelRevisionNode* makeNewNode()
{
	auto a = makeRefNode();
	return new ModelRevisionNode(a);
}

TEST(ModelRevisionNodeTest, CopyConstructor)
{
	auto a = makeRefNode();

	auto b = a;
	EXPECT_THAT(a.sEqual(b), IsTrue());

	b.setCoordOffset({ 0, 5000, 0 });
	EXPECT_THAT(a.sEqual(b), IsFalse());

	auto c = new ModelRevisionNode(a);
	EXPECT_THAT(a.sEqual(*c), IsTrue());

	c->setCoordOffset({ 0, 5000, 0 });
	EXPECT_THAT(a.sEqual(*c), IsFalse());

	delete c;

	auto d = makeNewNode();
	EXPECT_THAT(a.sEqual(*d), IsTrue());

	d->setCoordOffset({ 0, 5000, 0 });
	EXPECT_THAT(a.sEqual(*d), IsFalse());

	delete d;

	auto e = makeRefNode();
	EXPECT_THAT(a.sEqual(e), IsTrue());

	e.setCoordOffset({ 0, 5000, 0 });
	EXPECT_THAT(a.sEqual(e), IsFalse());
}