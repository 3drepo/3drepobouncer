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

using namespace repo::core::handler;
using namespace testing;

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
TEST(Sandbox, LinkedAggLeaves) {
	auto handler = getHandler();

	std::string database = "sandbox";
	std::string collection = "bakingTestCollection";

	// Clean collection first
	handler->dropCollection(database, collection);

	// Create DB with linked Entries
	LinkedSummary linkedEntries = fillDBWithLinkedEntries(handler, database, collection, 100);

	auto cursor = handler->getTransformThroughAggregation(database, collection, linkedEntries.leafNodeId);
	std::vector<repo::lib::RepoMatrix> matrices;
	for (auto document : (*cursor)) {
		// Get array "transforms"
		auto transforms = document.getMatrixFieldArray("transforms");

		// multiply matrices
		repo::lib::RepoMatrix matrix;

		for (auto transform : transforms)
		{
			matrix = matrix * transform;
		}

		// push final matrix into the vector
		matrices.push_back(matrix);
	}

	EXPECT_THAT(matrices.size(), 1);
	EXPECT_TRUE(matrices[0].equals(linkedEntries.fullTransform));
}

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