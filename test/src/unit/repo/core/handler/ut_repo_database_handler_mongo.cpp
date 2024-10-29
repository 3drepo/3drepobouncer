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
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_element.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/core/model/bson/repo_bson_builder.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include "../../../repo_test_matchers.h"
#include "../../../repo_test_utils.h"
#include "../../../repo_test_database_info.h"

using namespace repo::core::handler;
using namespace testing;

void initialiseFileManager()
{
	// Usually the fileManager will be initialised elsewhere, but if not it is
	// necessary for many for many db operations.
}

TEST(MongoDatabaseHandlerTest, GetHandlerDisconnectHandler)
{
	std::string errMsg;
	MongoDatabaseHandler* handler =
		MongoDatabaseHandler::getHandler(errMsg, REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
			1,
			REPO_GTEST_AUTH_DATABASE,
			REPO_GTEST_DBUSER, REPO_GTEST_DBPW);

	EXPECT_TRUE(handler);
	EXPECT_TRUE(errMsg.empty());

	EXPECT_TRUE(MongoDatabaseHandler::getHandler(REPO_GTEST_DBADDRESS));
	MongoDatabaseHandler::disconnectHandler();

	EXPECT_FALSE(MongoDatabaseHandler::getHandler(REPO_GTEST_DBADDRESS));

	MongoDatabaseHandler *wrongAdd = MongoDatabaseHandler::getHandler(errMsg, "blah", REPO_GTEST_DBPORT,
		1,
		REPO_GTEST_AUTH_DATABASE,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);

	EXPECT_FALSE(wrongAdd);
	MongoDatabaseHandler *wrongPort = MongoDatabaseHandler::getHandler(errMsg, REPO_GTEST_DBADDRESS, 0001,
		1,
		REPO_GTEST_AUTH_DATABASE,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);
	EXPECT_FALSE(wrongPort);

	//Check can connect without authentication
	MongoDatabaseHandler *noauth = MongoDatabaseHandler::getHandler(errMsg, REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT);

	EXPECT_TRUE(noauth);
	MongoDatabaseHandler::disconnectHandler();
	//ensure no crash when disconnecting the disconnected
	MongoDatabaseHandler::disconnectHandler();
}

TEST(MongoDatabaseHandlerTest, CreateBSONCredentials)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	EXPECT_TRUE(handler->createBSONCredentials("testdb", "username", "password"));
	EXPECT_TRUE(handler->createBSONCredentials("testdb", "username", "password", true));
	EXPECT_FALSE(handler->createBSONCredentials("", "username", "password"));
	EXPECT_FALSE(handler->createBSONCredentials("testdb", "", "password"));
	EXPECT_FALSE(handler->createBSONCredentials("testdb", "username", ""));
}

TEST(MongoDatabaseHandlerTest, GetAllFromCollectionTailable)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	auto goldenData = getGoldenForGetAllFromCollectionTailable();

	std::vector<repo::core::model::RepoBSON> bsons = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second);

	ASSERT_EQ(bsons.size(), goldenData.second.size());

	//Test limit and skip
	std::vector<repo::core::model::RepoBSON> bsonsLimitSkip = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second, 1, 1);

	ASSERT_EQ(bsonsLimitSkip.size(), 1);

	EXPECT_EQ(bsonsLimitSkip[0], bsons[1]);

	//test projection
	auto bsonsProjected = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second, 0, 0, { "_id", "shared_id" });

	std::vector<repo::lib::RepoUUID> ids;

	ASSERT_EQ(bsonsProjected.size(), bsons.size());
	for (int i = 0; i < bsons.size(); ++i)
	{
		ids.push_back(bsons[i].getUUIDField("_id"));
		EXPECT_EQ(bsons[i].getUUIDField("_id"), bsonsProjected[i].getUUIDField("_id"));
		EXPECT_EQ(bsons[i].getUUIDField("shared_id"), bsonsProjected[i].getUUIDField("shared_id"));
	}

	//test sort
	auto bsonsSorted = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second, 0, 0, {}, "_id", -1);

	std::sort(ids.begin(), ids.end());

	ASSERT_EQ(bsonsSorted.size(), ids.size());
	for (int i = 0; i < bsons.size(); ++i)
	{
		EXPECT_EQ(bsonsSorted[i].getUUIDField("_id"), ids[bsons.size() - i - 1]);
	}

	bsonsSorted = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second, 0, 0, {}, "_id", 1);

	ASSERT_EQ(bsonsSorted.size(), ids.size());
	for (int i = 0; i < bsons.size(); ++i)
	{
		EXPECT_EQ(bsonsSorted[i].getUUIDField("_id"), ids[i]);
	}

	//check error handling - make sure it doesn't crash
	EXPECT_EQ(0, handler->getAllFromCollectionTailable("", "").size());
	EXPECT_EQ(0, handler->getAllFromCollectionTailable("", "blah").size());
	EXPECT_EQ(0, handler->getAllFromCollectionTailable("blah", "").size());
	EXPECT_EQ(0, handler->getAllFromCollectionTailable("blah", "blah").size());
}

TEST(MongoDatabaseHandlerTest, GetCollections)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	auto goldenData = getCollectionList(REPO_GTEST_DBNAME1);

	auto collections = handler->getCollections(REPO_GTEST_DBNAME1);

	//Some version gives you system.indexes but some others don't
	ASSERT_EQ(goldenData.size(), collections.size());

	std::sort(goldenData.begin(), goldenData.end());
	collections.sort();

	auto colIt = collections.begin();
	auto gdIt = goldenData.begin();
	for (; colIt != collections.end(); ++colIt, ++gdIt)
	{
		EXPECT_EQ(*colIt, *gdIt);
	}

	goldenData = getCollectionList(REPO_GTEST_DBNAME2);
	collections = handler->getCollections(REPO_GTEST_DBNAME2);

	ASSERT_EQ(goldenData.size(), collections.size());

	std::sort(goldenData.begin(), goldenData.end());
	collections.sort();

	colIt = collections.begin();
	gdIt = goldenData.begin();
	for (; colIt != collections.end(); ++colIt, ++gdIt)
	{
		EXPECT_EQ(*colIt, *gdIt);
	}

	//check error handling - make sure it doesn't crash
	EXPECT_EQ(0, handler->getCollections("").size());
	EXPECT_EQ(0, handler->getCollections("blahblah").size());
}

TEST(MongoDatabaseHandlerTest, GetAdminDatabaseName)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	//This should not be an empty string
	EXPECT_FALSE(handler->getAdminDatabaseName().empty());
}

TEST(MongoDatabaseHandlerTest, CreateCollection)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	//no exception
	handler->createCollection("this_database", "newCollection");
	handler->createCollection("this_database", "");
	handler->createCollection("", "newCollection");
}

TEST(MongoDatabaseHandlerTest, DropCollection)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;
	//no exception
	EXPECT_TRUE(handler->dropCollection(REPO_GTEST_DROPCOL_TESTCASE.first, REPO_GTEST_DROPCOL_TESTCASE.second, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropCollection(REPO_GTEST_DROPCOL_TESTCASE.first, REPO_GTEST_DROPCOL_TESTCASE.second, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropCollection(REPO_GTEST_DROPCOL_TESTCASE.first, "", errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropCollection("", REPO_GTEST_DROPCOL_TESTCASE.second, errMsg));
	EXPECT_FALSE(errMsg.empty());
}

TEST(MongoDatabaseHandlerTest, DropDocument)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	auto testCase = getDataForDropCase();

	EXPECT_TRUE(handler->dropDocument(testCase.second, testCase.first.first, testCase.first.second, errMsg));
	EXPECT_TRUE(errMsg.empty());
	//Dropping something that doesn't exist apparently returns true...
	EXPECT_TRUE(handler->dropDocument(testCase.second, testCase.first.first, testCase.first.second, errMsg));
	EXPECT_TRUE(errMsg.empty());
	//test empty bson
	EXPECT_FALSE(handler->dropDocument(mongo::BSONObj(), testCase.first.first, testCase.first.second, errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropDocument(testCase.second, testCase.first.first, "", errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropDocument(testCase.second, "", testCase.first.first, errMsg));
	EXPECT_FALSE(errMsg.empty());
}

TEST(MongoDatabaseHandlerTest, InsertDocument)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	auto id = repo::lib::RepoUUID::createUUID();

	repo::core::model::RepoBSON testCase = BSON("_id" << id.toString() << "anotherField" << std::rand());
	std::string database = "sandbox";
	std::string collection = "sbCollection";

	EXPECT_TRUE(handler->insertDocument(database, collection, testCase, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();

	//The following will of cousre fail if findOneByCriteria is failing
	repo::core::model::RepoBSON result = handler->findOneByCriteria(database, collection, testCase);
	EXPECT_FALSE(result.isEmpty());

	std::set<std::string> fields = result.getFieldNames();
	for (const auto &fname : fields)
	{
		ASSERT_TRUE(testCase.hasField(fname));
		EXPECT_EQ(result.getField(fname), testCase.getField(fname));
	}

	EXPECT_FALSE(handler->insertDocument("", "insertCollection", repo::core::model::RepoBSON(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->insertDocument("testingInsert", "", repo::core::model::RepoBSON(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->insertDocument("", "", repo::core::model::RepoBSON(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}

TEST(MongoDatabaseHandlerTest, FindAllByUniqueIDs)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	repo::core::model::RepoBSONBuilder builder;

	for (int i = 0; i < uuidsToSearch.size(); ++i)
	{
		builder.append(std::to_string(i), uuidsToSearch[i]);
	}

	repo::core::model::RepoBSON search = builder.obj();

	auto results = handler->findAllByUniqueIDs(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ + ".scene", search);

	EXPECT_EQ(uuidsToSearch.size(), results.size());

	EXPECT_EQ(0, handler->findAllByUniqueIDs(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ, repo::core::model::RepoBSON()).size());
	EXPECT_EQ(0, handler->findAllByUniqueIDs(REPO_GTEST_DBNAME1, "", search).size());
	EXPECT_EQ(0, handler->findAllByUniqueIDs("", REPO_GTEST_DBNAME1_PROJ, search).size());
}

TEST(MongoDatabaseHandlerTest, FindAllByCriteria)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	repo::core::model::RepoBSON search = BSON("type" << "mesh");

	auto results = handler->findAllByCriteria(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ + ".scene", search);

	EXPECT_EQ(4, results.size());

	EXPECT_EQ(0, handler->findAllByCriteria(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ + ".scene", repo::core::model::RepoBSON()).size());
	EXPECT_EQ(0, handler->findAllByCriteria("", REPO_GTEST_DBNAME1_PROJ + ".scene", search).size());
	EXPECT_EQ(0, handler->findAllByCriteria(REPO_GTEST_DBNAME1, "", search).size());
}

TEST(MongoDatabaseHandlerTest, FindOneByCriteria)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;
	repo::core::model::RepoBSONBuilder builder;

	builder.append("_id", uuidsToSearch[0]);

	repo::core::model::RepoBSON search = builder.obj();

	auto results = handler->findOneByCriteria(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ + ".scene", search);

	EXPECT_FALSE(results.isEmpty());
	EXPECT_EQ(results.getField("_id"), search.getField("_id"));

	EXPECT_TRUE(handler->findOneByCriteria(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ + ".scene", repo::core::model::RepoBSON()).isEmpty());
	EXPECT_TRUE(handler->findOneByCriteria("", REPO_GTEST_DBNAME1_PROJ + ".scene", search).isEmpty());
	EXPECT_TRUE(handler->findOneByCriteria(REPO_GTEST_DBNAME1, "", search).isEmpty());
}

TEST(MongoDatabaseHandlerTest, FindOneBySharedID)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	repo::core::model::RepoNode result = handler->findOneBySharedID(REPO_GTEST_DBNAME_ROLEUSERTEST, "sampleProject.history", repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), "timestamp");
	EXPECT_FALSE(result.getUniqueID().isDefaultValue());
	EXPECT_EQ(result.getSharedID(), repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH));

	result = handler->findOneBySharedID(REPO_GTEST_DBNAME_ROLEUSERTEST, "sampleProject.history", repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), "");
	EXPECT_FALSE(result.getUniqueID().isDefaultValue());
	EXPECT_EQ(result.getSharedID(), repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH));

	result = handler->findOneBySharedID("", "sampleProject", repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), "timestamp");
	EXPECT_TRUE(result.getUniqueID().isDefaultValue());
	result = handler->findOneBySharedID(REPO_GTEST_DBNAME_ROLEUSERTEST, "", repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), "timestamp");
	EXPECT_TRUE(result.getUniqueID().isDefaultValue());
}

TEST(MongoDatabaseHandlerTest, UpsertDocument)
{
	// Attempt to update an existing document using a partial BSON

	auto handler = getHandler();
	ASSERT_TRUE(handler);

	auto id = repo::lib::RepoUUID::createUUID();

	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", id);
	builder.append("field1", "myString");
	builder.append("field2", 0);

	auto existing = builder.obj();

	std::string errMsg;

	auto collection = "upsertTestCollection";
	handler->dropCollection(REPO_GTEST_DBNAME3, collection, errMsg); // Should usually be empty but if this test has run before...
	handler->createCollection(REPO_GTEST_DBNAME3, collection);

	handler->insertDocument(REPO_GTEST_DBNAME3, collection, existing, errMsg);

	// Reading the document back, it should be identical

	auto documents = handler->getAllFromCollectionTailable(REPO_GTEST_DBNAME3, collection);

	EXPECT_THAT(documents.size(), Eq(1));
	EXPECT_THAT(documents[0], Eq(existing));

	// Create a partial document that adds a field, and updates a field

	repo::core::model::RepoBSONBuilder updateBuilder;
	updateBuilder.append("_id", id);
	updateBuilder.append("field2", 123);
	updateBuilder.append("field3", repo::lib::RepoUUID::createUUID());

	auto update = updateBuilder.obj();

	handler->upsertDocument(REPO_GTEST_DBNAME3, collection, update, false, errMsg); // Should use _id as the primary key

	documents = handler->getAllFromCollectionTailable(REPO_GTEST_DBNAME3, collection);

	// There should only be one document in the collection still, because it
	// should have been updated in place.

	EXPECT_THAT(documents.size(), Eq(1));

	// This time the document should be a combination of both previous BSONs

	auto document = documents[0];

	EXPECT_THAT(document.nFields(), Eq(4));

	EXPECT_THAT(document.getUUIDField("_id"), Eq(id));
	EXPECT_THAT(document.getStringField("field1"), Eq(existing.getStringField("field1"))); // The unchanged value
	EXPECT_THAT(document.getIntField("field2"), Eq(update.getIntField("field2"))); // The updated value
	EXPECT_THAT(document.getUUIDField("field3"), Eq(update.getUUIDField("field3"))); // The new value
}

TEST(MongoDatabaseHandlerTest, UpsertDocumentBinary)
{
	// Upserting is not supported for BSONS with bin mappings

	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	repo::core::model::RepoBSONBuilder builder;
	builder.appendLargeArray("bin", testing::makeRandomBinary());

	EXPECT_THROW({
		handler->upsertDocument(REPO_GTEST_DBNAME3, "sbCollection", builder.obj(), false, errMsg);
	},
	repo::lib::RepoException);
}

TEST(MongoDatabaseHandlerTest, InsertDocumentBinary)
{
	// Inserting is not supported for BSONS with bin mappings

	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	repo::core::model::RepoBSONBuilder builder;
	builder.appendLargeArray("bin", testing::makeRandomBinary());

	EXPECT_THROW({
		handler->insertDocument(REPO_GTEST_DBNAME3, "sbCollection", builder.obj(), errMsg);
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
	std::string errMsg;

	auto collection = "insertManyDocumentsBinary";
	handler->dropCollection(REPO_GTEST_DBNAME3, collection, errMsg);
	handler->createCollection(REPO_GTEST_DBNAME3, collection);
	handler->insertManyDocuments(REPO_GTEST_DBNAME3, collection, documents, errMsg);

	// Read back the documents; this should also populate the binary buffers

	auto actual = handler->getAllFromCollectionTailable(REPO_GTEST_DBNAME3, collection);

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
	handler->dropCollection(REPO_GTEST_DBNAME3, collection, errMsg);
	handler->createCollection(REPO_GTEST_DBNAME3, collection);
	handler->insertManyDocuments(REPO_GTEST_DBNAME3, collection, documents, errMsg, metadata);

	// Read back the documents

	auto actuals = handler->getAllFromCollectionTailable(REPO_GTEST_DBNAME3, collection);
	auto actual = actuals[0];

	// Get the ref node from the database. We have to access the BSON directly to
	// do this, because the RepoRef type is not designed to read metadata, only
	// write it.

	auto ref = actual.getBinaryReference();
	repo::core::model::RepoBSONBuilder criteriaBuilder;
	criteriaBuilder.append(REPO_LABEL_ID, ref.getStringField(REPO_LABEL_BINARY_FILENAME));

	auto refNode = handler->findOneByCriteria(
		REPO_GTEST_DBNAME3,
		collection + std::string(".") + REPO_COLLECTION_EXT_REF,
		criteriaBuilder.obj()
	);

	EXPECT_THAT(refNode.getUUIDField("x-uuid"), Eq(boost::get<repo::lib::RepoUUID>(metadata["x-uuid"])));
	EXPECT_THAT(refNode.getIntField("x-int"), Eq(boost::get<int>(metadata["x-int"])));
	EXPECT_THAT(refNode.getStringField("x-string"), Eq(boost::get<std::string>(metadata["x-string"])));
}