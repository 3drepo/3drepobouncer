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
#include <regex>
#include "repo_file_manager.h"
#include "../../../lib/repo_exception.h"
#include "../../model/repo_model_global.h"
#include "../../model/bson/repo_bson_factory.h"
#include "repo_file_handler_fs.h"
#include "repo_file_handler_gridfs.h"

using namespace repo::core::handler::fileservice;

FileManager* FileManager::manager = nullptr;

FileManager* FileManager::getManager() {
	if (!manager) throw repo::lib::RepoException("Trying to get File manager before it's instantiated!");

	return manager;
}

FileManager* FileManager::instantiateManager(
	const repo::lib::RepoConfig &config,
	repo::core::handler::AbstractDatabaseHandler *dbHandler
) {
	if (manager) disconnect();

	return manager = new FileManager(config, dbHandler);
}

bool FileManager::uploadFileAndCommit(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName,
	const std::vector<uint8_t>                   &bin,
	const repo::core::model::RepoBSON            &metadata)
{
	bool success = true;
	auto fileUUID = repo::lib::RepoUUID::createUUID();
	auto linkName = defaultHandler->uploadFile(databaseName, collectionNamePrefix, fileUUID.toString(), bin);
	if (success = !linkName.empty()) {
		success = upsertFileRef(
			databaseName,
			collectionNamePrefix,
			cleanFileName(fileName),
			linkName,
			defaultHandler->getType(),
			bin.size(),
			metadata);
	}

	return success;
}

bool FileManager::deleteFileAndRef(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName)
{
	bool success = true;
	repo::core::model::RepoBSON criteria = BSON(REPO_LABEL_ID << cleanFileName(fileName));
	repo::core::model::RepoRef ref = dbHandler->findOneByCriteria(
		databaseName,
		collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF,
		criteria);

	if (ref.isEmpty())
	{
		repoTrace << "Failed: cannot find file ref "
			<< cleanFileName(fileName) << " from "
			<< databaseName << "/"
			<< collectionNamePrefix << "." << REPO_COLLECTION_EXT_REF;
		success = false;
	}
	else
	{
		const auto keyName = ref.getRefLink();
		const auto type = ref.getType(); //Should return enum

		std::shared_ptr<AbstractFileHandler> handler = gridfsHandler;
		switch (type) {
		case repo::core::model::RepoRef::RefType::FS:
			handler = fsHandler;
			break;
		}

		if (handler) {
			success = handler->deleteFile(databaseName, collectionNamePrefix, keyName) &&
				dropFileRef(
					ref,
					databaseName,
					collectionNamePrefix);
		}
		else {
			repoError << "Trying to delete a file from " << repo::core::model::RepoRef::convertTypeAsString(type) << " but connection to this service is not configured.";
			success = false;
		}
	}

	return success;
}

repo::core::model::RepoRef FileManager::getFileRef(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName) {
	repo::core::model::RepoBSON criteria = BSON(REPO_LABEL_ID << cleanFileName(fileName));
	return dbHandler->findOneByCriteria(
		databaseName,
		collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF,
		criteria);
}

std::vector<uint8_t> FileManager::getFile(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName
) {
	std::vector<uint8_t> file;
	auto ref = getFileRef(databaseName, collectionNamePrefix, fileName);
	if (ref.isEmpty())
	{
		repoTrace << "Failed: cannot find file ref "
			<< cleanFileName(fileName) << " from "
			<< databaseName << "/"
			<< collectionNamePrefix << "." << REPO_COLLECTION_EXT_REF;
	}
	else
	{
		const auto keyName = ref.getRefLink();
		const auto type = ref.getType(); //Should return enum

		std::shared_ptr<AbstractFileHandler> handler = gridfsHandler;
		switch (type) {
		case repo::core::model::RepoRef::RefType::FS:
			handler = fsHandler;
			break;
		}

		if (handler) {
			repoTrace << "Getting file (" << keyName << ") from " << (type == repo::core::model::RepoRef::RefType::FS ? "FS" : "GridFS");
			file = handler->getFile(databaseName, collectionNamePrefix, keyName);
		}
		else {
			repoError << "Trying to read a file from " << repo::core::model::RepoRef::convertTypeAsString(type) << " but connection to this service is not configured.";
		}
	}

	return file;
}

/**
 * Get the file base on the the ref entry in database
 */
std::ifstream FileManager::getFileStream(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName
) {
	std::ifstream fs;
	auto ref = getFileRef(databaseName, collectionNamePrefix, fileName);
	if (ref.isEmpty())
	{
		repoTrace << "Failed: cannot find file ref "
			<< cleanFileName(fileName) << " from "
			<< databaseName << "/"
			<< collectionNamePrefix << "." << REPO_COLLECTION_EXT_REF;
	}
	else
	{
		const auto keyName = ref.getRefLink();
		const auto type = ref.getType(); //Should return enum

		std::shared_ptr<AbstractFileHandler> handler = gridfsHandler;
		switch (type) {
		case repo::core::model::RepoRef::RefType::FS:
			handler = fsHandler;
			break;
		}

		if (handler) {
			repoTrace << "Getting file (" << keyName << ") from " << (type == repo::core::model::RepoRef::RefType::FS ? "FS" : "GridFS");
			fs = handler->getFileStream(databaseName, collectionNamePrefix, keyName);
		}
		else {
			repoError << "Trying to read a file from " << repo::core::model::RepoRef::convertTypeAsString(type) << " but connection to this service is not configured.";
		}
	}

	return fs;
}

FileManager::FileManager(
	const repo::lib::RepoConfig &config,
	repo::core::handler::AbstractDatabaseHandler *dbHandler
) : dbHandler(dbHandler) {
	if (!dbHandler)
		throw repo::lib::RepoException("Trying to instantiate FileManager with a nullptr to database!");

	gridfsHandler = std::make_shared<GridFSFileHandler>(dbHandler);
	defaultHandler = gridfsHandler;

	auto fsConfig = config.getFSConfig();
	if (fsConfig.configured) {
		fsHandler = std::make_shared<FSFileHandler>(fsConfig.dir, fsConfig.nLevel);
		if (config.getDefaultStorageEngine() == repo::lib::RepoConfig::FileStorageEngine::FS)
			defaultHandler = fsHandler;
	}
}

std::string FileManager::cleanFileName(
	const std::string &fileName)
{
	std::string result;
	std::regex matchAllSlashes("(.*\/)+");
	std::regex matchUpToRevision(".*revision\/");

	result = std::regex_replace(fileName, matchUpToRevision, "");

	if (fileName.length() == result.length())
		result = std::regex_replace(fileName, matchAllSlashes, "");

	return result;
}

bool FileManager::dropFileRef(
	const repo::core::model::RepoBSON            bson,
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix)
{
	std::string errMsg;
	bool success = true;

	std::string collectionName = collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF;

	if (success = dbHandler->dropDocument(bson, databaseName, collectionName, errMsg))
	{
		repoInfo << "File ref for " << collectionName << " dropped.";
	}
	else
	{
		repoError << "Failed to drop " << collectionName << " file ref: " << errMsg;;
	}

	return success;
}

bool FileManager::upsertFileRef(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &id,
	const std::string                            &link,
	const repo::core::model::RepoRef::RefType    &type,
	const uint32_t                               &size,
	const repo::core::model::RepoBSON            &metadata)
{
	std::string errMsg;
	bool success = true;

	auto refObj = repo::core::model::RepoBSONFactory::makeRepoRef(id, type, link, size, metadata);
	std::string collectionName = collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF;
	success = dbHandler->upsertDocument(databaseName, collectionName, refObj, true, errMsg);
	if (!success)
	{
		repoError << "Failed to add " << collectionName << " file ref: " << errMsg;;
	}

	return success;
}