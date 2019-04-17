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

#ifdef S3_SUPPORT

#include <aws/core/Aws.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/S3Client.h>

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

S3FileHandler::S3FileHandler(
	const std::string &bucketName,
						const std::string &region) :
	AbstractFileHandler(), 
	bucketName(name),
	bucketRegion(region)
{
	Aws::SDKOptions options;
	Aws::InitAPI(options);
}

/**
 * A Deconstructor
 */
S3FileHandler::~S3FileHandler()
{
	Aws::SDKOptions options;
	Aws::ShutdownAPI(options);
}

bool S3FileHandler::deleteFile(
	const std::string &keyName)
{
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

	return success;
}

std::string S3FileHandler::uploadFile(
	const std::string          &keyName,
	const std::vector<uint8_t> &bin
	)
{
	const Aws::String awsKeyName = keyName.c_str();
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

	return success ? keyName : "";
}

#endif
