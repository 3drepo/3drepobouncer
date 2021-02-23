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

repo::core::model::MeshNode* createRandomMesh(const int nVertices, const int nFaces, const bool hasUV, const int primitiveSize, const std::vector<repo::lib::RepoUUID> &parent) {
	std::vector<repo::lib::RepoVector3D> vertices;
	std::vector<repo_face_t> faces;

	for (int i = 0; i < nVertices; ++i) {
		vertices.push_back({static_cast<float>(std::rand()), 
							static_cast<float>(std::rand()),
							static_cast<float>(std::rand())});
	}

	for (int i = 0; i < nFaces; ++i) {
		repo_face_t face;
		for (int j = 0; j < primitiveSize; ++j)
			face.push_back(std::rand() / nVertices);
		faces.push_back(face);
	}

	std::vector<std::vector<repo::lib::RepoVector2D>> uvs;
	if (hasUV) {
		std::vector<repo::lib::RepoVector2D> channel;
		for (int i = 0; i < nVertices; ++i) {
			channel.push_back({ (float)std::rand() / RAND_MAX, (float)std::rand() / RAND_MAX });
		}
		uvs.push_back(channel);
	}

	auto mesh = new repo::core::model::MeshNode(repo::core::model::RepoBSONFactory::makeMeshNode(vertices, faces, {}, {}, uvs, {}, {}, "mesh", parent));

	return mesh;
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
		meshes.insert(createRandomMesh(10, 3, false, 3, { rootID }));

	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_EQ(1, scene->getAllMeshes(OPTIMIZED_GRAPH).size());
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
		meshes.insert(createRandomMesh(10, 3, i == 1, 3, { rootID }));

	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene({}, dummy, meshes, dummy, dummy, dummy, trans);
	ASSERT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	ASSERT_FALSE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_TRUE(opt.apply(scene));
	EXPECT_TRUE(scene->hasRoot(DEFAULT_GRAPH));
	EXPECT_TRUE(scene->hasRoot(OPTIMIZED_GRAPH));

	EXPECT_EQ(2, scene->getAllMeshes(OPTIMIZED_GRAPH).size());
}

TEST(MultipartOptimizer, TestMixedPrimitives)
{
	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();

	auto nVertices = 10;
	auto nFaces = 3;
	repo::core::model::RepoNodeSet meshes, trans, dummy;
	trans.insert(root);
	meshes.insert(createRandomMesh(nVertices, nFaces, false, 2, { rootID }));
	meshes.insert(createRandomMesh(nVertices, nFaces, false, 2, { rootID }));
	meshes.insert(createRandomMesh(nVertices, nFaces, false, 3, { rootID }));
	meshes.insert(createRandomMesh(nVertices, nFaces, false, 3, { rootID }));
	meshes.insert(createRandomMesh(nVertices, nFaces, false, 3, { rootID }));
	meshes.insert(createRandomMesh(nVertices, nFaces, false, 1, { rootID })); // unsupported primitive types must be identified as such and not combined with known types

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


}
