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
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include "../../../repo_test_utils.h"
#include "../../../repo_test_matchers.h"
#include "../../../repo_test_mesh_utils.h"
#include <repo/core/model/bson/repo_bson_factory.h>

#include <repo/manipulator/modeloptimizer/repo_optimizer_trans_reduction.h>

using namespace repo::manipulator::modeloptimizer;
using namespace repo::core::model;
using namespace testing;

TEST(TransformationReductionOptimizerTest, Constructor)
{
	TransformationReductionOptimizer();
}

TEST(TransformationReductionOptimizerTest, Deconstructor)
{
	auto ptr = new TransformationReductionOptimizer();
	delete ptr;
}

TEST(TransformationReductionOptimizerTest, ApplyOptimizationEmpty)
{
	auto opt = TransformationReductionOptimizer();
	repo::core::model::RepoScene *empty = nullptr;
	repo::core::model::RepoScene *empty2 = new repo::core::model::RepoScene();

	EXPECT_FALSE(opt.apply(empty));
	EXPECT_FALSE(opt.apply(empty2));
}

TEST(TransformationReductionOptimizerTest, ApplyOptimizationSingle)
{
	// Create a scene with a mesh holding a single transform that can be combined
	// into the mesh itself.

	auto root = RepoBSONFactory::makeTransformationNode();

	auto matrix = repo::test::utils::mesh::makeTransform(true, true);
	auto transform_m = RepoBSONFactory::makeTransformationNode(matrix, "matrix", { root.getSharedID() });

	auto mesh = repo::test::utils::mesh::makeMeshNode(repo::test::utils::mesh::mesh_data(true, true, 0, 3, true, 0, 100));
	mesh.addParent(transform_m.getSharedID());

	auto scene = new RepoScene(
		{},
		{ new MeshNode(mesh) },
		{},
		{},
		{},
		{ new TransformationNode(root), new TransformationNode(transform_m) }
	);

	repo::manipulator::modeloptimizer::TransformationReductionOptimizer optimizer;
	optimizer.apply(scene);

	// We expect transform_m to be have been applied to the mesh, which should
	// now be a descendant of root.

	auto optRoot = scene->getRoot(RepoScene::GraphType::DEFAULT);

	// Check that the mesh(es) are direct descendents of the root node

	auto meshes = scene->getAllMeshes(RepoScene::GraphType::DEFAULT);
	EXPECT_THAT(meshes.size(), Eq(1));
	for (auto m : meshes) {
		EXPECT_THAT(m->getParentIDs(), ElementsAre(optRoot->getSharedID()));
	}

	// There should only be one transformation (the root)

	EXPECT_THAT(scene->getAllTransformations(RepoScene::GraphType::DEFAULT).size(), Eq(1));
	EXPECT_THAT(*(scene->getAllTransformations(RepoScene::GraphType::DEFAULT).begin()), Eq(optRoot));

	// Check that the name(s) have been changed

	for (auto m : meshes) {
		EXPECT_THAT(m->getName(), Eq(transform_m.getName()));
	}

	// Check that the lookups are intact

	for (auto m : scene->getAllMeshes(RepoScene::GraphType::DEFAULT))
	{
		EXPECT_THAT(scene->getNodeByUniqueID(RepoScene::GraphType::DEFAULT, m->getUniqueID())->getUniqueID(), Eq(m->getUniqueID()));
		EXPECT_THAT(scene->getNodeBySharedID(RepoScene::GraphType::DEFAULT, m->getSharedID())->getSharedID(), Eq(m->getSharedID()));
	}

	for (auto m : scene->getAllTransformations(RepoScene::GraphType::DEFAULT))
	{
		EXPECT_THAT(scene->getNodeByUniqueID(RepoScene::GraphType::DEFAULT, m->getUniqueID())->getUniqueID(), Eq(m->getUniqueID()));
		EXPECT_THAT(scene->getNodeBySharedID(RepoScene::GraphType::DEFAULT, m->getSharedID())->getSharedID(), Eq(m->getSharedID()));
	}

	// Check that the transform has been applied

	auto optMesh = dynamic_cast<const MeshNode*>(scene->getNodeByUniqueID(RepoScene::GraphType::DEFAULT, mesh.getUniqueID()));
	EXPECT_THAT(optMesh, NotNull());
	EXPECT_THAT(optMesh->sEqual(mesh.cloneAndApplyTransformation(matrix)), IsTrue());
}

TEST(TransformationReductionOptimizerTest, Metadata)
{
	// Create a scene with a mesh holding a single transform that can be combined
	// into the mesh itself.

	auto root = RepoBSONFactory::makeTransformationNode();

	std::vector<TransformationNode> transforms = {
		RepoBSONFactory::makeTransformationNode(repo::test::utils::mesh::makeTransform(true, true), "matrix", { root.getSharedID() }),
		RepoBSONFactory::makeTransformationNode(repo::test::utils::mesh::makeTransform(true, true), "matrix", { root.getSharedID() }),
		RepoBSONFactory::makeTransformationNode(repo::test::utils::mesh::makeTransform(true, true), "matrix", { root.getSharedID() })
	};

	std::vector<MeshNode> meshes = {
		repo::test::utils::mesh::makeMeshNode(repo::test::utils::mesh::mesh_data(true, true, 0, 3, true, 0, 100)),
		repo::test::utils::mesh::makeMeshNode(repo::test::utils::mesh::mesh_data(true, true, 0, 3, true, 0, 100)),
		repo::test::utils::mesh::makeMeshNode(repo::test::utils::mesh::mesh_data(true, true, 0, 3, true, 0, 100))
	};

	for (int i = 0; i < transforms.size(); i++)
	{
		transforms[i].addParent(root.getSharedID());
		meshes[i].addParent(transforms[i].getSharedID());
	}

	// Create some metadata - one node is associated with mesh 1, while a second
	// node is associated with two meshes.
	// All three meshes should have their transforms combined.

	repo::core::model::RepoRef::Metadata metadata1;
	metadata1["mesh"] = "0";
	auto m1 = RepoBSONFactory::makeMetaDataNode(metadata1, "metadata1", { meshes[0].getSharedID() });

	repo::core::model::RepoRef::Metadata metadata2;
	metadata2["mesh1"] = "1";
	metadata2["mesh2"] = "2";
	auto m2 = RepoBSONFactory::makeMetaDataNode(metadata2, "metadata2", { meshes[1].getSharedID(), meshes[2].getSharedID() });

	RepoNodeSet meshNodeSet;
	RepoNodeSet transformNodeSet;
	RepoNodeSet metaNodeSet;

	metaNodeSet.insert(new MetadataNode(m1));
	metaNodeSet.insert(new MetadataNode(m2));

	for (auto m : meshes)
	{
		meshNodeSet.insert(new MeshNode(m));
	}

	transformNodeSet.insert(new TransformationNode(root));
	for (auto t : transforms)
	{
		transformNodeSet.insert(new TransformationNode(t));
	}

	auto scene = new RepoScene(
		{},
		meshNodeSet,
		{},
		metaNodeSet,
		{},
		transformNodeSet
	);

	repo::manipulator::modeloptimizer::TransformationReductionOptimizer optimizer;
	optimizer.apply(scene);

	// We expect transform_m to be have been applied to the mesh, which should
	// now be a descendant of root.

	auto optRoot = scene->getRoot(RepoScene::GraphType::DEFAULT);

	// Check that root has not changed

	EXPECT_THAT(optRoot->sEqual(root), IsTrue());

	// Check that the meshes are direct descendents of the root node, and the
	// lookups are intact

	for (auto m : scene->getAllMeshes(RepoScene::GraphType::DEFAULT))
	{
		EXPECT_THAT(m->getParentIDs(), ElementsAre(optRoot->getSharedID()));
	}

	EXPECT_THAT(scene->getAllTransformations(RepoScene::GraphType::DEFAULT).size(), Eq(1));

	for (auto m : scene->getAllMeshes(RepoScene::GraphType::DEFAULT))
	{
		EXPECT_THAT(scene->getNodeByUniqueID(RepoScene::GraphType::DEFAULT, m->getUniqueID())->getUniqueID(), Eq(m->getUniqueID()));
		EXPECT_THAT(scene->getNodeBySharedID(RepoScene::GraphType::DEFAULT, m->getSharedID())->getSharedID(), Eq(m->getSharedID()));
	}

	for (auto m : scene->getAllTransformations(RepoScene::GraphType::DEFAULT))
	{
		EXPECT_THAT(scene->getNodeByUniqueID(RepoScene::GraphType::DEFAULT, m->getUniqueID())->getUniqueID(), Eq(m->getUniqueID()));
		EXPECT_THAT(scene->getNodeBySharedID(RepoScene::GraphType::DEFAULT, m->getSharedID())->getSharedID(), Eq(m->getSharedID()));
	}

	for (auto m : scene->getAllMetadata(RepoScene::GraphType::DEFAULT))
	{
		EXPECT_THAT(scene->getNodeByUniqueID(RepoScene::GraphType::DEFAULT, m->getUniqueID())->getUniqueID(), Eq(m->getUniqueID()));
		EXPECT_THAT(scene->getNodeBySharedID(RepoScene::GraphType::DEFAULT, m->getSharedID())->getSharedID(), Eq(m->getSharedID()));
	}

	// The metadatas should still be assigned to the correct nodes

	EXPECT_THAT(scene->getAllMetadata(RepoScene::GraphType::DEFAULT).size(), Eq(2));

	auto optm1 = scene->getNodeByUniqueID(RepoScene::GraphType::DEFAULT, m1.getUniqueID());
	ASSERT_TRUE(optm1);
	EXPECT_THAT(optm1->getParentIDs(), UnorderedElementsAre(meshes[0].getSharedID()));

	auto optm2 = scene->getNodeByUniqueID(RepoScene::GraphType::DEFAULT, m2.getUniqueID());
	ASSERT_TRUE(optm2);
	EXPECT_THAT(optm2->getParentIDs(), UnorderedElementsAre(meshes[1].getSharedID(), meshes[2].getSharedID() ));
}