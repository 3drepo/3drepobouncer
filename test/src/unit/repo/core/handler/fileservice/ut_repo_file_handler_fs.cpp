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
#include <repo/core/handler/fileservice/repo_file_handler_fs.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/lib/repo_exception.h>
#include <repo/lib/repo_utils.h>
#include "../../../../repo_test_database_info.h"

using namespace testing;
using namespace repo::core::handler::fileservice;

TEST(FSFileHandlerTest, GetHandler)
{
	EXPECT_NO_THROW(FSFileHandler(getDataPath("fileShare"), 2));
	EXPECT_THROW(FSFileHandler(getDataPath("fdlfkjdlkfsjd"), 2), repo::lib::RepoException);
	EXPECT_THROW(FSFileHandler(getDataPath(simpleModel), 2), repo::lib::RepoException);
}

FSFileHandler createHandler() {
	return FSFileHandler(getDataPath("fileShare"), 2);
}


TEST(FSFileHandlerTest, deleteFile)
{
	auto handler = createHandler();
	auto pathToFile = getDataPath("fileShare/deleteTest");
	ASSERT_TRUE(repo::lib::doesFileExist(pathToFile));
	EXPECT_TRUE(handler.deleteFile("a", "b", "deleteTest"));
	EXPECT_FALSE(repo::lib::doesFileExist(pathToFile));
	EXPECT_FALSE(handler.deleteFile("a", "b", "ThisFileDoesNotExist"));

}

TEST(FSFileHandlerTest, writeFile)
{
	auto handler = createHandler();
	std::vector<uint8_t> buffer;
	buffer.resize(1024);
	auto linker = handler.uploadFile("a", "b", "newFile", buffer);
	EXPECT_FALSE(linker.empty());
	auto fullPath = getDataPath("fileShare/" + linker);
	EXPECT_TRUE(repo::lib::doesFileExist(fullPath));
}

TEST(FSFileHandlerTest, readFileStream)
{
	auto handler = createHandler();
	auto pathToFile = getDataPath("fileShare/readTest");
	ASSERT_TRUE(repo::lib::doesFileExist(pathToFile));

	auto stream = handler.getFileStream("", "", "readTest");

	EXPECT_TRUE(stream.is_open());

	std::vector<char> buffer;
	buffer.resize(28);
	stream.read(buffer.data(), buffer.size());

	std::string str(buffer.begin(), buffer.end());
	EXPECT_THAT(str, Eq("This is a 3D Repo test file."));
}

TEST(FSFileHandlerTest, readFileStreamMultiAccess)
{
	// For the multi-threading in the clash engine, we need to be able to
	// get multiple file streams from the same file from the same handler
	// and be able to read from them independently. This tests that.

	auto handler = createHandler();
	auto pathToFile = getDataPath("fileShare/readTest");
	ASSERT_TRUE(repo::lib::doesFileExist(pathToFile));

	// Get first stream
	auto stream1 = handler.getFileStream("", "", "readTest");
	EXPECT_TRUE(stream1.is_open());

	// Get second stream
	auto stream2 = handler.getFileStream("", "", "readTest");
	EXPECT_TRUE(stream2.is_open());

	// Set position to somewhere in the middle for the first stream and read
	std::vector<char> buffer1;
	buffer1.resize(15);
	stream1.seekg(13, std::ios_base::beg);
	stream1.read(buffer1.data(), buffer1.size());
	std::string str1(buffer1.begin(), buffer1.end());
	EXPECT_THAT(str1, Eq("Repo test file."));

	// Read second stream from a different position.
	// In this case, the beginning.
	std::vector<char> buffer2;
	buffer2.resize(13);
	stream2.read(buffer2.data(), buffer2.size());
	std::string str2(buffer2.begin(), buffer2.end());
	EXPECT_THAT(str2, Eq("This is a 3D "));
}
