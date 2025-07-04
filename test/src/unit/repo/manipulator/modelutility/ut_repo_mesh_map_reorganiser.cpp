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
#include <repo/core/model/bson/repo_bson_factory.h>
#include <repo/manipulator/modelutility/repo_mesh_map_reorganiser.h>
#include <repo/manipulator/modeloptimizer/repo_optimizer_multipart.h>
#include <limits>
#include <unordered_set>
#include <test/src/unit/repo_test_mesh_utils.h>
#include <test/src/unit/repo_test_database_info.h>
#include <repo/manipulator/modelutility/repo_scene_builder.h>

using namespace repo::test::utils::mesh;
using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modeloptimizer;

#define DBMESHMAPREORGANISERTEST "meshMapReorganiserTest"

TEST(MeshMapReorganiser, VeryLargeMesh)
{
	// This snippet creates a Supermesh using the Multipart Optimizer

	auto opt = MultipartOptimizer();

	auto handler = getHandler();
	std::string database = DBMESHMAPREORGANISERTEST;
	std::string projectName = "VeryLargeMesh";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	sceneBuilder.addNode(createRandomMesh(327890, false, 3, { rootNodeId }));
	
	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	bool result = opt.processScene(
		database,
		projectName,
		revId,
		handler.get(),
		mockExporter.get()
	);

	EXPECT_TRUE(result);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 1);
	
	// Check our test data - the supermesh at this stage should have one, large mapping.
	auto supermesh = mockExporter->getSupermeshes()[0];
	EXPECT_EQ(supermesh.getMeshMapping().size(), 1);

	MeshMapReorganiser reSplitter(&supermesh, 65536, SIZE_MAX);

	auto remapped = reSplitter.getRemappedMesh();

	// Check that the primitive has been preserved
	EXPECT_EQ(remapped->getPrimitive(), supermesh.getPrimitive());

	// Check that the submesh ids have been preserved. Though the supermesh
	// will be split into chunks, there should only be one submesh id as there
	// aren't multiple elements.

	auto ids = remapped->getSubmeshIds();
	auto end = std::unique(ids.begin(), ids.end());
	auto count = end - ids.begin();
	EXPECT_EQ(count, 1);

	// The supermesh should be split into six chunks (with the same submesh ids)

	EXPECT_EQ(remapped->getMeshMapping().size(), 6);

	// Mapping ids should be identities

	for (auto m : remapped->getMeshMapping())
	{
		EXPECT_TRUE(m.mesh_id.isDefaultValue());
		EXPECT_TRUE(m.material_id.isDefaultValue());
		EXPECT_TRUE(m.shared_id.isDefaultValue());
	}

	// The usage lists should show that the mesh id from the original supermesh
	// is used in two places.

	auto splitMapping = reSplitter.getSplitMapping();
	EXPECT_EQ(splitMapping.size(), 1);
	auto originalId = supermesh.getMeshMapping()[0].mesh_id;
	auto usageIt = splitMapping.find(originalId);
	EXPECT_FALSE(usageIt == splitMapping.end());
	auto usage = usageIt->second;
	EXPECT_EQ(usage.size(), 6);
}


TEST(MeshMapReorganiser, MultipleTinyMeshes)
{
	// This snippet creates a Supermesh using the Multipart Optimizer

	auto opt = MultipartOptimizer();

	auto handler = getHandler();
	std::string database = DBMESHMAPREORGANISERTEST;
	std::string projectName = "InterleavedMixedSplit";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();
		
	const int NUM_SUBMESHES = 789;
	const int NUM_VERTICES = 256;
		
	std::vector<repo::lib::RepoUUID> meshIds;
	for (int i = 0; i < NUM_SUBMESHES; i++)
	{
		auto node = createRandomMesh(NUM_VERTICES, false, 3, { rootNodeId });
		meshIds.push_back(node->getUniqueID());
		sceneBuilder.addNode(std::move(node));
	}

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	bool result = opt.processScene(
		database,
		projectName,
		revId,
		handler.get(),
		mockExporter.get()
	);

	EXPECT_TRUE(result);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 1);

	// Check our test data - the supermesh at this stage should have one, large mapping.
	auto supermesh = mockExporter->getSupermeshes()[0];
	
	EXPECT_EQ(supermesh.getMeshMapping().size(), NUM_SUBMESHES);

	MeshMapReorganiser reSplitter(&supermesh, 65536, SIZE_MAX);

	auto remapped = reSplitter.getRemappedMesh();

	auto ids = remapped->getSubmeshIds();
	auto end = std::unique(ids.begin(), ids.end());
	auto count = end - ids.begin();
	EXPECT_EQ(count, NUM_SUBMESHES);

	// The supermesh should be split into two chunks (with the same submesh ids)

	auto expectedSplit = ceil((NUM_SUBMESHES * NUM_VERTICES) / 65536.0f);
	EXPECT_EQ(remapped->getMeshMapping().size(), expectedSplit);

	// Mapping ids should be identities

	for (auto m : remapped->getMeshMapping())
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

	for (auto id : meshIds)
	{
		auto split = splitMapping.find(id);

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

	auto handler = getHandler();
	std::string database = DBMESHMAPREORGANISERTEST;
	std::string projectName = "InterleavedMixedSplit";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);
	
	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();
		
	auto randNode1 = createRandomMesh(129, false, 3, { rootNodeId });
	auto randNode2 = createRandomMesh(65537, false, 3, { rootNodeId });
	auto randNode3 = createRandomMesh(1341, false, 3, { rootNodeId });
	auto randNode4 = createRandomMesh(68, false, 3, { rootNodeId });
	auto randNode5 = createRandomMesh(80000, false, 3, { rootNodeId });
	auto randNode6 = createRandomMesh(17981, false, 3, { rootNodeId });
	
	std::vector<repo::lib::RepoUUID> meshIds;
	meshIds.push_back(randNode1->getUniqueID());
	meshIds.push_back(randNode2->getUniqueID());
	meshIds.push_back(randNode3->getUniqueID());
	meshIds.push_back(randNode4->getUniqueID());
	meshIds.push_back(randNode5->getUniqueID());
	meshIds.push_back(randNode6->getUniqueID());

	sceneBuilder.addNode(std::move(randNode1));
	sceneBuilder.addNode(std::move(randNode2));
	sceneBuilder.addNode(std::move(randNode3));
	sceneBuilder.addNode(std::move(randNode4));
	sceneBuilder.addNode(std::move(randNode5));
	sceneBuilder.addNode(std::move(randNode6));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	bool result = opt.processScene(
		database,
		projectName,
		revId,
		handler.get(),
		mockExporter.get()
	);

	EXPECT_TRUE(result);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 1);
		
	// Check our test data	
	auto supermesh = mockExporter->getSupermeshes()[0];
	
	const int NUM_SUBMESHES = 6;
	EXPECT_EQ(supermesh.getMeshMapping().size(), NUM_SUBMESHES);

	MeshMapReorganiser reSplitter(&supermesh, 65536, SIZE_MAX);

	auto remapped = reSplitter.getRemappedMesh();

	auto splits = remapped->getMeshMapping();

	// Mapping ids should be identities

	for (auto m : splits)
	{
		EXPECT_TRUE(m.mesh_id.isDefaultValue());
		EXPECT_TRUE(m.material_id.isDefaultValue());
		EXPECT_TRUE(m.shared_id.isDefaultValue());
	}

	auto ids = remapped->getSubmeshIds();
	auto end = std::unique(ids.begin(), ids.end());
	auto count = end - ids.begin();
	EXPECT_EQ(count, NUM_SUBMESHES);

	// The usage lists should show that the mesh id from the original supermesh
	// is used in two places.

	auto splitMapping = reSplitter.getSplitMapping();
	EXPECT_EQ(splitMapping.size(), NUM_SUBMESHES);

	std::unordered_set<int> usages;
	for (auto id : meshIds)
	{
		auto split = splitMapping.find(id);

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