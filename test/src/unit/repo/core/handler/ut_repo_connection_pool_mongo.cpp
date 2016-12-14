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

#include <cstdlib>
#include <repo/core/handler/connectionpool/repo_connection_pool_mongo.h>
#include <gtest/gtest.h>

#include "../../../repo_test_database_info.h"

using namespace repo::core::handler::connectionPool;

mongo::BSONObj* createCredentialsBSON(
	const std::string &database,
	const std::string &user,
	const std::string &pw
)	
{
	std::string passwordDigest = mongo::DBClientWithCommands::createPasswordDigest(user, pw);
	return new mongo::BSONObj(BSON("user" << user <<
		"db" << database <<
		"pwd" << passwordDigest <<
		"digestPassword" << false));
}

TEST(MongoConnectionPoolTest, constructorTest)
{
	
	mongo::client::initialize();
	EXPECT_THROW(MongoConnectionPool(1, mongo::ConnectionString(mongo::HostAndPort("unknownAddress", 27017)), nullptr), mongo::DBException);
	EXPECT_THROW(MongoConnectionPool(1, mongo::ConnectionString(mongo::HostAndPort(REPO_GTEST_DBADDRESS, 12345)), nullptr), mongo::DBException);
	
	auto correctCred = createCredentialsBSON(REPO_GTEST_AUTH_DATABASE, REPO_GTEST_DBUSER, REPO_GTEST_DBPW);
	auto badCred = createCredentialsBSON(REPO_GTEST_AUTH_DATABASE, REPO_GTEST_DBUSER, "325325");
	auto badCred2 = createCredentialsBSON(REPO_GTEST_AUTH_DATABASE, "dlsfkj", REPO_GTEST_DBPW);
	auto badCred3 = createCredentialsBSON("test", REPO_GTEST_DBUSER, REPO_GTEST_DBPW);
	MongoConnectionPool(1, mongo::ConnectionString(mongo::HostAndPort(REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT)), correctCred);
	MongoConnectionPool(0, mongo::ConnectionString(mongo::HostAndPort(REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT)), nullptr);
	MongoConnectionPool(-1, mongo::ConnectionString(mongo::HostAndPort(REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT)), nullptr);

	EXPECT_THROW(MongoConnectionPool(1, mongo::ConnectionString(mongo::HostAndPort(REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT)), badCred), mongo::DBException);
	EXPECT_THROW(MongoConnectionPool(1, mongo::ConnectionString(mongo::HostAndPort(REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT)), badCred2), mongo::DBException);
	EXPECT_THROW(MongoConnectionPool(1, mongo::ConnectionString(mongo::HostAndPort(REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT)), badCred3), mongo::DBException);

	delete correctCred;
	delete badCred;
	delete badCred2;
	delete badCred3;
}

TEST(MongoConnectionPoolTest, workerMechanicsTest)
{
	auto correctCred = createCredentialsBSON(REPO_GTEST_AUTH_DATABASE, REPO_GTEST_DBUSER, REPO_GTEST_DBPW);

	MongoConnectionPool pool(2, mongo::ConnectionString(mongo::HostAndPort(REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT)), correctCred, 1, 10);

	mongo::DBClientBase* nul = nullptr;
	pool.returnWorker(nul);
	auto pop1 = pool.getWorker();
	auto pop2 = pool.getWorker();
	EXPECT_TRUE(pop1);
	EXPECT_TRUE(pop2);
	EXPECT_FALSE(pool.getWorker());

	pool.returnWorker(pop1);
	EXPECT_FALSE(pop1);
	pop1 = pool.getWorker();
	EXPECT_TRUE(pop1);

}