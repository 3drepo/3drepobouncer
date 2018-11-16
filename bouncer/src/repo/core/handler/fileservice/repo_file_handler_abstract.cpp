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

/**
* Abstract database handler which all database handler needs to inherit from
*/
#include "repo_file_handler_abstract.h"

#include <regex>

#include "../../model/repo_model_global.h"
#include "../../model/bson/repo_bson_builder.h"
#include "../../model/bson/repo_bson_ref.h"

using namespace repo::core::handler::fileservice;

std::string AbstractFileHandler::cleanFileName(
	const std::string &fileName)
{
	std::string result;
	std::regex matchAllSlahes("(.*\/)+");
	std::regex matchUpToRevision(".*revision\/");

	result = std::regex_replace(fileName, matchUpToRevision, "");

	if (fileName.length() == result.length())
		result = std::regex_replace(fileName, matchAllSlahes, "");

	return result;
}

bool AbstractFileHandler::upsertFileRef(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &id,
	const std::string                            &link,
	const std::string                            &type,
	const uint32_t                               &size)
{
	std::string errMsg;
	bool success = true;

	std::string collectionName = collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF;

	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, id);
	builder.append(REPO_LABEL_TYPE, type);
	builder.append(REPO_REF_LABEL_LINK, link);
	builder.append(REPO_REF_LABEL_SIZE, size);

	if (success &= handler->upsertDocument(databaseName, collectionName, builder.obj(), true, errMsg))
	{
		repoInfo << "File ref for " << collectionName << " added.";
	}
	else
	{
		repoError << "Failed to add " << collectionName << " file ref: " << errMsg;;
	}

	return success;
}
