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

#pragma once

#include <string>

#include "../repo_database_handler_abstract.h"

#include "../../model/bson/repo_bson_ref.h"

namespace repo{
	namespace core{
		namespace handler{
			namespace fileservice{
				class AbstractFileHandler
				{
				public:
					/**
					 * A Deconstructor
					 */
					virtual ~AbstractFileHandler(){}

					/**
					 * Upload file and commit ref entry to database.
					 */
					virtual bool uploadFileAndCommit(
						repo::core::handler::AbstractDatabaseHandler *handler,
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &fileName,
						const std::vector<uint8_t>                   &bin
						) = 0;

					/**
					 * Delete file ref and associated file from database.
					 */
					virtual bool deleteFileAndRef(
						repo::core::handler::AbstractDatabaseHandler *handler,
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &fileName
						) = 0;

				protected:
					/**
					 * Default constructor
					 */
					AbstractFileHandler(){};

					/**
					 * Cleans given filename by removing teamspace and model strings.
					 * e.g. cleanFileName("/teamspaceA/modelB/file.obj")
					 *        => "file.obj"
					 *      cleanFileName("/teamspaceA/modelB/revision/revC/file.obj")
					 *        => "revC/file.obj"
					 */
					std::string cleanFileName(
						const std::string &fileName);

					/**
					 * Delete file.
					 */
					virtual bool deleteFile(
						const std::string &fileName) = 0;

					/**
					 * Remove ref entry for file to database.
					 */
					bool dropFileRef(
						repo::core::handler::AbstractDatabaseHandler *handler,
						const repo::core::model::RepoBSON            bson,
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix);

					/**
					 * Upload file.
					 */
					virtual bool uploadFile(
						const std::string          &fileName,
						const std::vector<uint8_t> &bin
						) = 0;

					/**
					 * Add ref entry for file to database.
					 */
					bool upsertFileRef(
						repo::core::handler::AbstractDatabaseHandler *handler,
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &id,
						const std::string                            &link,
						const std::string                            &type,
						const uint32_t                               &size);
				};
			}
		}
	}
}
