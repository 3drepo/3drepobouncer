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

#include <repo/core/model/bson/repo_node_texture.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(TextureNodeTest, Constructor)
{
	TextureNode empty;

	EXPECT_TRUE(empty.isEmpty());
	EXPECT_EQ(NodeType::TEXTURE, empty.getTypeAsEnum());

	auto repoBson = RepoBSON(BSON("test" << "blah" << "test2" << 2));

	auto fromRepoBSON = TextureNode(repoBson);
	EXPECT_EQ(NodeType::TEXTURE, fromRepoBSON.getTypeAsEnum());
	EXPECT_EQ(fromRepoBSON.nFields(), repoBson.nFields());
	EXPECT_EQ(0, fromRepoBSON.getFileList().size());
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

TEST(TextureNodeTest, SEqualTest)
{
	TextureNode empty;
	EXPECT_TRUE(empty.sEqual(empty));

	std::vector<uint8_t> data1, data2;
	for (int i = 0; i < 10; ++i)
	{
		data1.push_back(rand());
		data2.push_back(rand());
	}

	auto text1 = RepoBSONFactory::makeTextureNode("text.png", (char*)data1.data(), data1.size(), data1.size(), 1);
	auto text2 = RepoBSONFactory::makeTextureNode("text.jpg", (char*)data1.data(), data1.size(), data1.size(), 1);
	auto text3 = RepoBSONFactory::makeTextureNode("text.png", (char*)data2.data(), data2.size(), data2.size(), 1);

	EXPECT_TRUE(text1.sEqual(text1));
	EXPECT_TRUE(text2.sEqual(text2));
	EXPECT_TRUE(text3.sEqual(text3));
	EXPECT_FALSE(text1.sEqual(empty));
	EXPECT_FALSE(text1.sEqual(text2));
	EXPECT_FALSE(text1.sEqual(text3));
	EXPECT_FALSE(text2.sEqual(text3));
}

TEST(TextureNodeTest, getterTest)
{
	TextureNode empty;
	EXPECT_TRUE(empty.sEqual(empty));

	std::vector<uint8_t> data1, data2;
	for (int i = 0; i < 10; ++i)
	{
		data1.push_back(rand());
	}

	auto text1 = RepoBSONFactory::makeTextureNode("text.png", (char*)data1.data(), data1.size(), data1.size(), 1);
	auto text2 = RepoBSONFactory::makeTextureNode("text.jpg", (char*)data1.data(), data1.size(), data1.size(), 1);

	EXPECT_EQ(0, empty.getRawData().size());
	auto ret = text1.getRawData();
	uint8_t *retInt = (uint8_t*)ret.data();
	for (int i = 0; i < data1.size(); ++i)
	{
		EXPECT_EQ(data1[i], retInt[i]);
	}

	EXPECT_EQ("", empty.getFileExtension());
	EXPECT_EQ("png", text1.getFileExtension());
	EXPECT_EQ("jpg", text2.getFileExtension());
}