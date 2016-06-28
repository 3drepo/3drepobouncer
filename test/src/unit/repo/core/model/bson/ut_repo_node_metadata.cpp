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

#include <repo/core/model/bson/repo_node_metadata.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;

static MetadataNode makeRandomMetaNode()
{
	uint32_t nItems = rand() % 10 + 1;
	std::vector<std::string> keys, values;
	for (int i = 0; i < nItems; ++i)
	{
		keys.push_back(getRandomString(rand() % 10 + 1));
		values.push_back(getRandomString(rand() % 10 + 1));
	}
	return	RepoBSONFactory::makeMetaDataNode(keys, values);
}

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(MetaNodeTest, Constructor)
{
	MetadataNode empty;

	EXPECT_TRUE(empty.isEmpty());
	EXPECT_EQ(NodeType::METADATA, empty.getTypeAsEnum());

	auto repoBson = RepoBSON(BSON("test" << "blah" << "test2" << 2));

	auto fromRepoBSON = MetadataNode(repoBson);
	EXPECT_EQ(NodeType::METADATA, fromRepoBSON.getTypeAsEnum());
	EXPECT_EQ(fromRepoBSON.nFields(), repoBson.nFields());
	EXPECT_EQ(0, fromRepoBSON.getFileList().size());
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
	auto metaNode2 = makeRandomMetaNode();

	EXPECT_TRUE(metaNode.sEqual(metaNode));
	EXPECT_FALSE(metaNode.sEqual(empty));
	EXPECT_FALSE(metaNode.sEqual(metaNode2));
}

TEST(MetaNodeTest, CloneAndAddMetadataTest)
{
	MetadataNode empty;

	auto metaNode = makeRandomMetaNode();
	auto metaNode2 = makeRandomMetaNode();

	auto meta1 = metaNode.getObjectField(REPO_NODE_LABEL_METADATA);
	auto meta2 = metaNode2.getObjectField(REPO_NODE_LABEL_METADATA);
	auto updatedMeta = empty.cloneAndAddMetadata(meta1);

	updatedMeta.sEqual(metaNode);

	auto updatedMeta2 = updatedMeta.cloneAndAddMetadata(meta2);

	auto resMeta = updatedMeta2.getObjectField(REPO_NODE_LABEL_METADATA);

	std::set<std::string> fieldNames1, fieldNames2;
	meta1.getFieldNames(fieldNames1);
	meta2.getFieldNames(fieldNames2);

	for (const auto &fieldName : fieldNames1)
	{
		//Skip the ones that would be overwritten
		if (meta2.hasField(fieldName)) continue;
		ASSERT_TRUE(resMeta.hasField(fieldName));
		EXPECT_EQ(resMeta.getField(fieldName), meta1.getField(fieldName));
	}

	for (const auto &fieldName : fieldNames2)
	{
		ASSERT_TRUE(resMeta.hasField(fieldName));
		EXPECT_EQ(resMeta.getField(fieldName), meta2.getField(fieldName));
	}
}