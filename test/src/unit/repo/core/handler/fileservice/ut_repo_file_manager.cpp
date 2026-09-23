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
#include <repo/core/model/bson/repo_node.h>

#include <repo/lib/repo_exception.h>
#include <repo/lib/repo_utils.h>
#include "../../../../repo_test_fileservice_info.h"

using namespace repo::core::handler::fileservice;
using namespace testing;

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

	EXPECT_TRUE(manager->deleteFileAndRef(db, col, fileName));

	auto res = handler->findOneByUniqueID(db, col + "." + REPO_COLLECTION_EXT_REF, fileName);
	EXPECT_TRUE(res.isEmpty());

	EXPECT_FALSE(repo::lib::doesFileExist(dataPathName));

	// Deleting a file a second time should not do anything, but not throw either
	EXPECT_FALSE(manager->deleteFileAndRef(db, col, fileName));
}

TEST(FileManager, FileHandle)
{
	auto handler = getHandler();
	auto manager = handler->getFileManager();

	ASSERT_TRUE(manager);
	auto db = "testFileManager";
	std::string col = "fileHandleTests";

	std::string content1 = "File Handle Test";
	std::string content2 = " File Contents";

	std::vector<uint8_t> expected1(content1.begin(), content1.end());
	std::vector<uint8_t> expected2(content2.begin(), content2.end());

	std::vector<uint8_t> expected(expected1.begin(), expected1.end());
	expected.insert(expected.end(), expected2.begin(), expected2.end());


	// Test 1: Get file handle, write to it twice, turn it in, then check.
	// No encoding
	{
		auto id = repo::lib::RepoUUID().createUUID();
		auto handle = manager->requestFileHandle(
			db,
			col,
			id,
			{}
		);

		EXPECT_THAT(handle, Ne(nullptr));

		// Write data first time
		handle->writeData(expected1);

		// Write data second time
		handle->writeData(expected2);

		// Turn in the handle
		bool success = manager->turnInFileHandleForUpload(std::move(handle));
		EXPECT_THAT(success, Eq(true));
		EXPECT_THAT(handle, Eq(nullptr));

		// Check Ref
		auto ref = manager->getFileRef(db, col, id);
		EXPECT_FALSE(ref.getRefLink().empty());

		// Check content
		auto actual = manager->getFile(db, col, id);
		EXPECT_EQ(expected, actual);
	}

	// Test 2: Get file handle, write to it twice, turn it in, then check.
	// Gzip Encoding
	{
		auto id = repo::lib::RepoUUID().createUUID();
		auto handle = manager->requestFileHandle(
			db,
			col,
			id,
			{},
			repo::core::handler::fileservice::FileManager::Encoding::Gzip
		);

		EXPECT_THAT(handle, Ne(nullptr));

		// Write data first time
		handle->writeData(expected1);

		// Write data second time
		handle->writeData(expected2);

		// Turn in the handle
		bool success = manager->turnInFileHandleForUpload(std::move(handle));
		EXPECT_THAT(success, Eq(true));
		EXPECT_THAT(handle, Eq(nullptr));

		// Check Ref
		auto ref = manager->getFileRef(db, col, id);
		EXPECT_FALSE(ref.getRefLink().empty());

		// Check content
		auto actual = manager->getFile(db, col, id, repo::core::handler::fileservice::FileManager::Encoding::Gzip);
		EXPECT_EQ(expected, actual);
	}

	// Test 3: Try turning in file handle without ever writing to it.
	{
		auto id = repo::lib::RepoUUID().createUUID();
		auto handle = manager->requestFileHandle(
			db,
			col,
			id,
			{},
			repo::core::handler::fileservice::FileManager::Encoding::Gzip
		);

		EXPECT_THAT(handle, Ne(nullptr));

		auto linkName = handle->getLinkName();

		// Turn in the handle
		bool success = manager->turnInFileHandleForUpload(std::move(handle));
		EXPECT_THAT(success, Ne(true));
		EXPECT_THAT(handle, Eq(nullptr));

		auto fullPath = getDataPath("fileShare") + "/" + linkName;
		EXPECT_FALSE(repo::lib::doesFileExist(fullPath));
	}

	// Test 4: File stream goes away while writing
	{
		auto id = repo::lib::RepoUUID().createUUID();
		auto handle = manager->requestFileHandle(
			db,
			col,
			id,
			{},
			repo::core::handler::fileservice::FileManager::Encoding::Gzip
		);

		EXPECT_THAT(handle, Ne(nullptr));

		// Write data first time
		handle->writeData(expected1);

		// Close file stream
		auto fileStream = handle->getFileStream();
		fileStream->close();

		// Attempt to write data a second time
		EXPECT_THROW(handle->writeData(expected2), repo::lib::RepoException);

		// Attempt to turn in the handle for upload
		EXPECT_THROW(manager->turnInFileHandleForUpload(std::move(handle)), repo::lib::RepoException);
	}

	// Test 5: Operate multiple file handles at the same time
	{
		auto id1 = repo::lib::RepoUUID().createUUID();
		auto handle1 = manager->requestFileHandle(
			db,
			col,
			id1,
			{},
			repo::core::handler::fileservice::FileManager::Encoding::Gzip
		);

		auto id2 = repo::lib::RepoUUID().createUUID();
		auto handle2 = manager->requestFileHandle(
			db,
			col,
			id2,
			{}
		);

		EXPECT_THAT(handle1, Ne(nullptr));
		EXPECT_THAT(handle2, Ne(nullptr));

		// Write data to first handle
		handle1->writeData(expected1);

		// Write data to second handle
		handle2->writeData(expected2);

		// Turn in first handle and check data
		{
			bool success = manager->turnInFileHandleForUpload(std::move(handle1));
			EXPECT_THAT(success, Eq(true));
			EXPECT_THAT(handle1, Eq(nullptr));

			// Check Ref
			auto ref = manager->getFileRef(db, col, id1);
			EXPECT_FALSE(ref.getRefLink().empty());

			// Check content
			auto actual = manager->getFile(db, col, id1, repo::core::handler::fileservice::FileManager::Encoding::Gzip);
			EXPECT_EQ(expected1, actual);
		}

		// Turn in second handle and check data
		{
			bool success = manager->turnInFileHandleForUpload(std::move(handle2));
			EXPECT_THAT(success, Eq(true));
			EXPECT_THAT(handle2, Eq(nullptr));

			// Check Ref
			auto ref = manager->getFileRef(db, col, id2);
			EXPECT_FALSE(ref.getRefLink().empty());

			// Check content
			auto actual = manager->getFile(db, col, id2);
			EXPECT_EQ(expected2, actual);
		}
	}
}