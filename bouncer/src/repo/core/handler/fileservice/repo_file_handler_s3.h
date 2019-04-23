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
					S3FileHandler(
						const std::string &bucketName,
						const std::string &region
						);


					/**
					* Upload file to S3.
					*/
					std::string uploadFile(
						const std::string          &keyName,
						const std::vector<uint8_t> &bin
					);

					/**
					* Delete file from S3.
					*/
					bool deleteFile(
						const std::string &keyName);

					repo::core::model::RepoRef::RefType getType() const {
						return repo::core::model::RepoRef::RefType::S3;
					}

		
				protected:
					/*
					*	================================= Protected Fields ========================================
					*/


					/*
					 *	================================= Private Functions =======================================
					 */


				private:
					/*
					 *	=================================== Private Fields ========================================
					 */

					std::string bucketName;
					std::string bucketRegion;

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

