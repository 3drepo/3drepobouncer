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
#include "repo_file_handler_gridfs.h"

#include "../../model/repo_model_global.h"
#include "../../../lib/repo_log.h"
#include "../../../lib/repo_exception.h"
#include "../../../lib/repo_utils.h"

using namespace repo::core::handler::fileservice;

GridFSFileHandler::GridFSFileHandler(
	repo::core::handler::AbstractDatabaseHandler* handler) :
	handler(handler)
{
	if (!handler) {
		throw repo::lib::RepoException("Cannot initialise GridFS handler - no connection to database");
	}
}

/**
 * A Deconstructor
 */
GridFSFileHandler::~GridFSFileHandler()
{
}

bool GridFSFileHandler::deleteFile(
	const std::string          &database,
	const std::string          &collection,
	const std::string			&keyName)
{
	std::string errMsg;
	auto success = handler->dropRawFile(database, collection, keyName, errMsg);
	if (!success) {
		repoError << "Failed to load file to GridFS: " << errMsg;
	}

	return success;
}

std::vector<uint8_t> GridFSFileHandler::getFile(
	const std::string          &database,
	const std::string          &collection,
	const std::string			&keyName)
{
	return handler->getRawFile(database, collection, keyName);
}

std::string GridFSFileHandler::uploadFile(
	const std::string          &database,
	const std::string          &collection,
	const std::string          &keyName,
	const std::vector<uint8_t> &bin
)
{
	std::string errMsg;
	auto success = handler->insertRawFile(database, collection, keyName, bin, errMsg);
	if (!success) {
		repoError << "Failed to load file to GridFS: " << errMsg;
	}

	return success ? keyName : "";
}