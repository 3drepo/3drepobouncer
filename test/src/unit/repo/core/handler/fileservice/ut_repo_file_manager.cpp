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
	return FileManager::instantiateManager(repo::lib::RepoConfig::fromFile(getDataPath("config/withFS.json")), getHandler());
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
