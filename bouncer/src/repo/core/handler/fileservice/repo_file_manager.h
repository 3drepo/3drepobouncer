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

#pragma once

#include <string>

#include "repo_file_handler_abstract.h"
#include "../repo_database_handler_abstract.h"
#include "../../model/bson/repo_bson_ref.h""
#include "../../../lib/repo_config.h"

namespace repo{
	namespace core{
		namespace handler{
			namespace fileservice{
				class FileManager
				{
				public:
					/**
					 * A Deconstructor
					 */
					~FileManager(){}

					/*
					* Returns file handler.
					* Get file manager instance
					* Throws RepoException if this is called before instantiateMananger is called.
					*/
					static FileManager* getManager();

					static void disconnect() {
						if (manager)
							delete manager;
					}

					static FileManager* instantiateManager(
						const repo::lib::RepoConfig &config,
						repo::core::handler::AbstractDatabaseHandler *dbHandler
					);

					/**
					 * Upload file and commit ref entry to database.
					 */
					bool uploadFileAndCommit(
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &fileName,
						const std::vector<uint8_t>                   &bin
						);

					/**
					 * Delete file ref and associated file from database.
					 */
					bool deleteFileAndRef(
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &fileName
						);

				private:
					/**
					 * Default constructor
					 */
					FileManager(const repo::lib::RepoConfig &config,
						repo::core::handler::AbstractDatabaseHandler *dbHandler);

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
					 * Remove ref entry for file to database.
					 */
					bool dropFileRef(
						const repo::core::model::RepoBSON            bson,
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix);
					/**
					 * Add ref entry for file to database.
					 */
					bool upsertFileRef(
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &id,
						const std::string                            &link,
						const std::string                            &type,
						const uint32_t                               &size);

					static FileManager* manager;
					repo::core::handler::AbstractDatabaseHandler *dbHandler;
					std::shared_ptr<AbstractFileHandler> defaultHandler, s3Handler, fsHandler;
				};
			}
		}
	}
}
