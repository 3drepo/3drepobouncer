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
#include <repo/core/handler/fileservice/repo_file_handler_fs.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/lib/repo_exception.h>
#include <repo/lib/repo_utils.h>
#include "../../../../repo_test_database_info.h"

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
