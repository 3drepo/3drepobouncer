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
#include <repo/core/handler/fileservice/repo_file_manager.h>
#include <repo/core/model/bson/repo_node.h>

#include <repo/lib/repo_exception.h>
#include <repo/lib/repo_utils.h>
#include "../../../../repo_test_fileservice_info.h"

using namespace repo::core::handler::fileservice;

static FileManager* getManagerDefaultFS()
{
	FileManager::disconnect();
	auto config = repo::lib::RepoConfig::fromFile(getDataPath("config/withFS.json"));
	config.configureFS(getDataPath("fileShare"));
	return FileManager::instantiateManager(config, getHandler());
}

TEST(FileManager, GetManager)
{
	FileManager::disconnect();
	EXPECT_THROW(FileManager::getManager(), repo::lib::RepoException);
	EXPECT_NO_THROW(getManagerDefaultFS());
}

TEST(FileManager, InstantiateManager)
{
	FileManager::disconnect();
	EXPECT_THROW(FileManager::instantiateManager(repo::lib::RepoConfig::fromFile(getDataPath("config/withFS.json")), nullptr), repo::lib::RepoException);
}

TEST(FileManager, UploadFileAndCommitStringId)
{
	// Test if we can upload a file and retrieve it with getRefNode
	// and getFileRef.
	// We should be able to do this when the filename is both a string
	// and a UUID. This test tests the string version.

	auto manager = getManagerDefaultFS();
	ASSERT_TRUE(manager);
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

	auto manager = getManagerDefaultFS();
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
	auto manager = getManagerDefaultFS();
	ASSERT_TRUE(manager);
	auto db = "testFileManager";
	std::string col = "testFileUpload";
	auto fileName = "testFileToRemove";
	auto dataPathName = getDataPath("fileShare/dir1/dir2/dir3/someFile");
	ASSERT_TRUE(repo::lib::doesFileExist(dataPathName));

	EXPECT_TRUE(manager->deleteFileAndRef(db, col, fileName));

	auto dbHandler = getHandler();
	auto res = dbHandler->findOneByCriteria(db, col + "." + REPO_COLLECTION_EXT_REF, BSON("_id" << fileName));
	EXPECT_TRUE(res.isEmpty());

	EXPECT_FALSE(repo::lib::doesFileExist(dataPathName));

	// Calling getHandler above may change the file manager instance that is cached
	// in manager, so get it again from then on.
	EXPECT_FALSE(FileManager::getManager()->deleteFileAndRef(db, col, fileName));
}