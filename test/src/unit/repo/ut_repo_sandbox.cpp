/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include <repo/core/handler/repo_database_handler_mongo.h>
#include <repo/core/handler/database/repo_query.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_element.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/core/model/bson/repo_bson_builder.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include "test/src/unit/repo_test_matchers.h"
#include "test/src/unit/repo_test_utils.h"
#include "test/src/unit/repo_test_database_info.h"

#include <mongocxx/exception/exception.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/pipeline.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/builder/basic/document.hpp>

#include <unordered_set>
#include <iostream>
#include <fstream>
#include <chrono>

using namespace repo::core::handler;
using namespace testing;
using namespace bsoncxx::builder::basic;

std::shared_ptr<repo::core::handler::MongoDatabaseHandler> getLocalHandler()
{
	auto handler = repo::core::handler::MongoDatabaseHandler::getHandler(
		REPO_GTEST_DBADDRESS,
		REPO_GTEST_DBPORT,
		"adminUser",
		"password"
	);

	auto config = repo::lib::RepoConfig::fromFile(getDataPath("config/withFS.json"));
	config.configureFS("D:\\Git\\local\\efs\\models"); //getDataPath("fileShare"));
	handler->setFileManager(std::make_shared<repo::core::handler::fileservice::FileManager>(config, handler));
	return handler;
}

struct LinkedSummary {
	repo::lib::RepoUUID rootNodeId;
	repo::lib::RepoUUID leafNodeId;
	repo::lib::RepoMatrix fullTransform;
};

repo::lib::RepoMatrix generateRandomMatrix() {
	std::vector<float> randValues;
	for (int i = 0; i < 16; ++i)
	{
		randValues.push_back((rand() % 1000) / 1000.f);
	}
	return repo::lib::RepoMatrix(randValues);
}

repo::core::model::RepoBSON createRootEntry()
{
	auto id = repo::lib::RepoUUID::createUUID();
	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", id);
	builder.append("transform", generateRandomMatrix());
	return builder.obj();
}

repo::core::model::RepoBSON createChildEntry(repo::lib::RepoUUID parentId)
{
	auto id = repo::lib::RepoUUID::createUUID();
	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", id);
	builder.append("parent", parentId);
	builder.append("transform", generateRandomMatrix());
	return builder.obj();
}

LinkedSummary fillDBWithLinkedEntries(std::shared_ptr<MongoDatabaseHandler> handler, std::string database, std::string collection, int noEntries)
{
	repo::lib::RepoMatrix fullTransform;

	auto rootEntry = createRootEntry();
	repo::lib::RepoUUID rootId = rootEntry.getField("_id").UUID();
	repo::lib::RepoMatrix rootTransform = rootEntry.getMatrixField("transform");
	handler->insertDocument(database, collection, rootEntry);

	fullTransform = rootTransform;

	repo::lib::RepoUUID parentId = rootId;
	std::vector<repo::core::model::RepoBSON> documents;
	for (int i = 0; i < noEntries; i++)
	{
		auto childEntry = createChildEntry(parentId);
		parentId = childEntry.getField("_id").UUID();
		documents.push_back(childEntry);

		repo::lib::RepoMatrix childTransform = childEntry.getMatrixField("transform");
		fullTransform = fullTransform * childTransform;
	}

	handler->insertManyDocuments(database, collection, documents);

	LinkedSummary summary;
	summary.rootNodeId = rootId;
	summary.leafNodeId = parentId;
	summary.fullTransform = fullTransform;
	return summary;
}

struct TreeInfo {
	std::vector<repo::core::model::RepoBSON> documents;
	std::vector<repo::lib::RepoUUID> leafNodeIds;
	std::vector<repo::lib::RepoMatrix> leafMatrices;
};

TreeInfo createTreeEntries(
	repo::lib::RepoUUID parentId,
	repo::lib::RepoMatrix parentMat,
	TreeInfo currentTree,
	int noLevelsRemaining,
	int noChildren) {

	if (noLevelsRemaining > 1) {
		int levelsRemaining = noLevelsRemaining - 1;

		std::vector<repo::core::model::RepoBSON> documents;
		std::vector<repo::lib::RepoUUID> leafNodeIds;
		std::vector<repo::lib::RepoMatrix> leafMatrices;

		for (int i = 0; i < noChildren; i++) {
			auto child = createChildEntry(parentId);

			currentTree.documents.push_back(child);

			auto childId = child.getField("_id").UUID();
			auto childMat = child.getMatrixField("transform");
			auto nextMat = parentMat * childMat;
			createTreeEntries(childId, nextMat, currentTree, levelsRemaining, noChildren);
		}
	}
	else {
		// We are in the leaf, store leaf id and matrix
		currentTree.leafNodeIds.push_back(parentId);
		currentTree.leafMatrices.push_back(parentMat);
	}

	return currentTree;
}

TreeInfo buildTree(
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	int noLevels,
	int noChildren)
{
	TreeInfo tree;

	auto rootEntry = createRootEntry();
	repo::lib::RepoUUID rootId = rootEntry.getField("_id").UUID();
	repo::lib::RepoMatrix rootTransform = rootEntry.getMatrixField("transform");
	handler->insertDocument(database, collection, rootEntry);

	tree = createTreeEntries(rootId, rootTransform, tree, noLevels, noChildren);

	handler->insertManyDocuments(database, collection, tree.documents);

	return tree;
}

repo::lib::RepoMatrix calculateMatrixRecursively(
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	repo::lib::RepoUUID nodeId,
	repo::lib::RepoMatrix matrix)
{
	auto document = handler->findOneByUniqueID(database, collection, nodeId.toString());

	repo::lib::RepoMatrix currMat = document.getMatrixField("transform");
	matrix = matrix * currMat;

	bool hasParent = document.hasField("parent");
	if (hasParent)
	{
		repo::lib::RepoUUID parent = document.getField("_id").UUID();
		return calculateMatrixRecursively(handler, database, collection, parent, matrix);
	}
	else
	{
		return matrix;
	}
}

TEST(Sandbox, TestLinkedCreation)
{
	auto handler = getHandler();

	std::string database = "sandbox";
	std::string collection = "bakingTestCollection";

	// Clean collection first
	handler->dropCollection(database, collection);

	// Create DB with linked Entries
	LinkedSummary linkedEntries = fillDBWithLinkedEntries(handler, database, collection, 100);

	// Check number of entries
	auto documents = handler->getAllFromCollectionTailable(database, collection);
	EXPECT_THAT(documents.size(), Eq(101));
}

TEST(Sandbox, TestTreeCreation)
{
	auto handler = getHandler();

	std::string database = "sandbox";
	std::string collection = "bakingTestCollection";

	// Clean collection first
	handler->dropCollection(database, collection);

	// Create DB with tree
	TreeInfo tree = buildTree(handler, database, collection, 5, 3);

	// Check number of entries
	auto documents = handler->getAllFromCollectionTailable(database, collection);
	int expNodeCount = pow(3, 5);
	EXPECT_THAT(documents.size(), Eq(expNodeCount));

	// Check number of leaves
	EXPECT_THAT(tree.leafNodeIds.size(), Eq(3 * 5));

	// Check for match between number of leaves and matrices
	EXPECT_THAT(tree.leafMatrices.size(), Eq(tree.leafNodeIds.size()));
}

// Approach 1: Recursive
TEST(Sandbox, LinkedRecursive) {
	auto handler = getHandler();

	std::string database = "sandbox";
	std::string collection = "bakingTestCollection";

	// Clean collection first
	handler->dropCollection(database, collection);

	// Create DB with linked Entries
	LinkedSummary linkedEntries = fillDBWithLinkedEntries(handler, database, collection, 100);

	repo::lib::RepoMatrix recursiveResult = calculateMatrixRecursively(handler, database, collection, linkedEntries.leafNodeId, repo::lib::RepoMatrix());
	EXPECT_TRUE(recursiveResult.equals(linkedEntries.fullTransform));
}

// Approach 2: Aggregate Pipeline from leaf
//	db.collection.aggregate([
//		{
//			$graphLookup: {
//				from: "collection",
//				startWith: tree.leafNodeId,
//				connectFromField: "parent",
//				connectToField: "_id",
//				as: parents
//			}
//		},
//		{
//			$project: {
//				"transforms": "$parents.transform"
//			}
// ])
//TEST(Sandbox, LinkedAggLeaves) {
//	auto handler = getHandler();
//
//	std::string database = "sandbox";
//	std::string collection = "bakingTestCollection";
//
//	// Clean collection first
//	handler->dropCollection(database, collection);
//
//	// Create DB with linked Entries
//	LinkedSummary linkedEntries = fillDBWithLinkedEntries(handler, database, collection, 100);
//
//	auto cursor = handler->getTransformThroughAggregation(database, collection, linkedEntries.leafNodeId);
//	std::vector<repo::lib::RepoMatrix> matrices;
//	for (auto document : (*cursor)) {
//		// Get array "transforms"
//		auto transforms = document.getMatrixFieldArray("transforms");
//
//		// multiply matrices
//		repo::lib::RepoMatrix matrix;
//
//		for (auto transform : transforms)
//		{
//			matrix = matrix * transform;
//		}
//
//		// push final matrix into the vector
//		matrices.push_back(matrix);
//	}
//
//	EXPECT_THAT(matrices.size(), 1);
//	EXPECT_TRUE(matrices[0].equals(linkedEntries.fullTransform));
//}

// Approach 3: Aggregate Pipeline from root
//	db.collection.aggregate([
//		TODO
// ])
TEST(Sandbox, LinkedAggRoot) {
	auto handler = getHandler();

	std::string database = "sandbox";
	std::string collection = "bakingTestCollection";

	// Clean collection first
	handler->dropCollection(database, collection);

	// Create DB with linked Entries
	LinkedSummary linkedEntries = fillDBWithLinkedEntries(handler, database, collection, 100);
}

TEST(Sandbox, SandboxTest01)
{
	auto handler = getHandler();

	std::string database = "sandbox";
	std::string collection = "bakingTestCollection";

	// Clean collection first
	handler->dropCollection(database, collection);

	// Create DB with linked Entries
	LinkedSummary tree = fillDBWithLinkedEntries(handler, database, collection, 100);

	// Check number of entries
	auto documents = handler->getAllFromCollectionTailable(database, collection);
	EXPECT_THAT(documents.size(), Eq(101));

}


mongocxx::pipeline createMeshNodeCountPipeline(std::string collection, repo::lib::RepoUUID revId) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};

	mongocxx::pipeline countPipeline;
	countPipeline.match(
		make_document(
			kvp("rev_id", revIdbinary),
			kvp("type", "mesh")
		)
	).project(
		make_document(
			kvp("_id", 1)
		)
	);

	return countPipeline;
}

mongocxx::pipeline createTranspPipeline(std::string collection, repo::lib::RepoUUID revId, bool uvParam, int primitiveParam) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};
	
	mongocxx::pipeline transpPipeline;
	transpPipeline.match(
		make_document(
			kvp("rev_id", revIdbinary),
			kvp("type", "material"),
			kvp("opacity", make_document(
				kvp("$ne", 1)
			))
		)
	).project(
		make_document(
			kvp("shared_id", 1),
			kvp("parents", 1)
		)
	).graph_lookup(
		make_document(
			kvp("from", collection),
			kvp("startWith", "$shared_id"),
			kvp("connectFromField", "shared_id"),
			kvp("connectToField", "parents"),
			kvp("as", "textureChildNodes"),
			kvp("restrictSearchWithMatch", make_document(
				kvp("type", "texture")
			))
		)
	).match(
		make_document(
			kvp("textureChildNodes", make_document(
				kvp("$size", 0)
			))
		)
	).project(
		make_document(
			kvp("parents", 1)
		)
	).unwind(
		"$parents"
	).lookup(
		make_document(
			kvp("from", collection),
			kvp("localField", "parents"),
			kvp("foreignField", "shared_id"),
			kvp("as", "mesh")
		)
	).match(
		make_document(
			kvp("mesh.0.primitive", primitiveParam),
			kvp("mesh.0.uv_channels_count", make_document(
				kvp("$exists", uvParam)
			))
		)
	).project(
		make_document(
			kvp("node", "$parents")
		)
	);

	return transpPipeline;
}

mongocxx::pipeline createNormalPipeline(std::string collection, repo::lib::RepoUUID revId, bool uvParam, int primitiveParam) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};

	mongocxx::pipeline normalPipeline;
	normalPipeline.match(
		make_document(
			kvp("rev_id", revIdbinary),
			kvp("type", "material"),
			kvp("opacity", 1)
		)
	).project(
		make_document(
			kvp("shared_id", 1),
			kvp("parents", 1)
		)
	).graph_lookup(
		make_document(
			kvp("from", collection),
			kvp("startWith", "$shared_id"),
			kvp("connectFromField", "shared_id"),
			kvp("connectToField", "parents"),
			kvp("as", "textureChildNodes"),
			kvp("restrictSearchWithMatch", make_document(
				kvp("type", "texture")
			))
		)
	).match(
		make_document(
			kvp("textureChildNodes", make_document(
				kvp("$size", 0)
			))
		)
	).project(
		make_document(
			kvp("parents", 1)
		)
	).unwind(
		"$parents"
	).lookup(
		make_document(
			kvp("from", collection),
			kvp("localField", "parents"),
			kvp("foreignField", "shared_id"),
			kvp("as", "mesh")
		)
	).match(
		make_document(
			kvp("mesh.0.primitive", primitiveParam),
			kvp("mesh.0.uv_channels_count", make_document(
				kvp("$exists", uvParam)
			))
		)
	).project(
		make_document(
			kvp("node", "$parents")
		)
	);

	return normalPipeline;
}

mongocxx::pipeline createTextureIdPipeline(std::string collection, repo::lib::RepoUUID revId) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};
	
	mongocxx::pipeline textureIdPipeline;
	textureIdPipeline.match(
		make_document(
			kvp("rev_id", revIdbinary),
			kvp("type", "texture")
		)
	).project(
		make_document(
			kvp("shared_id", 1)
		)
	);

	return textureIdPipeline;
}

mongocxx::pipeline createTextureNodesPipeline(std::string collection, repo::lib::RepoUUID revId, bool uvParam, int primitiveParam, repo::lib::RepoUUID texId) {

	auto texUuidData = texId.data();
	bsoncxx::types::b_binary texIdBinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		texUuidData.size(),
		texUuidData.data()
	};

	auto revUuidData = revId.data();
	bsoncxx::types::b_binary revIdBinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		revUuidData.size(),
		revUuidData.data()
	};


	mongocxx::pipeline textureNodePipeline;
	textureNodePipeline.match(
		make_document(
			kvp("rev_id", revIdBinary),
			kvp("shared_id", texIdBinary)
		)
	).project(
		make_document(
			kvp("parents", 1)
		)
	).unwind(
		"$parents"
	).lookup(
		make_document(
			kvp("from", collection),
			kvp("localField", "parents"),
			kvp("foreignField", "shared_id"),
			kvp("as", "material")
		)
	).project(
		make_document(
			kvp("material", 1)
		)
	).unwind(
		"$material"
	).project(
		make_document(
			kvp("materialParents", "$material.parents")
		)
	).unwind(
		"$materialParents"
	).lookup(
		make_document(
			kvp("from", collection),
			kvp("localField", "materialParents"),
			kvp("foreignField", "shared_id"),
			kvp("as", "mesh")
		)
	).match(
		make_document(
			kvp("mesh.0.primitive", primitiveParam),
			kvp("mesh.0.uv_channels_count", make_document(
				kvp("$exists", uvParam)
			))
		)
	).project(make_document(
		kvp("node", "$materialParents")
	));

	return textureNodePipeline;
}
TEST(Sandbox, SandboxCoverageTest) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// House model (small)
	//std::string collection = "5144ac65-6b9d-4236-b0a6-9186149a32c8.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("949EE0E2-13FB-4C07-9B49-9556F5E8F293");

	// "Medium" Model
	//std::string collection = "69d561b4-77d7-44a0-b295-d0db459d5270.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("3792C7CB-2854-4FF0-B957-278C154C0CB7");
	//int noMeshNodes = 245669;

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");
	 int noMeshNodes = 654211;

	// Get number of total entries
	//auto allDocs = handler->getAllFromCollectionTailable(database, collection);
	//EXPECT_THAT(allDocs.size(), Eq(2332));

	//Get number of mesh nodes
	//auto cursorMeshNodeCursor = handler->runAggregatePipeline(database, collection, createMeshNodeCountPipeline(collection, revId));
	//int noMeshNodes = 0;
	//for (auto document : (*cursorMeshNodeCursor)) {
	//	noMeshNodes++;
	//}
	//EXPECT_THAT(noMeshNodes, 1349);



	// Get number of transparent Nodes (no uv, primitive 2)
	auto cursorTranspNoUvPrim2 = handler->runAggregatePipeline(database, collection, createTranspPipeline(collection, revId, false, 2));
	int countTranspNoUvPrim2 = 0;
	for (auto document : (*cursorTranspNoUvPrim2)) {
		countTranspNoUvPrim2++;
	}
	std::cout << "MESH NODES, TRANSPARENT, NO UV, Prim 2: " << countTranspNoUvPrim2 << std::endl;

	// Get number of transparent Nodes (uv, primitive 2)
	auto cursorTranspUvPrim2 = handler->runAggregatePipeline(database, collection, createTranspPipeline(collection, revId, true, 2));
	int countTranspUvPrim2 = 0;
	for (auto document : (*cursorTranspUvPrim2)) {
		countTranspUvPrim2++;
	}
	std::cout << "MESH NODES, TRANSPARENT, UV, Prim 2: " << countTranspUvPrim2 << std::endl;

	// Get number of transparent Nodes (no uv, primitive 3)
	auto cursorTranspNoUvPrim3 = handler->runAggregatePipeline(database, collection, createTranspPipeline(collection, revId, false, 3));
	int countTranspNoUvPrim3 = 0;
	for (auto document : (*cursorTranspNoUvPrim3)) {
		countTranspNoUvPrim3++;
	}

	std::cout << "MESH NODES, TRANSPARENT, NO UV, Prim 3: " << countTranspNoUvPrim3 << std::endl;

	// Get number of transparent Nodes (uv, primitive 3)
	auto cursorTranspUvPrim3 = handler->runAggregatePipeline(database, collection, createTranspPipeline(collection, revId, true, 3));
	int countTranspUvPrim3 = 0;
	for (auto document : (*cursorTranspUvPrim3)) {
		countTranspUvPrim3++;
	}

	std::cout << "MESH NODES, TRANSPARENT, UV, Prim 3: " << countTranspUvPrim3 << std::endl;

	int countTranspTotal = countTranspNoUvPrim2 + countTranspNoUvPrim3 + countTranspUvPrim2 + countTranspUvPrim3;
	std::cout << "MESH NODES, TRANSPARENT, TOTAL: " << countTranspUvPrim2 << std::endl;




	// Get number of normal Nodes (no uv, primitive 2)
	auto cursorNormNoUvPrim2 = handler->runAggregatePipeline(database, collection, createNormalPipeline(collection, revId, false, 2));
	int countNormNoUvPrim2 = 0;
	for (auto document : (*cursorNormNoUvPrim2)) {
		countNormNoUvPrim2++;
	}
	std::cout << "MESH NODES, NORMAL, NO UV, Prim 2: " << countNormNoUvPrim2 << std::endl;

	// Get number of normal Nodes (uv, primitive 2)
	auto cursorNormUvPrim2 = handler->runAggregatePipeline(database, collection, createNormalPipeline(collection, revId, true, 2));
	int countNormUvPrim2 = 0;
	for (auto document : (*cursorNormUvPrim2)) {
		countNormUvPrim2++;
	}
	std::cout << "MESH NODES, NORMAL, UV, Prim 2: " << countNormUvPrim2 << std::endl;

	// Get number of normal Nodes (no uv, primitive 3)
	auto cursorNormNoUvPrim3 = handler->runAggregatePipeline(database, collection, createNormalPipeline(collection, revId, false, 3));
	int countNormNoUvPrim3 = 0;
	for (auto document : (*cursorNormNoUvPrim3)) {
		countNormNoUvPrim3++;
	}

	std::cout << "MESH NODES, NORMAL, NO UV, Prim 3: " << countNormNoUvPrim3 << std::endl;

	// Get number of normal Nodes (uv, primitive 3)
	auto cursorNormUvPrim3 = handler->runAggregatePipeline(database, collection, createNormalPipeline(collection, revId, true, 3));
	int countNormUvPrim3 = 0;
	for (auto document : (*cursorNormUvPrim3)) {
		countNormUvPrim3++;
	}

	std::cout << "MESH NODES, NORMAL, UV, Prim 3: " << countNormUvPrim3 << std::endl;

	int countNormTotal = countNormNoUvPrim2 + countNormNoUvPrim3 + countNormUvPrim2 + countNormUvPrim3;
	std::cout << "MESH NODES, NORMAL, TOTAL: " << countNormTotal << std::endl;



	// Get Textured nodes
	mongocxx::pipeline texIdPipeline = createTextureIdPipeline(collection, revId);
	auto cursorTexId = handler->runAggregatePipeline(database, collection, texIdPipeline);

	int countTextNodes = 0;	
	std::unordered_set<std::string> texIdsCovered;

	for (auto document : (*cursorTexId)) {

		auto texId = document.getUUIDField("shared_id");

		std::string texIdString = texId.toString();
		if (texIdsCovered.find(texIdString) == texIdsCovered.end()) {

			texIdsCovered.insert(texIdString);

			// Get number of textured Nodes (no uv, primitive 2)
			auto cursorTextNoUvPrim2 = handler->runAggregatePipeline(database, collection, createTextureNodesPipeline(collection, revId, false, 2, texId));
			int countTextNoUvPrim2 = 0;
			for (auto document : (*cursorTextNoUvPrim2)) {
				countTextNoUvPrim2++;
			}
			std::cout << "MESH NODES, TEXTURED, TEXID: " << texId.toString() << ", NO UV, Prim 2: " << countTextNoUvPrim2 << std::endl;

			// Get number of textured Nodes (uv, primitive 2)
			auto cursorTextUvPrim2 = handler->runAggregatePipeline(database, collection, createTextureNodesPipeline(collection, revId, true, 2, texId));
			int countTextUvPrim2 = 0;
			for (auto document : (*cursorTextUvPrim2)) {
				countTextUvPrim2++;
			}
			std::cout << "MESH NODES, TEXTURED, TEXID: " << texId.toString() << ", UV, Prim 2: " << countTextUvPrim2 << std::endl;

			// Get number of textured Nodes (no uv, primitive 3)
			auto cursorTextNoUvPrim3 = handler->runAggregatePipeline(database, collection, createTextureNodesPipeline(collection, revId, false, 3, texId));
			int countTextNoUvPrim3 = 0;
			for (auto document : (*cursorTextNoUvPrim3)) {
				countTextNoUvPrim3++;
			}

			std::cout << "MESH NODES, TEXTURED, TEXID: " << texId.toString() << ", NO UV, Prim 3: " << countTextNoUvPrim3 << std::endl;

			// Get number of textured Nodes (uv, primitive 3)
			auto cursorTextUvPrim3 = handler->runAggregatePipeline(database, collection, createTextureNodesPipeline(collection, revId, true, 3, texId));
			int countTextUvPrim3 = 0;
			for (auto document : (*cursorTextUvPrim3)) {
				countTextUvPrim3++;
			}

			std::cout << "MESH NODES, TEXTURED, TEXID: " << texId.toString() << ", UV, Prim 3: " << countTextUvPrim3 << std::endl;

			int countTextTotal = countTextNoUvPrim2 + countTextNoUvPrim3 + countTextUvPrim2 + countTextUvPrim3;
			std::cout << "MESH NODES, TEXTURED, TEXID: " << texId.toString() << ", TOTAL: " << countTextUvPrim2 << std::endl;

			countTextNodes += countTextTotal;
		}
	}

	std::cout << "UNIQUE TEX IDS: " << texIdsCovered.size() << std::endl;

	std::cout << "MESH NODES, TEXTURED, TOTAL: " << countTextNodes << std::endl;

	int foundMeshNodesTotal = countTranspTotal + countNormTotal + countTextNodes;
	std::cout << "MESH NODES, ALL, TOTAL: " << foundMeshNodesTotal << std::endl;

	EXPECT_THAT(foundMeshNodesTotal, Eq(noMeshNodes));

}

TEST(Sandbox, SandboxMultiRequestTest) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";

	repo::lib::RepoUUID uuid1 = repo::lib::RepoUUID("1CF40B3C-0960-426E-ACA7-83510F82F535");
	repo::lib::RepoUUID uuid2 = repo::lib::RepoUUID("D6DAF4E0-6976-4F86-9A08-5EF9937D8CF4");


	std::vector<repo::lib::RepoUUID> ids;
	ids.push_back(uuid1);
	ids.push_back(uuid2);

	repo::core::model::RepoBSONBuilder innerBson;
	innerBson.appendArray("$in", ids);
	repo::core::model::RepoBSONBuilder outerBson;
	outerBson.append("shared_id", innerBson.obj());
	auto filter = outerBson.obj();

	repo::core::model::RepoBSONBuilder projBson;
	projBson.append("shared_id", 1);
	auto projection = projBson.obj();

	auto cursor = handler->specialFind(database, collection, filter, projection);
	
	std::ofstream file;
	file.open("sharedIds.txt");
	
	
	for (auto& doc : cursor) {

		repo::core::model::RepoBSON bson = repo::core::model::RepoBSON(doc);
		auto sharedId = bson.getUUIDField("shared_id");	
		
		file << sharedId.toString() << std::endl;		
	}

	file.close();	
}

TEST(Sandbox, SandboxDumpSharedIds) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";



	repo::core::model::RepoBSONBuilder bson;	
	bson.append("type", "mesh");
	auto filter = bson.obj();

	repo::core::model::RepoBSONBuilder projBson;
	projBson.append("shared_id", 1);
	auto projection = projBson.obj();

	auto cursor = handler->specialFind(database, collection, filter, projection);

	std::ofstream file;
	file.open("sharedIds.txt");


	for (auto& doc : cursor) {

		repo::core::model::RepoBSON bson = repo::core::model::RepoBSON(doc);
		auto sharedId = bson.getUUIDField("shared_id");

		file << sharedId.toString() << std::endl;
	}

	file.close();
}

TEST(Sandbox, TimerTest) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";

	std::ofstream logFile;
	logFile.open("TimerTestLog.txt");

	std::ifstream file;
	file.open("sharedIds.txt");

	std::vector<repo::lib::RepoUUID> ids;
	std::string line;

	auto point1 = std::chrono::high_resolution_clock::now();
	while (std::getline(file, line)) {
		repo::lib::RepoUUID id = repo::lib::RepoUUID(line);
		ids.push_back(id);
	}
	auto point2 = std::chrono::high_resolution_clock::now();

	file.close();

	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(point2 - point1);
	logFile << "Reading file:" << duration.count() << " microseconds" << std::endl;
	logFile.close();

	EXPECT_THAT(ids.size(), Eq(654211));
}

TEST(Sandbox, FindMultiRequestPerfMeasurement) {
	
	auto handler = getLocalHandler();
	std::string database = "fthiel";
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene"; // XXL Model

	// Set up log file
	std::ofstream logFile;
	logFile.open("FindMultiRequestPerfLog.csv");
	logFile << "elements,find,iteration" << std::endl;

	// Read in all ids
	std::ifstream file;
	file.open("sharedIds.txt");
	std::vector<repo::lib::RepoUUID> ids;

	std::string line;
	while (std::getline(file, line)) {
		repo::lib::RepoUUID id = repo::lib::RepoUUID(line);
		ids.push_back(id);
	}
	file.close();

	// setup projection
	repo::core::model::RepoBSONBuilder projBson;	
	auto projection = projBson.obj();

	// Magnitude 1: 1 Document
	{
		int elements = 1;

		std::vector<repo::lib::RepoUUID> subVec;
		for (int i = 0; i < elements; i++) {
			subVec.push_back(ids[i]);
		}

		repo::core::model::RepoBSONBuilder innerBson;
		innerBson.appendArray("$in", subVec);
		repo::core::model::RepoBSONBuilder outerBson;
		outerBson.append("shared_id", innerBson.obj());
		auto filter = outerBson.obj();

		long long findDuration;
		auto cursor = handler->specialFindPerf(database, collection, filter, projection, findDuration);		

		auto preIterationMeasurement = std::chrono::high_resolution_clock::now();
		int count = 0;
		for (auto& doc : cursor) {
			count++;
		}
		auto postIterationMeasurement = std::chrono::high_resolution_clock::now();

		
		auto itDuration = std::chrono::duration_cast<std::chrono::microseconds>(postIterationMeasurement - preIterationMeasurement);
		logFile << elements << "," << findDuration << "," << itDuration.count() << std::endl;

		EXPECT_THAT(count, elements);
	}

	// Magnitude 2: 10 Documents
	{
		int elements = 10;

		std::vector<repo::lib::RepoUUID> subVec;
		for (int i = 0; i < elements; i++) {
			subVec.push_back(ids[i]);
		}

		repo::core::model::RepoBSONBuilder innerBson;
		innerBson.appendArray("$in", subVec);
		repo::core::model::RepoBSONBuilder outerBson;
		outerBson.append("shared_id", innerBson.obj());
		auto filter = outerBson.obj();

		long long findDuration;
		auto cursor = handler->specialFindPerf(database, collection, filter, projection, findDuration);

		auto preIterationMeasurement = std::chrono::high_resolution_clock::now();
		int count = 0;
		for (auto& doc : cursor) {
			count++;
		}
		auto postIterationMeasurement = std::chrono::high_resolution_clock::now();

		auto itDuration = std::chrono::duration_cast<std::chrono::microseconds>(postIterationMeasurement - preIterationMeasurement);
		logFile << elements << "," << findDuration << "," << itDuration.count() << std::endl;
	}

	// Magnitude 3: 100 Documents
	{
		int elements = 100;

		std::vector<repo::lib::RepoUUID> subVec;
		for (int i = 0; i < elements; i++) {
			subVec.push_back(ids[i]);
		}

		repo::core::model::RepoBSONBuilder innerBson;
		innerBson.appendArray("$in", subVec);
		repo::core::model::RepoBSONBuilder outerBson;
		outerBson.append("shared_id", innerBson.obj());
		auto filter = outerBson.obj();

		long long findDuration;
		auto cursor = handler->specialFindPerf(database, collection, filter, projection, findDuration);

		auto preIterationMeasurement = std::chrono::high_resolution_clock::now();
		int count = 0;
		for (auto& doc : cursor) {
			count++;
		}
		auto postIterationMeasurement = std::chrono::high_resolution_clock::now();

		auto itDuration = std::chrono::duration_cast<std::chrono::microseconds>(postIterationMeasurement - preIterationMeasurement);
		logFile << elements << "," << findDuration << "," << itDuration.count() << std::endl;

		EXPECT_THAT(count, elements);
	}

	// Magnitude 4: 1000 Documents
	{
		int elements = 1000;

		std::vector<repo::lib::RepoUUID> subVec;
		for (int i = 0; i < elements; i++) {
			subVec.push_back(ids[i]);
		}

		repo::core::model::RepoBSONBuilder innerBson;
		innerBson.appendArray("$in", subVec);
		repo::core::model::RepoBSONBuilder outerBson;
		outerBson.append("shared_id", innerBson.obj());
		auto filter = outerBson.obj();

		long long findDuration;
		auto cursor = handler->specialFindPerf(database, collection, filter, projection, findDuration);

		auto preIterationMeasurement = std::chrono::high_resolution_clock::now();
		int count = 0;
		for (auto& doc : cursor) {
			count++;
		}
		auto postIterationMeasurement = std::chrono::high_resolution_clock::now();

		auto itDuration = std::chrono::duration_cast<std::chrono::microseconds>(postIterationMeasurement - preIterationMeasurement);
		logFile << elements << "," << findDuration << "," << itDuration.count() << std::endl;

		EXPECT_THAT(count, elements);
	}

	// Magnitude 5: 10000 Documents
	{
		int elements = 10000;

		std::vector<repo::lib::RepoUUID> subVec;
		for (int i = 0; i < elements; i++) {
			subVec.push_back(ids[i]);
		}

		repo::core::model::RepoBSONBuilder innerBson;
		innerBson.appendArray("$in", subVec);
		repo::core::model::RepoBSONBuilder outerBson;
		outerBson.append("shared_id", innerBson.obj());
		auto filter = outerBson.obj();

		long long findDuration;
		auto cursor = handler->specialFindPerf(database, collection, filter, projection, findDuration);

		auto preIterationMeasurement = std::chrono::high_resolution_clock::now();
		int count = 0;
		for (auto& doc : cursor) {
			count++;
		}
		auto postIterationMeasurement = std::chrono::high_resolution_clock::now();

		auto itDuration = std::chrono::duration_cast<std::chrono::microseconds>(postIterationMeasurement - preIterationMeasurement);
		logFile << elements << "," << findDuration << "," << itDuration.count() << std::endl;

		EXPECT_THAT(count, elements);
	}

	// Magnitude 6: 100000 Documents
	{
		int elements = 100000;

		std::vector<repo::lib::RepoUUID> subVec;
		for (int i = 0; i < elements; i++) {
			subVec.push_back(ids[i]);
		}

		repo::core::model::RepoBSONBuilder innerBson;
		innerBson.appendArray("$in", subVec);
		repo::core::model::RepoBSONBuilder outerBson;
		outerBson.append("shared_id", innerBson.obj());
		auto filter = outerBson.obj();

		long long findDuration;
		auto cursor = handler->specialFindPerf(database, collection, filter, projection, findDuration);

		auto preIterationMeasurement = std::chrono::high_resolution_clock::now();
		int count = 0;
		for (auto& doc : cursor) {
			count++;
		}
		auto postIterationMeasurement = std::chrono::high_resolution_clock::now();

		auto itDuration = std::chrono::duration_cast<std::chrono::microseconds>(postIterationMeasurement - preIterationMeasurement);
		logFile << elements << "," << findDuration << "," << itDuration.count() << std::endl;

		EXPECT_THAT(count, elements);
	}

	// Magnitude 7: 582915 Documents
	{
		int elements = 582915;

		std::vector<repo::lib::RepoUUID> subVec;
		for (int i = 0; i < elements; i++) {
			subVec.push_back(ids[i]);
		}

		repo::core::model::RepoBSONBuilder innerBson;
		innerBson.appendArray("$in", subVec);
		repo::core::model::RepoBSONBuilder outerBson;
		outerBson.append("shared_id", innerBson.obj());
		auto filter = outerBson.obj();

		long long findDuration;
		auto cursor = handler->specialFindPerf(database, collection, filter, projection, findDuration);

		auto preIterationMeasurement = std::chrono::high_resolution_clock::now();
		int count = 0;
		for (auto& doc : cursor) {
			count++;
		}
		auto postIterationMeasurement = std::chrono::high_resolution_clock::now();

		auto itDuration = std::chrono::duration_cast<std::chrono::microseconds>(postIterationMeasurement - preIterationMeasurement);
		logFile << elements << "," << findDuration << "," << itDuration.count() << std::endl;

		EXPECT_THAT(count, elements);
	}

	logFile.close();	
}


TEST(Sandbox, GottaReadThemAll) {
	auto handler = getLocalHandler();
	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");

	// Set up log file
	std::ofstream logFile;
	logFile.open("ReadThemAll.log");
	logFile << "Starting operation" << std::endl;

	repo::core::model::RepoBSONBuilder projBson;
	auto projection = projBson.obj();

	repo::core::model::RepoBSONBuilder filterBson;
	filterBson.append("rev_id", revId);
	auto filter = filterBson.obj();

	long long findDuration;
	auto cursor = handler->specialFindPerf(database, collection, filter, projection, findDuration);

	logFile << "Find operation complete after " << findDuration << " microseconds" << std::endl;
	logFile.flush();

	auto preIterationMeasurement = std::chrono::high_resolution_clock::now();
	std::vector<repo::core::model::RepoBSON> documents;
	long long count = 0;
	for (auto& doc : cursor) {
		auto bson = repo::core::model::RepoBSON(doc);
		documents.push_back(bson);

		count++;
		if (count % 1000 == 0) {
			logFile << "Read " << count << " elements" << std::endl;
		}
	}
	auto postIterationMeasurement = std::chrono::high_resolution_clock::now();

	auto itDuration = std::chrono::duration_cast<std::chrono::microseconds>(postIterationMeasurement - preIterationMeasurement);
	logFile << "Iteration operation complete after " << itDuration.count() << " microseconds. Total number of elements: " << count << std::endl;

	EXPECT_THAT(documents.size(), Eq(2404268));
}