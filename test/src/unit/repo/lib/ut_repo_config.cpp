/**
*  Copyright (C) 2016 3D Repo Ltd
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
#include <repo/lib/repo_config.h>
#include <repo/lib/repo_exception.h>
#include <gtest/gtest.h>

#include "../../repo_test_database_info.h"

using namespace repo::lib;

TEST(RepoConfigTest, ConstructFromFile) 
{
	EXPECT_NO_THROW({
		auto config = RepoConfig::fromFile(getDataPath("/config/justDB.json"));
		EXPECT_TRUE(config.validate());
	});

	EXPECT_THROW(RepoConfig::fromFile(getDataPath("/empty.json")), RepoException);

	EXPECT_THROW(RepoConfig::fromFile(getDataPath("/config/doesntExist.json")), RepoException);

	EXPECT_NO_THROW({
		auto config = RepoConfig::fromFile(getDataPath("/config/withS3.json"));
		EXPECT_TRUE(config.validate());
		EXPECT_TRUE(config.getS3Config().configured);
		EXPECT_FALSE(config.getFSConfig().configured);
		EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::GRIDFS);
	});

	EXPECT_NO_THROW({
		auto config = RepoConfig::fromFile(getDataPath("/config/withS3Default.json"));
		EXPECT_TRUE(config.getS3Config().configured);
		EXPECT_FALSE(config.getFSConfig().configured);
		EXPECT_TRUE(config.validate());
		EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::S3);
	});


	EXPECT_NO_THROW({
		auto config = RepoConfig::fromFile(getDataPath("/config/withFS.json"));
		EXPECT_TRUE(config.validate());
		EXPECT_FALSE(config.getS3Config().configured);
		EXPECT_TRUE(config.getFSConfig().configured);
		EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::FS);
	});

	EXPECT_NO_THROW({
		auto config = RepoConfig::fromFile(getDataPath("/config/withAllDbDefault.json"));
		EXPECT_TRUE(config.validate());
		EXPECT_TRUE(config.getS3Config().configured);
		EXPECT_TRUE(config.getFSConfig().configured);
		EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::GRIDFS);
	});

	EXPECT_NO_THROW({
		auto config = RepoConfig::fromFile(getDataPath("/config/withAllNoDefault.json"));
		EXPECT_TRUE(config.validate());
		EXPECT_TRUE(config.getS3Config().configured);
		EXPECT_TRUE(config.getFSConfig().configured);
		EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::FS);
	});

	EXPECT_NO_THROW({
		auto config = RepoConfig::fromFile(getDataPath("/config/withAllS3Default.json"));
		EXPECT_TRUE(config.validate());
		EXPECT_TRUE(config.getS3Config().configured);
		EXPECT_TRUE(config.getFSConfig().configured);
		EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::S3);
	});





}

TEST(RepoConfigTest, dbConfigTest)
{
	std::string db = "database", user = "username", password = "password";
	int port = 10000;
	bool pwDigested = true;
	RepoConfig config = {db, port, user, password, pwDigested };
	auto dbData = config.getDatabaseConfig();
	EXPECT_EQ(dbData.addr, db);
	EXPECT_EQ(dbData.port, port);
	EXPECT_EQ(dbData.username, user);
	EXPECT_EQ(dbData.password, password);
	EXPECT_EQ(dbData.pwDigested, pwDigested);

	EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::GRIDFS);
}

repo::lib::RepoConfig createConfig() {

	std::string db = "database", user = "username", password = "password";
	int port = 10000;
	bool pwDigested = true;
	RepoConfig config = { db, port, user, password, pwDigested };

	return config;
}

TEST(RepoConfigTest, s3ConfigTest)
{
	auto config = createConfig();
	std::string bucketName = "b1", bucketRegion = "r1", bucketName1 = "b2", bucketRegion1 = "r2";
	config.configureS3(bucketName, bucketRegion, false);
	auto s3Conf = config.getS3Config();
	EXPECT_EQ(s3Conf.bucketName, bucketName);
	EXPECT_EQ(s3Conf.bucketRegion, bucketRegion);
	EXPECT_TRUE(s3Conf.configured);

	EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::GRIDFS);

	config.configureS3(bucketName1, bucketRegion1, true);

	s3Conf = config.getS3Config();
	EXPECT_EQ(s3Conf.bucketName, bucketName1);
	EXPECT_EQ(s3Conf.bucketRegion, bucketRegion1);
	EXPECT_TRUE(s3Conf.configured);

	EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::S3);
}

TEST(RepoConfigTest, fsConfigTest)
{
	auto config = createConfig();
	std::string dir1 = "p1", dir2 = "p2";
	int level1 = 10, level2 = 1;
	config.configureFS(dir1, level1, false);
	auto fsConf = config.getFSConfig();
	EXPECT_EQ(fsConf.dir, dir1);
	EXPECT_EQ(fsConf.nLevel, level1);
	EXPECT_TRUE(fsConf.configured);

	EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::GRIDFS);

	config.configureFS(dir2, level2, true);
	fsConf = config.getFSConfig();
	EXPECT_EQ(fsConf.dir, dir2);
	EXPECT_EQ(fsConf.nLevel, level2);
	EXPECT_TRUE(fsConf.configured);

	EXPECT_EQ(config.getDefaultStorageEngine(), repo::lib::RepoConfig::FileStorageEngine::FS);


}

TEST(RepoConfigTest, validationTestDB)
{
	EXPECT_TRUE(createConfig().validate());

	std::string dummy = "dummy";

	//database connection a negative port
	EXPECT_FALSE(RepoConfig(dummy, -2, dummy, dummy).validate());

	//database connection with no credentials
	EXPECT_TRUE(RepoConfig(dummy, 1, "", "").validate());

	//database connection with username but no password
	EXPECT_FALSE(RepoConfig(dummy, 1, dummy, "").validate());

	//database connection with password but no username
	EXPECT_FALSE(RepoConfig(dummy, 1, "", dummy).validate());

}

TEST(RepoConfigTest, validationTestS3)
{
	std::string dummy = "dummy";
	auto config = createConfig();

	config.configureS3(dummy, dummy);
	EXPECT_TRUE(config.validate());

	config.configureS3("", "");
	EXPECT_FALSE(config.validate());

	config.configureS3(dummy, "");
	EXPECT_FALSE(config.validate());

	config.configureS3("", dummy);
	EXPECT_FALSE(config.validate());
}

TEST(RepoConfigTest, validationTestFS)
{
	std::string dummy = "dummy";
	auto config = createConfig();

	config.configureFS(dummy, 1);
	EXPECT_TRUE(config.validate());

	config.configureFS("", 0);
	EXPECT_FALSE(config.validate());

	config.configureFS(dummy,-1);
	EXPECT_FALSE(config.validate());

}

