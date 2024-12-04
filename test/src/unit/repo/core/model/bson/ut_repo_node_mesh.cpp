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

#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_mesh_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::core::model;
using namespace repo::test::utils::mesh;
using namespace testing;

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(MeshNodeTest, EmptyConstructor)
{
	MeshNode empty;

	EXPECT_FALSE(empty.getNumVertices());
	EXPECT_EQ(NodeType::MESH, empty.getTypeAsEnum());
	EXPECT_EQ(MeshNode::Primitive::TRIANGLES, empty.getPrimitive());	// the default type for bson objects without the primitive label, for backwards compatibility
}

void MeshNodeTestDeserialise(mesh_data data)
{
	compareMeshNode(data, MeshNode(meshNodeTestBSONFactory(data)));
}

TEST(MeshNodeTest, Deserialise)
{
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 2, false, 0, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 2, false, 1, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 2, false, 2, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 2, true, 0, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 2, true, 1, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 2, true, 2, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 3, false, 0, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 3, false, 1, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 3, false, 2, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 3, true, 0, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 3, true, 1, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 0, 3, true, 2, 100));

	MeshNodeTestDeserialise(mesh_data(false, false, 0, 3, false, 0, 100));
	MeshNodeTestDeserialise(mesh_data(false, false, 1, 3, false, 0, 100));
	MeshNodeTestDeserialise(mesh_data(false, true, 0, 3, false, 0, 100));
	MeshNodeTestDeserialise(mesh_data(false, true, 1, 3, false, 0, 100));
	MeshNodeTestDeserialise(mesh_data(true, false, 0, 3, false, 0, 100));
	MeshNodeTestDeserialise(mesh_data(true, false, 1, 3, false, 0, 100));
	MeshNodeTestDeserialise(mesh_data(true, true, 1, 3, false, 0, 100));

	MeshNodeTestDeserialise(mesh_data(true, true, 3, 3, false, 0, 100));
}

TEST(MeshNodeTest, Serialise)
{
	MeshNode node;

	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_LABEL_ID), Eq(node.getUniqueID()));
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_SHARED_ID), IsFalse());
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_TYPE), Eq("mesh"));
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_PARENTS), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_LABEL_NAME), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_MESH_LABEL_BOUNDING_BOX), IsTrue());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_MESH_LABEL_VERTICES_COUNT), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_MESH_LABEL_VERTICES), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_MESH_LABEL_FACES_COUNT), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_MESH_LABEL_PRIMITIVE), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_MESH_LABEL_FACES), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_MESH_LABEL_NORMALS), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT), IsFalse());
	EXPECT_THAT(((RepoBSON)node).hasField(REPO_NODE_MESH_LABEL_UV_CHANNELS), IsFalse());

	node.setSharedID(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDField(REPO_NODE_LABEL_SHARED_ID), Eq(node.getSharedID()));

	node.addParent(repo::lib::RepoUUID::createUUID());
	node.addParent(repo::lib::RepoUUID::createUUID());
	EXPECT_THAT(((RepoBSON)node).getUUIDFieldArray(REPO_NODE_LABEL_PARENTS), node.getParentIDs());

	node.changeName("MyName");
	EXPECT_THAT(((RepoBSON)node).getStringField(REPO_NODE_LABEL_NAME), Eq(node.getName()));

	node.setVertices(makeVertices(100), true);
	EXPECT_THAT(((RepoBSON)node).getBoundsField(REPO_NODE_MESH_LABEL_BOUNDING_BOX), Eq(node.getBoundingBox()));
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_MESH_LABEL_VERTICES_COUNT), Eq(node.getNumVertices()));
	std::vector<repo::lib::RepoVector3D> vertices;
	((RepoBSON)node).getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_VERTICES, vertices);
	EXPECT_THAT(vertices, ElementsAreArray(node.getVertices()));

	node.setFaces(makeFaces(MeshNode::Primitive::LINES));
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_MESH_LABEL_FACES_COUNT), Eq(node.getNumFaces()));
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_MESH_LABEL_PRIMITIVE), Eq((int)node.getPrimitive()));
	std::vector<uint32_t> faces;
	((RepoBSON)node).getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_FACES, faces);
	EXPECT_THAT(faces[0], Eq((int)node.getPrimitive()));
	EXPECT_THAT(faces[1], Eq(node.getFaces()[0][0]));
	EXPECT_THAT(faces[2], Eq(node.getFaces()[0][1]));

	node.setFaces(makeFaces(MeshNode::Primitive::TRIANGLES));
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_MESH_LABEL_FACES_COUNT), Eq(node.getNumFaces()));
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_MESH_LABEL_PRIMITIVE), Eq((int)node.getPrimitive()));
	((RepoBSON)node).getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_FACES, faces);
	EXPECT_THAT(faces[0], Eq((int)node.getPrimitive()));
	EXPECT_THAT(faces[1], Eq(node.getFaces()[0][0]));
	EXPECT_THAT(faces[2], Eq(node.getFaces()[0][1]));
	EXPECT_THAT(faces[3], Eq(node.getFaces()[0][2]));

	node.setNormals(makeNormals(100));
	std::vector<repo::lib::RepoVector3D> normals;
	((RepoBSON)node).getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_NORMALS, normals);
	EXPECT_THAT(normals, ElementsAreArray(node.getNormals()));

	node.setUVChannel(0, makeUVs(100));
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT), Eq(node.getNumUVChannels()));
	std::vector<repo::lib::RepoVector2D> uvs;
	((RepoBSON)node).getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_UV_CHANNELS, uvs);
	EXPECT_THAT(uvs, ElementsAreArray(node.getUVChannelsSerialised()));

	node.setUVChannel(1, makeUVs(100));
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT), Eq(node.getNumUVChannels()));
	((RepoBSON)node).getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_UV_CHANNELS, uvs);
	EXPECT_THAT(uvs, ElementsAreArray(node.getUVChannelsSerialised()));

	node.setUVChannel(0, makeUVs(100));
	EXPECT_THAT(((RepoBSON)node).getIntField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT), Eq(node.getNumUVChannels()));
	((RepoBSON)node).getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_UV_CHANNELS, uvs);
	EXPECT_THAT(uvs, ElementsAreArray(node.getUVChannelsSerialised()));
}

TEST(MeshNodeTest, TypeTest)
{
	MeshNode node;

	EXPECT_EQ(REPO_NODE_TYPE_MESH, node.getType());
	EXPECT_EQ(NodeType::MESH, node.getTypeAsEnum());

	// It is expected that the value of these types can (and will) be cast to the
	// numerical size of the primitive for control flow, so make sure this happens
	// correctly.

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
	std::vector<repo::core::model::MeshNode> meshNodes;

	// Create a set of mesh nodes with all possible variations, which should result
	// in different formats

	meshNodes.push_back(MeshNode()); // empty

	// Triangles, with and without normals, and with and without one UV channel

	meshNodes.push_back(makeDeterministicMeshNode(3, false, 0));
	meshNodes.push_back(makeDeterministicMeshNode(3, false, 1));
	meshNodes.push_back(makeDeterministicMeshNode(3, true, 0));
	meshNodes.push_back(makeDeterministicMeshNode(3, true, 1));

	// (Different numbers of uv channels)

	meshNodes.push_back(makeDeterministicMeshNode(3, false, 2));
	meshNodes.push_back(makeDeterministicMeshNode(3, true, 2));

	// (Different primtives)

	meshNodes.push_back(makeDeterministicMeshNode(2, false, 0));
	meshNodes.push_back(makeDeterministicMeshNode(2, false, 1));
	meshNodes.push_back(makeDeterministicMeshNode(2, true, 0));
	meshNodes.push_back(makeDeterministicMeshNode(2, true, 1));

	// (Different numbers of uv channels)

	meshNodes.push_back(makeDeterministicMeshNode(2, false, 2));
	meshNodes.push_back(makeDeterministicMeshNode(2, true, 2));

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

	// Similar to the GetMFormatTest, meshes with different combinations of
	// attributes should not return equal. Different objects with the same
	// configurations however should.

	// Create two lists, with intra-list meshes being separate but equivalent,
	// and inter-list meshes being different.

	std::vector<repo::lib::RepoVector3D> emptyV, v;
	std::vector<repo_face_t> f;
	std::vector<std::vector<float>> bbox;
	std::vector<std::vector<repo::lib::RepoVector2D>> emptyUV, uvs;
	std::vector<repo_color4d_t> emptyCol, cols;
	v.resize(10);
	f.resize(10);
	cols.resize(10);
	bbox.resize(2);
	bbox[0].resize(3);
	bbox[1].resize(3);

	std::vector< std::vector<repo::core::model::MeshNode>> meshNodesLists;
	meshNodesLists.resize(2);

	for (int i = 0; i < 2; i++) {

		auto& meshNodes = meshNodesLists[i];

		// Create a set of mesh nodes with all possible variations, which should result
		// in different formats

		meshNodes.push_back(MeshNode()); // empty

		// Triangles, with and without normals, and with and without one UV channel

		meshNodes.push_back(makeDeterministicMeshNode(3, false, 0));
		meshNodes.push_back(makeDeterministicMeshNode(3, false, 1));
		meshNodes.push_back(makeDeterministicMeshNode(3, true, 0));
		meshNodes.push_back(makeDeterministicMeshNode(3, true, 1));

		// (Different numbers of uv channels)

		meshNodes.push_back(makeDeterministicMeshNode(3, false, 2));
		meshNodes.push_back(makeDeterministicMeshNode(3, true, 2));

		// (Different primtives)

		meshNodes.push_back(makeDeterministicMeshNode(2, false, 0));
		meshNodes.push_back(makeDeterministicMeshNode(2, false, 1));
		meshNodes.push_back(makeDeterministicMeshNode(2, true, 0));
		meshNodes.push_back(makeDeterministicMeshNode(2, true, 1));

		// (Different numbers of uv channels)

		meshNodes.push_back(makeDeterministicMeshNode(2, false, 2));
		meshNodes.push_back(makeDeterministicMeshNode(2, true, 2));

		// (Different buffer contents)

		// Every time makeDeterministicMeshNode is called it resets the random
		// seed, so the following should create identical copies to the above and
		// each entry will differ only by the calls to setVertices, etc, which
		// continue the random sequence.

		meshNodes.push_back(makeDeterministicMeshNode(3, false, 0));
		meshNodes[meshNodes.size() - 1].setVertices(makeVertices(100), true);

		meshNodes.push_back(makeDeterministicMeshNode(3, false, 0));
		meshNodes[meshNodes.size() - 1].setFaces(makeFaces(MeshNode::Primitive::TRIANGLES));

		meshNodes.push_back(makeDeterministicMeshNode(3, false, 1));
		meshNodes[meshNodes.size() - 1].setUVChannel(0, makeUVs(100));

		meshNodes.push_back(makeDeterministicMeshNode(3, true, 0));
		meshNodes[meshNodes.size() - 1].setNormals(makeNormals(100));

		meshNodes.push_back(makeDeterministicMeshNode(3, false, 2));
		meshNodes[meshNodes.size() - 1].setUVChannel(1, makeUVs(100));

		meshNodes.push_back(makeDeterministicMeshNode(2, false, 0));
		meshNodes[meshNodes.size() - 1].setVertices(makeVertices(100), true);

		meshNodes.push_back(makeDeterministicMeshNode(2, false, 0));
		meshNodes[meshNodes.size() - 1].setFaces(makeFaces(MeshNode::Primitive::LINES));

		meshNodes.push_back(makeDeterministicMeshNode(2, false, 1));
		meshNodes[meshNodes.size() - 1].setUVChannel(0, makeUVs(100));

		meshNodes.push_back(makeDeterministicMeshNode(2, true, 0));
		meshNodes[meshNodes.size() - 1].setNormals(makeNormals(100));

		meshNodes.push_back(makeDeterministicMeshNode(2, false, 2));
		meshNodes[meshNodes.size() - 1].setUVChannel(1, makeUVs(100));
	}

	for (size_t i = 0; i < meshNodesLists[0].size(); i++)
	{
		auto& meshNode1 = meshNodesLists[0][i];
		auto& meshNode2 = meshNodesLists[1][i];

		// The meshes between the two lists should be the same

		EXPECT_TRUE(meshNode1.sEqual(meshNode2));
	}

	auto& meshNodes = meshNodesLists[0];
	for (size_t i = 0; i < meshNodes.size(); i++)
	{
		auto& outer = meshNodes[i];
		for (size_t j = 0; j < meshNodes.size(); j++)
		{
			if (i != j)
			{
				auto& inner = meshNodes[j];
				EXPECT_FALSE(inner.sEqual(outer));
			}
		}
	}
}

TEST(MeshNodeTest, CloneAndApplyTransformation)
{
	std::vector<float> identity = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	std::vector<float> notId = {
		0.1f, 0, 0, 0,
		0, 0.5f, 0.12f, 0,
		0.5f, 0, 0.1f, 0,
		0, 0, 0, 1
	};

	auto mesh = makeDeterministicMeshNode(3, true, 2);
	MeshNode unchangedMesh = mesh.cloneAndApplyTransformation(identity);

	EXPECT_THAT(unchangedMesh.getVertices(), ElementsAreArray(mesh.getVertices()));
	EXPECT_THAT(unchangedMesh.getNormals(), ElementsAreArray(mesh.getNormals()));

	MeshNode changedMesh = mesh.cloneAndApplyTransformation(notId);
	EXPECT_THAT(changedMesh.getVertices(), Not(ElementsAreArray(mesh.getVertices())));
	EXPECT_THAT(changedMesh.getNormals(), Not(ElementsAreArray(mesh.getNormals())));
	EXPECT_THAT(changedMesh.getBoundingBox(), Not(Eq(mesh.getBoundingBox())));

	MeshNode empty;
	MeshNode newEmpty = empty.cloneAndApplyTransformation(identity);
	EXPECT_FALSE(newEmpty.getNumVertices());
}

TEST(MeshNodeTest, TransformBoundingBox)
{
	// Do this whole test using 64 bit vectors so we don't worry about
	// false positives due to quantisation error. Any real use that
	// uses floats should be aware of this.

	auto vertices = std::vector<repo::lib::RepoVector3D64>({
		{  1,  0,  0 },
		{  0,  1,  0 },
		{  0,  0,  1 },
		{ -1,  0,  0 },
		{  0, -1,  0 },
		{  0,  0, -1 },
		{ -1, -1, -1 },
		{  1,  1,  1 },
	});

	auto m = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D(10, 0, 0 )) * repo::lib::RepoMatrix::rotationX(0.12f) * repo::lib::RepoMatrix::rotationY(0.8f) * repo::lib::RepoMatrix::rotationZ(1.02f);

	auto transformed = vertices;
	EXPECT_THAT(vertices, ElementsAreArray(transformed));

	for (auto& v : transformed)
	{
		v = m * v;
	}
	EXPECT_THAT(vertices, Not(ElementsAreArray(transformed)));

	auto bb = getBoundingBox(vertices);
	MeshNode::transformBoundingBox(bb, m);

	auto tight = getBoundingBox(transformed);

	// Since an AABB is not perfectly tight to a mesh, a transformed AABB may be
	// larger than the tighest bounds computed for the transformed vertices.
	// It should always encompass the AABB however (never allow a false negative
	// in a bounds test).

	EXPECT_THAT(bb.min(), VectorLe(tight.min()));
	EXPECT_THAT(bb.max(), VectorGe(tight.max()));

	// The exception to this is when the AABB is a perfect fit (i.e. there is
	// at least one vertex in every corner).
	// Check this case as well.

	vertices = std::vector<repo::lib::RepoVector3D64>({
		{  0,  0,  0 },
		{  0,  0,  1 },
		{  0,  1,  1 },
		{  0,  1,  0 },
		{  1,  0,  0 },
		{  1,  0,  1 },
		{  1,  1,  1 },
		{  1,  1,  0 },
		{  0.9f, 0.5f, 0 },
		{  0.2f, 0.2f, 0.1f },
		{  0.99f, 0.21f, 0.56f },
		{  0.78f, 0.12f, 0.34f },
	});

	transformed = vertices;
	for (auto& v : transformed)
	{
		v = m * v;
	}
	EXPECT_THAT(vertices, Not(ElementsAreArray(transformed)));

	bb = getBoundingBox(vertices);
	MeshNode::transformBoundingBox(bb, m);
	EXPECT_THAT(bb, Eq(getBoundingBox(transformed)));
}

TEST(MeshNodeTest, ApplyTransform)
{
	auto m = makeDeterministicMeshNode(3, true, 1);
	auto t = makeTransform(true, true);

	auto v = m.getVertices();
	auto n = m.getNormals();
	auto uv = m.getUVChannelsSeparated()[0];
	auto f = m.getFaces();

	// Applying a transform should change the vertices (translate and rotate) and
	// normals (rotate only), while leaving faces and UVs unchanged.

	m.applyTransformation(t);

	for (auto& vv : v)
	{
		vv = t * vv;
	}

	for (auto& nn : n)
	{
		nn = t.transformDirection(nn);
	}

	EXPECT_THAT(m.getVertices(), Pointwise(PositionsNear(), v));
	EXPECT_THAT(m.getNormals(), Pointwise(DirectionsNear(), n));
	EXPECT_THAT(m.getUVChannelsSeparated()[0], Eq(uv));
	EXPECT_THAT(m.getFaces(), Eq(f));
}

TEST(MeshNodeTest, Modifiers)
{
	// Specify two reference meshes we intend to swap properties between.
	// (It is expected that the constructors have already been tested)

	auto m1 = makeDeterministicMeshNode(3, true, 2);
	auto m2 = makeDeterministicMeshNode(2, true, 2);
	m2.setVertices(makeVertices(m2.getNumVertices()),true);
	m2.setNormals(makeNormals(m2.getNumVertices()));
	m2.setFaces(makeFaces(m2.getPrimitive()));
	m2.setUVChannel(0, makeUVs(m2.getNumVertices()));
	m2.setUVChannel(1, makeUVs(m2.getNumVertices()));

	EXPECT_THAT((int)m1.getPrimitive(), Eq((int)MeshNode::Primitive::TRIANGLES));

	EXPECT_THAT(m1.getGrouping(), IsEmpty());

	m1.setGrouping("group1");
	EXPECT_THAT(m1.getGrouping(), Eq("group1"));

	m1.setGrouping("group2");
	EXPECT_THAT(m1.getGrouping(), Eq("group2"));

	auto bounds = getBoundingBox(m1.getVertices());
	EXPECT_THAT(m1.getBoundingBox(), Eq(bounds));

	m1.setFaces(m2.getFaces());
	EXPECT_THAT(m1.getFaces(), Eq(m2.getFaces()));

	m1.setNormals(m2.getNormals());
	EXPECT_THAT(m1.getNormals(), m2.getNormals());

	m1.setUVChannel(0, m2.getUVChannelsSeparated()[0]);
	m1.setUVChannel(1, m2.getUVChannelsSeparated()[1]);
	EXPECT_THAT(m1.getUVChannelsSerialised(),Eq(m2.getUVChannelsSerialised()));
	EXPECT_THAT(m1.getUVChannelsSeparated()[0], Eq(m2.getUVChannelsSeparated()[0]));
	EXPECT_THAT(m1.getUVChannelsSeparated()[1], Eq(m2.getUVChannelsSeparated()[1]));

	m1.setVertices(m2.getVertices(), false);
	EXPECT_THAT(m1.getVertices(), Eq(m2.getVertices()));
	EXPECT_THAT(m1.getBoundingBox(), Not(Eq(m2.getBoundingBox())));

	m1.setVertices(m2.getVertices(), true);
	EXPECT_THAT(m1.getBoundingBox(), Eq(m2.getBoundingBox()));

	m1.setVertices(makeVertices(m1.getNumVertices()), false);
	EXPECT_THAT(m1.getBoundingBox(), Not(Eq(getBoundingBox(m1.getVertices()))));
	m1.updateBoundingBox();
	EXPECT_THAT(m1.getBoundingBox(), Eq(getBoundingBox(m1.getVertices())));

	EXPECT_THAT(m1.getNumFaces(), Eq(m1.getFaces().size()));
	EXPECT_THAT(m1.getNumVertices(), Eq(m1.getVertices().size()));
	EXPECT_THAT(m1.getNumVertices(), Eq(m1.getNormals().size()));
}

// This function is used by the CopyConstructor Test to return a heap-allocated
// copy of a MeshNode originally allocated on the stack.
static MeshNode* makeNewMeshNode()
{
	auto a = makeDeterministicMeshNode(3, true, 2);
	return new MeshNode(a);
}

// This function is used by the CopyConstructor Test to return a stack-allocated
// copy of a MeshNode on the stack.
static MeshNode makeRefMeshNode()
{
	auto m = makeDeterministicMeshNode(3, true, 2);
	return m;
}

TEST(MeshNodeTest, CopyConstructor)
{
	// Tests that the copy-constructor is behaving as expected
	// We use the default shallow-copy copy constructor and it is assumed the
	// callers know this. (The most typical use-case will be to turn a MeshNode
	// stack allocated instance into a pointer.)

	auto a = makeDeterministicMeshNode(3, true, 2);

	auto b = a;
	EXPECT_THAT(a.sEqual(b), IsTrue());
	EXPECT_THAT(a.getParentIDs(), UnorderedElementsAreArray(b.getParentIDs()));
	EXPECT_THAT(a.getSharedID(), b.getSharedID());
	EXPECT_THAT(a.getUniqueID(), b.getUniqueID());

	b.setFaces(makeFaces(MeshNode::Primitive::TRIANGLES));
	EXPECT_THAT(a.sEqual(b), IsFalse());

	auto c = new MeshNode(a);
	EXPECT_THAT(a.sEqual(*c), IsTrue());
	EXPECT_THAT(a.getParentIDs(), UnorderedElementsAreArray(c->getParentIDs()));
	EXPECT_THAT(a.getSharedID(), c->getSharedID());
	EXPECT_THAT(a.getUniqueID(), c->getUniqueID());

	c->setFaces(makeFaces(MeshNode::Primitive::TRIANGLES));
	EXPECT_THAT(a.sEqual(*c), IsFalse());

	delete c;

	auto d = makeNewMeshNode();
	EXPECT_THAT(a.sEqual(*d), IsTrue());
	EXPECT_THAT(a.getParentIDs(), UnorderedElementsAreArray(d->getParentIDs()));
	EXPECT_THAT(a.getSharedID(), d->getSharedID());
	EXPECT_THAT(a.getUniqueID(), d->getUniqueID());

	d->setFaces(makeFaces(MeshNode::Primitive::TRIANGLES));
	EXPECT_THAT(a.sEqual(*d), IsFalse());

	delete d;

	auto e = makeRefMeshNode();
	EXPECT_THAT(a.sEqual(e), IsTrue());
	EXPECT_THAT(a.getParentIDs(), UnorderedElementsAreArray(e.getParentIDs()));
	EXPECT_THAT(a.getSharedID(), e.getSharedID());
	EXPECT_THAT(a.getUniqueID(), e.getUniqueID());

	e.setFaces(makeFaces(MeshNode::Primitive::TRIANGLES));
	EXPECT_THAT(a.sEqual(e), IsFalse());
}