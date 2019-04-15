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

#include "repo_file_handler_fs.h"

#include "../../model/repo_model_global.h"
#include "../../../lib/repo_log.h"

using namespace repo::core::handler::fileservice;

FSFileHandler::FSFileHandler(
	const std::string &dir,
	const int &nLevel) :
	AbstractFileHandler(),
	dirPath(dir),
	level(nLevel)
{
}

/**
 * A Deconstructor
 */
FSFileHandler::~FSFileHandler()
{
	
}

bool FSFileHandler::deleteFile(
	const std::string &keyName)
{/*
	const Aws::String awsKeyName = keyName.c_str();
	const Aws::String awsBucketName = bucketName.c_str();
	const Aws::String awsBucketRegion = bucketRegion.c_str();

	Aws::Client::ClientConfiguration clientConfig;
	if (!awsBucketRegion.empty())
		clientConfig.region = awsBucketRegion;
	clientConfig.scheme = Aws::Http::Scheme::HTTPS;

	repoTrace << "Deleting " << awsKeyName << " from S3 bucket: " << awsBucketName;

	Aws::S3::S3Client s3Client(clientConfig);

	Aws::S3::Model::DeleteObjectRequest objectRequest;
	objectRequest.WithBucket(awsBucketName).WithKey(awsKeyName);

	auto deleteObjectOutcome = s3Client.DeleteObject(objectRequest);

	bool success = deleteObjectOutcome.IsSuccess();

	if (!success)
	{
		repoTrace << "DeleteObject.error: " << deleteObjectOutcome.GetError();
	}

	return success;*/

	return false;
}

bool FSFileHandler::deleteFileAndRef(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName)
{
	//bool success = true;

	//repo::core::model::RepoBSON criteria = BSON(REPO_LABEL_ID << cleanFileName(fileName));
	//repo::core::model::RepoBSON bson = handler->findOneByCriteria(
	//		databaseName,
	//		collectionNamePrefix + "." + REPO_COLLECTION_EXT_REF,
	//		criteria);

	//if (bson.isEmpty())
	//{
	//	repoTrace << "Failed: cannot find file ref "
	//		<< cleanFileName(fileName) << " from "
	//		<< databaseName << "/"
	//		<< collectionNamePrefix << "." << REPO_COLLECTION_EXT_REF;
	//	success = false;
	//}
	//else
	//{
	//	std::string keyName = bson.getField(REPO_REF_LABEL_LINK).str();

	//	success = deleteFile(keyName) &&
	//		AbstractFileHandler::dropFileRef(
	//				handler,
	//				bson,
	//				databaseName,
	//				collectionNamePrefix);
	//}

	//return success;

	return false;
}

bool FSFileHandler::uploadFile(
	const std::string          &keyName,
	const std::vector<uint8_t> &bin
	)
{
	/*const Aws::String awsKeyName = keyName.c_str();
	const Aws::String awsBucketName = bucketName.c_str();
	const Aws::String awsBucketRegion = bucketRegion.c_str();

	Aws::Client::ClientConfiguration clientConfig;
	if (!awsBucketRegion.empty())
		clientConfig.region = awsBucketRegion;
	clientConfig.scheme = Aws::Http::Scheme::HTTPS;

	repoTrace << "Uploading " << awsKeyName << " to S3 bucket: " << awsBucketName << " (" << clientConfig.region << ")";

	Aws::S3::S3Client s3Client(clientConfig);

	Aws::S3::Model::PutObjectRequest objectRequest;
	objectRequest.WithBucket(awsBucketName).WithKey(awsKeyName);

	auto inputData = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream",
		std::stringstream::in | std::stringstream::out | std::stringstream::binary);

	inputData->write((const char*)&bin[0], bin.size() * sizeof(bin[0]));

	objectRequest.SetBody(inputData);

	auto putObjectOutcome = s3Client.PutObject(objectRequest);

	bool success =  putObjectOutcome.IsSuccess();

	if (!success)
	{
		repoTrace << "PutObject.error: " << putObjectOutcome.GetError();
	}

	return success;*/
	return false;
}

bool FSFileHandler::uploadFileAndCommit(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName,
	const std::vector<uint8_t>                   &bin)
{
	/*bool success = true;
	auto fileUUID = repo::lib::RepoUUID::createUUID();

	if (success &= uploadFile(fileUUID.toString(), bin))
	{
		auto fileSize = bin.size() * sizeof(bin[0]);

		success &= upsertFileRef(
				handler,
				databaseName,
				collectionNamePrefix,
				cleanFileName(fileName),
				fileUUID.toString(),
				fileSize);
	}

	return success;*/

	return false;
}

bool FSFileHandler::upsertFileRef(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &id,
	const std::string                            &link,
	const uint32_t                               &size)
{
	return AbstractFileHandler::upsertFileRef(handler,
			databaseName,
			collectionNamePrefix,
			id,
			link,
			REPO_REF_TYPE_S3,
			size);
}

