/**
*  Copyright (C) 2019 3D Repo Ltd
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
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include <repo/core/handler/fileservice/repo_file_manager.h>
#include <repo/core/model/repo_model_global.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/lib/repo_exception.h>
#include <repo/lib/repo_utils.h>
#include "../../../../repo_test_fileservice_info.h"
#include "../../../../repo_test_utils.h"
#include <filesystem>

using namespace repo::core::handler::fileservice;

TEST(FileManager, InstantiateManager)
{
	auto empty = std::weak_ptr<repo::core::handler::AbstractDatabaseHandler>();

	// FileManager must be initialised with the fs config
	EXPECT_THROW({
		new FileManager(repo::lib::RepoConfig::fromFile(getDataPath("config/withS3.json")), empty);
	},
	repo::lib::RepoException);
}

TEST(FileManager, UploadFileAndCommitStringId)
{
	// Test if we can upload a file and retrieve it with getRefNode
	// and getFileRef.
	// We should be able to do this when the filename is both a string
	// and a UUID. This test tests the string version.

	auto handler = getHandler();
	auto manager = handler->getFileManager();
	auto db = "testFileManager";
	std::string col = "fileUpload";

	std::string content = "Test File Contents 1";
	std::vector<uint8_t> expected(content.begin(), content.end());
	expected.resize(1024);

	auto id = repo::lib::RepoUUID().createUUID().toString();
	EXPECT_TRUE(manager->uploadFileAndCommit(db, col, id, expected));

	auto ref = manager->getFileRef(db, col, id);
	EXPECT_FALSE(ref.getRefLink().empty());

	auto actual = manager->getFile(db, col, id);
	EXPECT_EQ(expected, actual);
}

TEST(FileManager, UploadFileAndCommitUUIDId)
{
	// Test if we can upload a file and retrieve it with getRefNode
	// and getFileRef.
	// We should be able to do this when the filename is both a string
	// and a UUID. This test tests the UUID version.

	auto handler = getHandler();
	auto manager = handler->getFileManager();
	ASSERT_TRUE(manager);
	auto db = "testFileManager";
	std::string col = "fileUpload";

	std::string content = "Test File Contents 2";
	std::vector<uint8_t> expected(content.begin(), content.end());
	expected.resize(1024);

	auto id = repo::lib::RepoUUID().createUUID();
	EXPECT_TRUE(manager->uploadFileAndCommit(db, col, id, expected));

	auto ref = manager->getFileRef(db, col, id);
	EXPECT_FALSE(ref.getRefLink().empty());

	auto actual = manager->getFile(db, col, id);
	EXPECT_EQ(expected, actual);
}

TEST(FileManager, deleteFileAndRef)
{
	auto handler = getHandler();
	auto manager = handler->getFileManager();

	ASSERT_TRUE(manager);
	auto db = "testFileManager";
	std::string col = "testFileUpload";
	auto fileName = "testFileToRemove";
	auto dataPathName = getDataPath("fileShare/dir1/dir2/dir3/someFile");
	ASSERT_TRUE(repo::lib::doesFileExist(dataPathName));

	EXPECT_TRUE(manager->deleteFileAndRef(db, col, {fileName}));

	auto res = handler->findOneByUniqueID(db, col + "." + REPO_COLLECTION_EXT_REF, fileName);
	EXPECT_TRUE(res.isEmpty());

	EXPECT_FALSE(repo::lib::doesFileExist(dataPathName));

	// Deleting a file a second time should not do anything, but not throw either
	EXPECT_FALSE(manager->deleteFileAndRef(db, col, {fileName}));
}

TEST(FileManager, GetExtFsPath)
{
	using namespace repo::core::model;
	using namespace testing;

	auto handler = getHandler();

	std::string refId = repo::lib::RepoUUID().createUUID().toString();
	std::string teamspace = "FileManagerTest";
	std::string collection = testing::getRandomContainer();
	std::string extFilePath = "FileManager/external_file.txt";
	std::string extFileName = "external_file.txt";

	// Build an EXTFS node explicitly, as serialising an EXTFS node is not allowed

	{
		RepoBSONBuilder builder;
		builder.append(REPO_LABEL_ID, refId);
		builder.append(REPO_REF_LABEL_TYPE, RepoRef::REPO_REF_TYPE_EXTFS);
		builder.append(REPO_REF_LABEL_LINK, extFilePath);
		builder.append(REPO_REF_LABEL_NAME, extFileName);
		builder.append(REPO_REF_LABEL_SIZE, (int64_t)19);

		handler->insertDocument(
			teamspace,
			collection + "." + REPO_COLLECTION_EXT_REF,
			builder.obj()
		);
	}

	auto ref = handler->getFileManager()->getFileRef(teamspace, collection, refId);
	EXPECT_THAT(ref.getType(), Eq(RepoRef::RefType::EXTFS));

	{
		// This config file does not have an extfs path configured.

		auto config = repo::lib::RepoConfig::fromFile(getDataPath("config/configNoExtFs.json"));

		// Even though fs is not used in this test, a valid config must be provided
		// otherwise fs handler will throw.

		config.configureFS(getDataPath("fileShare"));

		// Extfs is optional, as not all bouncer operations (e.g. clash) require access
		// to it. When it is accessed though, FileManager should asser that it is
		// configured.

		handler->setFileManager(std::make_shared<repo::core::handler::fileservice::FileManager>(config, handler));

		EXPECT_THROW(handler->getFileManager()->getFilePath(ref), repo::lib::RepoException);
	}

	{
		auto config = repo::lib::RepoConfig::fromFile(getDataPath("config/config.json"));
		config.configureFS(getDataPath("fileShare"));
		config.configureExtFS(getDataPath(""));
		handler->setFileManager(std::make_shared<repo::core::handler::fileservice::FileManager>(config, handler));

		auto mountPoint = std::filesystem::path(getDataPath(""));
		auto expectedPath = mountPoint / extFilePath;

		EXPECT_THAT(handler->getFileManager()->getFilePath(ref), Eq(expectedPath.string()));
	}
}