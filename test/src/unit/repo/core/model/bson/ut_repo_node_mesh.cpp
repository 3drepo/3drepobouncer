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

#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"

using namespace repo::core::model;

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(MeshNodeTest, Constructor)
{
	MeshNode empty;

	EXPECT_TRUE(empty.isEmpty());
	EXPECT_EQ(NodeType::MESH, empty.getTypeAsEnum());

	auto repoBson = RepoBSON(BSON("test" << "blah" << "test2" << 2));

	auto fromRepoBSON = MeshNode(repoBson);
	EXPECT_EQ(NodeType::MESH, fromRepoBSON.getTypeAsEnum());
	EXPECT_EQ(fromRepoBSON.nFields(), repoBson.nFields());
	EXPECT_EQ(0, fromRepoBSON.getFileList().size());
}

TEST(MeshNodeTest, TypeTest)
{
	MeshNode node;

	EXPECT_EQ(REPO_NODE_TYPE_MESH, node.getType());
	EXPECT_EQ(NodeType::MESH, node.getTypeAsEnum());
}

TEST(MeshNodeTest, PositionDependantTest)
{
	MeshNode node;
	//mesh node should always be position dependant
	EXPECT_TRUE(node.positionDependant());
}

TEST(MeshNodeTest, GetMFormatTest)
{
	//Better to not rely on RepoBSONFactory, but it's so much easier...
	std::vector<repo_vector_t> emptyV, v;
	std::vector<repo_face_t> f;
	std::vector<std::vector<float>> bbox;
	std::vector<std::vector<repo_vector2d_t>> emptyUV, uvs;
	std::vector<repo_color4d_t> emptyCol, cols;
	v.resize(10);
	f.resize(10);
	for (auto &face : f)
		face.resize(3);
	cols.resize(10);

	MeshNode empty;
	auto emptyF = empty.getMFormat();

	auto mesh1 = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol);
	auto mesh1F = mesh1.getMFormat();

	EXPECT_NE(emptyF, mesh1F);

	auto mesh2 = RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol);
	auto mesh2F = mesh2.getMFormat();

	EXPECT_NE(emptyF, mesh2F);
	EXPECT_NE(mesh2F, mesh1F);

	uvs.resize(1);
	uvs[0].resize(10);
	auto mesh3 = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol);
	auto mesh3F = mesh3.getMFormat();

	EXPECT_NE(emptyF, mesh3F);
	EXPECT_NE(mesh3F, mesh1F);
	EXPECT_NE(mesh3F, mesh2F);

	auto mesh4 = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols);
	auto mesh4F = mesh4.getMFormat();

	EXPECT_NE(emptyF, mesh4F);
	EXPECT_NE(mesh4F, mesh1F);
	EXPECT_NE(mesh4F, mesh2F);
	EXPECT_NE(mesh4F, mesh3F);

	uvs.resize(2);
	uvs[1].resize(10);
	auto mesh5 = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol);
	auto mesh5F = mesh5.getMFormat();

	EXPECT_NE(emptyF, mesh5F);
	EXPECT_NE(mesh5F, mesh1F);
	EXPECT_NE(mesh5F, mesh2F);
	EXPECT_NE(mesh5F, mesh3F);
	EXPECT_NE(mesh5F, mesh4F);

	auto mesh6 = RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol);
	auto mesh6F = mesh6.getMFormat();

	EXPECT_NE(emptyF, mesh6F);
	EXPECT_NE(mesh6F, mesh1F);
	EXPECT_NE(mesh6F, mesh2F);
	EXPECT_NE(mesh6F, mesh3F);
	EXPECT_NE(mesh6F, mesh4F);
	EXPECT_NE(mesh6F, mesh5F);

	auto mesh7 = RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols);
	auto mesh7F = mesh7.getMFormat();

	EXPECT_NE(emptyF, mesh7F);
	EXPECT_NE(mesh7F, mesh1F);
	EXPECT_NE(mesh7F, mesh2F);
	EXPECT_NE(mesh7F, mesh3F);
	EXPECT_NE(mesh7F, mesh4F);
	EXPECT_NE(mesh7F, mesh5F);
	EXPECT_NE(mesh7F, mesh6F);

	auto mesh8 = RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, cols);
	auto mesh8F = mesh8.getMFormat();

	EXPECT_NE(emptyF, mesh7F);
	EXPECT_NE(mesh8F, mesh1F);
	EXPECT_NE(mesh8F, mesh2F);
	EXPECT_NE(mesh8F, mesh3F);
	EXPECT_NE(mesh8F, mesh4F);
	EXPECT_NE(mesh8F, mesh5F);
	EXPECT_NE(mesh8F, mesh6F);
	EXPECT_NE(mesh8F, mesh7F);
}

TEST(MeshNodeTest, SEqualTest)
{
	MeshNode mesh;
	RepoNode node;

	EXPECT_FALSE(mesh.sEqual(node));
	EXPECT_TRUE(mesh.sEqual(mesh));

	std::vector<repo_vector_t> emptyV, v;
	std::vector<repo_face_t> f;
	std::vector<std::vector<float>> bbox;
	std::vector<std::vector<repo_vector2d_t>> emptyUV, uvs;
	std::vector<repo_color4d_t> emptyCol, cols;
	v.resize(10);
	f.resize(10);
	for (auto &face : f)
		face.resize(3);
	cols.resize(10);

	auto mesh1 = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol);

	EXPECT_FALSE(mesh.sEqual(mesh1));

	v[2] = { 0.0, 0.5, 0.0 };

	auto mesh1changedV = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol);
	EXPECT_FALSE(mesh1.sEqual(mesh1changedV));

	f[2][2] = 1;

	auto mesh1changedF = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol);
	EXPECT_FALSE(mesh1.sEqual(mesh1changedF));

	auto mesh2 = RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol);
	EXPECT_FALSE(mesh2.sEqual(mesh1changedF));

	v[1] = { 0.0, 0.6, 0.0 };
	auto mesh2changedN = RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol);
	EXPECT_FALSE(mesh2.sEqual(mesh2changedN));

	uvs.resize(1);
	uvs[0].resize(10);
	auto mesh3 = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol);

	EXPECT_FALSE(mesh1changedF.sEqual(mesh3));

	uvs[0][3] = { 0.3, 0.1 };
	auto mesh3ChangedUV = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol);

	EXPECT_FALSE(mesh3ChangedUV.sEqual(mesh3));

	auto mesh4 = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols);
	EXPECT_FALSE(mesh1changedF.sEqual(mesh4));

	uvs.resize(2);
	uvs[1].resize(10);
	auto mesh5 = RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol);
	EXPECT_FALSE(mesh3ChangedUV.sEqual(mesh5));
}

TEST(MeshNodeTest, CloneAndApplyTransformation)
{
	std::vector<float> identity =
	{ 1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1 };

	std::vector<float> notId =
	{ 0.1f, 0, 0, 0,
	0, 0.5f, 0.12f, 0,
	0.5f, 0, 0.1f, 0,
	0, 0, 0, 1 };

	MeshNode empty;
	MeshNode newEmpty = empty.cloneAndApplyTransformation(identity);
	EXPECT_TRUE(newEmpty.isEmpty());

	std::vector<repo_vector_t> v;
	std::vector<repo_face_t> f;
	std::vector<std::vector<float>> bbox;

	v = { { 0.1, 0.2, 0.3 }, { 0.4, 0.5, 0.6 } };
	f.resize(1);

	auto mesh = RepoBSONFactory::makeMeshNode(v, f, v, bbox);
	MeshNode unchangedMesh = mesh.cloneAndApplyTransformation(identity);

	EXPECT_TRUE(compareVectors(v, unchangedMesh.getVertices()));

	MeshNode changedMesh = mesh.cloneAndApplyTransformation(notId);
	EXPECT_FALSE(compareVectors(v, changedMesh.getVertices()));
	EXPECT_FALSE(compareVectors(changedMesh.getNormals(), v));
	EXPECT_FALSE(compareVectors(changedMesh.getNormals(), changedMesh.getVertices()));
}