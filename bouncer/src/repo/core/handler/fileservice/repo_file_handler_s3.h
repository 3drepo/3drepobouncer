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

#pragma once

#include <string>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <iostream>
#include <fstream>
#include <boost/interprocess/streams/bufferstream.hpp>

#include "repo_file_handler_abstract.h"

namespace repo{
	namespace core{
		namespace handler{
			namespace fileservice{
				class S3FileHandler : public AbstractFileHandler
				{
				public:
					/*
					 *	=================================== Public Functions ========================================
					 */

					/**
					 * A Deconstructor
					 */
					~S3FileHandler();

					/**
					 * Returns file handler.
					 * S3FileHandler follows the singleton pattern.
					 */
					static S3FileHandler* getHandler(
						const std::string &bucketName,
						const std::string &region
						);

					/**
					 * Set bucket and region to use on S3.
					 */
					void setBucket(
						const std::string &bucketName,
						const std::string &region
						);

					/**
					 * Upload file to S3 and commit ref entry to database.
					 */
					bool uploadFileAndCommit(
						repo::core::handler::AbstractDatabaseHandler *handler,
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &fileName,
						const std::vector<uint8_t>                   &bin
						);

					/**
					 * Delete file ref and associated file from database.
					 */
					bool deleteFileAndRef(
						repo::core::handler::AbstractDatabaseHandler *handler,
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &fileName
						);

				protected:
					/*
					*	================================= Protected Fields ========================================
					*/
					static S3FileHandler *handler; /* !the single instance of this class*/

					/*
					 *	================================= Private Functions =======================================
					 */
					
					/**
					 * Upload file to S3.
					 */
					bool uploadFile(
						const std::string          &keyName,
						const std::vector<uint8_t> &bin
						);

					/**
					 * Delete file from S3.
					 */
					bool deleteFile(
						const std::string &keyName);

				private:
					/*
					 *	=================================== Private Fields ========================================
					 */

					Aws::String awsBucketName;
					Aws::String awsBucketRegion;

					Aws::SDKOptions options;

					/*
					 *	================================= Private Functions =======================================
					 */

					/**
					 * Constructor is private because this class follows the singleton pattern
					 */
					S3FileHandler();

					/**
					 * Returns file handler.
					 * S3FileHandler follows the singleton pattern.
					 */
					static S3FileHandler* getHandler();

					/**
					 * Add ref entry for file to database.
					 */
					bool upsertFileRef(
						repo::core::handler::AbstractDatabaseHandler *handler,
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &id,
						const std::string                            &link,
						const uint32_t                               &size);
				};
			}
		}
	}
}

