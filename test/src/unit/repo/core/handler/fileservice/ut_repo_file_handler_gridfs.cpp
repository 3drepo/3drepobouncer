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
#include <repo/core/handler/fileservice/repo_file_handler_gridfs.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/lib/repo_exception.h>
#include <repo/lib/repo_utils.h>
#include "../../../../repo_test_database_info.h"

using namespace repo::core::handler::fileservice;

TEST(GridFSFileHandlerTest, Constructor)
{
	
	EXPECT_NO_THROW({
		auto fileHandler = GridFSFileHandler(getHandler());
	});
	EXPECT_THROW(GridFSFileHandler(nullptr), repo::lib::RepoException);
}

GridFSFileHandler getGridFSHandler()
{
	auto dbHandler = getHandler();
	return GridFSFileHandler(dbHandler);
}

TEST(GridFSFileHandlerTest, deleteFile)
{
	auto handler = getGridFSHandler();
	auto dbHandler = getHandler();
	std::string db = "testFileManager";
	std::string col = "testFileUpload";
	std::string fName = "gridFSFile";

	ASSERT_TRUE(dbHandler->getRawFile(db, col, fName).size() > 0);
	EXPECT_TRUE(handler.deleteFile(db, col, fName));
	EXPECT_EQ(dbHandler->getRawFile(db, col, fName).size(), 0);
	
}

TEST(GridFSFileHandlerTest, writeFile)
{
	auto handler = getGridFSHandler();
	std::vector<uint8_t> buffer;
	uint32_t fSize = 1024;
	buffer.resize(fSize);
	std::string db = "testFileManager";
	std::string col = "testFileUpload";
	std::string fName = "testFileWrite";
	auto linker = handler.uploadFile(db, col, fName, buffer);
	EXPECT_EQ(fName, linker);
	auto dbHandler = getHandler();

	EXPECT_EQ(dbHandler->getRawFile(db, col, fName).size(), fSize);
}
