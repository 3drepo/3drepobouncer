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

#include <repo/core/model/bson/repo_node_material.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;

static MaterialNode makeRandomMaterial(
	repo_material_t &matProp)
{
	for (int i = 0; i < 3; ++i)
	{
		matProp.ambient.push_back(rand() / 100.f);
		matProp.diffuse.push_back(rand() / 100.f);
		matProp.specular.push_back(rand() / 100.f);
		matProp.emissive.push_back(rand() / 100.f);
		matProp.opacity = rand() / 100.f;
		matProp.shininess = rand() / 100.f;
		matProp.shininessStrength = rand() / 100.f;
		matProp.isWireframe = rand() % 2;
		matProp.isTwoSided = rand() % 2;
	}
	return RepoBSONFactory::makeMaterialNode(matProp);
}

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(MaterialNodeTest, Constructor)
{
	MaterialNode empty;

	EXPECT_TRUE(empty.isEmpty());
	EXPECT_EQ(NodeType::MATERIAL, empty.getTypeAsEnum());

	auto repoBson = RepoBSON(BSON("test" << "blah" << "test2" << 2));

	auto fromRepoBSON = MaterialNode(repoBson);
	EXPECT_EQ(NodeType::MATERIAL, fromRepoBSON.getTypeAsEnum());
	EXPECT_EQ(fromRepoBSON.nFields(), repoBson.nFields());
	EXPECT_EQ(0, fromRepoBSON.getFileList().size());
}

TEST(MaterialNodeTest, TypeTest)
{
	MaterialNode node;

	EXPECT_EQ(REPO_NODE_TYPE_MATERIAL, node.getType());
	EXPECT_EQ(NodeType::MATERIAL, node.getTypeAsEnum());
}

TEST(MaterialNodeTest, PositionDependantTest)
{
	MaterialNode node;
	EXPECT_FALSE(node.positionDependant());
}

TEST(MaterialNodeTest, SEqualTest)
{
	MaterialNode empty;
	EXPECT_TRUE(empty.sEqual(empty));

	repo_material_t dummy;
	auto matNode = makeRandomMaterial(dummy);
	auto matNode2 = makeRandomMaterial(dummy);
	EXPECT_FALSE(empty.sEqual(matNode));
	EXPECT_TRUE(matNode.sEqual(matNode));
	EXPECT_TRUE(matNode2.sEqual(matNode2));
	EXPECT_FALSE(matNode2.sEqual(matNode));
}

TEST(MaterialNodeTest, GetStructTest)
{
	MaterialNode empty;
	repo_material_t matProp;

	auto emptyStruct = empty.getMaterialStruct();
	EXPECT_EQ(0, emptyStruct.ambient.size());
	EXPECT_EQ(0, emptyStruct.diffuse.size());
	EXPECT_EQ(0, emptyStruct.ambient.size());
	EXPECT_EQ(0, emptyStruct.emissive.size());
	EXPECT_NE(emptyStruct.opacity, emptyStruct.opacity);
	EXPECT_NE(emptyStruct.shininess, emptyStruct.shininess);
	EXPECT_NE(emptyStruct.shininessStrength, emptyStruct.shininessStrength);
	EXPECT_FALSE(emptyStruct.isWireframe);
	EXPECT_FALSE(emptyStruct.isTwoSided);

	auto matNode = makeRandomMaterial(matProp);
	auto returnProp = matNode.getMaterialStruct();
	EXPECT_EQ(returnProp.ambient.size(), matProp.ambient.size());
	EXPECT_EQ(returnProp.ambient.size(), matProp.diffuse.size());
	EXPECT_EQ(returnProp.ambient.size(), matProp.ambient.size());
	EXPECT_EQ(returnProp.ambient.size(), matProp.emissive.size());
	EXPECT_EQ(returnProp.opacity, matProp.opacity);
	EXPECT_EQ(returnProp.shininess, matProp.shininess);
	EXPECT_EQ(returnProp.shininessStrength, matProp.shininessStrength);
	EXPECT_EQ(returnProp.isWireframe, returnProp.isWireframe);
	EXPECT_EQ(returnProp.isTwoSided, returnProp.isTwoSided);
}