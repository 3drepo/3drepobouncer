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

#include <repo/core/handler/repo_database_handler_mongo.h>
#include <repo/core/handler/database/repo_query.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_element.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/handler/fileservice/repo_blob_files_handler.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include "../../../repo_test_matchers.h"
#include "../../../repo_test_utils.h"
#include "../../../repo_test_database_info.h"

#include <mongocxx/exception/exception.hpp>
#include <mongocxx/exception/operation_exception.hpp>

#include <thread>

using namespace repo::core::handler;
using namespace testing;

TEST(MongoDatabaseHandlerTest, CheckTestDatabase)
{
	// This 'test' checks for the existence of the golden collection. If it is
	// not found, rebuilds it, allowing it to be manually inspected and commited.
	// Afterwards however this immediately fails, preventing the suite from passing
	// in an automated context until the tests run against a database restored
	// using the proper mechanism.

	auto handler = getHandler();
	auto existing = handler->getCollections(REPO_GTEST_DBNAME4);
	if (!std::count(existing.begin(), existing.end(), std::string(REPO_GTEST_COLNAME_GOLDEN1)))
	{
		auto records = getGoldenData();

		using namespace repo::core::model;

		std::vector<RepoBSON> documents;
		for (auto& record : records)
		{
			RepoBSONBuilder builder;
			builder.append("_id", record._id);
			builder.append("name", record.name);
			builder.append("counter", record.counter);
			builder.appendLargeArray("myData1", record.myData1);
			builder.appendLargeArray("myData2", record.myData2);
			documents.push_back(builder.obj());
		}

		handler->insertManyDocuments(REPO_GTEST_DBNAME4, REPO_GTEST_COLNAME_GOLDEN1, documents);

		FAIL() << "Cannot load " << REPO_GTEST_DBNAME4 << "." << REPO_GTEST_COLNAME_GOLDEN1 << ". The dataset will be rebuilt but must be manually inspected and commited after.";
	}
}

TEST(MongoDatabaseHandlerTest, GetHandler)
{
	// Set a low timeout for the tests so we aren't waiting around too long for
	// them to fail!
	MongoDatabaseHandler::ConnectionOptions options;
	options.timeout = 1000;

	{
		auto handler = MongoDatabaseHandler::getHandler(
			REPO_GTEST_DBADDRESS,
			REPO_GTEST_DBPORT,
			REPO_GTEST_DBUSER,
			REPO_GTEST_DBPW,
			options
		);
		EXPECT_TRUE(handler);
		EXPECT_NO_THROW(handler->testConnection());
	}

	// The expected exception will be thrown after the serverSelectionTimeoutMS,
	// which is 5000. We expect the exception to accurately describe the problem
	// (that the database is unavailable).
	{
		auto handler = MongoDatabaseHandler::getHandler(
			"blah",
			REPO_GTEST_DBPORT,
			REPO_GTEST_DBUSER,
			REPO_GTEST_DBPW,
			options
		);
		EXPECT_TRUE(handler);
		EXPECT_THROW(handler->testConnection(), std::exception); // (We actually expect a nested RepoException subclass)
	}

	// The expected exception will be thrown after the serverSelectionTimeoutMS,
	// which is 5000.
	{
		auto handler = MongoDatabaseHandler::getHandler(
			REPO_GTEST_DBADDRESS,
			0001,
			REPO_GTEST_DBUSER,
			REPO_GTEST_DBPW,
			options
		);
		EXPECT_TRUE(handler);
		EXPECT_THROW(handler->testConnection(), std::exception);
	}

	// In this case the exception should say that the authentication has failed
	{
		auto handler = MongoDatabaseHandler::getHandler(
			REPO_GTEST_DBADDRESS,
			REPO_GTEST_DBPORT,
			"invaliduser",
			REPO_GTEST_DBPW,
			options
		);
		EXPECT_TRUE(handler);
		EXPECT_THROW(handler->testConnection(), std::exception);
	}

	// This case tests when we provide a connection string, including existing
	// options. The test should succeed.
	{
		// This connection string includes an option, but expects the username and
		// password to be provided.
		std::string connectionString = "mongodb://" + REPO_GTEST_DBADDRESS + ":" + std::to_string(REPO_GTEST_DBPORT) + "/?authSource=admin";
		auto handler = MongoDatabaseHandler::getHandler(
			connectionString,
			REPO_GTEST_DBUSER,
			REPO_GTEST_DBPW,
			options
		);
		EXPECT_TRUE(handler);
		EXPECT_NO_THROW(handler->testConnection());
	}

	// This case tests when we provide a connection string, including existing
	// options. The test should succeed.
	{
		// This connection string includes the username and password,
		// so those in the config should be ignored.
		std::string connectionString = "mongodb://" + REPO_GTEST_DBUSER + ":" + REPO_GTEST_DBPW + "@" + REPO_GTEST_DBADDRESS + ":" + "27017" + "/?authSource=admin";
		auto handler = MongoDatabaseHandler::getHandler(
			connectionString,
			"invalidUser",
			"invalidPassword",
			options
		);
		EXPECT_TRUE(handler);
		EXPECT_NO_THROW(handler->testConnection());
	}
}

// Equality operator for use by GetAllFromCollectionTailable

bool operator== (const repo::core::model::RepoBSON& a, const  GoldenDocument& b)
{
	return
		a.getUUIDField("_id") == b._id &&
		a.getIntField("counter") == b.counter &&
		a.getStringField("name") == b.name &&
		a.getBinary("myData1") == b.myData1 &&
		a.getBinary("myData2") == b.myData2;
}

// A set of matchers dedicated to the GetAllFromCollectionTailable test
// until ranges are supported in gmock,
// https://github.com/google/googletest/issues/4512

MATCHER_P(GetAllFromCollectionTailable_CounterIsIncreasing, start, "")
{
	for (auto& b : arg)
	{
		auto counter = b.getIntField("counter");
		if (counter < *start) {
			return false;
		}
		*start = counter;
	}
	return true;
}

MATCHER_P(GetAllFromCollectionTailable_CounterIsDecreasing, start, "")
{
	for (auto& b : arg)
	{
		auto counter = b.getIntField("counter");
		if (counter > *start) {
			return false;
		}
		*start = counter;
	}
	return true;
}

void populateBinaryData(
	MongoDatabaseHandler* handler,
	std::string db,
	std::string col,
	std::vector<repo::core::model::RepoBSON>& bsons
) {
	repo::core::handler::fileservice::BlobFilesHandler blobHandler(handler->getFileManager(), db, col);

	for (auto& bson : bsons) {
		if (bson.hasFileReference()) {
			auto ref = bson.getBinaryReference();
			auto buffer = blobHandler.readToBuffer(fileservice::DataRef::deserialise(ref));
			bson.initBinaryBuffer(buffer);
		}
	}
}

void populateBinaryData(
	MongoDatabaseHandler* handler,
	std::string db,
	std::string col,
	repo::core::model::RepoBSON& bson
) {
	repo::core::handler::fileservice::BlobFilesHandler blobHandler(handler->getFileManager(), db, col);
	
	if (bson.hasFileReference()) {
		auto ref = bson.getBinaryReference();
		auto buffer = blobHandler.readToBuffer(fileservice::DataRef::deserialise(ref));
		bson.initBinaryBuffer(buffer);
	}
}

TEST(MongoDatabaseHandlerTest, GetAllFromCollectionTailable)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	auto goldenData = getGoldenData();

	std::unordered_map<repo::lib::RepoUUID, GoldenDocument, repo::lib::RepoUUIDHasher> lookup;
	for (auto d : goldenData) {
		lookup[d._id] = d;
	}

	auto db = REPO_GTEST_DBNAME4;
	auto col = REPO_GTEST_COLNAME_GOLDEN1;

	// Check that we can get all the documents, intact, including restoring the
	// binary buffers.
	// As there are a limited number of documents, we should have no problem
	// getting all of them in one call.

	// Note that, as a rule, the insertion order is left up to the database
	// handler (to choose what is most performant), so the first set of tests
	// must not assume the return order is the same as the golden set.

	{
		auto bsons = handler->getAllFromCollectionTailable(db, col);
		populateBinaryData(handler.get(), db, col, bsons);
		EXPECT_THAT(bsons, UnorderedElementsAreArray(goldenData));
	}

	// Test the skipping and limiting behaviour
	{
		auto bsons1 = handler->getAllFromCollectionTailable(db, col, 0, 25);
		auto bsons2 = handler->getAllFromCollectionTailable(db, col, 25, 25);
		populateBinaryData(handler.get(), db, col, bsons1);
		populateBinaryData(handler.get(), db, col, bsons2);

		// Sets should be disjoint to eachother

		EXPECT_THAT(bsons1, Not(AnyElementsOverlap(bsons2)));

		// But both be subsets of the golden data

		EXPECT_THAT(bsons1, IsSubsetOf(goldenData));
		EXPECT_THAT(bsons2, IsSubsetOf(goldenData));

		// And the combined set should match exactly

		std::vector<repo::core::model::RepoBSON> combined;
		std::copy(bsons1.begin(), bsons1.end(), std::back_inserter(combined));
		std::copy(bsons2.begin(), bsons2.end(), std::back_inserter(combined));
		EXPECT_THAT(combined, UnorderedElementsAreArray(goldenData));
	}

	// Reading over the end shouldn't result in an error
	{
		auto bsons = handler->getAllFromCollectionTailable(db, col, 40, 100);
		populateBinaryData(handler.get(), db, col, bsons);
		EXPECT_THAT(bsons.size(), Eq(10));
		EXPECT_THAT(bsons, IsSubsetOf(goldenData));
	}


	// Test the projection.
	// The behaviour of Mongo is that _id is always returned, so there is no
	// need to specify it explicitly.
	{
		auto projected = handler->getAllFromCollectionTailable(db, col, 0, 0, { "name" });

		// Check that only the field names (and _id) requested have been returned

		EXPECT_THAT(projected, Each(GetFieldNames(UnorderedElementsAre("_id", "name"))));

		// And that the names are associated with the correct id

		for (auto& b : projected)
		{
			EXPECT_THAT(b.getStringField("name"), Eq(lookup[b.getUUIDField("_id")].name));
		}

		// We shouldn't have any binaries, because we didn't include the _blob field in
		// the projection (currently, this is not automatically included, though it could
		// be in the future).

		EXPECT_THAT(projected, Each(Not(HasBinaryFields())));
	}

	// Projection with multiple field names
	{
		auto projected = handler->getAllFromCollectionTailable(db, col, 0, 0, { "name", "counter" });
		EXPECT_THAT(projected, Each(GetFieldNames(UnorderedElementsAre("_id", "name", "counter"))));
		for (auto& b : projected)
		{
			EXPECT_THAT(b.getStringField("name"), Eq(lookup[b.getUUIDField("_id")].name));
			EXPECT_THAT(b.getIntField("counter"), Eq(lookup[b.getUUIDField("_id")].counter));
		}
	}

	// Test sort
	{
		auto sorted = handler->getAllFromCollectionTailable(db, col, 0, 0, {}, "counter", 1);
		populateBinaryData(handler.get(), db, col, sorted);

		// Counter should be increasing

		auto previous = 0;
		EXPECT_THAT(sorted, GetAllFromCollectionTailable_CounterIsIncreasing(&previous));

		// And the other fields should match

		EXPECT_THAT(sorted, UnorderedElementsAreArray(goldenData));
	}

	// Sort should also work with both skip and limit
	{
		auto previous = 0;

		auto sorted1 = handler->getAllFromCollectionTailable(db, col, 0, 10, {}, "counter", 1);
		EXPECT_THAT(sorted1, GetAllFromCollectionTailable_CounterIsIncreasing(&previous));

		auto sorted2 = handler->getAllFromCollectionTailable(db, col, 10, 10, {}, "counter", 1);
		EXPECT_THAT(sorted2, GetAllFromCollectionTailable_CounterIsIncreasing(&previous));

		auto sorted3 = handler->getAllFromCollectionTailable(db, col, 20, 10, {}, "counter", 1);
		EXPECT_THAT(sorted3, GetAllFromCollectionTailable_CounterIsIncreasing(&previous));

		auto sorted4 = handler->getAllFromCollectionTailable(db, col, 30, 100, {}, "counter", 1); // And over the end
		EXPECT_THAT(sorted4, GetAllFromCollectionTailable_CounterIsIncreasing(&previous));
	}

	// Check that sort order works
	{
		auto previous = INT_MAX;
		auto sorted = handler->getAllFromCollectionTailable(db, col, 0, 0, {}, "counter",  -1);
		EXPECT_THAT(sorted, GetAllFromCollectionTailable_CounterIsDecreasing(&previous));
	}

	{
		auto documents = handler->getAllFromCollectionTailable(db, "blah");
		EXPECT_THAT(documents, IsEmpty());
	}
}

TEST(MongoDatabaseHandlerTest, GetCollections)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	auto goldenData = getCollectionList(REPO_GTEST_DBNAME1);
	auto collections = handler->getCollections(REPO_GTEST_DBNAME1);

	EXPECT_THAT(collections, UnorderedElementsAreArray(goldenData));

	goldenData = getCollectionList(REPO_GTEST_DBNAME2);
	collections = handler->getCollections(REPO_GTEST_DBNAME2);

	EXPECT_THAT(collections, UnorderedElementsAreArray(goldenData));

	// Nonexistent databases should return an empty array, but invalid database
	// names are not supported arguments.

	EXPECT_THAT(handler->getCollections(""), IsEmpty());
	EXPECT_THAT(handler->getCollections("blahblah"), IsEmpty());
}

TEST(MongoDatabaseHandlerTest, DropCollection)
{
	auto handler = getHandler();

	handler->dropCollection(REPO_GTEST_DROPCOL_TESTCASE.first, REPO_GTEST_DROPCOL_TESTCASE.second);
	EXPECT_THAT(handler->getCollections(REPO_GTEST_DROPCOL_TESTCASE.first), Not(IsSupersetOf({ REPO_GTEST_DROPCOL_TESTCASE.second })));

	// Performing the same operation should succeed, as a noop
	handler->dropCollection(REPO_GTEST_DROPCOL_TESTCASE.first, REPO_GTEST_DROPCOL_TESTCASE.second);
	EXPECT_THAT(handler->getCollections(REPO_GTEST_DROPCOL_TESTCASE.first), Not(IsSupersetOf({ REPO_GTEST_DROPCOL_TESTCASE.second })));

	// Invalid names are the same as addressing an empty database or collection

	EXPECT_NO_THROW(handler->dropCollection(REPO_GTEST_DROPCOL_TESTCASE.first, ""));
	EXPECT_NO_THROW(handler->dropCollection("", REPO_GTEST_DROPCOL_TESTCASE.second));
}

MATCHER_P(DropDocument_UnorderedNamesAre, names, "")
{
	std::vector<std::string> names;
	std::transform(arg.begin(), arg.end(), std::back_inserter(names), [](const repo::core::model::RepoBSON& b) {
		return b.getStringField("name");
	});
	return ExplainMatchResult(UnorderedElementsAreArray(names), names, result_listener);
}

TEST(MongoDatabaseHandlerTest, DropDocument)
{
	auto handler = getHandler();

	auto db = REPO_GTEST_DBNAME4;
	auto col = "dropCollection";

	// These names are also the document Ids (where the UUIDs are actual UUID types)

	auto names = std::vector<std::string>({
		"95577a50-1632-4c4c-90bd-e32087efba6a",
		"myDocument",
		"78f52f96-4208-48bf-8003-b38ecb70209c",
		"myDocument2"
	});

	// Check the collection is intact
	EXPECT_THAT(handler->getAllFromCollectionTailable(db, col), DropDocument_UnorderedNamesAre(names));

	{
		repo::core::model::RepoBSONBuilder builder;
		builder.append("_id", repo::lib::RepoUUID("95577a50-1632-4c4c-90bd-e32087efba6a"));
		auto q = builder.obj();

		handler->dropDocument(q, db, col);

		// The rest of the collection is intact

		std::remove(names.begin(), names.end(), "95577a50-1632-4c4c-90bd-e32087efba6a");
		EXPECT_THAT(handler->getAllFromCollectionTailable(db, col), DropDocument_UnorderedNamesAre(names));

		// Dropping a document a second time is a noop

		handler->dropDocument(q, db, col);
		EXPECT_THAT(handler->getAllFromCollectionTailable(db, col), DropDocument_UnorderedNamesAre(names));
	}

	// Providing a document without an Id should result in exception
	EXPECT_THROW(handler->dropDocument(repo::core::model::RepoBSON(), db, col), repo::lib::RepoException);

	// Check dropping documents works when Id is a string
	{
		repo::core::model::RepoBSONBuilder builder;
		builder.append("_id", "myDocument");
		auto q = builder.obj();

		handler->dropDocument(q, db, col);

		std::remove(names.begin(), names.end(), "myDocument");
		EXPECT_THAT(handler->getAllFromCollectionTailable(db, col), DropDocument_UnorderedNamesAre(names));

		// Dropping a document a second time is a noop

		handler->dropDocument(q, db, col);
		EXPECT_THAT(handler->getAllFromCollectionTailable(db, col), DropDocument_UnorderedNamesAre(names));

	}

	// Invalid names are a noop to match the behaviour of calling drop on a non-
	// existent collection
	{
		repo::core::model::RepoBSONBuilder builder;
		auto q = builder.obj();

		EXPECT_NO_THROW(handler->dropDocument(q, db, ""));
		EXPECT_NO_THROW(handler->dropDocument(q, "", col));
	}
}

TEST(MongoDatabaseHandlerTest, InsertDocument)
{
	auto handler = getHandler();

	std::string database = "sandbox";
	std::string collection = "sbCollection";

	// Create a random document to insert
	auto id = repo::lib::RepoUUID::createUUID();
	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", id);
	builder.append("myField", "myValue");
	auto bson = builder.obj();

	handler->insertDocument(database, collection, bson);

	// Can we get the document back?

	EXPECT_THAT(handler->findOneByUniqueID(database, collection, id), Eq(bson));

	// Invalid names will result in an exception
	EXPECT_THROW(handler->insertDocument("", collection, repo::core::model::RepoBSON()), std::exception);
	EXPECT_THROW(handler->insertDocument(database, "", repo::core::model::RepoBSON()), std::exception);
}

// The FindBy tests use a set of imports in the testMongoDatabaseHandler database.
// A number of these documents have flags manually set which uniquely identify them
// and ensure that there are no false positives.

TEST(MongoDatabaseHandlerTest, FindAllByCriteria)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	using namespace repo::core::handler::database;

	query::Eq search("type", std::string("mesh"));
	auto results = handler->findAllByCriteria(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ + ".scene", query::Eq("type", std::string("mesh")));

	EXPECT_EQ(4, results.size());

	EXPECT_THAT(handler->findAllByCriteria(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ + ".scene", query::RepoQueryBuilder()), IsEmpty());
	EXPECT_THAT(handler->findAllByCriteria("", REPO_GTEST_DBNAME1_PROJ + ".scene", search), IsEmpty());
	EXPECT_THAT(handler->findAllByCriteria(REPO_GTEST_DBNAME1, "", search), IsEmpty());
}

// The implementation of this is defined in repo_database_handler_mongo.cpp.
// It is not intended to be used outside that module, but this being that
// modules unit test is a special case.
repo::core::model::RepoBSON REPO_API_EXPORT makeQueryFilterDocument(const repo::core::handler::database::query::RepoQuery& query);

TEST(MongoDatabaseHandlerTest, FindOneByCriteria)
{
	auto handler = getHandler();
	auto db = REPO_GTEST_DBNAME4;

	using namespace repo::core::handler::database;

	// Use the query types to return a document from the middle of a set by id
	{
		auto col = "cube.scene";
		auto id = repo::lib::RepoUUID("c9bed762-9b63-4307-824d-40261ce7f022");
		query::Eq search("_id", id);
		auto result = handler->findOneByCriteria(db, col, search);
		populateBinaryData(handler.get(), db, col, result);

		EXPECT_THAT(result.getStringField("type"), Eq("mesh"));
		EXPECT_THAT(result.getBinary("faces"), Not(IsEmpty()));
	}

	// Query by id as a string
	{
		auto col = "cube.scene";
		auto id = std::string("9ecbebbc-c24d-4da5-b69b-388abfcfe314");
		query::Eq search("_id", id);
		auto result = handler->findOneByCriteria(db, col, search);
		populateBinaryData(handler.get(), db, col, result);

		EXPECT_THAT(result.getStringField("type"), Eq("mesh"));
		EXPECT_THAT(result.getBinary("faces"), Not(IsEmpty()));
	}

	// Check the sort field
	{
		auto col = "cube.history";
		auto id = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH);
		query::Eq search("shared_id", id);
		auto result = handler->findOneByCriteria(db, col, search, "timestamp");

		EXPECT_THAT(result.getUUIDField("_id"), Eq(repo::lib::RepoUUID("57f09047-ead5-453b-96ab-d90a582127bf")));
	}

	// Use of Or operator - should return the document with the highest timestamp
	// of the two provided
	{
		query::RepoQuery q = query::Or(
			query::Eq("_id", repo::lib::RepoUUID("3a28e39d-e901-4ca7-9453-9b9dde38d916")),
			query::Eq("_id", repo::lib::RepoUUID("e664b837-45aa-4ad3-b2da-6efcce6438e2"))
		);
		repo::core::model::RepoBSON bson = makeQueryFilterDocument(q);
		auto s = bson.toString();

		auto col = "cube.history";
		auto result = handler->findOneByCriteria(db, col,
			query::Or(
				query::Eq("_id", repo::lib::RepoUUID("3a28e39d-e901-4ca7-9453-9b9dde38d916")),
				query::Eq("_id", repo::lib::RepoUUID("e664b837-45aa-4ad3-b2da-6efcce6438e2"))
			),
			"timestamp");

		EXPECT_THAT(result.getUUIDField("_id"), Eq(repo::lib::RepoUUID("3a28e39d-e901-4ca7-9453-9b9dde38d916")));
	}

	// Whether a field exists
	{
		auto col = "cube.history";
		auto result1 = handler->findOneByCriteria(db, col,
			query::Exists("x", true),
			"timestamp");

		EXPECT_THAT(result1.getUUIDField("_id"), Eq(repo::lib::RepoUUID("57f09047-ead5-453b-96ab-d90a582127bf")));

		auto result2 = handler->findOneByCriteria(db, col,
			query::Exists("x", false),
			"timestamp");

		EXPECT_THAT(result2.getUUIDField("_id"), Eq(repo::lib::RepoUUID("0d35ee3d-03a8-433a-922a-ec2488d2aa90")));
	}

	// Composite queries
	{
		auto col = "cube.history";
		auto query = query::RepoQueryBuilder();

		query.append(query::Eq("shared_id", repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH)));
		EXPECT_THAT(
			handler->findOneByCriteria(db, col, query, "timestamp").getUUIDField("_id"),
			Eq(repo::lib::RepoUUID("57f09047-ead5-453b-96ab-d90a582127bf"))
		);

		query.append(query::Exists("incomplete", false));
		EXPECT_THAT(
			handler->findOneByCriteria(db, col, query, "timestamp").getUUIDField("_id"),
			Eq(repo::lib::RepoUUID("460cf713-faa8-41a9-9680-e2f659b0aeda"))
		);

		query.append(query::Exists("x", true));
		EXPECT_THAT(
			handler->findOneByCriteria(db, col, query, "timestamp").getUUIDField("_id"),
			Eq(repo::lib::RepoUUID("e664b837-45aa-4ad3-b2da-6efcce6438e2"))
		);
	}

	{
		auto col = "cube.history";
		auto query = query::RepoQueryBuilder();

		query.append(query::Or(
			query::Exists("incomplete", false),
			query::Eq(std::string("incomplete"), std::vector<int>({ 4, 5 }))
		));
		EXPECT_THAT(
			handler->findOneByCriteria(db, col, query, "timestamp").getUUIDField("_id"),
			Eq(repo::lib::RepoUUID("57f09047-ead5-453b-96ab-d90a582127bf"))
		);

		query.append(query::Exists("x", false));
		EXPECT_THAT(
			handler->findOneByCriteria(db, col, query, "timestamp").getUUIDField("_id"),
			Eq(repo::lib::RepoUUID("0d35ee3d-03a8-433a-922a-ec2488d2aa90"))
		);

		query.append(query::Exists("y", true));
		EXPECT_THAT(
			handler->findOneByCriteria(db, col, query, "timestamp").getUUIDField("_id"),
			Eq(repo::lib::RepoUUID("460cf713-faa8-41a9-9680-e2f659b0aeda"))
		);
	}
}

TEST(MongoDatabaseHandlerTest, FindOneByUniqueID)
{
	auto handler = getHandler();
	auto db = REPO_GTEST_DBNAME4;
	auto col = "cube.scene";

	// Find a document from the middle of a set based on the Id as a UUID
	{
		auto id = repo::lib::RepoUUID("f6d60d2c-5e78-4725-86c1-05b2fcff44fa");
		auto document = handler->findOneByUniqueID(db, col, id);
		populateBinaryData(handler.get(), db, col, document);

		EXPECT_THAT(document.getUUIDField("_id"), Eq(id));
		EXPECT_THAT(document.getStringField("x"), Eq("document4"));
		EXPECT_THAT(document.getBinary("faces"), Not(IsEmpty())); // Check that the binary data is loaded too
	}

	// Find a document from the middle of a set based on the Id as a string
	{
		auto id = "9ecbebbc-c24d-4da5-b69b-388abfcfe314";
		auto document = handler->findOneByUniqueID(db, col, id);
		populateBinaryData(handler.get(), db, col, document);

		EXPECT_THAT(document.getStringField("_id"), Eq(id));
		EXPECT_THAT(document.getStringField("x"), Eq("document6"));
		EXPECT_THAT(document.getBinary("faces"), Not(IsEmpty()));
	}

	// Searching for a non-existent Id should return an empty document
	{
		auto document = handler->findOneByUniqueID(db, col, repo::lib::RepoUUID("2f0036f2-9eae-4f41-aad9-c8c55abdb63e"));
		EXPECT_THAT(document.isEmpty(), IsTrue());
	}
}

TEST(MongoDatabaseHandlerTest, FindOneBySharedID)
{
	auto handler = getHandler();
	auto db = REPO_GTEST_DBNAME4;
	auto col = "cube.history";

	// Find a document from the middle of a set based on it's unique shared Id
	{
		auto document = handler->findOneBySharedID(db, col, repo::lib::RepoUUID("c4e48663-d9a0-44e6-afb2-489c3f336833"), "timestamp");
		EXPECT_THAT(document.getUUIDField("_id"), Eq(repo::lib::RepoUUID("3a28e39d-e901-4ca7-9453-9b9dde38d916")));
		EXPECT_THAT(document.getStringField("x"), Eq("document1"));
	}

	// Finding a shared UUID based on field ordering. The sort field by default
	// should be descending, so it should be the last document in the collection
	{
		auto document = handler->findOneBySharedID(db, col, repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), "timestamp");
		EXPECT_THAT(document.getUUIDField("_id"), Eq(repo::lib::RepoUUID("57f09047-ead5-453b-96ab-d90a582127bf")));
	}

	// Searching for a non-existent Id should return an empty document
	{
		auto document = handler->findOneBySharedID(db, col, repo::lib::RepoUUID("2f0036f2-9eae-4f41-aad9-c8c55abdb63e"), "timestamp");
		EXPECT_THAT(document.isEmpty(), IsTrue());
	}
}

TEST(MongoDatabaseHandlerTest, UpsertDocument)
{
	auto handler = getHandler();

	auto collection = "upsertTestCollection";

	// Add a couple of documents to the empty collection; these should
	// never be changed by the upsert operations.

	auto doc1 = makeRandomRepoBSON(0, 0, 0);
	auto doc2 = makeRandomRepoBSON(1, 0, 0);

	handler->insertManyDocuments(REPO_GTEST_DBNAME3, collection, { doc1, doc2 });

	// Attempt to update an existing document using a partial BSON

	auto id = repo::lib::RepoUUID::createUUID();

	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", id);
	builder.append("field1", "myString");
	builder.append("field2", 0);
	auto existing = builder.obj();

	handler->insertDocument(REPO_GTEST_DBNAME3, collection, existing);

	// Reading the document back, it should be identical

	auto documents = handler->getAllFromCollectionTailable(REPO_GTEST_DBNAME3, collection);

	EXPECT_THAT(documents, UnorderedElementsAre(existing, doc1, doc2));

	// Create a partial document that adds a field, and updates a field

	auto auuid = repo::lib::RepoUUID::createUUID();

	repo::core::model::RepoBSONBuilder updateBuilder;
	updateBuilder.append("_id", id);
	updateBuilder.append("field2", 123);
	updateBuilder.append("field3", auuid);
	auto update = updateBuilder.obj();

	handler->upsertDocument(REPO_GTEST_DBNAME3, collection, update, false); // Should use _id as the primary key

	documents = handler->getAllFromCollectionTailable(REPO_GTEST_DBNAME3, collection);

	// There should still only be three documents in the collection, because the
	// document should have been updated in place.

	EXPECT_THAT(documents.size(), Eq(3));

	// This time the document should be a combination of both previous BSONs

	repo::core::model::RepoBSONBuilder expectedBuilder;
	expectedBuilder.append("_id", id); // Id (must be) shared by both
	expectedBuilder.append("field1", "myString"); // Original document field, which is unchanged
	expectedBuilder.append("field2", 123); // Updated field value changed
	expectedBuilder.append("field3", auuid); // Field that only exists on update
	auto expected = expectedBuilder.obj();

	EXPECT_THAT(documents, UnorderedElementsAre(expected, doc1, doc2));

	// If overwrite is set to true, then the new document should displace the
	// old one, meaning any fields missing in the new document should be
	// removed.

	handler->upsertDocument(REPO_GTEST_DBNAME3, collection, update, true);
	documents = handler->getAllFromCollectionTailable(REPO_GTEST_DBNAME3, collection);

	EXPECT_THAT(documents, UnorderedElementsAre(update, doc1, doc2));
}

TEST(MongoDatabaseHandlerTest, UpsertDocumentBinary)
{
	// Upserting is not supported for BSONS with bin mappings

	auto handler = getHandler();
	repo::core::model::RepoBSONBuilder builder;
	builder.appendLargeArray("bin", testing::makeRandomBinary());

	EXPECT_THROW({
		handler->upsertDocument(REPO_GTEST_DBNAME3, "sbCollection", builder.obj(), false);
	},
	repo::lib::RepoException);
}

TEST(MongoDatabaseHandlerTest, InsertDocumentBinary)
{
	// Inserting is not supported for BSONS with bin mappings

	auto handler = getHandler();
	repo::core::model::RepoBSONBuilder builder;
	builder.appendLargeArray("bin", testing::makeRandomBinary());

	EXPECT_THROW({
		handler->insertDocument(REPO_GTEST_DBNAME3, "sbCollection", builder.obj());
	},
	repo::lib::RepoException);
}

// This special matcher ignores the _blobRef entry when comparing the BSON
// parts of two documents, but does consider the big files.

MATCHER(InsertManyDocumentsBinaryBSONMatcher, "") {
	const repo::core::model::RepoBSON& a = std::get<0>(arg);
	const repo::core::model::RepoBSON& b = std::get<1>(arg);

	std::set<std::string> fields;
	for (auto f : a.getFieldNames())
	{
		fields.insert(f);
	}
	for (auto f : b.getFieldNames())
	{
		fields.insert(f);
	}
	fields.erase("_blobRef");

	for (auto f : fields)
	{
		if (a.getField(f) != b.getField(f))
		{
			return false;
		}
	}

	auto refa = a.getBinariesAsBuffer();
	auto refb = b.getBinariesAsBuffer();

	if (refa.first != refb.first)
	{
		return false;
	}

	if (refa.second != refb.second)
	{
		return false;
	}

	return true;
}

TEST(MongoDatabaseHandlerTest, InsertManyDocumentsBinary)
{
	// InsertManyDocuments is a special method that should batch multiple binary
	// buffers into a single backing store

	// Create some documents, such that they contain a mix of binary buffer
	// sizes, including enough that we will likely exceed the size of one backing
	// store.

	// How the backing store is distributed is an implementation detail of the
	// FileManager, so we don't explicitly check that here - just that the data
	// is read back correctly.

	std::vector<int> sizes = {
		1 * 1024 * 1024,
		10 * 1024 * 1024,
		50 * 1024 * 1024,
		50 * 1024 * 1024,
		101 * 1024 * 1024,
		2 * 1024 * 1024,
		1 * 1024,
		100 * 1024,
		1,
		150
	};

	std::vector<repo::core::model::RepoBSON> documents;
	for (size_t i = 0; i < sizes.size(); i++)
	{
		repo::core::model::RepoBSONBuilder builder;
		builder.append(REPO_LABEL_ID, repo::lib::RepoUUID::createUUID());
		builder.appendLargeArray("bin", testing::makeRandomBinary(sizes[i]));
		documents.push_back(builder.obj());
	}

	// Collection to hold the documents

	auto handler = getHandler();
	ASSERT_TRUE(handler);

	auto collection = "insertManyDocumentsBinary";
	handler->dropCollection(REPO_GTEST_DBNAME3, collection);
	handler->insertManyDocuments(REPO_GTEST_DBNAME3, collection, documents);

	// Read back the documents; this should also populate the binary buffers

	auto actual = handler->getAllFromCollectionTailable(REPO_GTEST_DBNAME3, collection);
	populateBinaryData(handler.get(), REPO_GTEST_DBNAME3, collection, actual);

	// The original documents won't have the _blobRef document as this isn't
	// created until inside the handler, so we convert both sets to RepoBSONs
	// for the semantic comparision.

	EXPECT_THAT(actual, Pointwise(InsertManyDocumentsBinaryBSONMatcher(), documents));
}

TEST(MongoDatabaseHandlerTest, InsertManyDocumentsMetadata)
{
	// The binary blobs created may have metadata attached. This metadata is
	// added as first-part members to the ref node.
	// Make sure that is metadata is added properly.

	std::vector<repo::core::model::RepoBSON> documents;
	for (size_t i = 0; i < 1; i++)
	{
		repo::core::model::RepoBSONBuilder builder;
		builder.append(REPO_LABEL_ID, repo::lib::RepoUUID::createUUID());
		builder.appendLargeArray("bin", testing::makeRandomBinary());
		documents.push_back(builder.obj());
	}

	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	repo::core::handler::AbstractDatabaseHandler::Metadata metadata;
	metadata["x-uuid"] = repo::lib::RepoUUID::createUUID();
	metadata["x-int"] = 1;
	metadata["x-string"] = std::string("my metadata string");

	auto collection = "InsertManyDocumentsMetadata";
	handler->dropCollection(REPO_GTEST_DBNAME3, collection);
	handler->insertManyDocuments(REPO_GTEST_DBNAME3, collection, documents, metadata);

	// Read back the documents

	auto actuals = handler->getAllFromCollectionTailable(REPO_GTEST_DBNAME3, collection);
	auto actual = actuals[0];

	// Get the ref node from the database. We have to access the BSON directly to
	// do this, because the RepoRef type is not designed to read metadata, only
	// write it.

	auto ref = actual.getBinaryReference();

	auto refNode = handler->findOneByUniqueID(
		REPO_GTEST_DBNAME3,
		collection + std::string(".") + REPO_COLLECTION_EXT_REF,
		ref.getStringField(REPO_LABEL_BINARY_FILENAME)
	);

	EXPECT_THAT(refNode.getUUIDField("x-uuid"), Eq(boost::get<repo::lib::RepoUUID>(metadata["x-uuid"])));
	EXPECT_THAT(refNode.getIntField("x-int"), Eq(boost::get<int>(metadata["x-int"])));
	EXPECT_THAT(refNode.getStringField("x-string"), Eq(boost::get<std::string>(metadata["x-string"])));
}

std::vector<uint8_t> makeRandomBinaryFromUUID(repo::lib::RepoUUID id, size_t count)
{
	std::vector<uint8_t> bin;
	for (int i = 0; i < count; i++)
	{
		auto data = id.data();
		for (auto byte : data)
		{
			bin.push_back(byte);
		}
	}
	return bin;
}

bool checkBinaryAgainstUUID(repo::lib::RepoUUID id, std::vector<uint8_t>& bin, size_t count)
{
	// First, check length. Should be count * 16
	if (bin.size() != 16 * count)
		return false;

	// Then we go over it and check for corruption
	// by comparing incoming with the truth
	auto truth = id.data();
	for (int i = 0; i < count; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if (bin[(i * 16) + j] != truth[j])
				return false;
		}
	}

	return true;
}

void checkDocument(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	repo::core::model::RepoBSON& doc)
{
	EXPECT_TRUE(doc.hasField("_id"));
	EXPECT_TRUE(doc.hasField("shared_id"));

	bool hasNonBinaryPayload = doc.hasField("non_bin");
	EXPECT_TRUE(hasNonBinaryPayload);
	if (hasNonBinaryPayload)
	{
		auto nonBinPayload = doc.getStringField("non_bin");
		EXPECT_THAT(nonBinPayload, Eq("this is a test"));
	}

	// Extra check if there is a binary payload
	if (doc.hasField("binary_id"))
	{
		auto binId = doc.getUUIDField("binary_id");

		// Populate binary data
		// There is a known multi-threading danger here.
		// With insertMany, it is possible that the database entry
		// is inserted before the last active file is committed.
		// Bouncer can detect that the file ref is missing and
		// throws an exception.
		// Wait and retry a set amount of times before failing the test.
		int tried = 0;
		int tries = 30;
		bool success = false;
		while (!success) 
		{
			try
			{
				populateBinaryData(handler, database, collection, doc);
				success = true;
			}
			catch (repo::lib::RepoRefMissingException ex)
			{
				// Check whether we still have tries left
				if (tried < tries)
				{
					// We do. Increment number of tries, sleep for a second, then go again.
					tried++;
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}
				else
				{
					// We are out of tries. Fail the test.
					FAIL() << "Unable to retrieve ref after " << tries << " attempts. Aborting." << std::endl;
				}
			}
		}

		auto buffer = doc.getBinary("bin");
		bool binaryCheck = checkBinaryAgainstUUID(binId, buffer, binarySampleCount);
		doc.unloadBinaryBuffers();
		EXPECT_TRUE(binaryCheck);
	}
}

repo::core::model::RepoBSON generateSample(bool addBinary, int binarySampleCount = 1000)
{
	repo::core::model::RepoBSONBuilder builder;

	auto uniqueId = repo::lib::RepoUUID::createUUID();
	builder.append("_id", uniqueId);

	auto sharedId = repo::lib::RepoUUID::createUUID();
	builder.append("shared_id", sharedId);

	// Some none-binary payload
	builder.append("non_bin", "this is a test");

	if (addBinary)
	{
		auto binId = repo::lib::RepoUUID::createUUID();
		builder.appendLargeArray("bin", makeRandomBinaryFromUUID(binId, binarySampleCount));
		builder.append("binary_id", binId);
	}

	return builder.obj();
}

void checkThreadGetAllFromCollectionTailable(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{

		auto results = handler->getAllFromCollectionTailable(database, collection);

		// Go over results, check for presence of the test fields
		for (auto& doc : results)
		{
			checkDocument(handler, database, collection, binarySampleCount, doc);
			checked++;

			// Sanity break in case the collection holds more than noCasesToCheck
			if (checked > noCasesToCheck)
				break;
		}
	}
}

void checkThreadFindAllByCriteria(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{
		// Get some unique ids fist
		std::vector<repo::lib::RepoUUID> ids;
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;
			int randSkip = std::rand() % count;
			int randLimit = std::max(std::rand() % 150, 10);
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, randLimit, { "_id" });

			for (auto& doc : results)
			{
				auto id = doc.getUUIDField("_id");
				ids.push_back(id);
			}
		}

		// Now find the entries matching that criteria
		auto criteria = database::query::Eq("_id", ids);
		auto results = handler->findAllByCriteria(database, collection, criteria);

		// Go over results, check for presence of the test fields
		for (auto& doc : results)
		{
			checkDocument(handler, database, collection, binarySampleCount, doc);
			checked++;
		}
	}
}

void checkThreadFindAllByCriteriaWithProjection(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{
		// Get some unique ids fist
		std::vector<repo::lib::RepoUUID> ids;
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;
			int randSkip = std::rand() % count;
			int randLimit = std::max(std::rand() % 150, 10);
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, randLimit, { "_id" });

			for (auto& doc : results)
			{
				auto id = doc.getUUIDField("_id");
				ids.push_back(id);
			}
		}

		// Build projection that will exclude the shared_id field
		auto projection = database::query::RepoProjectionBuilder();
		projection.excludeField("shared_id");

		// Now find the entries matching that criteria
		auto criteria = database::query::Eq("_id", ids);
		auto results = handler->findAllByCriteria(database, collection, criteria, projection);

		// Go over results, check for presence of the test fields
		for (auto& doc : results)
		{
			checkDocument(handler, database, collection, binarySampleCount, doc);
			checked++;
		}
	}
}

void checkThreadFindCursorByCriteria(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{
		// Get some unique ids fist
		std::vector<repo::lib::RepoUUID> ids;
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;
			int randSkip = std::rand() % count;
			int randLimit = std::max(std::rand() % 150, 10);
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, randLimit, { "_id" });

			for (auto& doc : results)
			{
				auto id = doc.getUUIDField("_id");
				ids.push_back(id);
			}
		}

		// Now find the entries matching that criteria
		auto criteria = database::query::Eq("_id", ids);
		auto cursor = handler->findCursorByCriteria(database, collection, criteria);

		// Go over results, check for presence of the test fields
		for (auto doc : *cursor)
		{
			checkDocument(handler, database, collection, binarySampleCount, doc);
			checked++;
		}
	}
}

void checkThreadFindCursorByCriteriaWithProjection(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{
		// Get some unique ids fist
		std::vector<repo::lib::RepoUUID> ids;
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;
			int randSkip = std::rand() % count;
			int randLimit = std::max(std::rand() % 150, 10);			
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, randLimit, { "_id" });

			for (auto& doc : results)
			{
				auto id = doc.getUUIDField("_id");
				ids.push_back(id);
			}
		}

		// If no ids could be recovered, continue.
		// This can happen if the collection was dropped by another thread
		// after count was already done.
		if (ids.size() == 0)
			continue;

		// Build projection that will exclude the shared_id field
		auto projection = database::query::RepoProjectionBuilder();
		projection.excludeField("shared_id");

		// Now find the entries matching that criteria
		auto criteria = database::query::Eq("_id", ids);
		auto cursor = handler->findCursorByCriteria(database, collection, criteria, projection);

		// Go over results, check for presence of the test fields
		for (auto doc : *cursor)
		{
			checkDocument(handler, database, collection, binarySampleCount, doc);
			checked++;
		}
	}
}

void checkThreadFindOneByCriteria(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{
		// Get some random unique id
		repo::lib::RepoUUID id;
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;
			int randSkip = std::rand() % count;
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, 1, { "_id" });

			if (results.size() == 0)
				continue;
			id = results[0].getUUIDField("_id");
		}

		// Now find the entry matching that criteria
		auto criteria = database::query::Eq("_id", id);
		auto result = handler->findOneByCriteria(database, collection, criteria);

		// It is possible that the entry got dropped in the meantime.
		// That is not undesired behaviour, so we just continue and
		// hope for better luck next time.
		if (result.isEmpty())
			continue;

		// Check result
		checkDocument(handler, database, collection, binarySampleCount, result);
		checked++;
	}
}

void checkThreadFindOneBySharedId(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{
		// Get some random unique id
		repo::lib::RepoUUID id;
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;

			int randSkip = std::rand() % count;
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, 1, { "shared_id" });

			if (results.size() == 0)
				continue;

			id = results[0].getUUIDField("shared_id");
		}

		// Now find the entry matching that criteria
		auto result = handler->findOneBySharedID(database, collection, id, "_id");

		// It is possible that the entry got dropped in the meantime.
		// That is not undesired behaviour, so we just continue and
		// hope for better luck next time.
		if (result.isEmpty())
			continue;

		// Check result
		checkDocument(handler, database, collection, binarySampleCount, result);
		checked++;
	}
}

void checkThreadFindOneByUniqueId(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{
		// Get some random unique id
		repo::lib::RepoUUID id;
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;
			int randSkip = std::rand() % count;
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, 1, { "_id" });

			if (results.size() == 0)
				continue;
			id = results[0].getUUIDField("_id");
		}

		// Now find the entry matching that criteria
		auto result = handler->findOneByUniqueID(database, collection, id);

		// It is possible that the entry got dropped in the meantime.
		// That is not undesired behaviour, so we just continue and
		// hope for better luck next time.
		if (result.isEmpty())
			continue;

		// Check result
		checkDocument(handler, database, collection, binarySampleCount, result);
		checked++;
	}
}

void checkThreadFindOneByUniqueIdString(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{
		// Get some random unique id
		repo::lib::RepoUUID id;
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;
			int randSkip = std::rand() % count;
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, 1, { "_id" });

			if (results.size() == 0)
				continue;
			id = results[0].getUUIDField("_id");
		}

		// Now find the entry matching that criteria
		auto result = handler->findOneByUniqueID(database, collection, id.toString());

		// Check result
		// The write threads all produce LUUID fields for _id, so getting no result is the expected result
		EXPECT_TRUE(result.isEmpty());		
		checked++;
	}
}

void checkThreadGetCollection(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToCheck)
{
	int checked = 0;
	while (checked < noCasesToCheck)
	{
		auto collections = handler->getCollections(database);

		if (collections.size() == 0)
			continue;

		bool foundTarget = false;
		for (auto col : collections)
		{
			if (col == collection)
				foundTarget = true;
		}

		// No hard check. We cannot guarantee that there are any collections
		// or that the current one is existing right now (might not at the beginning
		// of the test). So as long as it does not throw a exception, we are happy if
		// it occasionially does not show up.
		if(foundTarget)
			checked++;
	}
}

void writeThreadInsertDocument(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToGenerate)
{
	int generated = 0;
	while (generated < noCasesToGenerate)
	{
		auto sample = generateSample(false);
		handler->insertDocument(database, collection, sample);
		generated++;
	}
}

void writeThreadInsertManyDocuments(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToGenerate)
{
	const int batchSize = 100;
	int generated = 0;

	while (generated < noCasesToGenerate) 
	{
		// Generate a batch of samples
		std::vector<repo::core::model::RepoBSON> samples;
		samples.reserve(batchSize);
		for (int i = 0; i < batchSize; i++)
		{
			auto sample = generateSample(true, binarySampleCount);
			samples.push_back(sample);
		}

		// Insert the batch
		handler->insertManyDocuments(database, collection, samples);

		generated += batchSize;
	}
}

void writeThreadUpsertDocument(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToGenerate)
{
	int generated = 0;
	while (generated < noCasesToGenerate)
	{
		// basically flip a coin whether to insert a new one or upsert an existing document
		bool generateNew = std::rand() % 2;
		if (generateNew)
		{
			auto sample = generateSample(false);
			handler->upsertDocument(database, collection, sample, true);
			generated++;
		}
		else
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;

			int randSkip = std::rand() % count;
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, 1, {});

			if (results.size() == 0)
				continue;

			auto doc = results[0];

			// Make sure the doc is valid before using it
			checkDocument(handler, database, collection, binarySampleCount, doc);
			
			// Copy values
			auto sample = repo::core::model::RepoBSONBuilder();
			sample.append("_id", doc.getUUIDField("_id"));
			sample.append("shared_id", doc.getUUIDField("shared_id"));
			sample.append("non_bin", doc.getStringField("non_bin"));
			if (doc.hasField("binary_id"))
			{
				sample.append("binary_id", doc.getUUIDField("binary_id"));
				sample.append("_blobRef", doc.getObjectField("_blobRef"));
			}

			// Add new field just so there is something new
			sample.append("upserted", true);

			// Upsert the document
			handler->upsertDocument(database, collection, sample.obj(), true);

			generated++;
		}
	}
}

void writeThreadBulkWriteContext(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToGenerate)
{
	int generated = 0;

	auto ctx = handler->getBulkWriteContext(database, collection);
	std::vector<repo::lib::RepoUUID> uids;

	while (generated < noCasesToGenerate)
	{
		// basically flip a coin whether to insert a new one or upsert an existing document
		bool generateNew = std::rand() % 2;
		if (generateNew)
		{
			auto sample = generateSample(false);
			ctx->insertDocument(sample);
			generated++;
		}
		else
		{
			int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
			if (count == 0)
				continue;

			int randSkip = std::rand() % count;
			auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, 1, {});

			if (results.size() == 0)
				continue;

			auto doc = results[0];

			// Make sure the doc is valid before using it
			checkDocument(handler, database, collection, binarySampleCount, doc);

			// Update the document with a ficticious parent id
			auto uid = doc.getUUIDField("_id");
			auto parentId = repo::lib::RepoUUID::createUUID();
			auto query = database::query::RepoUpdate(database::query::AddParent(uid, parentId));
			ctx->updateDocument(query);

			generated++;
		}
	}
	ctx->flush();
}

void writeThreadCreateIndex(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToGenerate)
{
	int generated = 0;
	while (generated < noCasesToGenerate)
	{
		handler->createIndex(database, collection, database::index::Ascending({ "_id", "shared_id" }), false, true);
		generated++;
	}
}

void dropThreadDropDocument(
	MongoDatabaseHandler* handler,
	std::string database,
	std::string collection,
	int binarySampleCount,
	int noCasesToDrop)
{
	int dropped = 0;
	while (dropped < noCasesToDrop)
	{
		int count = handler->count(database, collection, database::query::RepoQueryBuilder{});
		if (count == 0)
			continue;

		int randSkip = std::rand() % count;
		auto results = handler->getAllFromCollectionTailable(database, collection, randSkip, 1, {});

		if (results.size() == 0)
			continue;

		auto doc = results[0];

		// Make sure the doc is valid before using it
		checkDocument(handler, database, collection, binarySampleCount, doc);

		handler->dropDocument(doc, database, collection);

		dropped++;
	}
}

TEST(MongoDatabaseHandlerTest, SoakTestWriteVsWrite)
{
	std::string database = REPO_GTEST_DBNAME3;
	std::string collection = "mtTestCollectionWvsW";
	std::string refCollection = "mtTestCollectionWvsW.ref";
	int binarySampleCount = 10; // 1,600 byte per document
	int noCases = 2500; // per thread. Scaled to keep runtime in the minutes

	// Typedef for the function pointers
	typedef void (*fp)(MongoDatabaseHandler*, std::string, std::string, int, int);

	// Create array of function pointers
	std::pair<std::string, fp> writeFunctions[] =
	{
		{"InsertDocument", writeThreadInsertDocument},
		{"InsertManyDocuments", writeThreadInsertManyDocuments},
		{"UpsertDocument", writeThreadUpsertDocument},
		{"BulkWriteContext", writeThreadBulkWriteContext},
		{"CreateIndex", writeThreadCreateIndex}
	};
	
	// Create handler
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	// Initialise srand
	std::srand(std::time(nullptr));
	
	for (auto funcA : writeFunctions)
	{
		for (auto funcB : writeFunctions)
		{
			if (funcA == funcB)
				continue;

			std::cout << funcA.first << " vs. " << funcB.first << std::endl;
			std::vector<std::jthread> threads;

			// Prepare collections
			handler->dropCollection(database, collection);
			handler->dropCollection(database, refCollection);

			// Launch the threads from the first set
			for (int i = 0; i < 10; i++)
				threads.emplace_back(std::jthread{ funcA.second, handler.get(), database, collection, binarySampleCount, noCases});

			// Launch the threads from the second set
			for (int i = 0; i < 10; i++)
				threads.emplace_back(std::jthread{ funcB.second, handler.get(), database, collection, binarySampleCount, noCases });

			// Wait for all threads to complete
			for (auto& t : threads)
				t.join();
		}
	}
}

TEST(MongoDatabaseHandlerTest, SoakTestReadVsRead)
{
	std::string database = REPO_GTEST_DBNAME3;
	std::string collection = "mtTestCollectionRvsR";
	std::string refCollection = "mtTestCollectionRvsR.ref";
	int binarySampleCount = 10; // 1,600 byte per document
	int noCases = 300; // per thread. Scaled to keep the runtime in minutes.

	// Typedef for the function pointers
	typedef void (*fp)(MongoDatabaseHandler*, std::string, std::string, int, int);

	// Create array of function pointers
	std::pair<std::string, fp> checkFunctions[] =
	{
		{"GetAllFromCollectionTailable", checkThreadGetAllFromCollectionTailable},
		{"FindAllByCriteria", checkThreadFindAllByCriteria},
		{"FindAllByCriteriaWithProjection", checkThreadFindAllByCriteriaWithProjection},
		{"FindCursorByCriteria", checkThreadFindCursorByCriteria},
		{"FindCursorByCriteriaWithProjection", checkThreadFindCursorByCriteriaWithProjection},
		{"FindOneByCriteria", checkThreadFindOneByCriteria},
		{"FindOneBySharedId", checkThreadFindOneBySharedId},
		{"FindOneByUniqueId", checkThreadFindOneByUniqueId},
		{"FindOneByUniqueIdString", checkThreadFindOneByUniqueIdString},
		{"GetCollections", checkThreadGetCollection}
	};

	// Create handler
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	// Initialise srand
	std::srand(std::time(nullptr));

	// Create a collection with 10k test entries.
	// Half of them with binary payload and half without
	handler->dropCollection(database, collection);
	handler->dropCollection(database, refCollection);
	auto ctx = handler->getBulkWriteContext(database, collection);
	for (int i = 0; i < 10000; i++)
	{
		ctx->insertDocument(generateSample(i < 5000, binarySampleCount));
	}
	ctx->flush();

	// Add index to speed up look ups using the shared_id
	handler->createIndex(database, collection, repo::core::handler::database::index::Ascending({ "shared_id" }), false, true);

	for (auto funcA : checkFunctions)
	{
		for (auto funcB : checkFunctions)
		{
			if (funcA == funcB)
				continue;

			std::cout << funcA.first << " vs. " << funcB.first << std::endl;
			std::vector<std::jthread> threads;

			// Launch the threads from the first set
			for (int i = 0; i < 10; i++)
				threads.emplace_back(std::jthread{ funcA.second, handler.get(), database, collection, binarySampleCount, noCases });

			// Launch the threads from the second set
			for (int i = 0; i < 10; i++)
				threads.emplace_back(std::jthread{ funcB.second, handler.get(), database, collection, binarySampleCount, noCases });

			// Wait for all threads to complete
			for (auto& t : threads)
				t.join();
		}
	}
}

TEST(MongoDatabaseHandlerTest, SoakTestReadVsWrite)
{
	std::string database = REPO_GTEST_DBNAME3;
	std::string collection = "mtTestCollectionRvsW";
	std::string refCollection = "mtTestCollectionRvsW.ref";
	int binarySampleCount = 10; // 1,600 byte per document
	int noCasesWrite = 2500; // per thread
	int noCasesCheck = 300; // per thread

	// Typedef for the function pointers
	typedef void (*fp)(MongoDatabaseHandler*, std::string, std::string, int, int);

	// Create array of function pointers
	std::pair<std::string, fp> writeFunctions[] =
	{
		{"InsertDocument", writeThreadInsertDocument},
		{"InsertManyDocuments", writeThreadInsertManyDocuments},
		{"UpsertDocument", writeThreadUpsertDocument},
		{"BulkWriteContext", writeThreadBulkWriteContext}
	};

	// Create array of function pointers
	std::pair<std::string, fp> checkFunctions[] =
	{
		{"GetAllFromCollectionTailable", checkThreadGetAllFromCollectionTailable},
		{"FindAllByCriteria", checkThreadFindAllByCriteria},
		{"FindAllByCriteriaWithProjection", checkThreadFindAllByCriteriaWithProjection},
		{"FindCursorByCriteria", checkThreadFindCursorByCriteria},
		{"FindCursorByCriteriaWithProjection", checkThreadFindCursorByCriteriaWithProjection},
		{"FindOneByCriteria", checkThreadFindOneByCriteria},
		{"FindOneBySharedId", checkThreadFindOneBySharedId},
		{"FindOneByUniqueId", checkThreadFindOneByUniqueId},
		{"FindOneByUniqueIdString", checkThreadFindOneByUniqueIdString},
		{"GetCollections", checkThreadGetCollection}
	};

	// Create handler
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	// Initialise srand
	std::srand(std::time(nullptr));

	// Add index to speed up look ups using the shared_id
	handler->createIndex(database, collection, repo::core::handler::database::index::Ascending({ "shared_id" }), false, true);

	for (auto funcA : writeFunctions)
	{
		for (auto funcB : checkFunctions)
		{
			std::cout << funcA.first << " vs. " << funcB.first << std::endl;
			std::vector<std::jthread> threads;

			// Prepare collections
			handler->dropCollection(database, collection);
			handler->dropCollection(database, refCollection);

			// Launch the threads from the first set (write)
			for (int i = 0; i < 10; i++)
				threads.emplace_back(std::jthread{ funcA.second, handler.get(), database, collection, binarySampleCount, noCasesWrite });

			// Launch the threads from the second set (check)
			for (int i = 0; i < 10; i++)
				threads.emplace_back(std::jthread{ funcB.second, handler.get(), database, collection, binarySampleCount, noCasesCheck });

			// Wait for all threads to complete
			for (auto& t : threads)
				t.join();
		}
	}
}

TEST(MongoDatabaseHandlerTest, SoakTestDropVsRead)
{
	std::string database = REPO_GTEST_DBNAME3;
	std::string collection = "mtTestCollectionDvsR";
	std::string refCollection = "mtTestCollectionDvsR.ref";
	int binarySampleCount = 10; // 1,600 byte per document
	int noCasesDrop = 750; // per thread. Scaled to keep the runtime in minutes.
	int noCasesCheck = 500; // per thread. Scaled to keep the runtime in minutes.

	// Typedef for the function pointers
	typedef void (*fp)(MongoDatabaseHandler*, std::string, std::string, int, int);

	// Create array of function pointers
	std::pair<std::string, fp> checkFunctions[] =
	{
		{"GetAllFromCollectionTailable", checkThreadGetAllFromCollectionTailable},
		{"FindAllByCriteria", checkThreadFindAllByCriteria},
		{"FindAllByCriteriaWithProjection", checkThreadFindAllByCriteriaWithProjection},
		{"FindCursorByCriteria", checkThreadFindCursorByCriteria},
		{"FindCursorByCriteriaWithProjection", checkThreadFindCursorByCriteriaWithProjection},
		{"FindOneByCriteria", checkThreadFindOneByCriteria},
		{"FindOneBySharedId", checkThreadFindOneBySharedId},
		{"FindOneByUniqueId", checkThreadFindOneByUniqueId},
		{"FindOneByUniqueIdString", checkThreadFindOneByUniqueIdString},
		{"GetCollections", checkThreadGetCollection}
	};

	// Create handler
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	// Initialise srand
	std::srand(std::time(nullptr));

	for (auto funcA : checkFunctions)
	{
		std::cout << funcA.first << " vs. DropDocuments" << std::endl;
		std::vector<std::jthread> threads;

		// Create a collection with 10k test entries.
		// Half of them with binary payload and half without
		handler->dropCollection(database, collection);
		handler->dropCollection(database, refCollection);
		auto ctx = handler->getBulkWriteContext(database, collection);
		for (int i = 0; i < 10000; i++)
		{
			ctx->insertDocument(generateSample(i < 5000, binarySampleCount));
		}
		ctx->flush();

		// Add index to speed up look ups using the shared_id
		handler->createIndex(database, collection, repo::core::handler::database::index::Ascending({ "shared_id" }), false, true);

		// Launch the drop threads 
		for (int i = 0; i < 10; i++)
			threads.emplace_back(std::jthread{ dropThreadDropDocument, handler.get(), database, collection, binarySampleCount, noCasesDrop });

		// Launch the threads from the check set
		for (int i = 0; i < 10; i++)
			threads.emplace_back(std::jthread{ funcA.second, handler.get(), database, collection, binarySampleCount, noCasesCheck });

		// Wait for all threads to complete
		for (auto& t : threads)
			t.join();
	}
}

TEST(MongoDatabaseHandlerTest, SoakTestReadVsWriteVsDrop)
{
	std::string database = REPO_GTEST_DBNAME3;
	std::string collection = "mtTestCollectionRvsWvsD";
	std::string refCollection = "mtTestCollectionRvsWvsD.ref";
	int binarySampleCount = 10; // 1,600 byte per document
	int noCasesWrite = 30000; // per thread. Scaled to keep the runtime in minutes.
	int noCasesCheck = 3000; // per thread. Scaled to keep the runtime in minutes.
	int noCasesDrop = 7500; // per thread. Scaled to keep the runtime in minutes.

	// Typedef for the function pointers
	typedef void (*fp)(MongoDatabaseHandler*, std::string, std::string, int, int);

	// Create array of function pointers
	std::pair<int, fp> functions[] =
	{
		{noCasesWrite, writeThreadInsertDocument},
		{noCasesWrite, writeThreadInsertManyDocuments},
		{noCasesWrite, writeThreadUpsertDocument},
		{noCasesWrite, writeThreadBulkWriteContext},
		{noCasesWrite, writeThreadCreateIndex},
		{noCasesCheck, checkThreadGetAllFromCollectionTailable},
		{noCasesCheck, checkThreadFindAllByCriteria},
		{noCasesCheck, checkThreadFindAllByCriteriaWithProjection},
		{noCasesCheck, checkThreadFindCursorByCriteria},
		{noCasesCheck, checkThreadFindCursorByCriteriaWithProjection},
		{noCasesCheck, checkThreadFindOneByCriteria},
		{noCasesCheck, checkThreadFindOneBySharedId},
		{noCasesCheck, checkThreadFindOneByUniqueId},
		{noCasesCheck, checkThreadFindOneByUniqueIdString},
		{noCasesCheck, checkThreadGetCollection},
		{noCasesDrop, dropThreadDropDocument}
	};

	// Create handler
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	// Initialise srand
	std::srand(std::time(nullptr));

	// Prepare collections
	handler->dropCollection(database, collection);
	handler->dropCollection(database, refCollection);

	// Add index to speed up look ups using the shared_id
	handler->createIndex(database, collection, repo::core::handler::database::index::Ascending({ "shared_id" }), false, true);

	std::vector<std::jthread> threads;
	for (auto func : functions)
	{
		// Launch the threads
		threads.emplace_back(std::jthread{ func.second, handler.get(), database, collection, binarySampleCount, func.first });
	}

	// Wait for all threads to complete
	for (auto& t : threads)
		t.join();
}