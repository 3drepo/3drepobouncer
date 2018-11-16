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
 *  AWS S3 handler
 */

#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/logging/AWSLogging.h>

#include "repo_file_handler_s3.h"

#include "../../model/repo_model_global.h"
#include "../../../lib/repo_log.h"

using namespace Aws::Auth;
using namespace Aws::Http;
using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::Utils;

using namespace repo::core::handler::fileservice;

S3FileHandler* S3FileHandler::handler = NULL;

S3FileHandler::S3FileHandler() :
	AbstractFileHandler()
{
	Aws::Utils::Logging::InitializeAWSLogging(
			Aws::MakeShared<Aws::Utils::Logging::DefaultLogSystem>(
				"RunUnitTests", Aws::Utils::Logging::LogLevel::Trace, "aws_sdk_"));
	Aws::InitAPI(options);
}

/**
 * A Deconstructor
 */
S3FileHandler::~S3FileHandler()
{
	Aws::ShutdownAPI(options);
	Aws::Utils::Logging::ShutdownAWSLogging();
}

bool S3FileHandler::deleteFile(
	const std::string &keyName)
{
	const Aws::String awsKeyName = keyName.c_str();

	repoTrace << "Deleting " << awsKeyName << " from S3 bucket: " << awsBucketName;

	Aws::S3::S3Client s3Client;

	Aws::S3::Model::DeleteObjectRequest objectRequest;
	objectRequest.WithBucket(awsBucketName).WithKey(awsKeyName);

	auto deleteObjectOutcome = s3Client.DeleteObject(objectRequest);

	bool success = deleteObjectOutcome.IsSuccess();

	if (!success)
	{
		repoTrace << "DeleteObject.error: " << deleteObjectOutcome.GetError();
	}

	return success;
}

bool S3FileHandler::deleteFileAndRef(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName)
{
	return true;
}

S3FileHandler* S3FileHandler::getHandler(
	const std::string &bucketName,
	const std::string &region)
{
	if (getHandler())
		handler->setBucket(bucketName, region);

	return handler;
}

S3FileHandler* S3FileHandler::getHandler()
{
	if (!handler)
	{
		repoTrace << "Handler not present, instantiating new handler...";
		handler = new S3FileHandler();
	}
	else
	{
		repoTrace << "Found handler, returning existing handler";
	}

	return handler;
}

void S3FileHandler::setBucket(
	const std::string &bucketName,
	const std::string &region)
{
	awsBucketName = bucketName.c_str();
	awsBucketRegion = region.c_str();
}

bool S3FileHandler::uploadFile(
	const std::string          &keyName,
	const std::vector<uint8_t> &bin
	)
{
	const Aws::String awsKeyName = keyName.c_str();

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

	return success;
}

bool S3FileHandler::uploadFileAndCommit(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::string                            &databaseName,
	const std::string                            &collectionNamePrefix,
	const std::string                            &fileName,
	const std::vector<uint8_t>                   &bin)
{
	bool success = true;
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

	return success;
}

bool S3FileHandler::upsertFileRef(
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

