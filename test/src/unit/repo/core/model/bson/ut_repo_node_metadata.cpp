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

#include <repo/core/model/bson/repo_node_metadata.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::core::model;
using namespace testing;

std::unordered_map<std::string, repo::lib::RepoVariant> makeRandomMetadata(bool resetSeed = true)
{
	if (resetSeed) {
		restartRand();
	}
	std::unordered_map<std::string, repo::lib::RepoVariant> data;
	uint32_t nItems = rand() % 100 + 1;
	for (int i = 0; i < nItems; ++i)
	{
		repo::lib::RepoVariant v;
		switch (rand() % 6)
		{
		case 0:
			v = (bool)(rand() > RAND_MAX / 2);
			break;
		case 1:
			v = (int)(rand());
			break;
		case 2:
			v = (int64_t)(rand());
			break;
		case 3:
			v = (double)(rand());
			break;
		case 4:
			v = getRandomString(rand() % 10 + 1);
			break;
		case 5:
			v = getRandomTm();
		}
		data[getRandomString(rand() % 10 + 1)] = v;
	}
	return data;
}

MetadataNode makeRandomMetaNode(bool resetSeed = true)
{
	auto m = makeRandomMetadata(resetSeed);
	std::vector<std::string> keys;
	std::vector<repo::lib::RepoVariant> values;
	for (auto& p : m) {
		keys.push_back(p.first);
		values.push_back(p.second);
	}
	return RepoBSONFactory::makeMetaDataNode(keys, values);
}

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(MetaNodeTest, Constructor)
{
	MetadataNode empty;
	EXPECT_EQ(NodeType::METADATA, empty.getTypeAsEnum());
	EXPECT_FALSE(empty.getUniqueID().isDefaultValue());
	EXPECT_TRUE(empty.getSharedID().isDefaultValue());
}

TEST(MetaNodeTest, Serialise)
{
	auto node = makeRandomMetaNode();
	auto bson = (RepoBSON)node;

	EXPECT_THAT(bson.hasField(REPO_NODE_LABEL_ID), IsTrue());
	EXPECT_THAT(bson.hasField(REPO_NODE_LABEL_SHARED_ID), IsTrue());
	EXPECT_THAT(bson.hasField(REPO_NODE_LABEL_TYPE), IsTrue());
	EXPECT_THAT(bson.hasField(REPO_NODE_LABEL_METADATA), IsTrue());
}

/*
 * Tests the construction of a MetadataNode from BSON. This is the reference
 * database schema and should be effectively idential to the serialise method.
 */
TEST(MetaNodeTest, Deserialise)
{
	auto data = makeRandomMetadata();

	RepoBSONBuilder builder;
	builder.append(REPO_NODE_LABEL_ID, repo::lib::RepoUUID::createUUID());
	builder.append(REPO_NODE_LABEL_SHARED_ID, repo::lib::RepoUUID::createUUID());
	builder.append(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_METADATA);

	std::vector<RepoBSON> metaEntries;
	for (const auto& entry : data) {
		std::string key = entry.first;
		repo::lib::RepoVariant value = entry.second;

		if (!key.empty())
		{
			RepoBSONBuilder metaEntryBuilder;
			metaEntryBuilder.append(REPO_NODE_LABEL_META_KEY, key);

			// Pass variant on to the builder
			metaEntryBuilder.appendRepoVariant(REPO_NODE_LABEL_META_VALUE, value);

			metaEntries.push_back(metaEntryBuilder.obj());
		}
	}
	builder.appendArray(REPO_NODE_LABEL_METADATA, metaEntries);

	MetadataNode node(builder.obj());

	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getAllMetadata(), Eq(data));
	EXPECT_THAT(node.getAllMetadata().size(), Gt(0));
	EXPECT_THAT(node.getType(), Eq(REPO_NODE_TYPE_METADATA));
}

TEST(MetaNodeTest, Factory)
{
	auto m = makeRandomMetadata();
	std::vector<std::string> keys;
	std::vector<repo::lib::RepoVariant> values;
	for (auto& p : m) {
		keys.push_back(p.first);
		values.push_back(p.second);
	}

	auto node = RepoBSONFactory::makeMetaDataNode(keys, values);

	EXPECT_THAT(node.getUniqueID().isDefaultValue(), IsFalse());
	EXPECT_THAT(node.getSharedID().isDefaultValue(), IsFalse()); // The factory method should give MetadataNode a shared Id
	EXPECT_THAT(node.getName().empty(), IsTrue());
	EXPECT_THAT(node.getAllMetadata(), Eq(m));
}

TEST(MetaNodeTest, TypeTest)
{
	MetadataNode node;

	EXPECT_EQ(REPO_NODE_TYPE_METADATA, node.getType());
	EXPECT_EQ(NodeType::METADATA, node.getTypeAsEnum());
}

TEST(MetaNodeTest, PositionDependantTest)
{
	MetadataNode node;
	EXPECT_FALSE(node.positionDependant());
}

TEST(MetaNodeTest, SEqualTest)
{
	MetadataNode empty;
	EXPECT_TRUE(empty.sEqual(empty));

	auto metaNode = makeRandomMetaNode();
	auto metaNode2 = makeRandomMetaNode(false);

	EXPECT_TRUE(metaNode.sEqual(metaNode));
	EXPECT_FALSE(metaNode.sEqual(empty));
	EXPECT_FALSE(metaNode.sEqual(metaNode2));
}

TEST(MetaNodeTest, Sanitisation)
{
	// Sanitisation should take place as soon as the key is added, so the state
	// of the node at any given time matches what will be in the database

	std::unordered_map<std::string, repo::lib::RepoVariant> before = {
		{ "a", repo::lib::RepoVariant("b") },
		{ "c$d", repo::lib::RepoVariant("e") },
		{ "f..g",repo::lib::RepoVariant("h") }
	};

	std::unordered_map<std::string, repo::lib::RepoVariant> after = {
		{ "a", repo::lib::RepoVariant("b") },
		{ "c:d", repo::lib::RepoVariant("e") },
		{ "f::g",repo::lib::RepoVariant("h") }
	};

	MetadataNode node;

	node.setMetadata(before);
	EXPECT_THAT(node.getAllMetadata(), Eq(after));

	node = RepoBSONFactory::makeMetaDataNode(before);
	EXPECT_THAT(node.getAllMetadata(), Eq(after));

}

// This function is used by the CopyConstructor Test to return a heap-allocated
// copy of a MetadataNode originally allocated on the stack.
static MetadataNode* makeNewMetaNode()
{
	auto a = makeRandomMetaNode();
	return new MetadataNode(a);
}

// This function is used by the CopyConstructor Test to return a stack-allocated
// copy of a MetadataNode on the stack.
static MetadataNode makeRefMetaNode()
{
	auto m = makeRandomMetaNode();
	return m;
}

TEST(MetaNodeTest, CopyConstructor)
{
	auto a = makeRandomMetaNode();

	std::unordered_map<std::string, repo::lib::RepoVariant> other = {
		{ "a", repo::lib::RepoVariant("b") },
		{ "c:d", repo::lib::RepoVariant("e") },
		{ "f::g",repo::lib::RepoVariant("h") }
	};

	auto b = a;
	EXPECT_THAT(a.sEqual(b), IsTrue());

	b.setMetadata(other);
	EXPECT_THAT(a.sEqual(b), IsFalse());

	auto c = new MetadataNode(a);
	EXPECT_THAT(a.sEqual(*c), IsTrue());

	c->setMetadata(other);
	EXPECT_THAT(a.sEqual(*c), IsFalse());

	delete c;

	auto d = makeNewMetaNode();
	EXPECT_THAT(a.sEqual(*d), IsTrue());

	d->setMetadata(other);
	EXPECT_THAT(a.sEqual(*d), IsFalse());

	delete d;

	auto e = makeRefMetaNode();
	EXPECT_THAT(a.sEqual(e), IsTrue());

	e.setMetadata(other);
	EXPECT_THAT(a.sEqual(e), IsFalse());
}

TEST(MetaNodeTest, EmptyValues)
{
	// Check that empty strings are supported. Currently only the string type
	// has the concept of an empty value.

	RepoBSONBuilder builder;
	builder.append(REPO_NODE_LABEL_ID, repo::lib::RepoUUID::createUUID());
	builder.append(REPO_NODE_LABEL_SHARED_ID, repo::lib::RepoUUID::createUUID());
	builder.append(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_METADATA);

	std::vector<RepoBSON> metaEntries;
	{
		RepoBSONBuilder metaEntryBuilder;
		metaEntryBuilder.append(REPO_NODE_LABEL_META_KEY, std::string("myKey"));
		metaEntryBuilder.append(REPO_NODE_LABEL_META_VALUE, std::string(""));
		metaEntries.push_back(metaEntryBuilder.obj());
	}
	builder.appendArray(REPO_NODE_LABEL_METADATA, metaEntries);

	MetadataNode node(builder.obj());

	auto metadata = node.getAllMetadata();
	auto& value = metadata["myKey"];

	EXPECT_THAT(value, Eq(repo::lib::RepoVariant(std::string(""))));
}