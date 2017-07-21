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
#include <repo/core/handler/repo_database_handler_mongo.h>
#include <repo/core/model/bson/repo_node.h>
#include "../../../repo_test_database_info.h"

using namespace repo::core::handler;

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

TEST(MongoDatabaseHandlerTest, CountItemsInCollection)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	auto goldenData = getCollectionCounts(REPO_GTEST_DBNAME1);
	for (const auto &pair : goldenData)
	{
		std::string message;
		EXPECT_EQ(pair.second, handler->countItemsInCollection(REPO_GTEST_DBNAME1, pair.first, message));
		EXPECT_TRUE(message.empty());
	}

	goldenData = getCollectionCounts(REPO_GTEST_DBNAME2);
	for (const auto &pair : goldenData)
	{
		std::string message;
		EXPECT_EQ(pair.second, handler->countItemsInCollection(REPO_GTEST_DBNAME2, pair.first, message));
		EXPECT_TRUE(message.empty());
	}

	std::string message;
	EXPECT_EQ(0, handler->countItemsInCollection("", "", message));
	EXPECT_FALSE(message.empty());

	message.clear();
	EXPECT_EQ(0, handler->countItemsInCollection("", "blah", message));
	EXPECT_FALSE(message.empty());

	message.clear();
	EXPECT_EQ(0, handler->countItemsInCollection("blah", "", message));
	EXPECT_FALSE(message.empty());

	message.clear();
	EXPECT_EQ(0, handler->countItemsInCollection("blah", "blah", message));
	EXPECT_TRUE(message.empty());
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

	EXPECT_EQ(bsonsLimitSkip[0].toString(), bsons[1].toString());

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

	for (const auto &col : collections)
	{
		std::cout << col << std::endl;
	}

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

TEST(MongoDatabaseHandlerTest, GetCollectionStats)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	auto golden = getCollectionStats();
	std::string message;
	repo::core::model::CollectionStats stats = handler->getCollectionStats(golden.first.first, golden.first.second, message);

	EXPECT_TRUE(message.empty());
	EXPECT_FALSE(stats.isEmpty());
	ASSERT_TRUE(stats.nFields() > golden.second.nFields());
	std::set<std::string> fields;
	golden.second.getFieldNames(fields);

	for (const auto &fieldName : fields)
	{
		ASSERT_TRUE(stats.hasField(fieldName));
		EXPECT_EQ(stats.getField(fieldName).toString(), golden.second.getField(fieldName).toString());
	}
	//check error handling - make sure it doesn't crash
	message.clear();
	EXPECT_TRUE(handler->getCollectionStats("", golden.first.second, message).isEmpty());
	EXPECT_FALSE(message.empty());

	message.clear();
	EXPECT_TRUE(handler->getCollectionStats(golden.first.first, "", message).isEmpty());
	EXPECT_FALSE(message.empty());

	message.clear();
	//Shoudl return with collection not found
	EXPECT_EQ(0, handler->getCollectionStats("nonExistent", "blah", message).getField("ok").Double());
	EXPECT_TRUE(message.empty());

	message.clear();
	EXPECT_EQ(0.0, handler->getCollectionStats(golden.first.first, "blah", message).getField("ok").Double());
	EXPECT_TRUE(message.empty());
}

TEST(MongoDatabaseHandlerTest, GetDatabases)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	std::list<std::string> dbList = handler->getDatabases();

	//there are probably bogus databases in there due to the previous tests automatically creating them
	//so just check that the 2 we are looking for is in there.

	EXPECT_FALSE(std::find(dbList.begin(), dbList.end(), REPO_GTEST_DBNAME1) == dbList.end());
	EXPECT_FALSE(std::find(dbList.begin(), dbList.end(), REPO_GTEST_DBNAME2) == dbList.end());
}

TEST(MongoDatabaseHandlerTest, GetDatabasesWithProjects)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string fakeDB = "nonExistentDB";
	auto dbWithProjects = handler->getDatabasesWithProjects({ REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME2, fakeDB });
	ASSERT_EQ(3, dbWithProjects.size());

	//Should be able to find all 3 database on the returning map
	EXPECT_FALSE(dbWithProjects.find(REPO_GTEST_DBNAME1) == dbWithProjects.end());
	EXPECT_FALSE(dbWithProjects.find(REPO_GTEST_DBNAME2) == dbWithProjects.end());
	EXPECT_FALSE(dbWithProjects.find(fakeDB) == dbWithProjects.end());

	EXPECT_EQ(dbWithProjects[REPO_GTEST_DBNAME1].size(), 2);
	EXPECT_EQ(dbWithProjects[REPO_GTEST_DBNAME2].size(), 1);
	EXPECT_EQ(dbWithProjects[fakeDB].size(), 0);

	EXPECT_EQ(dbWithProjects[REPO_GTEST_DBNAME1].front(), REPO_GTEST_DBNAME1_PROJ);
	EXPECT_EQ(dbWithProjects[REPO_GTEST_DBNAME2].front(), REPO_GTEST_DBNAME2_PROJ);

	//If i change the extenion to something that doesn't exist, it shouldn't give me any projects
	auto dbWithProjects2 = handler->getDatabasesWithProjects({ REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME2, fakeDB }, "blahblah");
	EXPECT_FALSE(dbWithProjects2.find(REPO_GTEST_DBNAME1) == dbWithProjects2.end());
	EXPECT_FALSE(dbWithProjects2.find(REPO_GTEST_DBNAME2) == dbWithProjects2.end());
	EXPECT_FALSE(dbWithProjects2.find(fakeDB) == dbWithProjects2.end());

	EXPECT_EQ(dbWithProjects2[REPO_GTEST_DBNAME1].size(), 0);
	EXPECT_EQ(dbWithProjects2[REPO_GTEST_DBNAME2].size(), 0);
	EXPECT_EQ(dbWithProjects2[fakeDB].size(), 0);
}

TEST(MongoDatabaseHandlerTest, GetProjects)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	auto projects = handler->getProjects(REPO_GTEST_DBNAME1, "history");
	ASSERT_EQ(projects.size(), 2);
	EXPECT_EQ(projects.front(), REPO_GTEST_DBNAME1_PROJ);

	projects = handler->getProjects(REPO_GTEST_DBNAME1, "blah");
	EXPECT_EQ(projects.size(), 0);

	projects = handler->getProjects("noDB", "history");
	EXPECT_EQ(projects.size(), 0);
}

TEST(MongoDatabaseHandlerTest, GetAdminDatabaseRoles)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	//This should be non zero.
	EXPECT_TRUE(handler->getAdminDatabaseRoles().size());
}

TEST(MongoDatabaseHandlerTest, GetAdminDatabaseName)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	//This should not be an empty string
	EXPECT_FALSE(handler->getAdminDatabaseName().empty());
}

TEST(MongoDatabaseHandlerTest, GetStandardDatabaseRoles)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	//This should be non zero.
	EXPECT_TRUE(handler->getStandardDatabaseRoles().size());
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

TEST(MongoDatabaseHandlerTest, DropDatabase)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	EXPECT_TRUE(handler->dropDatabase(REPO_GTEST_DROPCOL_TESTCASE.first, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	//Apparently if you drop the database that doesn't exist it still returns true... Which is inconsistent to dropCollection..
	EXPECT_TRUE(handler->dropDatabase(REPO_GTEST_DROPCOL_TESTCASE.first, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropDatabase("", errMsg));
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

TEST(MongoDatabaseHandlerTest, DropRole)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;
	EXPECT_TRUE(handler->dropRole(repo::core::model::RepoRole(REPO_GTEST_DROPROLETEST), errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropRole(repo::core::model::RepoRole(REPO_GTEST_DROPROLETEST), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropRole(repo::core::model::RepoRole(mongo::BSONObj()), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}

TEST(MongoDatabaseHandlerTest, DropUser)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;
	EXPECT_TRUE(handler->dropUser(repo::core::model::RepoUser(REPO_GTEST_DROPUSERTEST), errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropUser(repo::core::model::RepoUser(REPO_GTEST_DROPUSERTEST), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->dropUser(repo::core::model::RepoUser(mongo::BSONObj()), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}

TEST(MongoDatabaseHandlerTest, InsertDocument)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	repo::core::model::RepoBSON testCase = BSON("_id" << "testID" << "anotherField" << std::rand());
	std::string database = "sandbox";
	std::string collection = "sbCollection";
	EXPECT_TRUE(handler->insertDocument(database, collection, testCase, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();

	//The following will of cousre fail if findOneByCriteria is failing
	repo::core::model::RepoBSON result = handler->findOneByCriteria(database, collection, testCase);
	EXPECT_FALSE(result.isEmpty());

	std::set<std::string> fields;
	result.getFieldNames(fields);
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

TEST(MongoDatabaseHandlerTest, InsertRawFile)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	std::vector<uint8_t> binary;
	for (int i = 0; i < 100; ++i)
	{
		binary.push_back(std::rand());
	}

	EXPECT_TRUE(handler->insertRawFile("randomTest", "randomCol", "rawFileName", binary, errMsg));
	EXPECT_TRUE(errMsg.empty());

	std::vector<uint8_t> result = handler->getRawFile("randomTest", "randomCol", "rawFileName");
	ASSERT_EQ(result.size(), binary.size());
	for (int i = 0; i < binary.size(); ++i)
		EXPECT_EQ(binary[i], result[i]);

	errMsg.clear();
	EXPECT_FALSE(handler->insertRawFile("randomTest", "", "rawFileName", binary, errMsg));
	EXPECT_FALSE(errMsg.empty());

	errMsg.clear();
	EXPECT_FALSE(handler->insertRawFile("", "randomCol", "rawFileName", binary, errMsg));
	EXPECT_FALSE(errMsg.empty());

	errMsg.clear();
	EXPECT_FALSE(handler->insertRawFile("randomTest", "randomCol", "", binary, errMsg));
	EXPECT_FALSE(errMsg.empty());

	errMsg.clear();
	EXPECT_FALSE(handler->insertRawFile("randomTest", "randomCol", "rawFileName", std::vector<uint8_t>(), errMsg));
	EXPECT_FALSE(errMsg.empty());
}

TEST(MongoDatabaseHandlerTest, InsertRole)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	repo::core::model::RepoRole roleTest = repo::core::model::RepoRole(BSON("db" << "admin" << "role" << "insertRoleTest"));

	EXPECT_TRUE(handler->insertRole(roleTest, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();

	//The following will of cousre fail if findOneByCriteria is failing
	repo::core::model::RepoBSON result = handler->findOneByCriteria("admin", "system.roles", roleTest);
	EXPECT_FALSE(result.isEmpty());

	std::set<std::string> fields;
	roleTest.getFieldNames(fields);
	for (const auto &fname : fields)
	{
		ASSERT_TRUE(result.hasField(fname));
		EXPECT_EQ(result.getField(fname), roleTest.getField(fname));
	}

	EXPECT_FALSE(handler->insertRole(repo::core::model::RepoRole(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->insertRole(repo::core::model::RepoRole(BSON("role" << "insertRoleTest")), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->insertRole(repo::core::model::RepoRole(BSON("db" << "admin")), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();

	//drop it after the test is done
	handler->dropRole(roleTest, errMsg);
}

TEST(MongoDatabaseHandlerTest, InsertUser)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;
	repo::core::model::RepoUser userTest = repo::core::model::RepoUser(BSON("db" << "admin" << "user" << "insertUserTest"
		<< REPO_USER_LABEL_CREDENTIALS << BSON(REPO_USER_LABEL_CLEARTEXT << "123")));
	EXPECT_TRUE(handler->insertUser(userTest, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();

	//The following will of cousre fail if findOneByCriteria is failing
	repo::core::model::RepoBSON result = handler->findOneByCriteria("admin", "system.users", BSON("_id" << "admin.insertUserTest"));
	EXPECT_FALSE(result.isEmpty());

	EXPECT_FALSE(handler->insertUser(repo::core::model::RepoUser(), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->insertUser(repo::core::model::RepoUser(BSON("_id" << "insertUserTest")), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
	EXPECT_FALSE(handler->insertUser(repo::core::model::RepoUser(BSON("db" << "admin")), errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();

	handler->dropUser(userTest, errMsg);
}

TEST(MongoDatabaseHandlerTest, UpsertDocument)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	repo::core::model::RepoBSON testCase = BSON("_id" << repo::lib::RepoUUID::createUUID().toString() << "anotherField" << std::rand());
	std::string database = "sandbox";
	std::string collection = "sbCollection";
	EXPECT_TRUE(handler->upsertDocument(database, collection, testCase, false, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();

	//The following will of cousre fail if findOneByCriteria is failing
	repo::core::model::RepoBSON result = handler->findOneByCriteria(database, collection, testCase);
	EXPECT_FALSE(result.isEmpty());

	std::set<std::string> fields;
	result.getFieldNames(fields);
	for (const auto &fname : fields)
	{
		ASSERT_TRUE(testCase.hasField(fname));
		EXPECT_EQ(result.getField(fname), testCase.getField(fname));
	}

	//upserting the same thing twice shouldn't cause any errors
	EXPECT_TRUE(handler->upsertDocument(database, collection, testCase, false, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();

	std::string id = testCase.getStringField("_id");
	repo::core::model::RepoBSON searchCriteria = BSON("_id" << id);
	repo::core::model::RepoBSON extraFields = BSON("_id" << id << "extraField" << std::rand());

	EXPECT_TRUE(handler->upsertDocument(database, collection, extraFields, false, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();

	//The following will of cousre fail if findOneByCriteria is failing
	result = handler->findOneByCriteria(database, collection, searchCriteria);
	EXPECT_FALSE(result.isEmpty());

	EXPECT_TRUE(result.hasField("anotherField"));
	EXPECT_TRUE(result.hasField("extraField"));

	EXPECT_TRUE(handler->upsertDocument(database, collection, extraFields, true, errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();

	//The following will of cousre fail if findOneByCriteria is failing
	result = handler->findOneByCriteria(database, collection, searchCriteria);
	EXPECT_FALSE(result.isEmpty());

	EXPECT_FALSE(result.hasField("anotherField"));
	EXPECT_TRUE(result.hasField("extraField"));

	EXPECT_FALSE(handler->upsertDocument(database, "", extraFields, false, errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();

	EXPECT_FALSE(handler->upsertDocument("", collection, extraFields, false, errMsg));
	EXPECT_FALSE(errMsg.empty());
	errMsg.clear();
}

TEST(MongoDatabaseHandlerTest, UpdateRole)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	EXPECT_TRUE(handler->updateRole(repo::core::model::RepoRole(REPO_GTEST_UPDATEROLETEST), errMsg));
	EXPECT_TRUE(errMsg.empty());

	//The following will of cousre fail if findOneByCriteria is failing
	repo::core::model::RepoRole result = repo::core::model::RepoRole(
		handler->findOneByCriteria("admin", "system.roles", repo::core::model::RepoRole(REPO_GTEST_UPDATEROLETEST)));
	EXPECT_FALSE(result.isEmpty());
	EXPECT_EQ(0, result.getPrivileges().size());

	EXPECT_FALSE(handler->updateRole(repo::core::model::RepoRole(), errMsg));
	EXPECT_FALSE(errMsg.empty());
}

TEST(MongoDatabaseHandlerTest, UpdateUser)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	std::string errMsg;

	EXPECT_TRUE(handler->updateUser(repo::core::model::RepoUser(REPO_GTEST_UPDATEUSERTEST), errMsg));
	EXPECT_TRUE(errMsg.empty());
	errMsg.clear();
	//The following will of cousre fail if findOneByCriteria is failing
	std::string userID = std::string(REPO_GTEST_UPDATEUSERTEST.getStringField("db")) + "." + std::string(REPO_GTEST_UPDATEUSERTEST.getStringField("user"));
	repo::core::model::RepoBSON search = BSON("_id" << userID);
	repo::core::model::RepoUser result = repo::core::model::RepoUser(
		handler->findOneByCriteria("admin", "system.users", repo::core::model::RepoUser(search)));
	EXPECT_FALSE(result.isEmpty());
	EXPECT_TRUE(result.getRolesBSON().isEmpty());

	EXPECT_FALSE(handler->updateUser(repo::core::model::RepoUser(), errMsg));
	EXPECT_FALSE(errMsg.empty());

	handler->dropUser(search, errMsg);
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
	EXPECT_FALSE(result.isEmpty());
	EXPECT_EQ(result.getSharedID(), repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH));

	result = handler->findOneBySharedID(REPO_GTEST_DBNAME_ROLEUSERTEST, "sampleProject.history", repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), "");
	EXPECT_FALSE(result.isEmpty());
	EXPECT_EQ(result.getSharedID(), repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH));

	result = handler->findOneBySharedID("", "sampleProject", repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), "timestamp");
	EXPECT_TRUE(result.isEmpty());
	result = handler->findOneBySharedID(REPO_GTEST_DBNAME_ROLEUSERTEST, "", repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH), "timestamp");
	EXPECT_TRUE(result.isEmpty());
}

TEST(MongoDatabaseHandlerTest, GetRawFile)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	auto file = handler->getRawFile(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ + ".history", REPO_GTEST_RAWFILE_FETCH_TEST);

	EXPECT_EQ(REPO_GTEST_RAWFILE_FETCH_SIZE, file.size());

	EXPECT_EQ(0, handler->getRawFile(REPO_GTEST_DBNAME1, REPO_GTEST_DBNAME1_PROJ + ".history", "some_non_existent_file").size());
	EXPECT_EQ(0, handler->getRawFile("", REPO_GTEST_DBNAME1_PROJ + ".history", REPO_GTEST_RAWFILE_FETCH_TEST).size());
	EXPECT_EQ(0, handler->getRawFile(REPO_GTEST_DBNAME1, "", REPO_GTEST_RAWFILE_FETCH_TEST).size());
}