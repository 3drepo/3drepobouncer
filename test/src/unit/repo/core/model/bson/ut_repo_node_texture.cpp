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

#include <repo/core/model/bson/repo_node_texture.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;
using namespace testing;

static std::vector<uint8_t> makeRandomData()
{
	std::vector<uint8_t> data;
	for (int i = 0; i < 10000; i++)
	{
		data.push_back(rand() % 256);
	}
	return data;
}

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(TextureNodeTest, Constructor)
{
	TextureNode node;
	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsTrue());
	EXPECT_THAT(node.getRevision().isDefaultValue(), IsTrue());
	EXPECT_THAT(node.getName(), IsEmpty());
	EXPECT_THAT(node.getWidth(), Eq(0));
	EXPECT_THAT(node.getHeight(), Eq(0));
	EXPECT_THAT(node.getFileExtension(), IsEmpty());
	EXPECT_THAT(node.getRawData(), IsEmpty());
}

TEST(TextureNodeTest, TypeTest)
{
	TextureNode node;
	EXPECT_EQ(REPO_NODE_TYPE_TEXTURE, node.getType());
	EXPECT_EQ(NodeType::TEXTURE, node.getTypeAsEnum());
}

TEST(TextureNodeTest, PositionDependantTest)
{
	TextureNode node;
	EXPECT_FALSE(node.positionDependant());
}

TEST(TextureNodeTest, Deserialise)
{
	auto uniqueId = repo::lib::RepoUUID::createUUID();
	auto sharedId = repo::lib::RepoUUID::createUUID();
	auto name = "myName";
	auto revisionId = repo::lib::RepoUUID::createUUID();
	auto width = 1024;
	auto height = 768;
	auto ext = "png";
	auto data = makeRandomData();

	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_ID, uniqueId);
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId);
	builder.append(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_TEXTURE);
	builder.append(REPO_NODE_LABEL_NAME, name);
	builder.append(REPO_NODE_REVISION_ID, revisionId);
	builder.append(REPO_LABEL_WIDTH, width);
	builder.append(REPO_LABEL_HEIGHT, height);
	builder.append(REPO_NODE_LABEL_EXTENSION, ext);
	builder.appendLargeArray(REPO_LABEL_DATA, data);

	auto node = TextureNode(builder.obj());

	EXPECT_THAT(node.getUniqueID(), Eq(uniqueId));
	EXPECT_THAT(node.getSharedID(), Eq(sharedId));
	EXPECT_THAT(node.getRevision(), Eq(revisionId));
	EXPECT_THAT(node.getName(), Eq(name));
	EXPECT_THAT(node.getWidth(), Eq(width));
	EXPECT_THAT(node.getHeight(), Eq(height));
	EXPECT_THAT(node.getFileExtension(), Eq(ext));
	EXPECT_THAT(node.getRawData(), Eq(data));
}

TEST(TextureNodeTest, Serialise)
{
	TextureNode node;

	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID).isDefaultValue(), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_SHARED_ID), IsFalse());
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_TYPE), Eq(REPO_NODE_TYPE_TEXTURE));
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_NAME), IsFalse());
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_LABEL_WIDTH), node.getWidth());
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_LABEL_HEIGHT), node.getHeight());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_EXTENSION), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasBinField(REPO_LABEL_DATA), IsFalse());

	node.setUniqueID(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_ID), node.getUniqueID());

	node.setSharedID(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_SHARED_ID), node.getSharedID());

	node.setRevision(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_REVISION_ID), node.getRevision());

	node.changeName("name");
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_NAME), node.getName());

	node.setData(
		makeRandomData(),
		1024,
		768,
		"png"
	);
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_LABEL_WIDTH), node.getWidth());
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_LABEL_HEIGHT), node.getHeight());
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_EXTENSION), node.getFileExtension());
	std::vector<uint8_t> d2;
	((RepoBSON)node).getBinaryFieldAsVector(REPO_LABEL_DATA, d2);
	EXPECT_THAT(d2, node.getRawData());
}

TEST(TextureNodeTest, Factory)
{
	auto data = makeRandomData();

	auto width = 1024;
	auto height = 768;

	auto node = RepoBSONFactory::makeTextureNode("MyFilename.jpg", (char*)data.data(), data.size(), width, height);

	EXPECT_THAT(node.isEmpty(), IsFalse());

	// By convention, sharedId is always initialised in the factory
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsFalse());

	// The filename should become the node's name
	EXPECT_THAT(node.getName(), Eq("MyFilename.jpg"));

	// The factory extracts the extension from the filename
	EXPECT_THAT(node.getFileExtension(), Eq("jpg"));

	EXPECT_THAT(node.getWidth(), width);
	EXPECT_THAT(node.getHeight(), height);
	EXPECT_THAT(node.getRawData(), Eq(data));

	node = RepoBSONFactory::makeTextureNode("", (char*)data.data(), data.size(), width, height);

	// No filename results in empty extension
	EXPECT_THAT(node.getName(), IsEmpty());
	EXPECT_THAT(node.getFileExtension(), IsEmpty());
	EXPECT_THAT(node.getWidth(), width);
	EXPECT_THAT(node.getHeight(), height);
	EXPECT_THAT(node.getRawData(), Eq(data));

	// No texture content (should show a warning, but otherwise succeed)
	node = RepoBSONFactory::makeTextureNode("", 0, 0, width, height);
	EXPECT_THAT(node.getRawData(), IsEmpty());
	EXPECT_THAT(node.getWidth(), width);
	EXPECT_THAT(node.getHeight(), height);
	EXPECT_THAT(node.getName(), IsEmpty());
	EXPECT_THAT(node.getFileExtension(), IsEmpty());
	EXPECT_THAT(node.isEmpty(), IsTrue());

	// A filename but with no extension
	node = RepoBSONFactory::makeTextureNode("/efs/my/directory/file", (char*)data.data(), data.size(), width, height);
	EXPECT_THAT(node.getName(), Eq("/efs/my/directory/file"));
	EXPECT_THAT(node.getFileExtension(), IsEmpty());
	EXPECT_THAT(node.getWidth(), width);
	EXPECT_THAT(node.getHeight(), height);
	EXPECT_THAT(node.getRawData(), Eq(data));

	// And with parents

	auto parents = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
	});

	node = RepoBSONFactory::makeTextureNode("/efs/my/directory/file", (char*)data.data(), data.size(), width, height, parents);

	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parents));
}

TEST(TextureNodeTest, Methods)
{
	// All the texture node members are highly coupled, so there is just one method
	// to update it.

	auto data1 = makeRandomData();

	TextureNode node;
	node.setData(data1, 1099, 1081, "jpg");

	EXPECT_THAT(node.getWidth(), Eq(1099));
	EXPECT_THAT(node.getHeight(), Eq(1081));
	EXPECT_THAT(node.getFileExtension(), Eq("jpg"));
	EXPECT_THAT(node.getRawData(), Eq(data1));
}

TEST(TextureNodeTest, SEqualTest)
{
	TextureNode empty;
	EXPECT_TRUE(empty.sEqual(empty));

	auto data1 = makeRandomData();
	auto data2 = makeRandomData();

	// For semantic comparison, the only thing that should matter is the texture
	// contents.

	std::vector<TextureNode> nodes1;
	std::vector<TextureNode> nodes2;

	nodes1.push_back(RepoBSONFactory::makeTextureNode("text.png", (char*)data1.data(), data1.size(), 1024, 768));
	nodes1.push_back(RepoBSONFactory::makeTextureNode("text.jpg", (char*)data1.data(), data1.size(), 1024, 768));
	nodes1.push_back(RepoBSONFactory::makeTextureNode("text.png", (char*)data1.data(), data1.size(), 1024, 768));
	nodes1.push_back(RepoBSONFactory::makeTextureNode("text.png", (char*)data1.data(), data1.size(), 768, 768));
	nodes1.push_back(RepoBSONFactory::makeTextureNode("text.png", (char*)data1.data(), data1.size(), 1024, 1024));

	nodes2.push_back(RepoBSONFactory::makeTextureNode("text.png", (char*)data2.data(), data2.size(), 1024, 768));
	nodes2.push_back(RepoBSONFactory::makeTextureNode("text.jpg", (char*)data2.data(), data2.size(), 1024, 768));
	nodes2.push_back(RepoBSONFactory::makeTextureNode("text.png", (char*)data2.data(), data2.size(), 1024, 768));
	nodes2.push_back(RepoBSONFactory::makeTextureNode("text.png", (char*)data2.data(), data2.size(), 768, 768));
	nodes2.push_back(RepoBSONFactory::makeTextureNode("text.png", (char*)data2.data(), data2.size(), 1024, 1024));

	// The nodes between the two lists should be different
	for (size_t i = 0; i < nodes1.size(); i++)
	{
		EXPECT_FALSE(nodes1[i].sEqual(nodes2[i]));
	}

	// And within the lists should be the same
	for (size_t i = 0; i < nodes1.size(); i++)
	{
		auto& outer = nodes1[i];
		for (size_t j = 0; j < nodes1.size(); j++)
		{
			auto& inner = nodes1[j];
			EXPECT_TRUE(inner.sEqual(outer));
		}
	}
}

// This function is used by the CopyConstructor Test to return a stack-allocated
// copy of a TextureNode on the stack.
TextureNode makeRefNode()
{
	auto a = TextureNode();
	a.setSharedID(repo::lib::RepoUUID("e624aab0-f983-49fb-9263-1991288cb449"));
	a.setRevision(repo::lib::RepoUUID("ff802471-3e81-455e-9c1e-907040504325"));
	auto data = makeRandomData();
	a.setData(data, 1024, 768, "jpg");
	return a;
}

// This function is used by the CopyConstructor Test to return a heap-allocated
// copy of a TextureNode originally allocated on the stack.
TextureNode* makeNewNode()
{
	auto a = makeRefNode();
	return new TextureNode(a);
}

TEST(TextureNodeTest, CopyConstructor)
{
	restartRand(); // Ensure nodes generated from the functions above are identical
	auto a = makeRefNode();

	auto b = a;
	EXPECT_THAT(a.sEqual(b), IsTrue());

	b.setData({}, 100, 100);
	EXPECT_THAT(a.sEqual(b), IsFalse());

	auto c = new TextureNode(a);
	EXPECT_THAT(a.sEqual(*c), IsTrue());

	c->setData({}, 100, 100);
	EXPECT_THAT(a.sEqual(*c), IsFalse());

	delete c;

	restartRand();
	auto d = makeNewNode();
	EXPECT_THAT(a.sEqual(*d), IsTrue());

	d->setData({}, 100, 100);
	EXPECT_THAT(a.sEqual(*d), IsFalse());

	delete d;

	restartRand();
	auto e = makeRefNode();
	EXPECT_THAT(a.sEqual(e), IsTrue());

	e.setData({}, 100, 100);
	EXPECT_THAT(a.sEqual(e), IsFalse());
}