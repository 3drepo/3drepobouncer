/**
*  Copyright (C) 2024 3D Repo Ltd
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
#include <repo/manipulator/modelutility/repo_mesh_map_reorganiser.h>
#include <repo/manipulator/modeloptimizer/repo_optimizer_multipart.h>
#include <limits>
#include <unordered_set>
#include <test/src/unit/repo_test_mesh_utils.h>

using namespace repo::test::utils::mesh;
using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modeloptimizer;

#pragma optimize("", off)

TEST(MeshMapReorganiser, SingleLargeMesh)
{
	// This snippet creates a Supermesh using the Multipart Optimizer

	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();
	repo::core::model::RepoNodeSet meshes, trans;
	trans.insert(root);
	meshes.insert(createRandomMesh(128000, false, 3, { rootID }));
	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, {}, meshes, {}, {}, {}, trans);
	opt.apply(scene);

	auto supermesh = (repo::core::model::SupermeshNode*)*scene->getAllSupermeshes(repo::core::model::RepoScene::GraphType::OPTIMIZED).begin();

	// Check our test data - the supermesh at this stage should have one, large mapping.
	EXPECT_EQ(supermesh->getMeshMapping().size(), 1);

	MeshMapReorganiser reSplitter(supermesh, 65536, SIZE_MAX);

	auto remapped = reSplitter.getRemappedMesh();

	// Check that the primitive has been preserved
	EXPECT_EQ(remapped.getPrimitive(), supermesh->getPrimitive());

	// Check that the submesh ids have been preserved. Though the supermesh
	// will be split into chunks, there should only be one submesh id as there
	// aren't multiple elements.

	auto ids = remapped.getSubmeshIds();
	auto end = std::unique(ids.begin(), ids.end());
	auto count = end - ids.begin();
	EXPECT_EQ(count, 1); 

	// The supermesh should be split into two chunks (with the same submesh ids)

	EXPECT_EQ(remapped.getMeshMapping().size(), 2);

	// Mapping ids should be identities

	for (auto m : remapped.getMeshMapping())
	{
		EXPECT_TRUE(m.mesh_id.isDefaultValue());
		EXPECT_TRUE(m.material_id.isDefaultValue());
		EXPECT_TRUE(m.shared_id.isDefaultValue());
	}

	// The usage lists should show that the mesh id from the original supermesh
	// is used in two places.

	auto splitMapping = reSplitter.getSplitMapping();
	EXPECT_EQ(splitMapping.size(), 1);
	auto originalId = supermesh->getMeshMapping()[0].mesh_id;
	auto usageIt = splitMapping.find(originalId);
	EXPECT_FALSE(usageIt == splitMapping.end());
	auto usage = usageIt->second;
	EXPECT_EQ(usage.size(), 2);
}


TEST(MeshMapReorganiser, MultipleTinyMeshes)
{
	// This snippet creates a Supermesh using the Multipart Optimizer

	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();
	repo::core::model::RepoNodeSet meshes, trans;
	trans.insert(root);

	const int NUM_SUBMESHES = 789;
	const int NUM_VERTICES = 256;

	for (int i = 0; i < NUM_SUBMESHES; i++)
	{
		meshes.insert(createRandomMesh(NUM_VERTICES, false, 3, { rootID }));
	}

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, {}, meshes, {}, {}, {}, trans);
	opt.apply(scene);

	auto supermesh = (repo::core::model::SupermeshNode*)*scene->getAllSupermeshes(repo::core::model::RepoScene::GraphType::OPTIMIZED).begin();

	// Check our test data - the supermesh at this stage should have one, large mapping.
	EXPECT_EQ(supermesh->getMeshMapping().size(), NUM_SUBMESHES);

	MeshMapReorganiser reSplitter(supermesh, 65536, SIZE_MAX);

	auto remapped = reSplitter.getRemappedMesh();

	auto ids = remapped.getSubmeshIds();
	auto end = std::unique(ids.begin(), ids.end());
	auto count = end - ids.begin();
	EXPECT_EQ(count, NUM_SUBMESHES);

	// The supermesh should be split into two chunks (with the same submesh ids)

	auto expectedSplit = ceil((NUM_SUBMESHES * NUM_VERTICES) / 65536.0f);
	EXPECT_EQ(remapped.getMeshMapping().size(), expectedSplit);

	// Mapping ids should be identities

	for (auto m : remapped.getMeshMapping())
	{
		EXPECT_TRUE(m.mesh_id.isDefaultValue());
		EXPECT_TRUE(m.material_id.isDefaultValue());
		EXPECT_TRUE(m.shared_id.isDefaultValue());
	}

	// The usage lists should show that the mesh id from the original supermesh
	// is used in two places.

	auto splitMapping = reSplitter.getSplitMapping();
	EXPECT_EQ(splitMapping.size(), NUM_SUBMESHES);

	// Check all the original submesh ids are present in the split mapping

	std::unordered_set<int> usages;

	for (auto m : meshes)
	{
		auto split = splitMapping.find(m->getUniqueID());

		// Every original mesh id should be present in the split mappings
		EXPECT_TRUE(split != splitMapping.end());

		// None of the meshes are large enough to need splitting, so they
		// should all be used in at most one chunk
		EXPECT_EQ(split->second.size(), 1);

		usages.insert(split->second[0]);
	}

	// All the splits should be referenced by at least one submesh

	EXPECT_EQ(usages.size(), expectedSplit);
}

TEST(MeshMapReorganiser, InterleavedMixedSplit)
{
	// Create a supermesh that contains a mix of small meshes, and large meshes
	// that will need to be split.

	auto opt = MultipartOptimizer();
	auto root = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	auto rootID = root->getSharedID();
	repo::core::model::RepoNodeSet meshes, trans;
	trans.insert(root);

	meshes.insert(createRandomMesh(129, false, 3, { rootID }));
	meshes.insert(createRandomMesh(65537, false, 3, { rootID }));
	meshes.insert(createRandomMesh(1341, false, 3, { rootID }));
	meshes.insert(createRandomMesh(68, false, 3, { rootID }));
	meshes.insert(createRandomMesh(80000, false, 3, { rootID }));
	meshes.insert(createRandomMesh(17981, false, 3, { rootID }));

	const int NUM_SUBMESHES = 6;

	repo::core::model::RepoScene* scene = new repo::core::model::RepoScene({}, {}, meshes, {}, {}, {}, trans);
	opt.apply(scene);

	auto supermesh = (repo::core::model::SupermeshNode*)*scene->getAllSupermeshes(repo::core::model::RepoScene::GraphType::OPTIMIZED).begin();

	// Check our test data

	EXPECT_EQ(supermesh->getMeshMapping().size(), NUM_SUBMESHES);

	MeshMapReorganiser reSplitter(supermesh, 65536, SIZE_MAX);

	auto remapped = reSplitter.getRemappedMesh();

	auto splits = remapped.getMeshMapping();

	// Mapping ids should be identities

	for (auto m : splits)
	{
		EXPECT_TRUE(m.mesh_id.isDefaultValue());
		EXPECT_TRUE(m.material_id.isDefaultValue());
		EXPECT_TRUE(m.shared_id.isDefaultValue());
	}

	auto ids = remapped.getSubmeshIds();
	auto end = std::unique(ids.begin(), ids.end());
	auto count = end - ids.begin();
	EXPECT_EQ(count, NUM_SUBMESHES);

	// The usage lists should show that the mesh id from the original supermesh
	// is used in two places.

	auto splitMapping = reSplitter.getSplitMapping();
	EXPECT_EQ(splitMapping.size(), NUM_SUBMESHES);

	std::unordered_set<int> usages;
	for (auto m : meshes)
	{
		auto split = splitMapping.find(m->getUniqueID());

		// Every original mesh id should be present in the split mappings
		EXPECT_TRUE(split != splitMapping.end());

		for (auto usage : split->second)
		{
			usages.insert(usage);
		}
	}

	// All the splits should be referenced by at least one submesh

	EXPECT_EQ(usages.size(), splits.size());
}