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

struct TreeSummary {
	repo::lib::RepoUUID rootNodeId;
	repo::lib::RepoUUID leafNodeId;
	repo::lib::RepoMatrix fullTransform;
};

repo::core::model::RepoBSON createRootEntry()
{
	auto id = repo::lib::RepoUUID::createUUID();
	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", id);	
	return builder.obj();
}

repo::core::model::RepoBSON createChildEntry(repo::lib::RepoUUID parentId)
{
	auto id = repo::lib::RepoUUID::createUUID();
	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", id);
	builder.append("parent", parentId);
	return builder.obj();
}

TreeSummary fillDBWithLinkedEntries(std::shared_ptr<MongoDatabaseHandler> handler, std::string database, std::string collection, int noEntries)
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

	TreeSummary summary;
	summary.rootNodeId = rootId;
	summary.leafNodeId = parentId;
	summary.fullTransform = fullTransform;
	return summary;
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

TEST(Sandbox, SandboxTest01)
{
	auto handler = getHandler();

	std::string database = "sandbox";
	std::string collection = "bakingTestCollection";

	// Clean collection first
	handler->dropCollection(database, collection);

	// Create DB with linked Entries
	TreeSummary tree = fillDBWithLinkedEntries(handler, database, collection, 100);

	// Check number of entries
	auto documents = handler->getAllFromCollectionTailable(database, collection);
	EXPECT_THAT(documents.size(), Eq(101));

	// Approach 1: Recursive
	repo::lib::RepoMatrix recursiveResult = calculateMatrixRecursively(handler, database, collection, tree.leafNodeId, repo::lib::RepoMatrix());
	EXPECT_TRUE(recursiveResult.equals(tree.fullTransform));

	// Approach 2: 
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
	auto cursor = handler->getTransformThroughAggregation(database, collection, tree.leafNodeId);
	std::vector<repo::lib::RepoMatrix> matrices;
	for (auto document : cursor) {
		// Get array "transforms"
		// multiply matrices
		// push final matrix into the vector
	}

	EXPECT_THAT(matrices.size(), 1);
	EXPECT_TRUE(matrices[0].equals(tree.fullTransform));
	
			
	
}