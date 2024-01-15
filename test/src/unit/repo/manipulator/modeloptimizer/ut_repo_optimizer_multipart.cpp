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

#include <gtest/gtest.h>
#include <cstdlib>
#include <repo/manipulator/modeloptimizer/repo_optimizer_multipart.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <limits>
#include <boost/functional/hash.hpp>

using namespace repo::manipulator::modeloptimizer;

const static repo::core::model::RepoScene::GraphType DEFAULT_GRAPH = repo::core::model::RepoScene::GraphType::DEFAULT;
const static repo::core::model::RepoScene::GraphType OPTIMIZED_GRAPH = repo::core::model::RepoScene::GraphType::OPTIMIZED;

TEST(MultipartOptimizer, ConstructorTest)
{
	MultipartOptimizer();
}

TEST(MultipartOptimizer, DeconstructorTest)
{
	auto ptr = new MultipartOptimizer();
	delete ptr;
}

TEST(MultipartOptimizer, ApplyOptimizationTestEmpty)
{
	auto opt = MultipartOptimizer();
	repo::core::model::RepoScene *empty = nullptr;
	repo::core::model::RepoScene *empty2 = new repo::core::model::RepoScene();

	EXPECT_FALSE(opt.apply(empty));
	EXPECT_FALSE(opt.apply(empty2));

	delete empty2;
}

// Builds a triangle soup MeshNode with exactly nVertices, and faces
// constructed such that each vertex is referenced at least once.

repo::core::model::MeshNode* createRandomMesh(const int nVertices, const bool hasUV, const int primitiveSize, const std::vector<repo::lib::RepoUUID> &parent) {
	std::vector<repo::lib::RepoVector3D> vertices;
	std::vector<repo_face_t> faces;

	std::vector<std::vector<float>> bbox = { 
		{FLT_MAX, FLT_MAX, FLT_MAX},
		{FLT_MIN, FLT_MIN, FLT_MIN}
	};

	for (int i = 0; i < nVertices; ++i) {
		repo::lib::RepoVector3D vertex = {	
			static_cast<float>(std::rand()),
			static_cast<float>(std::rand()),
			static_cast<float>(std::rand()) 
		};
		vertices.push_back(vertex);

		if (vertex.x < bbox[0][0])
		{
			bbox[0][0] = vertex.x;
		}
		if (vertex.y < bbox[0][1])
		{
			bbox[0][1] = vertex.y;
		}
		if (vertex.z < bbox[0][2])
		{
			bbox[0][2] = vertex.z;
		}

		if (vertex.x > bbox[1][0])
		{
			bbox[1][0] = vertex.x;
		}
		if (vertex.y > bbox[1][1])
		{
			bbox[1][1] = vertex.y;
		}
		if (vertex.z > bbox[1][2])
		{
			bbox[1][2] = vertex.z;
		}
	}

	int vertexIndex = 0;
	do
	{
		repo_face_t face;
		for (int j = 0; j < primitiveSize; j++)
		{
			face.push_back((vertexIndex++ % vertices.size()));
		}
		faces.push_back(face);
	} while (vertexIndex < vertices.size());

	std::vector<std::vector<repo::lib::RepoVector2D>> uvs;
	if (hasUV) {
		std::vector<repo::lib::RepoVector2D> channel;
		for (int i = 0; i < nVertices; ++i) {
			channel.push_back({ (float)std::rand() / RAND_MAX, (float)std::rand() / RAND_MAX });
		}
		uvs.push_back(channel);
	}

	auto mesh = new repo::core::model::MeshNode(repo::core::model::RepoBSONFactory::makeMeshNode(vertices, faces, {}, bbox, uvs, {}, "mesh", parent));

	return mesh;
}

// The functions below compare the geometry of the original MeshNodes with the 
// stash MeshNodes as a triangle soup. The geometry should be identical.

struct GenericFace
{
	std::vector<float> data;
	int hit;

	void push(repo::lib::RepoVector2D v)
	{
		data.push_back(v.x);
		data.push_back(v.y);
	}

	void push(repo::lib::RepoVector3D v)
	{
		data.push_back(v.x);
		data.push_back(v.y);
		data.push_back(v.z);
	}

	const long hash() const
	{
		size_t hash = 0;
		for (const auto value : data)
		{
			boost::hash_combine(hash, value);
		}
		return hash;
	}

	const bool equals(const GenericFace& other) const
	{
		return data == other.data;
	}
};

void addFaces(repo::core::model::MeshNode* mesh, std::vector<GenericFace>& faces)
{
	if (mesh == nullptr)
	{
		return;
	}

	auto vertices = mesh->getVertices();
	auto channels = mesh->getUVChannelsSeparated();
	auto normals = mesh->getNormals();

	for (const auto face : mesh->getFaces())
	{
		GenericFace portableFace;
		portableFace.hit = 0;

		for (const auto index : face)
		{
			portableFace.push(vertices[index]);

			for (const auto channel : channels)
			{
				portableFace.push(channel[index]);
			}

			if (normals.size())
			{
				portableFace.push(normals[index]);
			}
		}

		faces.push_back(portableFace);
	}
}


bool compareMeshes(repo::core::model::RepoNodeSet original, repo::core::model::RepoNodeSet stash)
{
	std::vector<GenericFace> defaultFaces;
	for (const auto node : original)
	{		
		addFaces(dynamic_cast<repo::core::model::MeshNode*>(node), defaultFaces);
	}

	std::vector<GenericFace> stashFaces;
	for (const auto node : stash)
	{
		addFaces(dynamic_cast<repo::core::model::MeshNode*>(node), stashFaces);
	}

	if (defaultFaces.size() != stashFaces.size())
	{
		return false;
	}

	// Faces are compared exactly using a hash table for speed.

	std::map<long, std::vector<GenericFace>> actual;

	for (auto& face : stashFaces)
	{
		actual[face.hash()].push_back(face);
	}

	for (auto& face : defaultFaces)
	{
		auto& others = actual[face.hash()];
		for (auto& other : others) {
			if (other.hit) 
			{
				continue; // Each actual face may only be matched once
			}
			if (other.equals(face))
			{
				face.hit++;
				other.hit++;
				break;
			}
		}
	}

	// Did we find a match for all faces?

	for (const auto& face : defaultFaces)
	{
		if (!face.hit)
		{
			return false;
		}
	}

	return true;
}

TEST(MultipartOptimizer, TestAllMerged)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();

	auto nMesh = 3;
	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	for (int i = 0; i < nMesh; ++i)
		meshes.insert(createRandomMesh(10, false, 3, { rootID }));

	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_EQ(1, scene->getAllMeshes(OPTIMIZED_GRAPH).size());

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}

TEST(MultipartOptimizer, TestWithUV)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();

	auto nMesh = 3;
	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	for (int i = 0; i < nMesh; ++i)
		meshes.insert(createRandomMesh(10, i == 1, 3, { rootID }));

	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_EQ(2, scene->getAllMeshes(OPTIMIZED_GRAPH).size());

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}

TEST(MultipartOptimizer, TestMixedPrimitives)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();

	auto nVertices = 10;
	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	meshes.insert(createRandomMesh(nVertices, false, 2, { rootID }));
	meshes.insert(createRandomMesh(nVertices, false, 2, { rootID }));
	meshes.insert(createRandomMesh(nVertices, false, 3, { rootID }));
	meshes.insert(createRandomMesh(nVertices, false, 3, { rootID }));
	meshes.insert(createRandomMesh(nVertices, false, 3, { rootID }));
	meshes.insert(createRandomMesh(nVertices, false, 1, { rootID })); // unsupported primitive types must be identified as such and not combined with known types

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_EQ(3, scene->getAllMeshes(OPTIMIZED_GRAPH).size());

	// ensure that the batching has been successful.

	auto nodes = scene->getAllMeshes(OPTIMIZED_GRAPH);
	for (auto& node : nodes)
	{
		auto meshNode = dynamic_cast<repo::core::model::MeshNode*>(node);
		switch (meshNode->getPrimitive())
		{
		case repo::core::model::MeshNode::Primitive::LINES:
			ASSERT_EQ(nVertices * 2, meshNode->getVertices().size());
			break;
		case repo::core::model::MeshNode::Primitive::TRIANGLES:
			ASSERT_EQ(nVertices * 3, meshNode->getVertices().size());
			break;
		case repo::core::model::MeshNode::Primitive::UNKNOWN: // Currently, points is an unsupported type, so while it is in the enum the factory will never set it
			ASSERT_EQ(nVertices * 1, meshNode->getVertices().size());
			break;
		default:
			repoTrace << (int)meshNode->getPrimitive();
			EXPECT_TRUE(false);
			break;
		}
	}

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}

TEST(MultipartOptimizer, TestSingleLargeMesh)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();

	auto nMesh = 3;
	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	meshes.insert(createRandomMesh(65536, false, 2, { rootID }));

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_EQ(1, scene->getAllMeshes(OPTIMIZED_GRAPH).size());

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}

TEST(MultipartOptimizer, TestSingleOversizedMesh)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();

	auto nMesh = 3;
	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	meshes.insert(createRandomMesh(1200000 + 1, false, 3, { rootID })); // 1200000 comes from the const in repo_optimizer_multipart.cpp

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_EQ(2, scene->getAllMeshes(OPTIMIZED_GRAPH).size()); // even with one triangle over, large meshes should be split

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}

TEST(MultipartOptimizer, TestMultipleOversizedMeshes)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();

	auto nMesh = 3;
	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	meshes.insert(createRandomMesh(65536, false, 3, { rootID }));
	meshes.insert(createRandomMesh(65537, false, 3, { rootID }));
	meshes.insert(createRandomMesh(128537, false, 3, { rootID }));

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	// Mesh splitting is not determinsitic so we don't check the final mesh 
	// count in this test

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}

TEST(MultipartOptimizer, TestMultiplesMeshes)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();


	// These vertex counts, along with the primitive size, are multiples of
	// the max supermesh size and are designed to trip up supermeshing edge 
	// cases

	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	meshes.insert(createRandomMesh(16384, false, 2, { rootID }));
	meshes.insert(createRandomMesh(16384, false, 2, { rootID }));
	meshes.insert(createRandomMesh(16384, false, 2, { rootID }));
	meshes.insert(createRandomMesh(16384, false, 2, { rootID }));

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_EQ(1, scene->getAllMeshes(OPTIMIZED_GRAPH).size());

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}

TEST(MultipartOptimizer, TestMultipleSmallAndLargeMeshes)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();

	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	meshes.insert(createRandomMesh(16384, false, 2, { rootID }));
	meshes.insert(createRandomMesh(16384, false, 2, { rootID }));
	meshes.insert(createRandomMesh(16384, false, 2, { rootID }));
	meshes.insert(createRandomMesh(16384, false, 2, { rootID }));
	meshes.insert(createRandomMesh(65536, false, 2, { rootID }));
	meshes.insert(createRandomMesh(128000, false, 3, { rootID }));
	meshes.insert(createRandomMesh(16384, false, 3, { rootID }));
	meshes.insert(createRandomMesh(8000, false, 3, { rootID }));

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}

TEST(MultipartOptimizer, TestTinyMeshes)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();

	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	for (size_t i = 0; i < 10000; i++)
	{
		meshes.insert(createRandomMesh(4, false, 3, { rootID }));
	}

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	// Make sure no supermesh has more than 5000 mappings (submeshes). We won't 
	// see the effects in the unit test but when we try to commit the node
	// it will fail.

	for (const auto stash : scene->getAllMeshes(OPTIMIZED_GRAPH))
	{
		auto mapping = dynamic_cast<repo::core::model::MeshNode*>(stash)->getMeshMapping();
		EXPECT_LE(mapping.size(), 5000);
	}

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}