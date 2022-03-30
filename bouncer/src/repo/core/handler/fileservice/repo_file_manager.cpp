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
#include "repo_file_handler_s3.h"

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
	const std::vector<uint8_t>                   &bin)
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
			bin.size());
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

		std::shared_ptr<AbstractFileHandler> handler = gridfsHandler;;
		switch (type) {
		case repo::core::model::RepoRef::RefType::S3:
			handler = s3Handler;
			break;
		case repo::core::model::RepoRef::RefType::FS:
			handler = fsHandler;
			break;
		}

		if (handler) {
			success = defaultHandler->deleteFile(databaseName, collectionNamePrefix, keyName) &&
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

FileManager::FileManager(
	const repo::lib::RepoConfig &config,
	repo::core::handler::AbstractDatabaseHandler *dbHandler
) : dbHandler(dbHandler) {
	if (!dbHandler)
		throw repo::lib::RepoException("Trying to instantiate FileManager with a nullptr to database!");

	gridfsHandler = std::make_shared<GridFSFileHandler>(dbHandler);
	defaultHandler = gridfsHandler;
#ifdef S3_SUPPORT
	auto s3Config = config.getS3Config();
	if (s3Config.configured) {
		s3Handler = std::make_shared<S3FileHandler>(s3Config.bucketName, s3Config.bucketRegion);
		if (config.getDefaultStorageEngine() == repo::lib::RepoConfig::FileStorageEngine::S3)
			defaultHandler = s3Handler;
	}
#endif

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
	const uint32_t                               &size)
{
	std::string errMsg;
	bool success = true;

	auto refObj = repo::core::model::RepoBSONFactory::makeRepoRef(id, type, link, size);
	std::string collectionName = collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF;

	if (success = dbHandler->upsertDocument(databaseName, collectionName, refObj, true, errMsg))
	{
		repoInfo << "File ref for " << collectionName << " added.";
	}
	else
	{
		repoError << "Failed to add " << collectionName << " file ref: " << errMsg;;
	}

	return success;
}