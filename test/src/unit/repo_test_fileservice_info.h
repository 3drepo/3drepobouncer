/**
*  Copyright (C) 2018 3D Repo Ltd
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

#pragma once

#include <repo/core/handler/fileservice/repo_file_manager.h>
#include <repo/lib/repo_exception.h>
#include <repo/lib/repo_config.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <boost/filesystem.hpp>
#include "repo_test_database_info.h"

//Test S3 address
const static std::string REPO_GTEST_S3_BUCKET = "3drepo-sandbox";
const static std::string REPO_GTEST_S3_REGION = "eu-west-2";

using namespace repo::core::handler::fileservice;

static FileManager* getFileManager()
{
	try {
		return FileManager::getManager();
	}
	catch (const repo::lib::RepoException) {
		return FileManager::instantiateManager(repo::lib::RepoConfig::fromFile(getDataPath("config/config.json")), getHandler());
	}
		
}
