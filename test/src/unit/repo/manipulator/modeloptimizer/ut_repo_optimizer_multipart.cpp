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
#include <limits>
#include <test/src/unit/repo_test_mesh_utils.h>
#include <repo/core/model/bson/repo_bson_factory.h>

using namespace repo::manipulator::modeloptimizer;
using namespace repo::test::utils::mesh;

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

// The functions below compare the geometry of the original MeshNodes with the 
// stash MeshNodes as a triangle soup. The geometry should be identical.


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

	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene({}, meshes, dummy, dummy, dummy, trans);
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

	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene({}, meshes, dummy, dummy, dummy, trans);
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

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, meshes, dummy, dummy, dummy, trans);
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

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, meshes, dummy, dummy, dummy, trans);
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

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, meshes, dummy, dummy, dummy, trans);
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

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, meshes, dummy, dummy, dummy, trans);
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

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, meshes, dummy, dummy, dummy, trans);
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

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, meshes, dummy, dummy, dummy, trans);
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

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, meshes, dummy, dummy, dummy, trans);
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
		auto mapping = dynamic_cast<repo::core::model::SupermeshNode*>(stash)->getMeshMapping();
		EXPECT_LE(mapping.size(), 5000);
	}

	EXPECT_TRUE(compareMeshes(scene->getAllMeshes(DEFAULT_GRAPH), scene->getAllMeshes(OPTIMIZED_GRAPH)));
}