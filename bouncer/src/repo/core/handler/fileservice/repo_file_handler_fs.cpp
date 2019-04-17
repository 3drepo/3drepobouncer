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
#include <stdio.h>
#include "repo_file_handler_fs.h"

#include "../../model/repo_model_global.h"
#include "../../../lib/repo_log.h"
#include "../../../lib/repo_exception.h"
#include "../../../lib/repo_utils.h"

using namespace repo::core::handler::fileservice;

FSFileHandler::FSFileHandler(
	const std::string &dir,
	const int &nLevel) :
	AbstractFileHandler(),
	dirPath(dir),
	level(nLevel)
{
	if (!repo::lib::doesDirExist(dir)) {
		throw repo::lib::RepoException("Cannot initialise fileshare: " + dir + " is does not exist/is not a directory");
	}
}

/**
 * A Deconstructor
 */
FSFileHandler::~FSFileHandler()
{
	
}

bool FSFileHandler::deleteFile(
	const std::string &keyName)
{
	bool success = false;
	
	auto fullPath = boost::filesystem::absolute(keyName, dirPath);
	if (repo::lib::doesFileExist(fullPath)) {
		auto fileStr = fullPath.string();
		success = std::remove(fileStr.c_str()) == 0;
	}
	return success;
}

bool FSFileHandler::uploadFile(
	const std::string          &keyName,
	const std::vector<uint8_t> &bin
	)
{
	bool success = false;
	//Determine file directory hierachy
	// write file

	return success;
}


