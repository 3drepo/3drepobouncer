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
#include <cstdlib>
#include <limits>
#include <unordered_set>
#include <algorithm>
#include <repo/manipulator/modeloptimizer/repo_optimizer_multipart.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <repo/manipulator/modelutility/repo_scene_builder.h>
#include <test/src/unit/repo_test_mesh_utils.h>
#include <test/src/unit/repo_test_database_info.h>
#include <test/src/unit/repo_test_random_generator.h>

using namespace repo::manipulator::modeloptimizer;
using namespace repo::test::utils::mesh;

#define DBMULTIPARTOPTIMIZERTEST "multipartOptimiserTest"

// The functions below compare the geometry of the original MeshNodes with the
// stash MeshNodes as a triangle soup. The geometry should be identical.

TEST(MultipartOptimizer, TestAllMerged)
{

	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestAllMerged";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	auto nMesh = 3;
	
	for (int i = 0; i < nMesh; ++i) {
		auto randNode = createRandomMesh(10, false, 3, "", { rootNodeId });
		sceneBuilder.addNode(std::move(randNode));			
	}

	sceneBuilder.finalise();
	
	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	MultipartOptimizer opt(handler.get(), mockExporter.get());
	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 1);
	
	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestWithUV)
{
	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestWithUV";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	// The new mpOpt drops geometry that has no material, so we add a texture node as well
	auto texNode = repo::core::model::RepoBSONFactory::makeTextureNode("", 0, 0, 0, 0, { rootNodeId });
	sceneBuilder.addNode(texNode);
	auto texNodeId = texNode.getUniqueID();

	auto nMesh = 3;
	
	for (int i = 0; i < nMesh; ++i) {
		auto randNode = createRandomMesh(10, i == 1, 3, "", { rootNodeId });
		if (i == 1) {
			randNode->setTextureId(texNodeId);
		}
		sceneBuilder.addNode(std::move(randNode));
	}

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	MultipartOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 2);
	
	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestMixedPrimitives)
{
	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestMixedPrimitives";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	auto nVertices = 10;
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 3, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 3, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 3, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 1, "", { rootNodeId })); // unsupported primitive types must be identified as such and not combined with known types

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	MultipartOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 2); // The new mpOpt just ignores unsupported primitives
	
	// ensure that the batching has been successful.	
	for (auto& node : mockExporter->getSupermeshes())
	{		
		switch (node.getPrimitive())
		{
		case repo::core::model::MeshNode::Primitive::LINES:
			ASSERT_EQ(nVertices * 2, node.getVertices().size());
			break;
		case repo::core::model::MeshNode::Primitive::TRIANGLES:
			ASSERT_EQ(nVertices * 3, node.getVertices().size());
			break;
		default:
			repoTrace << (int)node.getPrimitive();
			EXPECT_TRUE(false); // No other topologies should be encountered in this test.
			break;
		}
	}

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestSingleLargeMesh)
{
	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestSingleLargeMesh";
	auto revId = repo::lib::RepoUUID::createUUID();


	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();
	
	auto largeMesh = createRandomMesh(65536, false, 2, "", { rootNodeId });
	sceneBuilder.addNode(std::move(largeMesh));
	
	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	MultipartOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 1);

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestSingleOversizedMesh)
{
	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestSingleOversizedMesh";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

		
	sceneBuilder.addNode(createRandomMesh(REPO_MP_MAX_VERTEX_COUNT + 1, false, 3, "", { rootNodeId }));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	MultipartOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 2); // even with one triangle over, large meshes should be split

	EXPECT_TRUE(checkSupermeshVertCounts(mockExporter.get(), REPO_MP_MAX_VERTEX_COUNT));

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestMultipleOversizedMeshes)
{
	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestMultipleOversizedMeshes";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	sceneBuilder.addNode(createRandomMesh(65536, false, 3, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(65537, false, 3, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(128537, false, 3, "", { rootNodeId }));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));
	
	MultipartOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	// Mesh splitting is not determinsitic so we don't check the final mesh
	// count in this test

	EXPECT_TRUE(checkSupermeshVertCounts(mockExporter.get(), REPO_MP_MAX_VERTEX_COUNT));

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestSplittingLocality)
{
	// Our splitting algorithm should not merely break a larger mesh down into smaller ones randomly.
	// Instead, the vertices of the resulting supermeshes should be colocated.
	// This tests checks for that by creating a mesh that consists of multiple clusters of triangle soups.
	// These clusters are separated by empty space and, if working correctly, the splitting algorithm
	// should split the vertices so that the resulting supermeshes contain only vertices belonging to
	// one of the cluster origins and not multiples.

	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestSplitting";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	// Generate random cluster positions.

	testing::RepoRandomGenerator random;
	
	// Parameters for cluster generation
	int noOrigins = 4;
	int clusterRadius = 100;
	double clusterSeparation = clusterRadius * 4;	
	double minDistFromOrigin = 0;
	double maxDistFromOrigin = 10000;

	// Generate origins so that they don't come to close to each other.
	std::vector<repo::lib::RepoVector3D> clusterOrigins;
	for (int i = 0; i < noOrigins; i++) {
		repo::lib::RepoVector3D newOrigin;
		bool collides;

		do
		{
			newOrigin = random.vector({ minDistFromOrigin, maxDistFromOrigin });

			collides = false;
			for (auto otherOrigin : clusterOrigins)
			{
				if ((newOrigin - otherOrigin).norm() <= clusterSeparation)
				{
					collides = true;
					break;
				}
			}

		} while (collides);

		clusterOrigins.push_back(newOrigin);
	}	
	
	// Create node and finalise scene
	int nVertices = REPO_MP_MAX_VERTEX_COUNT * noOrigins;
	sceneBuilder.addNode(createRandomClusteredMesh(nVertices, false, 3, "", { rootNodeId }, clusterRadius, clusterOrigins));
	sceneBuilder.finalise();

	// Process with mock exporter
	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));
	auto opt = MultipartOptimizer(handler.get(), mockExporter.get());
	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_TRUE(mockExporter->getSupermeshCount() >= 4); // should be split into at least four supermeshes

	EXPECT_TRUE(checkSupermeshVertCounts(mockExporter.get(), REPO_MP_MAX_VERTEX_COUNT));

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));

	// Check the split result
	EXPECT_TRUE(checkClusteredMeshSplit(mockExporter.get(), clusterOrigins));
}

TEST(MultipartOptimizer, TestMultiplesMeshes)
{
	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestMultiplesMeshes";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();


	// These vertex counts, along with the primitive size, are multiples of
	// the max supermesh size and are designed to trip up supermeshing edge
	// cases
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, "", { rootNodeId }));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	MultipartOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 1);

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestMultipleSmallAndLargeMeshes)
{
	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestMultipleSmallAndLargeMeshes";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	sceneBuilder.addNode(createRandomMesh(16384, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(65536, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(128000, false, 3, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 3, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(8000, false, 3, "", { rootNodeId }));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	MultipartOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestTinyMeshes)
{
	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestTinyMeshes";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	for (size_t i = 0; i < 10000; i++)
	{
		sceneBuilder.addNode(createRandomMesh(4, false, 3, "", { rootNodeId }));
	}

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	MultipartOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	// Make sure no supermesh has more than 5000 mappings (submeshes). We won't
	// see the effects in the unit test but when we try to commit the node
	// it will fail.

	for (const auto supermeshNode : mockExporter->getSupermeshes())
	{
		auto mapping = supermeshNode.getMeshMapping();
		EXPECT_LE(mapping.size(), 5000);
	}

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestMeshGroupings)
{
	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestAllMerged";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	auto nVertices = 10;
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 2, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 2, "group1", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 2, "group2", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 3, "", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 3, "group1", { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 3, "group2", { rootNodeId }));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	MultipartOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 6); // Six groups should have been formed

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get())
	);
}

namespace {

	// These methods are for use exclusively by TestBranchGroupings.

	void createChildBranches(
		repo::manipulator::modelutility::RepoSceneBuilder& sceneBuilder,
		std::shared_ptr<repo::core::model::TransformationNode> parent,
		int numChildren,
		int depth,
		std::unordered_map<std::string, repo::lib::RepoUUID>& nodes)
	{
		if (depth <= 0) {
			return;
		}
		for (size_t i = 0; i < numChildren; ++i) {
			auto name = parent->getName() + "_" + std::to_string(i);
			auto child = sceneBuilder.addNode(
				repo::core::model::RepoBSONFactory::makeTransformationNode({}, name, {parent->getSharedID()})
			);
			nodes[name] = child->getSharedID();
			createChildBranches(sceneBuilder, child, numChildren, depth - 1, nodes);
		}
	}

	void createMetadataNode(
		repo::manipulator::modelutility::RepoSceneBuilder& sceneBuilder,
		const repo::lib::RepoUUID& parentId,
		const std::string& branchGroup)
	{
		std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
		metadata[REPO_METADATA_GROUPING_FLOOR] = repo::lib::RepoVariant(branchGroup);
		sceneBuilder.addNode(
			repo::core::model::RepoBSONFactory::makeMetaDataNode(metadata, {}, {parentId})
		);
	}

	void removeDuplicateIds(std::vector<repo::lib::RepoUUID>& ids) {
		std::unordered_set<repo::lib::RepoUUID, repo::lib::RepoUUIDHasher> seen;
		ids.erase(std::remove_if(ids.begin(), ids.end(), [&seen](const repo::lib::RepoUUID& id) {
			if (seen.find(id) != seen.end()) {
				return true;
			}
			else {
				seen.insert(id);
				return false;
			}
		}), ids.end());
	}
}

TEST(MultipartOptimizer, TestBranchGroupings)
{
	using namespace repo::core::model;
	using namespace repo::manipulator::modelutility;

	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestBranchGroupings";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = RepoSceneBuilder(handler, database, projectName, revId);
	auto rootNode = sceneBuilder.addNode(RepoBSONFactory::makeTransformationNode({}, "rootNode", {}));

	// Creates a number of children under rootNode

	auto nVertices = 10;

	// Create a tree of sprawing branches. We will add meshes and metadata nodes to these.
	// Each branch will have at least three children.

	std::unordered_map<std::string, repo::lib::RepoUUID> nodes;
	createChildBranches(sceneBuilder, rootNode, 4, 4, nodes);

	// This type sets up the *expected* groupings

	struct BranchGroup {
		RepoSceneBuilder& sceneBuilder;

		std::string name;
		std::vector<repo::lib::RepoUUID> nodeIds;

		std::unordered_map<repo::lib::RepoUUID, std::string, repo::lib::RepoUUIDHasher> nodeIdsToGrouping;

		void addNode(std::unique_ptr<repo::core::model::MeshNode> node) {
			nodeIds.push_back(node->getUniqueID());
			nodeIdsToGrouping[node->getUniqueID()] = node->getGrouping();
			if (node->getParentIDs().empty() || node->getParentIDs()[0].isDefaultValue()) {
				throw repo::lib::RepoException("Mesh nodes must have a valid parent - in this test one of the hardcoded parent keys probably has a typo.");
			}
			sceneBuilder.addNode(std::move(node));
		}
	};

	BranchGroup expectedGroupNull {sceneBuilder, ""};
	BranchGroup expectedGroup1  {sceneBuilder, "branchGroup1"};
	BranchGroup expectedGroup2  {sceneBuilder, "branchGroup2"};
	BranchGroup expectedGroup3  {sceneBuilder, "branchGroup3"};
	BranchGroup expectedGroup4  {sceneBuilder, "branchGroup4"};

	// Meshes may belong to the root node (and should have the null group).

	expectedGroupNull.addNode(createRandomMesh(nVertices, false, 3, "", {rootNode->getSharedID()}));
	expectedGroupNull.addNode(createRandomMesh(nVertices, false, 3, "", {rootNode->getSharedID()}));
	expectedGroupNull.addNode(createRandomMesh(nVertices, false, 3, "", {rootNode->getSharedID()}));

	// The grouping logic must be robust to split meshes too

	expectedGroupNull.addNode(createRandomMesh(REPO_MP_MAX_VERTEX_COUNT + nVertices, false, 3, "", {rootNode->getSharedID()}));

	rootNode = nullptr; // Release rootnode or it won't be comitted.

	// Meshes further down the tree, which have no grouping ancestor, should also
	// be in the null group.

	expectedGroupNull.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_0"]}));
	expectedGroupNull.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_0_1_1"]}));
	expectedGroupNull.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_0_1_2_1"]}));

	expectedGroupNull.addNode(createRandomMesh(nVertices, false, 3, "a", {nodes["rootNode_0"]}));
	expectedGroupNull.addNode(createRandomMesh(nVertices, false, 3, "a", {nodes["rootNode_0_3"]}));
	expectedGroupNull.addNode(createRandomMesh(nVertices, false, 3, "b", {nodes["rootNode_0_3_2_1"]}));

	createMetadataNode(sceneBuilder, nodes["rootNode_1"], "branchGroup1");

	// All mesh nodes under branch group 1 should be grouped together, regardless
	// of depth.

	expectedGroup1.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_1"]}));
	expectedGroup1.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_1_1_1"]}));
	expectedGroup1.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_1_1_2_1"]}));

	expectedGroup1.addNode(createRandomMesh(REPO_MP_MAX_VERTEX_COUNT + nVertices, false, 3, "", {nodes["rootNode_1_1_2_1"]}));
	expectedGroup1.addNode(createRandomMesh(REPO_MP_MAX_VERTEX_COUNT + nVertices, false, 3, "", {nodes["rootNode_1_1_2_2"]}));

	// Branch group 3 should take precedence over branch group 2, for nodes
	// further down.

	createMetadataNode(sceneBuilder, nodes["rootNode_2"], "branchGroup2");
	createMetadataNode(sceneBuilder, nodes["rootNode_2_1_2"], "branchGroup3");

	expectedGroup2.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_2_1"]}));
	expectedGroup3.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_2_1_2"]}));
	expectedGroup3.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_2_1_2_0"]}));

	// Branches are grouped by name, so even if their only common ancestor is far
	// removed, they should still be grouped together.

	createMetadataNode(sceneBuilder, nodes["rootNode_0_2"], "branchGroup4");
	createMetadataNode(sceneBuilder, nodes["rootNode_2_2_2"], "branchGroup4");

	expectedGroup4.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_0_2_0_0"]}));
	expectedGroup4.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_0_2_2_0"]}));
	expectedGroup4.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_2_2_2"]}));
	expectedGroup4.addNode(createRandomMesh(nVertices, false, 3, "", {nodes["rootNode_2_2_2_1"]}));

	// MeshNodes with a grouping should be in their own supermesh. They should
	// always be in the same supermesh regardless of where they are in the tree,
	// so long as they have the same branch group (which for these is null).

	expectedGroup4.addNode(createRandomMesh(nVertices, false, 3, "a", {nodes["rootNode_0_2_1"]}));
	expectedGroup4.addNode(createRandomMesh(nVertices, false, 3, "a", {nodes["rootNode_2_2_2_3"]}));
	expectedGroup4.addNode(createRandomMesh(nVertices, false, 3, "b", {nodes["rootNode_2_2_2_1"]}));

	expectedGroup2.addNode(createRandomMesh(nVertices, false, 3, "a", {nodes["rootNode_2_1_1_0"]}));
	expectedGroup2.addNode(createRandomMesh(nVertices, false, 3, "c", {nodes["rootNode_2_1_3"]}));
	expectedGroup2.addNode(createRandomMesh(nVertices, false, 3, "a", {nodes["rootNode_2_1_3_1"]}));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(handler.get(), database, projectName, revId, std::vector<double>({0, 0, 0}));

	MultipartOptimizer opt(handler.get(), mockExporter.get());
	opt.splitByFloor = true;
	opt.processScene(
		database,
		projectName,
		revId
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	std::unordered_map<std::string, std::vector<const repo::core::model::SupermeshNode*>> supermeshMap; // By branch group

	for (auto& sm : mockExporter->getSupermeshes()) {
		supermeshMap[sm.getGrouping()].push_back(&sm);
	}

	auto compareSupermesh = [&](const std::vector<const repo::core::model::SupermeshNode*>& supermeshNodes, const BranchGroup& group) {
		std::vector<repo::lib::RepoUUID> actualIds;
		for (const auto& supermeshNode : supermeshNodes) {

			std::optional<std::string> runningMeshNodeGrouping = std::nullopt;

			auto mapping = supermeshNode->getMeshMapping();
			for (const auto& pair : mapping) {
				actualIds.push_back(pair.mesh_id);

				// MeshNode resource groupings must not cross supermeshes. This snippet
				// checks that the grouping of every meshNode in this supermesh is the
				// same.

				const auto& meshNodeGrouping = group.nodeIdsToGrouping.at(pair.mesh_id);
				if (!runningMeshNodeGrouping) {
					runningMeshNodeGrouping = meshNodeGrouping;
				}
				else {
					EXPECT_THAT(runningMeshNodeGrouping.value(), testing::Eq(meshNodeGrouping));
				}
			}

			// A single branch grouping may have multiple supermeshes, because there is a
			// lot of geometry, or because there are multiple MeshNode grouping entries 
			// (even though these will not be public).

			EXPECT_THAT(supermeshNode->getGrouping(), testing::Eq(group.name));
		}

		// Ultimately, between all the supermeshes, under the branch group, we should
		// have exactly the meshNode's that should be in those groups, regardless of how
		// they are distributed.

		// Remove duplicates because split meshes will show up multiple times
		removeDuplicateIds(actualIds);

		EXPECT_THAT(actualIds, testing::UnorderedElementsAreArray(group.nodeIds));
	};

	compareSupermesh(supermeshMap[""], expectedGroupNull);
	compareSupermesh(supermeshMap["branchGroup1"], expectedGroup1);
	compareSupermesh(supermeshMap["branchGroup2"], expectedGroup2);
	compareSupermesh(supermeshMap["branchGroup3"], expectedGroup3);
	compareSupermesh(supermeshMap["branchGroup4"], expectedGroup4);

	// Due to the large meshes and groupings, we expect a specific number of
	// supermeshes

	EXPECT_THAT(supermeshMap[""].size(), testing::Eq(5)); // 6 combined meshnodes, 2 combined "a", 1 "b", 1 from large split, 1 from large split
	EXPECT_THAT(supermeshMap["branchGroup1"].size(), testing::Eq(5)); // 3 combined meshnodes, 1 from split, 1 from split, 1 from split, 1 from split
	EXPECT_THAT(supermeshMap["branchGroup2"].size(), testing::Eq(3)); // 1 meshnode, 2x "a" and 1 "c"
	EXPECT_THAT(supermeshMap["branchGroup3"].size(), testing::Eq(1)); // 2 combined meshnodes
	EXPECT_THAT(supermeshMap["branchGroup4"].size(), testing::Eq(3)); // 4 combined meshnodes, 2 "a" and 1 "b"
}