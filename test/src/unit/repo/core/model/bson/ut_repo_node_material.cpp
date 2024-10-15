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

#include <repo/core/model/bson/repo_node_material.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;
using namespace testing;

std::vector<float> makeColour()
{
	return std::vector<float>({
		rand() / float(RAND_MAX),
		rand() / float(RAND_MAX),
		rand() / float(RAND_MAX),
		rand() / float(RAND_MAX)
	});
}

static repo_material_t makeRandomMaterialStruct()
{
	repo_material_t matProp;
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
	return matProp;
}

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

TEST(MaterialNodeTest, Deserialise)
{
	auto id = repo::lib::RepoUUID::createUUID();
	auto rev_id = repo::lib::RepoUUID::createUUID();
	auto parents = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID()
	});
	auto sharedId = repo::lib::RepoUUID::createUUID();
	auto type = "material";
	auto ambient = makeColour();
	auto diffuse = makeColour();
	auto specular = makeColour();
	auto emissive = makeColour();
	auto opacity = 1.0;
	auto shininess = 0.0;
	auto shininess_strength = 0.0;
	auto line_weight = 1.0;

	RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, id);
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId);
	builder.append(REPO_NODE_LABEL_TYPE, type);
	builder.appendArray(REPO_NODE_LABEL_PARENTS, parents);
	builder.appendArray(REPO_NODE_MATERIAL_LABEL_AMBIENT, ambient);
	builder.appendArray(REPO_NODE_MATERIAL_LABEL_DIFFUSE, diffuse);
	builder.appendArray(REPO_NODE_MATERIAL_LABEL_SPECULAR, specular);
	builder.appendArray(REPO_NODE_MATERIAL_LABEL_EMISSIVE, emissive);
	builder.append(REPO_NODE_MATERIAL_LABEL_OPACITY, opacity);
	builder.append(REPO_NODE_MATERIAL_LABEL_SHININESS, shininess);
	builder.append(REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH, shininess_strength);
	builder.append(REPO_NODE_MATERIAL_LABEL_LINE_WEIGHT, line_weight);
	builder.append(REPO_NODE_MATERIAL_LABEL_WIREFRAME, true);
	builder.append(REPO_NODE_MATERIAL_LABEL_TWO_SIDED, true);

	auto bson = builder.obj();

	auto material = MaterialNode(bson);

	// Make sure that the superclass parameters come through

	EXPECT_THAT(material.getUniqueID(), Eq(id));
	EXPECT_THAT(material.getSharedID(), Eq(sharedId));
	EXPECT_THAT(material.getType(), "material");
	EXPECT_THAT(material.getParentIDs(), UnorderedElementsAreArray(parents));

	// Material parameters are accessed via the struct

	auto s = material.getMaterialStruct();

	EXPECT_THAT(s.ambient, ElementsAreArray(ambient));
	EXPECT_THAT(s.diffuse, ElementsAreArray(diffuse));
	EXPECT_THAT(s.specular, ElementsAreArray(specular));
	EXPECT_THAT(s.emissive, ElementsAreArray(emissive));
	EXPECT_THAT(s.opacity, Eq(opacity));
	EXPECT_THAT(s.shininess, Eq(shininess));
	EXPECT_THAT(s.shininessStrength, Eq(shininess_strength));
	EXPECT_THAT(s.lineWeight, Eq(line_weight));
	EXPECT_THAT(s.isTwoSided, IsTrue());
	EXPECT_THAT(s.isWireframe, IsTrue());
}

TEST(MaterialNodeTest, Serialise)
{
	MaterialNode node;
	repo_material_t material;

	material.ambient = makeColour();
	material.diffuse = makeColour();
	material.emissive = makeColour();
	material.specular = makeColour();
	material.isTwoSided = true;
	material.isWireframe = true;
	material.lineWeight = 1;
	material.opacity = 1;
	material.shininess = 0.1;
	material.shininessStrength = 0.5;
	material.texturePath = "My/Texture/Path";

	node.setMaterialStruct(material);

	auto bson = (RepoBSON)node;

	// Make sure that the superclass properties are serialised

	EXPECT_THAT(bson.getUUIDField(REPO_LABEL_ID), Eq(node.getUniqueID()));

	EXPECT_THAT(bson.hasField(REPO_NODE_LABEL_SHARED_ID), IsFalse());

	// And that the constant type is written
	
	EXPECT_THAT(bson.getStringField(REPO_NODE_LABEL_TYPE), "material");

	// And that the material struct is written

	EXPECT_THAT(bson.getFloatArray(REPO_NODE_MATERIAL_LABEL_AMBIENT), ElementsAreArray(material.ambient));
	EXPECT_THAT(bson.getFloatArray(REPO_NODE_MATERIAL_LABEL_DIFFUSE), ElementsAreArray(material.diffuse));
	EXPECT_THAT(bson.getFloatArray(REPO_NODE_MATERIAL_LABEL_SPECULAR), ElementsAreArray(material.specular));
	EXPECT_THAT(bson.getFloatArray(REPO_NODE_MATERIAL_LABEL_EMISSIVE), ElementsAreArray(material.emissive));
	EXPECT_THAT(bson.getDoubleField(REPO_NODE_MATERIAL_LABEL_OPACITY), Eq(material.opacity));
	EXPECT_THAT(bson.getDoubleField(REPO_NODE_MATERIAL_LABEL_SHININESS), Eq(material.shininess));
	EXPECT_THAT(bson.getDoubleField(REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH), Eq(material.shininessStrength));
	EXPECT_THAT(bson.getDoubleField(REPO_NODE_MATERIAL_LABEL_LINE_WEIGHT), Eq(material.lineWeight));
	EXPECT_THAT(bson.getBoolField(REPO_NODE_MATERIAL_LABEL_WIREFRAME), Eq(material.isWireframe));
	EXPECT_THAT(bson.getBoolField(REPO_NODE_MATERIAL_LABEL_TWO_SIDED), Eq(material.isTwoSided));

	// (Note we do *not* serialise the texture path, even if it is populated)
}

TEST(MaterialNodeTest, Factory)
{
	MaterialNode node;
	repo_material_t material;

	material.ambient = makeColour();
	material.diffuse = makeColour();
	material.emissive = makeColour();
	material.specular = makeColour();
	material.isTwoSided = true;
	material.isWireframe = true;
	material.lineWeight = 1;
	material.opacity = 1;
	material.shininess = 0.1;
	material.shininessStrength = 0.5;
	material.texturePath = "My/Texture/Path";

	node = RepoBSONFactory::makeMaterialNode(material);

	EXPECT_THAT(node.getMaterialStruct().ambient, Eq(material.ambient));
	EXPECT_THAT(node.getMaterialStruct().diffuse, Eq(material.diffuse));
	EXPECT_THAT(node.getMaterialStruct().specular, Eq(material.specular));
	EXPECT_THAT(node.getMaterialStruct().emissive, Eq(material.emissive));
	EXPECT_THAT(node.getMaterialStruct().isTwoSided, Eq(material.isTwoSided));
	EXPECT_THAT(node.getMaterialStruct().isWireframe, Eq(material.isWireframe));
	EXPECT_THAT(node.getMaterialStruct().lineWeight, Eq(material.lineWeight));
	EXPECT_THAT(node.getMaterialStruct().opacity, Eq(material.opacity));
	EXPECT_THAT(node.getMaterialStruct().shininess, Eq(material.shininess));
	EXPECT_THAT(node.getMaterialStruct().shininessStrength, Eq(material.shininessStrength));

	material.ambient = makeColour();
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().ambient, Eq(material.ambient));

	material.diffuse = makeColour();
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().diffuse, Eq(material.diffuse));

	material.specular = makeColour();
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().specular, Eq(material.specular));

	material.emissive = makeColour();
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().emissive, Eq(material.emissive));

	material.isTwoSided = false;
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().isTwoSided, Eq(material.isTwoSided));

	material.isWireframe = false;
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().isWireframe, Eq(material.isWireframe));

	material.lineWeight = 0.5;
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().lineWeight, Eq(material.lineWeight));

	material.opacity = 0;
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().opacity, Eq(material.opacity));

	material.shininess = 1.5;
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().shininess, Eq(material.shininess));

	material.shininessStrength = 2;
	node = RepoBSONFactory::makeMaterialNode(material);
	EXPECT_THAT(node.getMaterialStruct().shininessStrength, Eq(material.shininessStrength));

	node = RepoBSONFactory::makeMaterialNode(material, "name");
	EXPECT_THAT(node.getName(), Eq("name"));

	auto parents = std::vector<repo::lib::RepoUUID>({
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID()
	});

	node = RepoBSONFactory::makeMaterialNode(material, "name", parents);
	EXPECT_THAT(node.getParentIDs(), UnorderedElementsAreArray(parents));
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