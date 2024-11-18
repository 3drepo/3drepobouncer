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
#include "repo/core/handler/repo_database_handler_abstract.h"
#include "repo/lib/repo_exception.h"
#include "repo/core/model/repo_model_global.h"
#include "repo/core/model/bson/repo_bson.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/model/bson/repo_bson_builder.h"
#include "repo_file_handler_fs.h"
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <istream>

using namespace repo::core::handler::fileservice;

FileManager::FileManager(
	const repo::lib::RepoConfig& config,
	std::weak_ptr<AbstractDatabaseHandler> handler)
	:dbHandler(handler)
{
	auto fsConfig = config.getFSConfig();
	if (fsConfig.configured) {
		fsHandler = std::make_shared<FSFileHandler>(fsConfig.dir, fsConfig.nLevel);
	}
	else {
		throw repo::lib::RepoException("Filestore configuration must be provided.");
	}
}

std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> FileManager::getDbHandler()
{
	return dbHandler.lock();
}

template<typename IdType>
bool FileManager::uploadFileAndCommit(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const IdType                                 &id,
	const std::vector<uint8_t>                   &bin,
	const Metadata                               &metadata,
	const Encoding                               &encoding)
{
	bool success = true;
	auto fileUUID = repo::lib::RepoUUID::createUUID();

	// Create local references that can be reassigned, if we need to do any
	// further processing...

	const std::vector<uint8_t>* fileContents = &bin;
	auto fileMetadata = metadata;

	switch (encoding)
	{
		case Encoding::Gzip:
		{
			// Use stringstream as a binary container to hold the compressed data
			std::ostringstream compressedstream;

			// Bufferstream operates directly over the user provided array
			boost::interprocess::bufferstream uncompressed((char*)bin.data(), bin.size());

			// In stream form for filtering_istream
			std::istream uncompressedStream(uncompressed.rdbuf());

			boost::iostreams::filtering_istream in;
			in.push(boost::iostreams::gzip_compressor());
			in.push(uncompressedStream); // For some reason bufferstream is ambigous between stream and streambuf, so wrap it unambiguously

			boost::iostreams::copy(in, compressedstream);

			auto compresseddata = compressedstream.str();
			fileContents = new std::vector<uint8_t>(compresseddata.begin(), compresseddata.end());

			fileMetadata["encoding"] = std::string("gzip");
		}
		break;
	}

	auto linkName = fsHandler->uploadFile(databaseName, collectionNamePrefix, fileUUID.toString(), *fileContents);
	if (success = !linkName.empty()) {
		success = upsertFileRef(
			databaseName,
			collectionNamePrefix,
			id,
			linkName,
			fsHandler->getType(),
			fileContents->size(),
			fileMetadata);
	}

	// If we've created a new vector, clean it up. metadata doesn't need to be
	// deleted because Mongo BSONs have built in smart-pointers allowing them to
	// be passed by value.

	if (fileContents != &bin)
	{
		delete fileContents;
	}

	return success;
}

bool FileManager::deleteFileAndRef(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName)
{
	bool success = true;

	repo::core::model::RepoBSON node = getDbHandler()->findOneByUniqueID(
		databaseName,
		collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF,
		cleanFileName(fileName)
	);

	if (node.isEmpty())
	{
		repoTrace << "Failed: cannot find file ref "
			<< cleanFileName(fileName) << " from "
			<< databaseName << "/"
			<< collectionNamePrefix << "." << REPO_COLLECTION_EXT_REF;
		success = false;
	}
	else
	{
		repo::core::model::RepoRefT<std::string> ref(node); // The argument is a string, so we are only considering this type of ref node.
		const auto keyName = ref.getRefLink();
		const auto type = ref.getType(); //Should return enum

		std::shared_ptr<AbstractFileHandler> handler = nullptr;
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

repo::core::model::RepoRefT<std::string> FileManager::getFileRef(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName) {
	return repo::core::model::RepoRefT<std::string>(
		getDbHandler()->findOneByUniqueID(
			databaseName,
			collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF,
			cleanFileName(fileName)
		)
	);
}

repo::core::model::RepoRefT<repo::lib::RepoUUID> FileManager::getFileRef(
	const std::string& databaseName,
	const std::string& collectionNamePrefix,
	const repo::lib::RepoUUID& id) {
	return repo::core::model::RepoRefT<repo::lib::RepoUUID>(
		getDbHandler()->findOneByUniqueID(
			databaseName,
			collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF,
			id
		)
	);
}

template<typename IdType>
std::vector<uint8_t> FileManager::getFile(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const IdType                                 &fileName
) {
	std::vector<uint8_t> file;
	auto ref = getFileRef(databaseName, collectionNamePrefix, fileName);
	const auto keyName = ref.getRefLink();
	const auto type = ref.getType(); //Should return enum

	switch (type) {
	case repo::core::model::RepoRef::RefType::FS:
	{
		repoTrace << "Getting file (" << keyName << ") from FS";
		file = fsHandler->getFile(databaseName, collectionNamePrefix, keyName);
	}
	break;
	default:
		repoError << "Trying to read a file from " << repo::core::model::RepoRef::convertTypeAsString(type) << " but connection to this service is not configured.";
	}

	return file;
}

// Explicit instantations for the two id types supported

template std::vector<uint8_t> FileManager::getFile(const std::string&, const std::string&, const std::string&);
template std::vector<uint8_t> FileManager::getFile(const std::string&, const std::string&, const repo::lib::RepoUUID&);

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
	const auto keyName = ref.getRefLink();
	const auto type = ref.getType(); //Should return enum

	switch (type) {
	case repo::core::model::RepoRef::RefType::FS:
	{
		repoTrace << "Getting file (" << keyName << ") from FS";
		fs = fsHandler->getFileStream(databaseName, collectionNamePrefix, keyName);
	}
	break;
	default:
		repoError << "Trying to read a file from " << repo::core::model::RepoRef::convertTypeAsString(type) << " but connection to this service is not configured.";
	}

	return fs;
}

std::string FileManager::getFilePath(
	const repo::core::model::RepoRef& ref
)
{
	if (ref.getType() != repo::core::model::RepoRef::RefType::FS)
	{
		repoTrace << "Failed: can only get paths of files stored with fs";
		return "";
	}
	return fsHandler->getFilePath(ref.getRefLink());
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

	if (success = getDbHandler()->dropDocument(bson, databaseName, collectionName, errMsg))
	{
		repoInfo << "File ref for " << collectionName << " dropped.";
	}
	else
	{
		repoError << "Failed to drop " << collectionName << " file ref: " << errMsg;;
	}

	return success;
}

repo::core::model::RepoRefT<std::string> FileManager::makeRefNode(
	const std::string& id,
	const std::string& link,
	const repo::core::model::RepoRef::RefType& type,
	const uint32_t& size,
	const repo::core::model::RepoRef::Metadata& metadata)
{
	return repo::core::model::RepoBSONFactory::makeRepoRef(cleanFileName(id), type, link, size, metadata);
}

repo::core::model::RepoRefT<repo::lib::RepoUUID> FileManager::makeRefNode(
	const repo::lib::RepoUUID& id,
	const std::string& link,
	const repo::core::model::RepoRef::RefType& type,
	const uint32_t& size,
	const repo::core::model::RepoRef::Metadata& metadata)
{
	return repo::core::model::RepoBSONFactory::makeRepoRef(id, type, link, size, metadata);
}

template<typename IdType>
bool FileManager::upsertFileRef(
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const IdType                                 &id,
	const std::string                            &link,
	const repo::core::model::RepoRef::RefType    &type,
	const uint32_t                               &size,
	const repo::core::model::RepoRef::Metadata   &metadata)
{
	std::string errMsg;
	bool success = true;

	auto refObj = makeRefNode(id, link, type, size, metadata);
	std::string collectionName = collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF;
	success = getDbHandler()->upsertDocument(databaseName, collectionName, refObj, true, errMsg);
	if (!success)
	{
		repoError << "Failed to add " << collectionName << " file ref: " << errMsg;;
	}

	return success;
}

// Explicit instantations for the two possible fileName types

template bool FileManager::uploadFileAndCommit<std::string>(
	const std::string&,
	const std::string&,
	const std::string&,
	const std::vector<uint8_t>&,
	const Metadata&,
	const Encoding&);

template bool FileManager::uploadFileAndCommit<repo::lib::RepoUUID>(
	const std::string&,
	const std::string&,
	const repo::lib::RepoUUID&,
	const std::vector<uint8_t>&,
	const Metadata&,
	const Encoding&);