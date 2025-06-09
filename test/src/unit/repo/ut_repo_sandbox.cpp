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

#include <repo/core/handler/fileservice/repo_blob_files_handler.h>
#include <repo/core/handler/fileservice/repo_file_manager.h>

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

#include "bouncer/src/repo/manipulator/modeloptimizer/bvh/bvh.hpp"
#include "bouncer/src/repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp"

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
	config.configureFS("C:\\Git\\local\\efs\\models"); //getDataPath("fileShare"));
	handler->setFileManager(std::make_shared<repo::core::handler::fileservice::FileManager>(config, handler));
	return handler;
}

std::shared_ptr<repo::core::handler::MongoDatabaseHandler> getLocalHandler(std::shared_ptr<FileManager>& fileManager)
{
	auto handler = repo::core::handler::MongoDatabaseHandler::getHandler(
		REPO_GTEST_DBADDRESS,
		REPO_GTEST_DBPORT,
		"adminUser",
		"password"
	);

	auto config = repo::lib::RepoConfig::fromFile(getDataPath("config/withFS.json"));
	config.configureFS("C:\\Git\\local\\efs\\models"); //getDataPath("fileShare"));
	fileManager = std::make_shared<repo::core::handler::fileservice::FileManager>(config, handler);
	handler->setFileManager(fileManager);
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

mongocxx::pipeline createTransformPipeline(std::string collection, repo::lib::RepoUUID revId, std::vector<repo::lib::RepoUUID> idArray) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};

	bsoncxx::builder::basic::array binIdArray;
	for (int i = 0; i < idArray.size(); i++) {
		auto idData = idArray[i].data();
		bsoncxx::types::b_binary idbinary{
			bsoncxx::binary_sub_type::k_uuid_deprecated,
			idData.size(),
			idData.data()
		};
		binIdArray.append(idbinary);
	}

	mongocxx::pipeline transfPipeline;
	transfPipeline.match(
		make_document(
			kvp("shared_id", make_document(
				kvp("$in", binIdArray)
			))
		)
	).graph_lookup(
		make_document(
			kvp("from", collection),
			kvp("startWith", "$parents"),
			kvp("connectFromField", "parents"),
			kvp("connectToField", "shared_id"),
			kvp("as", "allParents"),
			kvp("depthField", "level"),
			kvp("restrictSearchWithMatch", make_document(
				kvp("type", "transformation")
			))
		)
	).project(
		make_document(
			kvp("sortedParents", make_document(
				kvp("$sortArray", make_document(
					kvp("input", "$allParents"),
					kvp("sortBy", make_document(
						kvp("level", 1)
					))
				))
			))
		)
	).project(
		make_document(
			kvp("transforms", make_document(
				kvp("$concatArrays", make_array(
					"$sortedParents.matrix"
				))
			))
		)
	);

	return transfPipeline;
}

mongocxx::pipeline createTransformPipeline(std::string collection, repo::lib::RepoUUID revId, repo::lib::RepoUUID id) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};

	
	auto idData = id.data();
	bsoncxx::types::b_binary idbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		idData.size(),
		idData.data()
	};
	
	mongocxx::pipeline transfPipeline;
	transfPipeline.match(
		make_document(
			kvp("shared_id", idbinary)
		)
	).graph_lookup(
		make_document(
			kvp("from", collection),
			kvp("startWith", "$parents"),
			kvp("connectFromField", "parents"),
			kvp("connectToField", "shared_id"),
			kvp("as", "allParents"),
			kvp("depthField", "level"),
			kvp("restrictSearchWithMatch", make_document(
				kvp("type", "transformation")
			))
		)
	).project(
		make_document(
			kvp("sortedParents", make_document(
				kvp("$sortArray", make_document(
					kvp("input", "$allParents"),
					kvp("sortBy", make_document(
						kvp("level", 1)
					))
				))
			))
		)
	).project(
		make_document(
			kvp("transforms", make_document(
				kvp("$concatArrays", make_array(
					"$sortedParents.matrix"
				))
			))
		)
	);

	return transfPipeline;
}

mongocxx::pipeline createAllTransformsPipeline(std::string collection, repo::lib::RepoUUID revId) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};


	mongocxx::pipeline transfPipeline;
	transfPipeline.match(
		make_document(
			kvp("type", "mesh")
		)
	).graph_lookup(
		make_document(
			kvp("from", collection),
			kvp("startWith", "$parents"),
			kvp("connectFromField", "parents"),
			kvp("connectToField", "shared_id"),
			kvp("as", "allParents"),
			kvp("depthField", "level"),
			kvp("restrictSearchWithMatch", make_document(
				kvp("type", "transformation")
			))
		)
	).project(
		make_document(
			kvp("sortedParents", make_document(
				kvp("$sortArray", make_document(
					kvp("input", "$allParents"),
					kvp("sortBy", make_document(
						kvp("level", 1)
					))
				))
			))
		)
	).project(
		make_document(
			kvp("transforms", make_document(
				kvp("$concatArrays", make_array(
					"$sortedParents.matrix"
				))
			))
		)
	);

	return transfPipeline;
}

mongocxx::pipeline createTranspPipeline(std::string collection, repo::lib::RepoUUID revId, int primitiveParam) {
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
			kvp("mesh.0.primitive", primitiveParam)
		)
	).project(
		make_document(
			kvp("shared_id", "$parents")
		)
	);

	return transpPipeline;
}

mongocxx::pipeline createNormalPipeline(std::string collection, repo::lib::RepoUUID revId, int primitiveParam) {
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
			kvp("mesh.0.primitive", primitiveParam)
		)
	).project(
		make_document(
			kvp("shared_id", "$parents")
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
			kvp("_id", 0),
			kvp("shared_id", 1)
		)
	);

	return textureIdPipeline;
}

//mongocxx::pipeline createTextureNodesPipeline(std::string collection, repo::lib::RepoUUID revId, repo::lib::RepoUUID texId) {
//
//	auto texUuidData = texId.data();
//	bsoncxx::types::b_binary texIdBinary{
//		bsoncxx::binary_sub_type::k_uuid_deprecated,
//		texUuidData.size(),
//		texUuidData.data()
//	};
//
//	auto revUuidData = revId.data();
//	bsoncxx::types::b_binary revIdBinary{
//		bsoncxx::binary_sub_type::k_uuid_deprecated,
//		revUuidData.size(),
//		revUuidData.data()
//	};
//
//
//	mongocxx::pipeline textureNodePipeline;
//	textureNodePipeline.match(
//		make_document(
//			kvp("rev_id", revIdBinary),
//			kvp("shared_id", texIdBinary)
//		)
//	).project(
//		make_document(
//			kvp("parents", 1)
//		)
//	).unwind(
//		"$parents"
//	).lookup(
//		make_document(
//			kvp("from", collection),
//			kvp("localField", "parents"),
//			kvp("foreignField", "shared_id"),
//			kvp("as", "material")
//		)
//	).project(
//		make_document(
//			kvp("material", 1)
//		)
//	).unwind(
//		"$material"
//	).project(
//		make_document(
//			kvp("materialParents", "$material.parents")
//		)
//	).unwind(
//		"$materialParents"
//	).lookup(
//		make_document(
//			kvp("from", collection),
//			kvp("localField", "materialParents"),
//			kvp("foreignField", "shared_id"),
//			kvp("as", "mesh")
//		)
//	).match(
//		make_document(
//			kvp("mesh.0.primitive", 3),
//			kvp("mesh.0.uv_channels_count", make_document(
//				kvp("$exists", true)
//			))
//		)
//	).project(make_document(
//		kvp("shared_id", "$materialParents")
//	));
//
//	return textureNodePipeline;
//}

mongocxx::pipeline createTextureNodesPipeline(std::string collection, repo::lib::RepoUUID revId, repo::lib::RepoUUID texId) {

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
	).project(make_document(
		kvp("shared_id", "$materialParents")
	));

	return textureNodePipeline;
}
TEST(Sandbox, SandboxCoverageTest) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// House model (small)
	std::string collection = "5144ac65-6b9d-4236-b0a6-9186149a32c8.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("949EE0E2-13FB-4C07-9B49-9556F5E8F293");
	int noMeshNodes = 1349;

	// "Medium" Model
	//std::string collection = "69d561b4-77d7-44a0-b295-d0db459d5270.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("3792C7CB-2854-4FF0-B957-278C154C0CB7");
	//int noMeshNodes = 245669;

	// XXL Model
	//std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");
	//int noMeshNodes = 654211;

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



	// Get number of transparent Nodes (primitive 2)
	auto cursorTranspPrim2 = handler->runAggregatePipeline(database, collection, createTranspPipeline(collection, revId, 2));
	int countTranspPrim2 = 0;
	for (auto document : (*cursorTranspPrim2)) {
		countTranspPrim2++;
	}
	std::cout << "MESH NODES, TRANSPARENT, Prim 2: " << countTranspPrim2 << std::endl;


	// Get number of transparent Nodes (primitive 3)
	auto cursorTranspPrim3 = handler->runAggregatePipeline(database, collection, createTranspPipeline(collection, revId, 3));
	int countTranspPrim3 = 0;
	for (auto document : (*cursorTranspPrim3)) {
		countTranspPrim3++;
	}

	std::cout << "MESH NODES, TRANSPARENT, Prim 3: " << countTranspPrim3 << std::endl;

	int countTranspTotal = countTranspPrim2 + countTranspPrim3;
	std::cout << "MESH NODES, TRANSPARENT, TOTAL: " << countTranspTotal << std::endl;




	// Get number of normal Nodes (no uv, primitive 2)
	auto cursorNormPrim2 = handler->runAggregatePipeline(database, collection, createNormalPipeline(collection, revId, 2));
	int countNormPrim2 = 0;
	for (auto document : (*cursorNormPrim2)) {
		countNormPrim2++;
	}
	std::cout << "MESH NODES, NORMAL Prim 2: " << countNormPrim2 << std::endl;


	// Get number of normal Nodes (no uv, primitive 3)
	auto cursorNormPrim3 = handler->runAggregatePipeline(database, collection, createNormalPipeline(collection, revId, 3));
	int countNormPrim3 = 0;
	for (auto document : (*cursorNormPrim3)) {
		countNormPrim3++;
	}

	std::cout << "MESH NODES, NORMAL Prim 3: " << countNormPrim3 << std::endl;



	int countNormTotal = countNormPrim2 + countNormPrim3;
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
			auto cursorText = handler->runAggregatePipeline(database, collection, createTextureNodesPipeline(collection, revId, texId));
			int countText = 0;
			for (auto document : (*cursorText)) {
				countText++;
			}
			std::cout << "MESH NODES, TEXTURED, TEXID: " << texId.toString() << ": " << countText << std::endl;

			countTextNodes += countText;
		}
	}

	std::cout << "UNIQUE TEX IDS: " << texIdsCovered.size() << std::endl;

	std::cout << "MESH NODES, TEXTURED, TOTAL: " << countTextNodes << std::endl;

	int foundMeshNodesTotal = countTranspTotal + countNormTotal + countTextNodes;
	std::cout << "MESH NODES, ALL, TOTAL: " << foundMeshNodesTotal << std::endl;

	EXPECT_THAT(foundMeshNodesTotal, Eq(noMeshNodes));

}

TEST(Sandbox, TransparentLookupTest) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// House model (small)
	//std::string collection = "5144ac65-6b9d-4236-b0a6-9186149a32c8.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("949EE0E2-13FB-4C07-9B49-9556F5E8F293");

	// "Medium" Model
	//std::string collection = "69d561b4-77d7-44a0-b295-d0db459d5270.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("3792C7CB-2854-4FF0-B957-278C154C0CB7");

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");

	auto cursor = handler->runAggregatePipeline(database, collection, createTranspPipeline(collection, revId, 2));
	int count = 0;
	for (auto document : (*cursor)) {
		count++;
	}

	EXPECT_THAT(count, Eq(1187));
}

TEST(Sandbox, AllTransformsTest) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// House model (small)
	//std::string collection = "1d08cae9-ba1f-4b40-b96e-6ad1a0059901.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("CDB96441-5A9D-4EB1-8439-6124B2DC5EEE");

	// "Medium" Model
	//std::string collection = "1a874c4b-891c-4586-864c-3bbb408a71d3.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("6E2F4803-34AA-4719-A797-693C842342D0");

	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");

	repo::core::model::RepoBSONBuilder filter;
	filter.append("rev_id", revId);
	filter.append("type", "transformation");

	repo::core::model::RepoBSONBuilder projection;
	projection.append("shared_id", 1);
	// projection.append("matrix", 1);
	projection.append("matrixObj", 1);
	// projection.append("parents", 1);
	projection.append("parent", 1);
	projection.append("_id", 0);

	std::ofstream logFile;
	logFile.open("GetAllTransforms.log", std::ios_base::app);
	logFile << "timestamp,event" << std::endl;

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,QueryStart" << std::endl;
	auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,QueryEnd" << std::endl;
	
	std::vector<repo::core::model::RepoBSON> transformNodes;
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,IterationStart" << std::endl;
	for (auto doc : cursor) {
		auto bson = repo::core::model::RepoBSON(doc);
		transformNodes.push_back(bson);
	}	 
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,IterationEnd" << std::endl;

	EXPECT_THAT(transformNodes.size(), Eq(874870));
}

TEST(Sandbox, AllTransformLookupTest) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// House model (small)
	//std::string collection = "5144ac65-6b9d-4236-b0a6-9186149a32c8.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("949EE0E2-13FB-4C07-9B49-9556F5E8F293");

	// "Medium" Model
	//std::string collection = "69d561b4-77d7-44a0-b295-d0db459d5270.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("3792C7CB-2854-4FF0-B957-278C154C0CB7");

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");



	auto cursor = handler->runAggregatePipeline(database, collection, createAllTransformsPipeline(collection, revId));

	int count = 0;
	std::vector<repo::lib::RepoMatrix> nodeTransforms;
	for (auto document : (*cursor)) {
		auto transforms = document.getMatrixFieldArray("transforms");
		repo::lib::RepoMatrix finalTransform;
		for (int i = 0; i < transforms.size(); i++) {
			finalTransform = finalTransform * transforms[i];
		}
		nodeTransforms.push_back(finalTransform);
		count++;
	}

	EXPECT_THAT(count, Eq(654211));
}

TEST(Sandbox, GroupTransformLookupTest) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// House model (small)
	//std::string collection = "5144ac65-6b9d-4236-b0a6-9186149a32c8.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("949EE0E2-13FB-4C07-9B49-9556F5E8F293");

	// "Medium" Model
	//std::string collection = "69d561b4-77d7-44a0-b295-d0db459d5270.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("3792C7CB-2854-4FF0-B957-278C154C0CB7");

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");

	std::vector<repo::lib::RepoUUID> ids;

	std::ifstream idFile;
	idFile.open("GroupIds_1.log");
	std::string line;
	while (std::getline(idFile, line)) {
		repo::lib::RepoUUID id = repo::lib::RepoUUID(line);
		ids.push_back(id);
	}
	idFile.close();


	//repo::lib::RepoUUID id1 = repo::lib::RepoUUID("91fe0649-c6e0-4d21-becb-adac6e97314a");
	//repo::lib::RepoUUID id2 = repo::lib::RepoUUID("25a53aed-0368-4d26-ae87-6677ee2880a4");

	//ids.push_back(id1);
	//ids.push_back(id2);


	auto cursor = handler->runAggregatePipeline(database, collection, createTransformPipeline(collection, revId, ids));
	int count = 0;
	for (auto document : (*cursor)) {
		count++;
	}

	EXPECT_THAT(count, Eq(1000));
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

TEST(Sandbox, SandboxDumpOpaqueNodeIds) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");

	std::ofstream file;
	file.open("opaqueNodeIds.txt");

	auto cursorPrim2 = handler->runAggregatePipeline(database, collection, createNormalPipeline(collection, revId, 2));
	for (auto doc : (*cursorPrim2)) {		
		auto sharedId = doc.getUUIDField("shared_id");
		file << sharedId.toString() << std::endl;
	}

	auto cursorPrim3 = handler->runAggregatePipeline(database, collection, createNormalPipeline(collection, revId, 3));
	for (auto doc : (*cursorPrim3)) {
		auto sharedId = doc.getUUIDField("shared_id");
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

	auto point1 = std::chrono::system_clock::now();
	logFile << "Point 1: " << point1.time_since_epoch().count() << "00" << std::endl;
	while (std::getline(file, line)) {
		repo::lib::RepoUUID id = repo::lib::RepoUUID(line);
		ids.push_back(id);
	}
	auto point2 = std::chrono::system_clock::now();
	logFile << "Point 2: " << point2.time_since_epoch().count() << "00" << std::endl;

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

void processTransparentGroup(
	std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId,
	int primitive) {

	// 1. Get the ids for the group
	std::vector <repo::lib::RepoUUID> transpIds;
	auto transpCursor = handler->runAggregatePipeline(database, collection, createTranspPipeline(collection, revId, primitive));
	for (auto document : (*transpCursor)) {
		transpIds.push_back(document.getUUIDField("shared_id"));
	}

	// 2. Process the group

}

void processGroup(
	std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId,
	mongocxx::v_noabi::pipeline pipeline,
	int batchSize,
	std::string groupType) {

	std::ofstream logFile;
	logFile.open("processGroup.log", std::ios_base::app);

	// 1. Get the ids for the group
	std::vector <repo::lib::RepoUUID> groupIds;
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group Id Query Start" << std::endl;
	auto transpCursor = handler->runAggregatePipeline(database, collection, pipeline);
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group Id Query End" << std::endl;
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group Id Iteration Start" << std::endl;
	for (auto document : (*transpCursor)) {
		groupIds.push_back(document.getUUIDField("shared_id"));
	}
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group Id Iteration End" << std::endl;

	// 2. Process the group by pulling info of the nodes	
	int processed = 0;

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group start batching" << std::endl;
	std::vector<repo::lib::RepoUUID> batchIds;
	while (processed < groupIds.size()) {

		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group start processing of new batch" << std::endl;

		// Split off the next batch
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group start breaking off batch" << std::endl;
		batchIds.clear();
		for (int i = 0; (i < batchSize) && ((processed + i) < groupIds.size()); i++) {
			batchIds.push_back(groupIds[processed + i]);
		}
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group end breaking off batch" << std::endl;

		// Keep track of remaining
		processed += batchIds.size();

		// Pull information from batch via find
		repo::core::model::RepoBSONBuilder innerBson;
		innerBson.appendArray("$in", batchIds);
		repo::core::model::RepoBSONBuilder outerBson;
		outerBson.append("shared_id", innerBson.obj());
		auto filter = outerBson.obj();

		repo::core::model::RepoBSONBuilder projBson;
		auto projection = projBson.obj();

		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find query start" << std::endl;
		auto rawCursor = handler->rawCursorFind(database, collection, filter, projection);
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find query end" << std::endl;
		std::vector<repo::core::model::RepoBSON> batchInfos;
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find iteration Start" << std::endl;
		for (auto& doc : rawCursor) {
			repo::core::model::RepoBSON bson = repo::core::model::RepoBSON(doc);
			batchInfos.push_back(bson);
		}
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find iteration end" << std::endl;

		// Pull transformations via aggregation for the batch
		// and multiply the matrices
		std::vector<repo::lib::RepoMatrix> nodeTransforms;
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform query start" << std::endl;
		auto transformCursor = handler->runAggregatePipeline(database, collection, createTransformPipeline(collection, revId, batchIds));
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform query end" << std::endl;
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform iteration start" << std::endl;
		for (auto document : (*transformCursor)) {
			auto transforms = document.getMatrixFieldArray("transforms");
			repo::lib::RepoMatrix finalTransform;
			for (int i = 0; i < transforms.size(); i++) {
				finalTransform = finalTransform * transforms[i];
			}
			nodeTransforms.push_back(finalTransform);
		}
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform iteration end" << std::endl;

		// Generation of the tree goes here


		// Rest of Clustering goes here

		// Rest of Supermeshing goes here
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group end batch" << std::endl;
	}
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group end batching" << std::endl;
	logFile.close();

}

//std::vector<repo::lib::RepoUUID> getTexIds(
//	std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
//	std::string database,
//	std::string collection,
//	repo::lib::RepoUUID revId) {
//
//	std::vector<repo::lib::RepoUUID> texIds;
//	auto cursor = handler->runAggregatePipeline(database, collection, createTextureIdPipeline(collection, revId));
//	for (auto document : (*cursor)) {
//		texIds.push_back(document.getUUIDField("shared_id"));
//	}
//
//	return texIds;
//}

// This is wrong currently. Returns shared ids of nodes with textures. Not texture IDs
std::vector<repo::lib::RepoUUID> getTexIds(
	std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId) {

	repo::core::model::RepoBSONBuilder filter;
	filter.append("rev_id", revId);
	filter.append("type", "texture");

	repo::core::model::RepoBSONBuilder projection;
	filter.append("_id", 0);
	filter.append("shared_id", 1);

	std::vector<repo::lib::RepoUUID> texIds;
	auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());	
	for (auto document : cursor) {
		auto bson = repo::core::model::RepoBSON(document);
		texIds.push_back(bson.getUUIDField("shared_id")); 
	}

	return texIds;
}

TEST(Sandbox, SortingPrototype) {

	auto handler = getLocalHandler();
	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");

	// Sort nodes
	int sortingBatchSize = 10000;

	// Transparent group
	processGroup(handler, database, collection, revId, createTranspPipeline(collection, revId, 2), sortingBatchSize, "transparent, prim 2");
	processGroup(handler, database, collection, revId, createTranspPipeline(collection, revId, 3), sortingBatchSize, "transparent, prim 3");





	// Opaque group
	processGroup(handler, database, collection, revId, createNormalPipeline(collection, revId, 2), sortingBatchSize, "opaque, prim 2");
	processGroup(handler, database, collection, revId, createNormalPipeline(collection, revId, 3), sortingBatchSize, "opaque, prim 3");



	// Texture Groups
	// 1. Get the texture IDs
	auto texIds = getTexIds(handler, database, collection, revId);

	// 2. For each texture ID in the group:
	for (int i = 0; i < texIds.size(); i++) {
		auto texId = texIds[i];

		// 2.1. Process the group
		std::string groupStr = "textured, " + texId.toString();
		processGroup(handler, database, collection, revId, createTextureNodesPipeline(collection, revId, texId), sortingBatchSize, groupStr);
	}
}


// Filter approach

mongocxx::pipeline createTranspListPipeline(std::string collection, repo::lib::RepoUUID revId) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};

	mongocxx::pipeline transpListPipe;
	transpListPipe.match(
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
			kvp("localField", "shared_id"),
			kvp("foreignField", "shared_id"),
			kvp("as", "mesh")
		)
	).project(
		make_document(
			kvp("shared_id", "$parents")
		)
	);

	return transpListPipe;
}

mongocxx::pipeline createOpaqueListPipeline(std::string collection, repo::lib::RepoUUID revId) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};

	mongocxx::pipeline opaqueListPipe;
	opaqueListPipe.match(
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
			kvp("localField", "shared_id"),
			kvp("foreignField", "shared_id"),
			kvp("as", "mesh")
		)
	).project(
		make_document(
			kvp("shared_id", "$parents")
		)
	);

	return opaqueListPipe;
}

mongocxx::pipeline createTextListPipeline(std::string collection, repo::lib::RepoUUID revId) {
	auto uuidData = revId.data();
	bsoncxx::types::b_binary revIdbinary{
		bsoncxx::binary_sub_type::k_uuid_deprecated,
		uuidData.size(),
		uuidData.data()
	};

	mongocxx::pipeline textListPipe;
	textListPipe.match(
		make_document(
			kvp("rev_id", revIdbinary),
			kvp("type", "texture")
		)
	).project(
		make_document(
			kvp("_id", 0),
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
			kvp("_id", 0),
			kvp("material", 1)
		)
	).unwind(
		"$material"
	).project(
		make_document(
			kvp("_id", 0),
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
	).project(
		make_document(
			kvp("_id", 0),
			kvp("shared_id", "$materialParents")
		)
	);

	return textListPipe;
}

void markTexturedNodes(
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId,
	int batchSize) {

	std::ofstream logFile;
	logFile.open("processGroupFiltered.log", std::ios_base::app);

	// Add texture IDs to nodes

	// Get texture IDs first
	std::vector<repo::lib::RepoUUID> texIds;
	{
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get texture ids query start" << std::endl;
		auto cursor = handler->runAggregatePipeline(database, collection, createTextureIdPipeline(collection, revId));
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get texture ids query end" << std::endl;
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get texture ids iteration start" << std::endl;
		for (auto doc : (*cursor)) {
			auto texId = doc.getUUIDField("shared_id");
			texIds.push_back(texId);
		}
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get texture ids iteration end" << std::endl;
	}

	// Go over all tex Ids
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Assign all texture ids start" << std::endl;
	for (repo::lib::RepoUUID texId : texIds) {
		// Get node ids for this tex id
		std::vector<repo::lib::RepoUUID> meshNodes;
		{
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get textured nodes for texId query start" << std::endl;
			auto cursor = handler->runAggregatePipeline(database, collection, createTextureNodesPipeline(collection, revId, texId));
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get textured nodes for texId query end" << std::endl;
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get textured nodes for texId iteration start" << std::endl;
			for (auto doc : (*cursor)) {
				auto nodeId = doc.getUUIDField("shared_id");
				meshNodes.push_back(nodeId);
			}
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get textured nodes for texId iteration end" << std::endl;
		}

		// Construct filter
		repo::core::model::RepoBSONBuilder inClause;
		inClause.appendArray("$in", meshNodes);

		repo::core::model::RepoBSONBuilder filter;
		filter.append("shared_id", inClause.obj());


		repo::core::model::RepoBSONBuilder newField;
		newField.append("materialProperties.textureId", texId);
		repo::core::model::RepoBSONBuilder update;
		update.append("$set", newField.obj());

		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Update textured nodes for texId query start" << std::endl;
		handler->updateMany(database, collection, filter.obj(), update.obj());
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Update textured nodes for texId query end" << std::endl;
	}
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Assign all texture ids end" << std::endl;

	
	
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Create texture index start" << std::endl;	
	handler->createIndex(database, collection, repo::core::handler::database::index::Ascending({"rev_id", "materialProperties.textureId", "shared_id"}), true);
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Create texture index end" << std::endl;

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Mark textured nodes end" << std::endl;
	logFile.close();
}

void updateOpaqueNodeBatch(
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	std::vector<repo::lib::RepoUUID> batch
) {
	std::ofstream logFile;
	logFile.open("processGroupFiltered.log", std::ios_base::app);
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Mark opaque nodes batch start" << std::endl;

	// Construct filter
	repo::core::model::RepoBSONBuilder textExistClause;
	textExistClause.append("$exists", false);

	repo::core::model::RepoBSONBuilder textClause;
	textClause.append("materialProperties.textureId", textExistClause.obj());

	repo::core::model::RepoBSONBuilder inClause;
	inClause.appendArray("$in", batch);

	repo::core::model::RepoBSONBuilder idClause;
	idClause.append("shared_id", inClause.obj());

	std::vector<repo::core::model::RepoBSON> filterConditions{ idClause.obj(), textClause.obj() };
	repo::core::model::RepoBSONBuilder opaqueFilter;
	opaqueFilter.appendArray("$and", filterConditions);

	repo::core::model::RepoBSONBuilder newField;
	newField.append("materialProperties.isOpaque", true);
	repo::core::model::RepoBSONBuilder update;
	update.append("$set", newField.obj());

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Update opaque nodes with isOpaque start" << std::endl;
	handler->updateMany(database, collection, opaqueFilter.obj(), update.obj());
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Update opaque nodes with isOpaque end" << std::endl;


	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Mark opaque nodes batch end" << std::endl;
	logFile.close();
}

void updateOpaqueNodesInBatches(
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	std::vector<repo::lib::RepoUUID> nodes,
	int batchSize
) {
	std::vector<repo::lib::RepoUUID> currentBatch;
	for (int i = 0; i < nodes.size(); i++) {
		currentBatch.push_back(nodes[i]);

		if (currentBatch.size() >= batchSize) {
			updateOpaqueNodeBatch(handler, database, collection, currentBatch);
			currentBatch.clear();
		}
	}

	if (currentBatch.size() > 0) {
		updateOpaqueNodeBatch(handler, database, collection, currentBatch);
	}
}

void markOpaqueNodes(
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId,
	int batchSize) {

	std::ofstream logFile;
	logFile.open("processGroupFiltered.log", std::ios_base::app);
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Mark opaque nodes start" << std::endl;

	// Get list for mesh nodes with opaque material
	std::vector<repo::lib::RepoUUID> opaqueNodeIds;
	{
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get opaque nodes query start" << std::endl;
		auto cursor = handler->runAggregatePipeline(database, collection, createOpaqueListPipeline(collection, revId));
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get opaque nodes query end" << std::endl;
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get opaque nodes iteration start" << std::endl;
		for (auto doc : (*cursor)) {
			opaqueNodeIds.push_back(doc.getUUIDField("shared_id"));
		}
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get opaque nodes iteration end" << std::endl;
	}

	logFile.close();

	updateOpaqueNodesInBatches(handler, database, collection, opaqueNodeIds, batchSize);

	logFile.open("processGroupFiltered.log", std::ios_base::app);

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Create opaque indexes start" << std::endl;
	handler->createIndex(database, collection, repo::core::handler::database::index::Ascending({"rev_id", "materialProperties.isOpaque", "primitive", "shared_id"}), true);
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Create opaque indexes end" << std::endl;

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Mark opaque nodes end" << std::endl;
	logFile.close();
}

void updateTransparentNodeBatch(
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	std::vector<repo::lib::RepoUUID> batch
) {
	std::ofstream logFile;
	logFile.open("processGroupFiltered.log", std::ios_base::app);
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Mark transparent nodes batch start" << std::endl;

	// Construct filter
	repo::core::model::RepoBSONBuilder textExistClause;
	textExistClause.append("$exists", false);

	repo::core::model::RepoBSONBuilder textClause;
	textClause.append("materialProperties.textureId", textExistClause.obj());

	repo::core::model::RepoBSONBuilder inClause;
	inClause.appendArray("$in", batch);

	repo::core::model::RepoBSONBuilder idClause;
	idClause.append("shared_id", inClause.obj());

	std::vector<repo::core::model::RepoBSON> filterConditions{ idClause.obj(), textClause.obj() };
	repo::core::model::RepoBSONBuilder transparentFilter;
	transparentFilter.appendArray("$and", filterConditions);

	repo::core::model::RepoBSONBuilder newField;
	newField.append("materialProperties.isTransparent", true);
	repo::core::model::RepoBSONBuilder update;
	update.append("$set", newField.obj());

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Update transparent nodes with isTransparent start" << std::endl;
	handler->updateMany(database, collection, transparentFilter.obj(), update.obj());
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Update transparent nodes with isTransparent end" << std::endl;
	
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Mark transparent nodes batch end" << std::endl;
	logFile.close();
}

void updateTransparentNodesInBatches(
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	std::vector<repo::lib::RepoUUID> nodes,
	int batchSize
) {	
	std::vector<repo::lib::RepoUUID> currentBatch;
	for (int i = 0; i < nodes.size(); i++) {
		currentBatch.push_back(nodes[i]);		
		
		if (currentBatch.size() >= batchSize) {
			updateTransparentNodeBatch(handler, database, collection, currentBatch);
			currentBatch.clear();
		}
	}

	if (currentBatch.size() > 0) {
		updateTransparentNodeBatch(handler, database, collection, currentBatch);
	}
}

void markTransparentNodes(
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId,
	int batchSize) {

	std::ofstream logFile;
	logFile.open("processGroupFiltered.log", std::ios_base::app);
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Mark transparent nodes start" << std::endl;

	// Get list for mesh nodes with transparent material
	std::vector<repo::lib::RepoUUID> transpNodeIds;
	{
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get transparent nodes query start" << std::endl;
		auto cursor = handler->runAggregatePipeline(database, collection, createTranspListPipeline(collection, revId));
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get transparent nodes query end" << std::endl;
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get transparent nodes iteration start" << std::endl;
		for (auto doc : (*cursor)) {
			transpNodeIds.push_back(doc.getUUIDField("shared_id"));
		}
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Get transparent nodes iteration end" << std::endl;
	}

	logFile.close();

	updateTransparentNodesInBatches(handler, database, collection, transpNodeIds, batchSize);

	logFile.open("processGroupFiltered.log", std::ios_base::app);

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Create transparent indexes start" << std::endl;
	handler->createIndex(database, collection, repo::core::handler::database::index::Ascending({"rev_id", "materialProperties.isTransparent", "primitive", "shared_id"}), true);
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Create transparent indexes end" << std::endl;

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << "Mark transparent nodes end" << std::endl;
	logFile.close();
}

TEST(Sandbox, FilterCoverageTest) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// House model (small)
	//std::string collection = "1d08cae9-ba1f-4b40-b96e-6ad1a0059901.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("CDB96441-5A9D-4EB1-8439-6124B2DC5EEE");
	//int noMeshNodes = 1349;

	// "Medium" Model
	//std::string collection = "1a874c4b-891c-4586-864c-3bbb408a71d3.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("6E2F4803-34AA-4719-A797-693C842342D0");
	//int noMeshNodes = 245669;

	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");
	int noMeshNodes = 654211;

	int batchSize = 550000;


	// Set texture field in db
	markTexturedNodes(handler, database, collection, revId, batchSize);	

	// Set opaque field in db
	markOpaqueNodes(handler, database, collection, revId, batchSize);


	// Set transparent field in db
	markTransparentNodes(handler, database, collection, revId, batchSize);

	// Check whether each meshNode has a field attributed to them.
	// Count opaques
	int opaqueCount = 0;
	{
		// Construct find filter
		repo::core::model::RepoBSONBuilder filter;
		filter.append("materialProperties.isOpaque", true);

		// Construct projection
		repo::core::model::RepoBSONBuilder projection;
		projection.append("shared_id", 1);

		auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());
		for (auto doc : cursor) {
			opaqueCount++;
		}
	}

	// Count transparents
	int transparentCount = 0;
	{
		// Construct find filter
		repo::core::model::RepoBSONBuilder filter;
		filter.append("materialProperties.isTransparent", true);

		// Construct projection
		repo::core::model::RepoBSONBuilder projection;
		projection.append("shared_id", 1);

		auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());
		for (auto doc : cursor) {
			transparentCount++;
		}
	}

	// Count textured
	int texturedCount = 0;
	{
		// Construct find filter
		repo::core::model::RepoBSONBuilder existClause;
		existClause.append("$exists", true);
		repo::core::model::RepoBSONBuilder filter;
		filter.append("materialProperties.textureId", existClause.obj());

		// Construct projection
		repo::core::model::RepoBSONBuilder projection;
		projection.append("shared_id", 1);

		auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());
		for (auto doc : cursor) {
			texturedCount++;
		}
	}
	
	int totalNodes = opaqueCount + transparentCount + texturedCount;
	EXPECT_THAT(totalNodes, Eq(noMeshNodes));
}

void processGroupFiltered(
	std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId,	
	repo::core::model::RepoBSON query,
	int batchSize,
	std::string groupType) {

	std::ofstream logFile;
	logFile.open("processGroupFiltered.log", std::ios_base::app);

	// 1. Get the ids for the group	
	repo::core::model::RepoBSONBuilder projection;
	projection.append("_id", 0);
	projection.append("shared_id", 1);
	std::vector<repo::lib::RepoUUID> groupIds;
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group Id Query Start" << std::endl;	
	auto cursor = handler->rawCursorFind(database, collection, query, projection.obj());
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group Id Query End" << std::endl;
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group Id Iteration Start" << std::endl;
	for (auto doc : cursor) {
		repo::core::model::RepoBSON bson = repo::core::model::RepoBSON(doc);
		groupIds.push_back(bson.getUUIDField("shared_id"));
	}
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group Id Iteration End" << std::endl;
	//auto groupIds = handler->findAllByCriteria(database, collection, query, projection.obj());
	
	// 2. Process the group by pulling info of the nodes
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group start batching" << std::endl;
	std::vector<repo::lib::RepoUUID> currentIdBatch;
	for (int i = 0; i < groupIds.size(); i++) {

		currentIdBatch.push_back(groupIds[i]);

		if (currentIdBatch.size() >= batchSize) {

			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group start processing of new batch" << std::endl;

			// Pull information from batch via find
			repo::core::model::RepoBSONBuilder innerBson;
			innerBson.appendArray("$in", currentIdBatch);
			repo::core::model::RepoBSONBuilder outerBson;
			outerBson.append("shared_id", innerBson.obj());
			auto filter = outerBson.obj();

			repo::core::model::RepoBSONBuilder projBson;
			auto projection = projBson.obj();

			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find query start" << std::endl;
			auto rawCursor = handler->rawCursorFind(database, collection, filter, projection);
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find query end" << std::endl;
			std::vector<repo::core::model::RepoBSON> batchInfos;
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find iteration Start" << std::endl;
			for (auto& doc : rawCursor) {
				repo::core::model::RepoBSON bson = repo::core::model::RepoBSON(doc);
				batchInfos.push_back(bson);
			}
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find iteration end" << std::endl;

			// Pull transformations via aggregation for the batch
			// and multiply the matrices
			std::vector<repo::lib::RepoMatrix> nodeTransforms;
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform query start" << std::endl;
			auto transformCursor = handler->runAggregatePipeline(database, collection, createTransformPipeline(collection, revId, currentIdBatch));
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform query end" << std::endl;
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform iteration start" << std::endl;
			for (auto document : (*transformCursor)) {
				auto transforms = document.getMatrixFieldArray("transforms");
				repo::lib::RepoMatrix finalTransform;
				for (int i = 0; i < transforms.size(); i++) {
					finalTransform = finalTransform * transforms[i];
				}
				nodeTransforms.push_back(finalTransform);
			}
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform iteration end" << std::endl;

			// Generation of the tree goes here

			// Rest of Clustering goes here

			// Rest of Supermeshing goes here
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group end batch" << std::endl;
			
			currentIdBatch.clear();
		}	
	}

	if (currentIdBatch.size() > 0) {

		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group start processing of last batch" << std::endl;

		// Pull information from batch via find
		repo::core::model::RepoBSONBuilder innerBson;
		innerBson.appendArray("$in", currentIdBatch);
		repo::core::model::RepoBSONBuilder outerBson;
		outerBson.append("shared_id", innerBson.obj());
		auto filter = outerBson.obj();

		repo::core::model::RepoBSONBuilder projBson;
		auto projection = projBson.obj();

		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find query start" << std::endl;
		auto rawCursor = handler->rawCursorFind(database, collection, filter, projection);
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find query end" << std::endl;
		std::vector<repo::core::model::RepoBSON> batchInfos;
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find iteration Start" << std::endl;
		for (auto& doc : rawCursor) {
			repo::core::model::RepoBSON bson = repo::core::model::RepoBSON(doc);
			batchInfos.push_back(bson);
		}
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group find iteration end" << std::endl;

		// Pull transformations via aggregation for the batch
		// and multiply the matrices
		std::vector<repo::lib::RepoMatrix> nodeTransforms;
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform query start" << std::endl;
		auto transformCursor = handler->runAggregatePipeline(database, collection, createTransformPipeline(collection, revId, currentIdBatch));
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform query end" << std::endl;
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform iteration start" << std::endl;
		for (auto document : (*transformCursor)) {
			auto transforms = document.getMatrixFieldArray("transforms");
			repo::lib::RepoMatrix finalTransform;
			for (int i = 0; i < transforms.size(); i++) {
				finalTransform = finalTransform * transforms[i];
			}
			nodeTransforms.push_back(finalTransform);
		}
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group transform iteration end" << std::endl;

		// Generation of the tree goes here

		// Rest of Clustering goes here

		// Rest of Supermeshing goes here
		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group end last batch" << std::endl;
	}

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00, " << groupType << " group end batching" << std::endl;
	logFile.close();



}

TEST(Sandbox, FilterSortingPrototype) {

	auto handler = getLocalHandler();
	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");

	// Add the fields to the collection
	int markingBatchSize = 10000;
	markTexturedNodes(handler, database, collection, revId, markingBatchSize);
	markOpaqueNodes(handler, database, collection, revId, markingBatchSize);
	markTransparentNodes(handler, database, collection, revId, markingBatchSize);
	
	// Sort nodes
	int sortingBatchSize = 10000;

	// Transparent group (primitive 2)
	{
		repo::core::model::RepoBSONBuilder query;
		query.append("materialProperties.isTransparent", true);
		query.append("primitive", 2);
		processGroupFiltered(handler, database, collection, revId, query.obj(), sortingBatchSize, "transparent, prim 2");
	}

	// Transparent group (primitive 3)
	{
		repo::core::model::RepoBSONBuilder query;
		query.append("materialProperties.isTransparent", true);
		query.append("primitive", 3);
		processGroupFiltered(handler, database, collection, revId, query.obj(), sortingBatchSize, "transparent, prim 3");
	}

	// Opaque group (primitive 2)
	{
		repo::core::model::RepoBSONBuilder query;
		query.append("materialProperties.isOpaque", true);
		query.append("primitive", 2);
		processGroupFiltered(handler, database, collection, revId, query.obj(), sortingBatchSize, "opaque, prim 2");
	}

	// Opaque group (primitive 3)
	{
		repo::core::model::RepoBSONBuilder query;
		query.append("materialProperties.isOpaque", true);
		query.append("primitive", 3);
		processGroupFiltered(handler, database, collection, revId, query.obj(), sortingBatchSize, "opaque, prim 3");
	}

	// Texture Groups
	// 1. Get the texture IDs
	auto texIds = getTexIds(handler, database, collection, revId);

	// 2. For each texture ID in the group:
	for (int i = 0; i < texIds.size(); i++) {
		auto texId = texIds[i];

		// 2.1. Process the group
		{
			repo::core::model::RepoBSONBuilder query;			
			query.append("materialProperties.textureId", texId);
			std::string groupStr = "textured, " + texId.toString();
			processGroupFiltered(handler, database, collection, revId, query.obj(), sortingBatchSize, groupStr);
		}
	}
}

TEST(Sandbox, GetTransformTestAggregations) {

	auto handler = getLocalHandler();
	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");

	// Load file with ids
	std::vector<repo::lib::RepoUUID> ids;
	std::ifstream idFile;
	idFile.open("opaqueNodeIds.log");
	std::string line;
	while (std::getline(idFile, line)) {
		repo::lib::RepoUUID id = repo::lib::RepoUUID(line);
		ids.push_back(id);
	}
	idFile.close();

	int batchSize = 10000;

	std::ofstream logFile;
	logFile.open("GetTransformTestAggregations.log");
	logFile << "timestamp,event" << std::endl;
	
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,LookupStart" << std::endl;
	std::vector<repo::lib::RepoUUID> currentIdBatch;
	for (int i = 0; i < ids.size(); i++) {

		currentIdBatch.push_back(ids[i]);

		if (currentIdBatch.size() >= batchSize) {
			std::vector<repo::lib::RepoMatrix> nodeTransforms;
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,QueryStart" << std::endl;
			auto transformCursor = handler->runAggregatePipeline(database, collection, createTransformPipeline(collection, revId, currentIdBatch));
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,QueryEnd" << std::endl;
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,IterationStart" << std::endl;
			for (auto document : (*transformCursor)) {
				auto transforms = document.getMatrixFieldArray("transforms");
				repo::lib::RepoMatrix finalTransform;
				for (int i = 0; i < transforms.size(); i++) {
					finalTransform = finalTransform * transforms[i];
				}
				nodeTransforms.push_back(finalTransform);
			}
			logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,IterationEnd" << std::endl;

			currentIdBatch.clear();
		}

	}
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,LookupEnd" << std::endl;
	logFile.close();

}

TEST(Sandbox, GetTransformTestIndexed) {

	auto handler = getLocalHandler();
	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");


	// Load file with ids
	std::vector<repo::lib::RepoUUID> ids;
	std::ifstream idFile;
	idFile.open("opaqueNodeIds.log");
	std::string line;
	while (std::getline(idFile, line)) {
		repo::lib::RepoUUID id = repo::lib::RepoUUID(line);
		ids.push_back(id);
	}
	idFile.close();

	std::ofstream logFile;
	logFile.open("GetTransformTestIndexed.log");
	logFile << "timestamp,event" << std::endl;

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,LookupStart" << std::endl;

	for (int i = 0; i < ids.size(); i++) {

		auto nodeId = ids[i];
		
		repo::core::model::RepoBSONBuilder initialFilter;
		initialFilter.append("shared_id", nodeId);
		repo::core::model::RepoBSONBuilder initialProjection;
		initialProjection.append("parents", 1);
		initialProjection.append("_id", 0);
		
		auto node = handler->rawFindOne(database, collection, initialFilter.obj(), initialProjection.obj());
		
		repo::lib::RepoMatrix transform;
		while (node.hasField("parents")) {
			std::vector<repo::lib::RepoUUID> parents = node.getUUIDFieldArray("parents");

			repo::core::model::RepoBSONBuilder filter;
			filter.append("shared_id", parents[0]);
			repo::core::model::RepoBSONBuilder projectionParents;
			projectionParents.append("parents", 1);
			projectionParents.append("_id", 0);
			node = handler->rawFindOne(database, collection, filter.obj(), projectionParents.obj());

			repo::core::model::RepoBSONBuilder projectionMatrix;
			projectionMatrix.append("matrix", 1);
			projectionMatrix.append("_id", 0);
			auto matNode = handler->rawFindOne(database, collection, filter.obj(), projectionMatrix.obj());

			auto matrix = matNode.getMatrixField("matrix");
			transform = transform * matrix;
		}

		logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,Processed " << i << " out of " << ids.size() << std::endl;
	}

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,LookupEnd" << std::endl;
	logFile.close();
}

repo::core::model::RepoBSON convertToObjectMatrix(repo::lib::RepoMatrix matrix) {
	auto matData = matrix.getData();
	
	repo::core::model::RepoBSONBuilder row0;
	row0.append("00", matData[0]);
	row0.append("01", matData[1]);
	row0.append("02", matData[2]);
	row0.append("03", matData[3]);

	repo::core::model::RepoBSONBuilder row1;
	row1.append("10", matData[4]);
	row1.append("11", matData[5]);
	row1.append("12", matData[6]);
	row1.append("13", matData[7]);

	repo::core::model::RepoBSONBuilder row2;
	row2.append("20", matData[8]);
	row2.append("21", matData[9]);
	row2.append("22", matData[10]);
	row2.append("23", matData[11]);

	repo::core::model::RepoBSONBuilder row3;
	row3.append("30", matData[12]);
	row3.append("31", matData[13]);
	row3.append("32", matData[14]);
	row3.append("33", matData[15]);

	repo::core::model::RepoBSONBuilder matObj;
	matObj.append("row0", row0.obj());
	matObj.append("row1", row1.obj());
	matObj.append("row2", row2.obj());
	matObj.append("row3", row3.obj());

	return matObj.obj();
}

repo::lib::RepoMatrix convertToObjectMatrix(repo::core::model::RepoBSON matrix) {
	
	std::vector<float> data;

	auto row0 = matrix.getObjectField("row0");
	data.push_back(row0.getField("00").Double());
	data.push_back(row0.getField("01").Double());
	data.push_back(row0.getField("02").Double());
	data.push_back(row0.getField("03").Double());

	auto row1 = matrix.getObjectField("row1");
	data.push_back(row1.getField("10").Double());
	data.push_back(row1.getField("11").Double());
	data.push_back(row1.getField("12").Double());
	data.push_back(row1.getField("13").Double());

	auto row2 = matrix.getObjectField("row2");
	data.push_back(row2.getField("20").Double());
	data.push_back(row2.getField("21").Double());
	data.push_back(row2.getField("22").Double());
	data.push_back(row2.getField("23").Double());

	auto row3 = matrix.getObjectField("row3");
	data.push_back(row3.getField("30").Double());
	data.push_back(row3.getField("31").Double());
	data.push_back(row3.getField("32").Double());
	data.push_back(row3.getField("33").Double());
	
	return repo::lib::RepoMatrix(data);	
}

TEST(Sandbox, ConvertTransformsToObjMatrices) {
	auto handler = getLocalHandler();
	std::string database = "fthiel";

	// XXL Model
	std::string collection = "20523f34-f3d5-4cb6-9ebe-63b744b486b3.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("75FAFCC9-4AD4-488E-BCE2-35A4F8C7463C");

	std::vector<std::tuple<repo::lib::RepoUUID, repo::core::model::RepoBSON>> objMatrices;

	repo::core::model::RepoBSONBuilder filter;
	filter.append("type", "transformation");
	repo::core::model::RepoBSONBuilder projection;
	projection.append("shared_id", 1);
	projection.append("matrix", 1);
	projection.append("_id", 0);
	auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());

	for (auto doc : cursor) {
		auto bson = repo::core::model::RepoBSON(doc);
		auto sharedId = bson.getUUIDField("shared_id");
		auto matObj = convertToObjectMatrix(bson.getMatrixField("matrix"));
		
		repo::core::model::RepoBSONBuilder upFilter;
		upFilter.append("shared_id", sharedId);
		repo::core::model::RepoBSONBuilder newField;
		newField.append("matrixObj", matObj);
		repo::core::model::RepoBSONBuilder update;
		update.append("$set", newField.obj());

		handler->updateOne(database, collection, upFilter.obj(), update.obj());
	}
}

TEST(Sandbox, ConvertTransformsNodes) {
	auto handler = getLocalHandler();
	std::string database = "fthiel";

	// House model (small)
	//std::string collection = "1d08cae9-ba1f-4b40-b96e-6ad1a0059901.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("CDB96441-5A9D-4EB1-8439-6124B2DC5EEE");

	// "Medium" Model
	//std::string collection = "1a874c4b-891c-4586-864c-3bbb408a71d3.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("6E2F4803-34AA-4719-A797-693C842342D0");

	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");

	std::vector<std::tuple<repo::lib::RepoUUID, repo::core::model::RepoBSON>> objMatrices;

	repo::core::model::RepoBSONBuilder filter;
	filter.append("type", "transformation");
	repo::core::model::RepoBSONBuilder projection;
	projection.append("shared_id", 1);
	projection.append("matrix", 1);
	projection.append("parents", 1);
	projection.append("_id", 0);
	auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());

	for (auto doc : cursor) {
		auto bson = repo::core::model::RepoBSON(doc);
		auto sharedId = bson.getUUIDField("shared_id");
		auto matObj = convertToObjectMatrix(bson.getMatrixField("matrix"));



		repo::core::model::RepoBSONBuilder upFilter;
		upFilter.append("shared_id", sharedId);
		repo::core::model::RepoBSONBuilder newFields;
		newFields.append("matrixObj", matObj);

		if (bson.hasField("parents")) {
			auto parentsArray = bson.getUUIDFieldArray("parents");
			repo::lib::RepoUUID parentUUID = parentsArray[0];
			newFields.append("parent", parentUUID);		
		}

		repo::core::model::RepoBSONBuilder update;
		update.append("$set", newFields.obj());

		handler->updateOne(database, collection, upFilter.obj(), update.obj());
	}
}

// BVH TREE

TEST(Sandbox, PullAllBVHInfo) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// House model (small)
	//std::string collection = "1d08cae9-ba1f-4b40-b96e-6ad1a0059901.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("CDB96441-5A9D-4EB1-8439-6124B2DC5EEE");

	// "Medium" Model
	//std::string collection = "1a874c4b-891c-4586-864c-3bbb408a71d3.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("6E2F4803-34AA-4719-A797-693C842342D0");

	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");

	repo::core::model::RepoBSONBuilder filter;
	filter.append("rev_id", revId);
	filter.append("materialProperties.isOpaque", true);
	filter.append("primitive", 3);

	repo::core::model::RepoBSONBuilder projection;
	projection.append("_id", 0);
	projection.append("shared_id", 1);	
	projection.append("bounding_box", 1);
	projection.append("vertices_count", 1);
	projection.append("parents", 1);

	std::ofstream logFile;
	logFile.open("GetAllBVHInfo.log", std::ios_base::app);
	logFile << "timestamp,event" << std::endl;

	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,QueryStart" << std::endl;
	auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,QueryEnd" << std::endl;

	std::vector<repo::core::model::RepoBSON> bvhNodes;
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,IterationStart" << std::endl;
	for (auto doc : cursor) {
		auto bson = repo::core::model::RepoBSON(doc);
		bvhNodes.push_back(bson);
	}
	logFile << std::chrono::system_clock::now().time_since_epoch().count() << "00,IterationEnd" << std::endl;

	EXPECT_THAT(bvhNodes.size(), Eq(478546));
}

using Scalar = float;
using Bvh = bvh::Bvh<Scalar>;
using BvhVector3 = bvh::Vector3<Scalar>;

class MeshNodeLight {
	repo::lib::RepoUUID sharedId;
	std::uint32_t numVertices = 0;
	repo::lib::RepoUUID parent;
	repo::lib::RepoBounds bounds;

public:
	MeshNodeLight(repo::core::model::RepoBSON bson)
	{
		if (bson.hasField("shared_id")) {
			sharedId = bson.getUUIDField("shared_id");
		}
		if (bson.hasField("vertices_count")) {
			numVertices = bson.getIntField("vertices_count");
		}
		if (bson.hasField("parents")) {
			auto parents = bson.getUUIDFieldArray("parents");
			parent = parents[0];
		}
		if (bson.hasField("bounding_box")) {
			bounds = bson.getBoundsField("bounding_box");
		}
	}

	repo::lib::RepoUUID getSharedId() const {
		return sharedId;
	}

	std::uint32_t getNumVertices() const
	{
		return numVertices;
	}

	repo::lib::RepoBounds getBoundingBox() const
	{
		return bounds;
	}

	repo::lib::RepoUUID getParent() const
	{
		return parent;
	}

};

void traverseTree(const repo::core::model::RepoBSON root, const std::unordered_map<repo::lib::RepoUUID, std::vector<repo::core::model::RepoBSON>, repo::lib::RepoUUIDHasher> childNodeMap, std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher>& leafTransforms) {
	std::stack<repo::core::model::RepoBSON> bsonStack;
	std::stack<repo::lib::RepoMatrix> matStack;

	repo::lib::RepoMatrix identity;

	bsonStack.push(root);
	matStack.push(identity);

	while (!bsonStack.empty()) {
		auto topBson = bsonStack.top();
		bsonStack.pop();
		auto topMat = matStack.top();
		matStack.pop();

		auto nodeId = topBson.getUUIDField("shared_id");
		auto matrix = topBson.getMatrixField("matrix");

		auto newMat = matrix * topMat;

		if (childNodeMap.contains(nodeId)) {
			auto children = childNodeMap.at(nodeId);
			for (auto child : children) {
				bsonStack.push(child);
				matStack.push(newMat);
			}
		}
		else {
			leafTransforms.insert({ nodeId, newMat });
		}
	}
}

std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> getAllTransforms (
	std::shared_ptr<MongoDatabaseHandler> handler,
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId) {
	

	repo::core::model::RepoBSONBuilder filter;
	filter.append("rev_id", revId);
	filter.append("type", "transformation");

	repo::core::model::RepoBSONBuilder projection;
	projection.append("_id", 0);
	projection.append("shared_id", 1);
	projection.append("matrix", 1);	
	projection.append("parents", 1);
	


	auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());

	//TransformNode rootNode;
	repo::core::model::RepoBSON rootNode;
	std::unordered_map<repo::lib::RepoUUID, std::vector<repo::core::model::RepoBSON>, repo::lib::RepoUUIDHasher> childNodeMap;
	for (auto doc : cursor) {
		auto bson = repo::core::model::RepoBSON(doc);
		
		if (bson.hasField("parents")) {
			auto parentId = bson.getUUIDFieldArray("parents")[0];

			if (childNodeMap.contains(parentId)) {
				childNodeMap.at(parentId).push_back(bson);
			}
			else {
				auto children = std::vector<repo::core::model::RepoBSON>{ bson };
				childNodeMap.insert({ parentId, children });
			}
		}
		else {
			rootNode = bson;
		}
	}

	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> leafToTransformMap;
	traverseTree(rootNode, childNodeMap, leafToTransformMap);

	return leafToTransformMap;
}

void flattenBvhSandbox(
	const Bvh& bvh,
	std::vector<size_t>& leaves,
	std::vector<size_t>& branches
)
{
	std::queue<size_t> nodeQueue; // Using a queue instead of a stack means the child nodes are handled later, resulting in a breadth first traversal
	nodeQueue.push(0);
	do {
		auto index = nodeQueue.front();
		nodeQueue.pop();

		auto& node = bvh.nodes[index];

		if (node.is_leaf())
		{
			leaves.push_back(index);
		}
		else
		{
			nodeQueue.push(node.first_child_or_primitive);
			nodeQueue.push(node.first_child_or_primitive + 1);

			branches.push_back(index);
		}
	} while (!nodeQueue.empty());
}

// Gets the vertex counts of each node in the Bvh, when the Bvh primitives are
// MeshNodes.
std::vector<size_t> getVertexCounts(
	const Bvh& bvh,
	const std::vector<MeshNodeLight>& primitives
)
{	// To do this, get the nodes in list form, 'bottom up', in order to set the
	// vertex counts of each leaf node.

	std::vector<size_t> leaves;
	std::vector<size_t> flattened;
	flattenBvhSandbox(bvh, leaves, flattened);

	// Next initialise the vertex count of the leaves

	std::vector<size_t> vertexCounts;
	vertexCounts.resize(bvh.node_count);

	for (const auto nodeIndex : leaves)
	{
		size_t vertexCount = 0;
		auto& node = bvh.nodes[nodeIndex];
		for (int i = 0; i < node.primitive_count; i++)
		{
			auto primitiveIndex = bvh.primitive_indices[node.first_child_or_primitive + i];

			if (primitiveIndex < 0 || primitiveIndex >= primitives.size())
			{
				repoError << "Bvh primitive index out of range. This means something has gone wrong with the BVH construction.";
			}

			vertexCount += primitives[primitiveIndex].getNumVertices();
		}

		vertexCounts[nodeIndex] = vertexCount;
	}

	// Now, moving backwards, add up for each branch node the number of vertices in its children

	std::reverse(flattened.begin(), flattened.end());

	for (const auto nodeIndex : flattened)
	{
		auto& node = bvh.nodes[nodeIndex];
		vertexCounts[nodeIndex] = vertexCounts[node.first_child_or_primitive] + vertexCounts[node.first_child_or_primitive + 1];
	}

	return vertexCounts;
}

// Gets the branch nodes that contain fewer than REPO_MP_MAX_VERTEX_COUNT beneath
// them in total.
static const size_t REPO_MP_MAX_VERTEX_COUNT = 1200000;
std::vector<size_t> getSupermeshBranchNodesSandbox(
	const Bvh& bvh,
	std::vector<size_t> vertexCounts)
{
	std::vector<size_t> branchNodes;
	std::stack<size_t> nodeStack;
	nodeStack.push(0);
	do {
		auto index = nodeStack.top();
		nodeStack.pop();

		auto& node = bvh.nodes[index];
		auto numVertices = vertexCounts[index];

		// As we are traversing top-down, the vertex counts get smaller.
		// As soon as a node's count drops below the target, all the nodes in
		// that branch can become a mesh.

		if (numVertices < REPO_MP_MAX_VERTEX_COUNT || node.is_leaf()) // This node is the head of a branch that should become a group
		{
			branchNodes.push_back(index);
		}
		else
		{
			nodeStack.push(node.first_child_or_primitive);
			nodeStack.push(node.first_child_or_primitive + 1);
		}
	} while (!nodeStack.empty());

	return branchNodes;
}

std::vector<size_t> getBranchPrimitivesSandbox(
	const Bvh& bvh,
	size_t head
)
{
	std::vector<size_t> primitives;

	std::stack<size_t> nodeStack;
	nodeStack.push(head);
	do
	{
		auto node = bvh.nodes[nodeStack.top()];
		nodeStack.pop();

		if (node.is_leaf())
		{
			for (int i = 0; i < node.primitive_count; i++)
			{
				auto primitive = bvh.primitive_indices[node.first_child_or_primitive + i];
				primitives.push_back(primitive);
			}
		}
		else
		{
			nodeStack.push(node.first_child_or_primitive);
			nodeStack.push(node.first_child_or_primitive + 1);
		}
	} while (!nodeStack.empty());

	return primitives;
}

static const size_t REPO_MP_MAX_MESHES_IN_SUPERMESH = 5000;
void splitBigClustersSandbox(std::vector<std::vector<MeshNodeLight>>& clusters)
{
	auto clustersToSplit = std::vector<std::vector<MeshNodeLight>>();
	clusters.erase(std::remove_if(clusters.begin(), clusters.end(),
		[&](std::vector<MeshNodeLight> cluster)
		{
			if (cluster.size() > REPO_MP_MAX_MESHES_IN_SUPERMESH)
			{
				clustersToSplit.push_back(cluster);
				return true;
			}
			else
			{
				return false;
			}
		}),
		clusters.end()
	);

	std::vector<MeshNodeLight> cluster;
	for (auto const clusterToSplit : clustersToSplit)
	{
		int i = 0;
		while (i < clusterToSplit.size())
		{
			cluster.push_back(clusterToSplit[i++]);
			if (cluster.size() >= REPO_MP_MAX_MESHES_IN_SUPERMESH)
			{
				clusters.push_back(std::vector<MeshNodeLight>(cluster));
				cluster.clear();
			}
		}

		if (cluster.size())
		{
			clusters.push_back(std::vector<MeshNodeLight>(cluster));
		}
	}
}

TEST(Sandbox, PullBVHInfoAndBuildTree) {
	auto handler = getLocalHandler();

	std::string database = "fthiel";

	// House model (small)
	//std::string collection = "1d08cae9-ba1f-4b40-b96e-6ad1a0059901.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("CDB96441-5A9D-4EB1-8439-6124B2DC5EEE");

	// "Medium" Model
	//std::string collection = "1a874c4b-891c-4586-864c-3bbb408a71d3.scene";
	//repo::lib::RepoUUID revId = repo::lib::RepoUUID("6E2F4803-34AA-4719-A797-693C842342D0");

	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");

	// Get transforms first
	auto transformMap = getAllTransforms(handler, database, collection, revId);

	repo::core::model::RepoBSONBuilder filter;
	filter.append("rev_id", revId);
	filter.append("materialProperties.isOpaque", true);
	filter.append("primitive", 3);

	repo::core::model::RepoBSONBuilder projection;
	projection.append("_id", 0);
	projection.append("shared_id", 1);
	projection.append("bounding_box", 1);
	projection.append("vertices_count", 1);
	projection.append("parents", 1);

	auto cursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());

	std::vector<MeshNodeLight> nodes;
	std::vector<bvh::BoundingBox<Scalar>> boundingBoxes;

	for (auto doc : cursor) {
		auto bson = repo::core::model::RepoBSON(doc);
		auto node = MeshNodeLight(bson);		
		nodes.push_back(node);

		auto bounds = node.getBoundingBox();

		// Transform bounds
		auto parentId = node.getParent();
		if (!transformMap.contains(parentId))
			throw new std::invalid_argument("This should not happen");
		
		auto transMat = transformMap.at(node.getParent());
		auto newMinBound = transMat * bounds.min();
		auto newMaxBound = transMat * bounds.max();

		auto min = BvhVector3(newMinBound.x, newMinBound.y, newMinBound.z);
		auto max = BvhVector3(newMaxBound.x, newMaxBound.y, newMaxBound.z);
		boundingBoxes.push_back(bvh::BoundingBox<Scalar>(min, max));
	}

	auto centers = std::vector<BvhVector3>();
	for (const auto& bounds : boundingBoxes)
	{
		centers.push_back(bounds.center());
	}

	auto globalBounds = bvh::compute_bounding_boxes_union(boundingBoxes.data(), boundingBoxes.size());


	// CREATE BVH TREE

	Bvh bvh;
	bvh::SweepSahBuilder<Bvh> builder(bvh);
	builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());

	// The tree contains all the submesh bounds grouped in space.
	// Create a list of vertex counts for each node so we can decide where to
	// prune in order to build the clusters.
	auto vertexCounts = getVertexCounts(bvh, nodes);

	// Next, traverse the tree again, but this time depth first, cutting the tree
	// at places the vertex count drops below a target threshold.
	auto branchNodes = getSupermeshBranchNodesSandbox(bvh, vertexCounts);

	// Finally, get all the leaf nodes for each branch in order to build the
	// groups of MeshNodes
	std::vector<std::vector<MeshNodeLight>> clusters;
	for (const auto head : branchNodes)
	{
		std::vector<MeshNodeLight> cluster;
		for (const auto primitive : getBranchPrimitivesSandbox(bvh, head))
		{
			cluster.push_back(nodes[primitive]);
		}
		clusters.push_back(cluster);
	}

	// If clusters contain too many meshes, we can exceed the maximum BSON size.
	// Do a quick check and split any clusters that are too large.
	splitBigClustersSandbox(clusters);

	std::ofstream clusterFile;
	clusterFile.open("clusterFile.csv");

	for (auto cluster : clusters) {
		for (int i = 0; i < cluster.size(); i++) {
			const MeshNodeLight& node = cluster[i];

			if (i != 0) {
				clusterFile << ",";
			}

			clusterFile << node.getSharedId().toString();
		}

		clusterFile << std::endl;
	}

	clusterFile.close();

	//std::ofstream clusterLog;
	//clusterLog.open("clusterLog.csv");
	//clusterLog << "cluster index,noNodes,noUniqueFiles,clusterSize" << std::endl;

	//// Get cluster information
	//int clusterIndex = 0;
	//for (auto cluster : clusters) {
	//	std::vector<repo::lib::RepoUUID> sharedIds;
	//	for (const MeshNodeLight &node : cluster) {
	//		sharedIds.push_back(node.getSharedId());
	//	}

	//	repo::core::model::RepoBSONBuilder inClause;
	//	inClause.appendArray("$in", sharedIds);
	//	repo::core::model::RepoBSONBuilder infoFilter;
	//	infoFilter.append("shared_id", inClause.obj());

	//	repo::core::model::RepoBSONBuilder infoProjection;
	//	infoProjection.append("_id", 0);
	//	infoProjection.append("_blobRef", 1);

	//	
	//	auto cursorInfo = handler->rawCursorFind(database, collection, infoFilter.obj(), infoProjection.obj());
	//	
	//	std::set<std::string> uniqueFiles;
	//	for (auto doc : cursorInfo)
	//	{
	//		auto bson = repo::core::model::RepoBSON(doc);
	//		auto binRef = bson.getBinaryReference();
	//		auto fileName = binRef.getStringField("name");

	//		if (!uniqueFiles.contains(fileName))
	//			uniqueFiles.insert(fileName);
	//	}

	//	long long sizeSum = 0;
	//	for (auto fileName : uniqueFiles) {
	//		std::string refCollection = collection + ".ref";

	//		repo::core::model::RepoBSONBuilder refFilter;
	//		refFilter.append("_id", fileName);

	//		repo::core::model::RepoBSONBuilder refProj;
	//		refProj.append("_id", 0);
	//		refProj.append("size", 1);

	//		auto cursorRef = handler->rawCursorFind(database, refCollection, refFilter.obj(), refProj.obj());
	//		for (auto doc : cursorRef) {
	//			auto bson = repo::core::model::RepoBSON(doc);
	//			sizeSum += bson.getIntField("size");
	//		}
	//	}
	//	
	//	clusterLog << clusterIndex << "," << cluster.size() << "," << uniqueFiles.size() << "," << sizeSum << std::endl;
	//	clusterIndex++;
	//}

	//clusterLog.close();

	EXPECT_THAT(boundingBoxes.size(), Eq(478546));	
}

class GeomNode {
	repo::lib::RepoUUID sharedId;
	std::vector<uint8_t> bufferedData;

public: 
	GeomNode(repo::lib::RepoUUID sharedId, std::vector<uint8_t>& buffer)
	{
		this->sharedId = sharedId;
		this->bufferedData = std::vector<uint8_t>(buffer.begin(), buffer.end());
	}

	void setBuffer(const std::vector<uint8_t>& buffer)
	{
		this->bufferedData = std::vector<uint8_t>(buffer.begin(), buffer.end());
	}
};

TEST(Sandbox, GeomdataLegacy) {
	
	// Load cluster file
	std::ifstream clusterFile;
	clusterFile.open("clusterFile.csv");
	std::string line;
	std::vector <repo::lib::RepoUUID> nodes;
	while (std::getline(clusterFile, line)) {
		std::vector<std::string> strings;
		boost::split(strings, line, boost::is_any_of(","));

		std::vector<repo::lib::RepoUUID> uids;
		for (std::string stringId : strings) {
			nodes.push_back(repo::lib::RepoUUID(stringId));
		}		
	}

	EXPECT_THAT(nodes.size(), Eq(478836));

	std::shared_ptr<FileManager> fileManager;
	auto handler = getLocalHandler(fileManager);

	std::string database = "fthiel";
	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");

	fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

	std::vector<GeomNode> nodesWithGeom;
	for (auto uid : nodes) {
		repo::core::model::RepoBSONBuilder infoFilter;
		infoFilter.append("shared_id", uid);

		repo::core::model::RepoBSONBuilder infoProjection;
		infoProjection.append("_id", 0);
		infoProjection.append("_blobRef", 1);

		auto cursorInfo = handler->rawCursorFind(database, collection, infoFilter.obj(), infoProjection.obj());

		std::set<std::string> uniqueFiles;
		for (auto doc : cursorInfo)
		{
			auto bson = repo::core::model::RepoBSON(doc);
			auto binRef = bson.getBinaryReference();
			auto buffer = blobHandler.readToBuffer(fileservice::DataRef::deserialise(binRef));

			auto newNode = GeomNode(uid, buffer);
			nodesWithGeom.push_back(newNode);
		}
	}

	EXPECT_THAT(nodesWithGeom.size(), Eq(478836));
}

TEST(Sandbox, GeomdataPerCluster) {

	// Load cluster file
	std::ifstream clusterFile;
	clusterFile.open("clusterFile.csv");
	std::string line;
	std::vector < std::vector<repo::lib::RepoUUID>> clusters;
	while (std::getline(clusterFile, line)) {
		std::vector<std::string> strings;
		boost::split(strings, line, boost::is_any_of(","));

		std::vector<repo::lib::RepoUUID> uids;
		for (std::string stringId : strings) {
			uids.push_back(repo::lib::RepoUUID(stringId));
		}
		clusters.push_back(uids);
	}

	EXPECT_THAT(clusters.size(), Eq(607));

	std::shared_ptr<FileManager> fileManager;
	auto handler = getLocalHandler(fileManager);

	std::string database = "fthiel";
	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");

	fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

	for (auto cluster : clusters) {
		std::vector<GeomNode> nodesWithGeom;

		repo::core::model::RepoBSONBuilder inClause;
		inClause.appendArray("$in", cluster);
		repo::core::model::RepoBSONBuilder infoFilter;
		infoFilter.append("shared_id", inClause.obj());

		repo::core::model::RepoBSONBuilder infoProjection;
		infoProjection.append("_id", 0);
		infoProjection.append("shared_id", 1);
		infoProjection.append("_blobRef", 1);

		auto cursorInfo = handler->rawCursorFind(database, collection, infoFilter.obj(), infoProjection.obj());
		
		std::vector<repo::core::model::RepoBSON> nodeInfos;
		for (auto doc : cursorInfo)
		{
			auto bson = repo::core::model::RepoBSON(doc);
			nodeInfos.push_back(bson);
		}

		// separate loop to keep cursor separate from file operations and free it up as soon as possible
		for (auto bson : nodeInfos)
		{
			auto uid = bson.getUUIDField("shared_id");
			auto binRef = bson.getBinaryReference();
			auto buffer = blobHandler.readToBuffer(fileservice::DataRef::deserialise(binRef));

			auto newNode = GeomNode(uid, buffer);
			nodesWithGeom.push_back(newNode);
		}		
	}
}

TEST(Sandbox, GeomdataSingle) {
	// Load cluster file
	std::ifstream clusterFile;
	clusterFile.open("clusterFile.csv");
	std::string line;
	std::vector < std::vector<repo::lib::RepoUUID>> clusters;
	while (std::getline(clusterFile, line)) {
		std::vector<std::string> strings;
		boost::split(strings, line, boost::is_any_of(","));

		std::vector<repo::lib::RepoUUID> uids;
		for (std::string stringId : strings) {
			uids.push_back(repo::lib::RepoUUID(stringId));
		}
		clusters.push_back(uids);
	}

	EXPECT_THAT(clusters.size(), Eq(607));

	std::shared_ptr<FileManager> fileManager;
	auto handler = getLocalHandler(fileManager);

	std::string database = "fthiel";
	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");

	fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

	for (auto cluster : clusters) {
		repo::core::model::RepoBSONBuilder inClause;
		inClause.appendArray("$in", cluster);
		repo::core::model::RepoBSONBuilder infoFilter;
		infoFilter.append("shared_id", inClause.obj());

		repo::core::model::RepoBSONBuilder infoProjection;
		infoProjection.append("_id", 0);
		infoProjection.append("shared_id", 1);
		infoProjection.append("_blobRef", 1);

		auto cursorInfo = handler->rawCursorFind(database, collection, infoFilter.obj(), infoProjection.obj());

		std::vector<repo::core::model::RepoBSON> nodeInfos;
		for (auto doc : cursorInfo)
		{
			auto bson = repo::core::model::RepoBSON(doc);
			nodeInfos.push_back(bson);
		}

		// separate loop to keep cursor separate from file operations and free it up as soon as possible
		for (auto bson : nodeInfos)
		{
			auto uid = bson.getUUIDField("shared_id");
			auto binRef = bson.getBinaryReference();
			auto buffer = blobHandler.readToBuffer(fileservice::DataRef::deserialise(binRef));

			auto newNode = GeomNode(uid, buffer);			
		}
	}
}

class StreamingMeshNode {


	class SupermeshingData {

		repo::lib::RepoUUID uniqueId;

		// Geometry
		std::vector<repo::lib::RepoVector3D> vertices;
		std::vector<repo::lib::repo_face_t> faces;
		std::vector<repo::lib::RepoVector3D> normals;

	public:
		SupermeshingData(repo::core::model::RepoBSON& bson, std::vector<uint8_t>& buffer)
		{

			this->uniqueId = bson.getUUIDField("_id");

			deserialise(bson, buffer);
		}

		repo::lib::RepoUUID getUniqueId() const {
			return uniqueId;
		}

		std::uint32_t getNumFaces() const {
			return faces.size();
		}
		std::vector<repo::lib::repo_face_t>& getFaces()
		{
			return faces;
		}

		std::uint32_t getNumVertices() const {
			return vertices.size();
		}
		const std::vector<repo::lib::RepoVector3D>& getVertices() const {
			return vertices;
		}

		const std::vector<repo::lib::RepoVector3D>& getNormals() const
		{
			return normals;
		}

	private:
		void deserialise(
			repo::core::model::RepoBSON& bson,
			std::vector<uint8_t>& buffer)
		{
			auto blobRefBson = bson.getObjectField("_blobRef");
			auto elementsBson = blobRefBson.getObjectField("elements");

			if (elementsBson.hasField("vertices")) {
				auto vertBson = elementsBson.getObjectField("vertices");
				deserialiseVector(vertBson, buffer, vertices);
			}

			if (elementsBson.hasField("normals")) {
				auto vertBson = elementsBson.getObjectField("normals");
				deserialiseVector(vertBson, buffer, normals);
			}

			if (elementsBson.hasField("faces")) {

				int32_t faceCount = bson.getIntField("faces_count");
				faces.reserve(faceCount);

				std::vector<uint32_t> serialisedFaces = std::vector<uint32_t>();
				auto faceBson = elementsBson.getObjectField("faces");
				deserialiseVector(faceBson, buffer, serialisedFaces);

				// Retrieve numbers of vertices for each face and subsequent
				// indices into the vertex array.
				// In API level 1, mesh is represented as
				// [n1, v1, v2, ..., n2, v1, v2...]

				int mNumIndicesIndex = 0;
				while (serialisedFaces.size() > mNumIndicesIndex)
				{
					int mNumIndices = serialisedFaces[mNumIndicesIndex];
					if (serialisedFaces.size() > mNumIndicesIndex + mNumIndices)
					{
						repo::lib::repo_face_t face;
						face.resize(mNumIndices);
						for (int i = 0; i < mNumIndices; ++i)
							face[i] = serialisedFaces[mNumIndicesIndex + 1 + i];
						faces.push_back(face);
						mNumIndicesIndex += mNumIndices + 1;
					}
					else
					{
						repoError << "Cannot copy all faces. Buffer size is smaller than expected!";
					}
				}

			}
		}

		template <class T>
		void deserialiseVector(
			repo::core::model::RepoBSON& bson,
			std::vector<uint8_t>& buffer,
			std::vector<T>& vec)
		{
			auto start = bson.getIntField("start");
			auto size = bson.getIntField("size");

			vec.resize(size / sizeof(T));
			memcpy(vec.data(), buffer.data() + (sizeof(uint8_t) * start), size);
		}
	};



	repo::lib::RepoUUID sharedId;
	std::uint32_t numVertices = 0;
	repo::lib::RepoUUID parent;
	repo::lib::RepoBounds bounds;

	std::unique_ptr<SupermeshingData> supermeshingData;
	

public:
	StreamingMeshNode(repo::core::model::RepoBSON bson)
	{
		if (bson.hasField("shared_id")) {
			sharedId = bson.getUUIDField("shared_id");
		}
		if (bson.hasField("vertices_count")) {
			numVertices = bson.getIntField("vertices_count");
		}
		if (bson.hasField("parents")) {
			auto parents = bson.getUUIDFieldArray("parents");
			parent = parents[0];
		}
		if (bson.hasField("bounding_box")) {
			bounds = bson.getBoundsField("bounding_box");
		}
	}

	bool supermeshingDataLoaded() {
		return supermeshingData != nullptr;
	}

	void LoadSupermeshingData(repo::core::model::RepoBSON bson, std::vector<uint8_t>& buffer) {
		if (supermeshingDataLoaded())
		{
			// Throw warning
		}

		supermeshingData = std::make_unique<SupermeshingData>(bson, buffer);
	}

	void UnloadSupermeshingData() {
		supermeshingData.reset();
	}

	repo::lib::RepoUUID getSharedId() const {
		return sharedId;
	}

	std::uint32_t getNumVertices() const
	{
		return numVertices;
	}

	repo::lib::RepoBounds getBoundingBox() const
	{
		return bounds;
	}

	repo::lib::RepoUUID getParent() const
	{
		return parent;
	}

	// Requiring the supermeshing data to be loaded

	repo::lib::RepoUUID getUniqueId() {
		if (supermeshingDataLoaded()) {
			return supermeshingData->getUniqueId();
		}
		else {
			// Throw error
			throw std::exception("Supermesh data not loaded");
		}
	}

	std::uint32_t getNumFaces() {
		if (supermeshingDataLoaded()) {
			return supermeshingData->getNumFaces();
		}
		else {
			// Throw error
			throw std::exception("Supermesh data not loaded");
		}
	}

	std::vector<repo::lib::repo_face_t>& getFaces()
	{
		if (supermeshingDataLoaded()) {
			return supermeshingData->getFaces();
		}
		else {
			// Throw error			
			throw std::exception("Supermesh data not loaded");
		}
	}

	std::uint32_t getNumVertices() {
		if (supermeshingDataLoaded()) {
			return supermeshingData->getNumVertices();
		}
		else {
			// Throw error
			throw std::exception("Supermesh data not loaded");
		}
	}
	const std::vector<repo::lib::RepoVector3D>& getVertices() {
		if (supermeshingDataLoaded()) {
			return supermeshingData->getVertices();
		}
		else {
			// Throw error			
			throw std::exception("Supermesh data not loaded");
		}
	}

	const std::vector<repo::lib::RepoVector3D>& getNormals()
	{
		if (supermeshingDataLoaded()) {
			return supermeshingData->getNormals();
		}
		else {
			// Throw error			
			throw std::exception("Supermesh data not loaded");
		}
	}
};

TEST(Sandbox, StreamingMeshNodeTest) {

	std::shared_ptr<FileManager> fileManager;
	auto handler = getLocalHandler(fileManager);

	std::string database = "fthiel";
	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");

	// select mesh node with a lot of geometry
	repo::lib::RepoUUID nodeId = repo::lib::RepoUUID("42EAF9FE-6D5B-47DE-A248-CCB28EF7421E");

	// Get data for clustering phase first

	repo::core::handler::database::query::Eq filter1("shared_id", nodeId);
	
	repo::core::handler::database::query::RepoProjectionBuilder projection1;
	projection1.excludeField("_id");
	projection1.includeField("shared_id");
	projection1.includeField("bounding_box");
	projection1.includeField("vertices_count");
	projection1.includeField("parents");

	auto results1 = handler->findAllByCriteria(database, collection, filter1, projection1);
	auto bson1 = results1[0];

	// Create node with these original data
	StreamingMeshNode sNode = StreamingMeshNode(bson1);

	// This is where we measure the memory for the first time
	auto sid = sNode.getSharedId();


	// Now get data for supermeshing phase

	// Create filter
	auto filter2 = repo::core::handler::database::query::Eq("shared_id", nodeId);

	// Create projection
	repo::core::handler::database::query::RepoProjectionBuilder projection2;
	projection2.excludeField("_id");
	projection2.includeField("shared_id");
	projection2.includeField("vertices_count");
	projection2.includeField("faces_count");
	projection2.includeField("_blobRef");

	auto results2 = handler->findAllByCriteria(database, collection, filter2, projection2);
	auto bson2 = results2[0];

	fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);
	{
		auto binRef = bson2.getBinaryReference();
		auto buffer = blobHandler.readToBuffer(repo::core::handler::fileservice::DataRef::deserialise(binRef));
		sNode.LoadSupermeshingData(bson2, buffer);
	}
	// This is where we measure the memory for the second time
	int vertNum = sNode.getNumVertices();
	int faceNum = sNode.getNumFaces();
	EXPECT_THAT(vertNum, Eq(8529013));
	EXPECT_THAT(faceNum, Eq(2927089));

	// Now unload the geometry data again
	sNode.UnloadSupermeshingData();

	// This is where we measure the memory for the third time


	repo::lib::RepoUUID sid2 = sNode.getSharedId();

	Sleep(3000);

	repo::lib::RepoUUID sid3 = sNode.getSharedId();	
}

TEST(Sandbox, NewQueryTest) {
	std::shared_ptr<FileManager> fileManager;
	auto handler = getLocalHandler(fileManager);

	std::string database = "fthiel";
	// XXL Model
	std::string collection = "624bc24b-00b8-4434-ac21-e73fb819b529.scene";
	repo::lib::RepoUUID revId = repo::lib::RepoUUID("63752F30-0EB9-4090-8233-AE1B63B86E32");

	int rawCursorCount = 0;
	int newCursorCount = 0;

	// Get nodes the previous way
	{
		// Construct filter
		repo::core::model::RepoBSONBuilder filter;
		filter.append("rev_id", revId);
		filter.append("type", "transformation");

		// Construct projection
		repo::core::model::RepoBSONBuilder projection;
		projection.append("_id", 0);
		projection.append("shared_id", 1);
		projection.append("matrix", 1);
		projection.append("parents", 1);

		auto rawCursor = handler->rawCursorFind(database, collection, filter.obj(), projection.obj());
	
		for (auto doc : rawCursor) {
			rawCursorCount++;
		}
	}


	// Get nodes the "new and proper" way
	{
		repo::core::handler::database::query::RepoQueryBuilder filter;
		filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
		filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_TRANSFORMATION)));

		repo::core::handler::database::query::RepoProjectionBuilder projection;
		projection.excludeField(REPO_NODE_LABEL_ID);
		projection.includeField(REPO_NODE_LABEL_SHARED_ID);
		projection.includeField(REPO_NODE_LABEL_MATRIX);
		projection.includeField(REPO_NODE_LABEL_PARENTS);

		auto cursor = handler->findCursorByCriteria(database, collection, filter, projection);

		for (auto bson : (*cursor)) {
			newCursorCount++;
		}
	}

	EXPECT_THAT(rawCursorCount, Gt(0));
	EXPECT_THAT(newCursorCount, Gt(0));
	EXPECT_THAT(newCursorCount, Eq(rawCursorCount));
}