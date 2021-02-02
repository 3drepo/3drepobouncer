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

	EXPECT_EQ(MeshNode::Primitive::TRIANGLES, empty.getPrimitive());	// the default type for bson objects without the primitive label, for backwards compatibility

	auto repoBson = RepoBSON(BSON("test" << "blah" << "test2" << 2));

	auto fromRepoBSON = MeshNode(repoBson);
	EXPECT_EQ(NodeType::MESH, fromRepoBSON.getTypeAsEnum());
	EXPECT_EQ(fromRepoBSON.nFields(), repoBson.nFields());
	EXPECT_EQ(0, fromRepoBSON.getFileList().size());
	EXPECT_EQ(MeshNode::Primitive::TRIANGLES, fromRepoBSON.getPrimitive());
}

TEST(MeshNodeTest, TypeTest)
{
	MeshNode node;

	EXPECT_EQ(REPO_NODE_TYPE_MESH, node.getType());
	EXPECT_EQ(NodeType::MESH, node.getTypeAsEnum());

	// It is expected that the value of these types can (and will) be cast to the numerical size of the primitive
	// for control flow, so make sure this happens correctly.

	EXPECT_EQ(0, static_cast<int>(MeshNode::Primitive::UNKNOWN));
	EXPECT_EQ(1, static_cast<int>(MeshNode::Primitive::POINTS));
	EXPECT_EQ(2, static_cast<int>(MeshNode::Primitive::LINES));
	EXPECT_EQ(3, static_cast<int>(MeshNode::Primitive::TRIANGLES));
	EXPECT_EQ(4, static_cast<int>(MeshNode::Primitive::QUADS));
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
	std::vector<repo::lib::RepoVector3D> emptyV, v;
	std::vector<repo_face_t> f;
	std::vector<std::vector<float>> bbox;
	std::vector<std::vector<repo::lib::RepoVector2D>> emptyUV, uvs;
	std::vector<repo_color4d_t> emptyCol, cols;
	v.resize(10);
	f.resize(10);
	for (auto &face : f)
		face.resize(3);
	cols.resize(10);
	uvs.resize(1);
	uvs[0].resize(10);

	std::vector<repo::core::model::MeshNode> meshNodes;

	// Create a set of mesh nodes with all possible variations that should result in different formats
	meshNodes.push_back(MeshNode()); // empty
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

	// (Different numbers of uv channels)

	uvs.resize(2);
	uvs[1].resize(10);

	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

	// (Different primtives)

	uvs.resize(1);
	uvs[0].resize(10);

	for (auto& face : f)
		face.resize(2);

	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

	uvs.resize(2);
	uvs[1].resize(10);

	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
	meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

	// All formats should be distinct from eachother and the empty mesh

	for (auto& meshNodeOuter : meshNodes)
	{
		for (auto& meshNodeInner : meshNodes)
		{
			if (&meshNodeOuter != &meshNodeInner)
			{
				auto mesh1F = meshNodeOuter.getMFormat();
				auto mesh2F = meshNodeInner.getMFormat();
				EXPECT_NE(mesh1F, mesh2F);
			}
		}
	}
}

TEST(MeshNodeTest, SEqualTest)
{
	MeshNode mesh;
	RepoNode node;

	EXPECT_FALSE(mesh.sEqual(node));
	EXPECT_TRUE(mesh.sEqual(mesh));

	// Similar to the GetMFormatTest, meshes with different combinations of attributes should not return equal. Different objects
	// with the same configurations however should.

	// Create two lists, with intra-list meshes being separate but equivalent, and inter-list meshes being different

	std::vector<repo::lib::RepoVector3D> emptyV, v;
	std::vector<repo_face_t> f;
	std::vector<std::vector<float>> bbox;
	std::vector<std::vector<repo::lib::RepoVector2D>> emptyUV, uvs;
	std::vector<repo_color4d_t> emptyCol, cols;
	v.resize(10);
	f.resize(10);
	cols.resize(10);

	std::vector< std::vector<repo::core::model::MeshNode>> meshNodesLists;
	meshNodesLists.resize(2);

	for (int i = 0; i < 2; i++) {

		auto& meshNodes = meshNodesLists[i];

		// Create a set of mesh nodes with all possible variations that should result in different formats
		meshNodes.push_back(MeshNode()); // empty

		for (auto& face : f)
			face.resize(3);

		uvs.resize(1);
		uvs[0].resize(10);

		v[2] = { 0.0, 0.0, 0.0 };
		f[2][1] = 0;

		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

		// Different buffer contents

		v[2] = { 0.0, 0.5, 0.0 };

		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

		f[2][1] = 1;

		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

		// (Different numbers of uv channels)

		uvs.resize(2);
		uvs[1].resize(10);

		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

		// (Different primtives)

		for (auto& face : f)
			face.resize(2);

		uvs.resize(1);
		uvs[0].resize(10);

		v[2] = { 0.0, 0.0, 0.0 };
		f[2][1] = 0;

		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

		v[2] = { 0.0, 1.0, 0.0 };

		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

		f[2][1] = 2;

		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, emptyUV, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, emptyUV, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

		uvs.resize(2);
		uvs[1].resize(10);

		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, emptyV, bbox, uvs, cols));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, emptyCol));
		meshNodes.push_back(RepoBSONFactory::makeMeshNode(v, f, v, bbox, uvs, cols));

	}

	for (size_t i = 0; i < meshNodesLists[0].size(); i++)
	{
		auto& meshNode1 = meshNodesLists[0][i];
		auto& meshNode2 = meshNodesLists[1][i];

		// The meshes in the two lists should be the same 
		// (note that resize was used to initialise the buffers above, so they should be initialised to the type defaults not random memory)

		EXPECT_TRUE(meshNode1.sEqual(meshNode2));
	}

	auto& meshNodes = meshNodesLists[0];

	for (auto& meshNodeOuter : meshNodes)
	{
		for (auto& meshNodeInner : meshNodes)
		{
			if (&meshNodeOuter != &meshNodeInner)
			{
				EXPECT_FALSE(meshNodeInner.sEqual(meshNodeOuter));
			}
		}
	}
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

	std::vector<repo::lib::RepoVector3D> v;
	std::vector<repo_face_t> f;
	std::vector<std::vector<float>> bbox;

	v = { { 0.1f, 0.2f, 0.3f }, { 0.4f, 0.5f, 0.6f } };
	f.resize(1);

	auto mesh = RepoBSONFactory::makeMeshNode(v, f, v, bbox);
	MeshNode unchangedMesh = mesh.cloneAndApplyTransformation(identity);

	EXPECT_TRUE(compareStdVectors(v, unchangedMesh.getVertices()));

	MeshNode changedMesh = mesh.cloneAndApplyTransformation(notId);
	EXPECT_FALSE(compareStdVectors(v, changedMesh.getVertices()));
	EXPECT_FALSE(compareStdVectors(changedMesh.getNormals(), v));
	EXPECT_FALSE(compareStdVectors(changedMesh.getNormals(), changedMesh.getVertices()));
}

TEST(MeshNodeTest, CloneAndApplyMeshMapping)
{
	MeshNode empty;
	std::vector<repo_mesh_mapping_t> mapping, emptyMapping;

	mapping.resize(5);

	for (auto &map : mapping)
	{
		map.min = { rand() / 100.0f, rand() / 100.0f, rand() / 100.0f };
		map.max = { rand() / 100.0f, rand() / 100.0f, rand() / 100.0f };
		map.mesh_id = repo::lib::RepoUUID::createUUID();
		map.material_id = repo::lib::RepoUUID::createUUID();
		map.vertFrom = rand();
		map.vertTo = rand();
		map.triFrom = rand();
		map.triTo = rand();
	}

	auto withMappings = empty.cloneAndUpdateMeshMapping(mapping);

	auto withMappings2 = empty.cloneAndUpdateMeshMapping(emptyMapping);

	auto meshMappings = withMappings.getMeshMapping();
	EXPECT_NE(meshMappings.size(), withMappings2.getMeshMapping().size());

	ASSERT_EQ(meshMappings.size(), mapping.size());
	for (int i = 0; i < meshMappings.size(); ++i)
	{
		EXPECT_EQ(meshMappings[i].min, mapping[i].min);
		EXPECT_EQ(meshMappings[i].max, mapping[i].max);
		EXPECT_EQ(meshMappings[i].mesh_id, mapping[i].mesh_id);
		EXPECT_EQ(meshMappings[i].material_id, mapping[i].material_id);
		EXPECT_EQ(meshMappings[i].vertFrom, mapping[i].vertFrom);
		EXPECT_EQ(meshMappings[i].vertTo, mapping[i].vertTo);
		EXPECT_EQ(meshMappings[i].triFrom, mapping[i].triFrom);
		EXPECT_EQ(meshMappings[i].triTo, mapping[i].triTo);
	}

	std::vector<repo_mesh_mapping_t> moreMappings;
	moreMappings.resize(2);

	for (auto &map : moreMappings)
	{
		map.min = { rand() / 100.0f, rand() / 100.0f, rand() / 100.0f };
		map.max = { rand() / 100.0f, rand() / 100.0f, rand() / 100.0f };
		map.mesh_id = repo::lib::RepoUUID::createUUID();
		map.material_id = repo::lib::RepoUUID::createUUID();
		map.vertFrom = rand();
		map.vertTo = rand();
		map.triFrom = rand();
		map.triTo = rand();
	}

	auto updatedMappings = withMappings.cloneAndUpdateMeshMapping(moreMappings);
	EXPECT_EQ(updatedMappings.getMeshMapping().size(), mapping.size() + moreMappings.size());

	auto overwriteMappings = withMappings.cloneAndUpdateMeshMapping(moreMappings, true);
	EXPECT_EQ(overwriteMappings.getMeshMapping().size(), moreMappings.size());
}

TEST(MeshNodeTest, Getters)
{
	MeshNode empty;

	std::vector<repo::lib::RepoVector3D> v, n;
	std::vector<repo_face_t> f;
	std::vector<std::vector<float>> bbox;
	std::vector<std::vector<repo::lib::RepoVector2D>> uvs;
	std::vector<repo_color4d_t> cols;

	uvs.resize(2);
	for (int i = 0; i < 10; ++i)
	{
		v.push_back({ rand() / 100.0f, rand() / 100.0f, rand() / 100.0f });
		n.push_back({ rand() / 100.0f, rand() / 100.0f, rand() / 100.0f });
		uvs[0].push_back({ rand() / 100.0f, rand() / 100.0f });
		uvs[1].push_back({ rand() / 100.0f, rand() / 100.0f });
		cols.push_back({ rand() / 100.0f, rand() / 100.0f, rand() / 100.0f, rand() / 100.0f });
		f.push_back({ (uint32_t)rand(), (uint32_t)rand(), (uint32_t)rand() });
	}
	bbox.push_back({ rand() / 100.0f, rand() / 100.0f, rand() / 100.0f });
	bbox.push_back({ rand() / 100.0f, rand() / 100.0f, rand() / 100.0f });

	auto mesh = RepoBSONFactory::makeMeshNode(v, f, n, bbox, uvs, cols);

	EXPECT_EQ(0, empty.getVertices().size());
	auto resVertices = mesh.getVertices();
	EXPECT_EQ(v.size(), resVertices.size());
	EXPECT_TRUE(compareStdVectors(v, resVertices));

	EXPECT_EQ(0, empty.getFaces().size());
	auto resFaces = mesh.getFaces();
	EXPECT_EQ(f.size(), resFaces.size());
	for (int i = 0; i < resFaces.size(); ++i)
	{
		EXPECT_TRUE(compareStdVectors(resFaces[i], f[i]));
	}

	EXPECT_EQ(0, empty.getNormals().size());
	auto resNormals = mesh.getNormals();
	EXPECT_EQ(n.size(), resNormals.size());
	EXPECT_TRUE(compareStdVectors(resNormals, n));

	EXPECT_EQ(0, empty.getUVChannelsSeparated().size());

	EXPECT_EQ(uvs.size(), mesh.getUVChannelsSeparated().size());
	for (int i = 0; i < uvs.size(); ++i)
	{
		auto uvChannel = mesh.getUVChannelsSeparated().at(i);
		EXPECT_TRUE(compareStdVectors(uvs[i], uvChannel));
	}

	EXPECT_EQ(0, empty.getColors().size());
	EXPECT_EQ(cols.size(), mesh.getColors().size());
	EXPECT_TRUE(compareVectors(cols, mesh.getColors()));

	auto retBbox = mesh.getBoundingBox();
	std::vector<repo::lib::RepoVector3D> bboxInVect;
	for (int i = 0; i < bbox.size(); ++i)
	{
		bboxInVect.push_back({ bbox[i][0], bbox[i][1], bbox[i][2] });
	}
	EXPECT_TRUE(compareStdVectors(retBbox, bboxInVect));
}