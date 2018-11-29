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
#include <repo/core/handler/fileservice/repo_file_handler_s3.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <boost/filesystem.hpp>

//Test Database address
const static std::string REPO_GTEST_S3_BUCKET = "3drepo-sandbox";
const static std::string REPO_GTEST_S3_REGION = "eu-west-2";

static repo::core::handler::fileservice::S3FileHandler* getFileHandler()
{
	return 	repo::core::handler::fileservice::S3FileHandler::getHandler(REPO_GTEST_S3_BUCKET, REPO_GTEST_S3_REGION);
}

