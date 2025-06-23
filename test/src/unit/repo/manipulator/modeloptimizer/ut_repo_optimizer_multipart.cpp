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
#include <repo/manipulator/modelutility/repo_scene_builder.h>
#include <test/src/unit//repo_test_database_info.h>

using namespace repo::manipulator::modeloptimizer;
using namespace repo::test::utils::mesh;

#define DBMULTIPARTOPTIMIZERTEST "multipartOptimiserTest"

// The functions below compare the geometry of the original MeshNodes with the
// stash MeshNodes as a triangle soup. The geometry should be identical.

TEST(MultipartOptimizer, TestAllMerged)
{
	auto opt = MultipartOptimizer();

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
		auto randNode = createRandomMesh(10, false, 3, { rootNodeId });
		sceneBuilder.addNode(std::move(randNode));			
	}

	sceneBuilder.finalise();
	
	auto mockExporter = std::make_unique<TestModelExport>(database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

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
	
	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestWithUV)
{
	auto opt = MultipartOptimizer();

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
		auto randNode = createRandomMesh(10, i == 1, 3, { rootNodeId });
		if (i == 1) {
			randNode->setTextureId(texNodeId);
		}
		sceneBuilder.addNode(std::move(randNode));
	}

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	bool result = opt.processScene(
		database,
		projectName,
		revId,
		handler.get(),
		mockExporter.get()
	);

	EXPECT_TRUE(result);

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
	auto opt = MultipartOptimizer();

	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestMixedPrimitives";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	auto nVertices = 10;
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 3, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 3, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 3, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(nVertices, false, 1, { rootNodeId })); // unsupported primitive types must be identified as such and not combined with known types

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	bool result = opt.processScene(
		database,
		projectName,
		revId,
		handler.get(),
		mockExporter.get()
	);

	EXPECT_TRUE(result);

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
	auto opt = MultipartOptimizer();

	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestSingleLargeMesh";
	auto revId = repo::lib::RepoUUID::createUUID();


	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();
	
	auto largeMesh = createRandomMesh(65536, false, 2, { rootNodeId });
	sceneBuilder.addNode(std::move(largeMesh));
	
	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

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

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestSingleOversizedMesh)
{
	auto opt = MultipartOptimizer();

	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestSingleOversizedMesh";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

		
	sceneBuilder.addNode(createRandomMesh(1200000 + 1, false, 3, { rootNodeId })); // 1200000 comes from the const in repo_optimizer_multipart.cpp

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	bool result = opt.processScene(
		database,
		projectName,
		revId,
		handler.get(),
		mockExporter.get()
	);

	EXPECT_TRUE(result);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_EQ(mockExporter->getSupermeshCount(), 2); // even with one triangle over, large meshes should be split

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestMultipleOversizedMeshes)
{
	auto opt = MultipartOptimizer();

	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestMultipleOversizedMeshes";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	sceneBuilder.addNode(createRandomMesh(65536, false, 3, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(65537, false, 3, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(128537, false, 3, { rootNodeId }));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	bool result = opt.processScene(
		database,
		projectName,
		revId,
		handler.get(),
		mockExporter.get()
	);

	EXPECT_TRUE(result);

	EXPECT_TRUE(mockExporter->isFinalised());

	// Mesh splitting is not determinsitic so we don't check the final mesh
	// count in this test

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestMultiplesMeshes)
{
	auto opt = MultipartOptimizer();

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
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, { rootNodeId }));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

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

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));

	EXPECT_TRUE(false);
}

TEST(MultipartOptimizer, TestMultipleSmallAndLargeMeshes)
{
	auto opt = MultipartOptimizer();

	auto handler = getHandler();
	std::string database = DBMULTIPARTOPTIMIZERTEST;
	std::string projectName = "TestMultipleSmallAndLargeMeshes";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	sceneBuilder.addNode(createRandomMesh(16384, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(65536, false, 2, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(128000, false, 3, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(16384, false, 3, { rootNodeId }));
	sceneBuilder.addNode(createRandomMesh(8000, false, 3, { rootNodeId }));

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	bool result = opt.processScene(
		database,
		projectName,
		revId,
		handler.get(),
		mockExporter.get()
	);

	EXPECT_TRUE(result);

	EXPECT_TRUE(mockExporter->isFinalised());

	EXPECT_TRUE(compareMeshes(
		database,
		projectName,
		revId,
		mockExporter.get()));
}

TEST(MultipartOptimizer, TestTinyMeshes)
{
	auto opt = MultipartOptimizer();

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
		sceneBuilder.addNode(createRandomMesh(4, false, 3, { rootNodeId }));
	}

	sceneBuilder.finalise();

	auto mockExporter = std::make_unique<TestModelExport>(database, projectName, revId, std::vector<double>({ 0, 0, 0 }));

	bool result = opt.processScene(
		database,
		projectName,
		revId,
		handler.get(),
		mockExporter.get()
	);

	EXPECT_TRUE(result);

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