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
#include "../../model/bson/repo_bson_ref.h"
#include "../../../lib/repo_config.h"

namespace repo {
	namespace core {
		namespace handler {
			namespace fileservice {
				class FileManager
				{
				public:
					/**
					 * A Deconstructor
					 */
					~FileManager() {}

					// The FileManager's definition of Metadata. Consumers of this
					// do not need to know about RepoRef, so it has its own type
					// alias in case they diverge.
					using Metadata = repo::core::model::RepoRef::Metadata;

					/*
					* Returns file handler.
					* Get file manager instance
					* Throws RepoException if this is called before instantiateMananger is called.
					*/
					static FileManager* getManager();

					static void disconnect() {
						if (manager)
							delete manager;
						manager = nullptr;
					}

					static FileManager* instantiateManager(
						const repo::lib::RepoConfig &config,
						repo::core::handler::AbstractDatabaseHandler *dbHandler
					);

					/*
					* Possible options for static compression of stored files
					*/
					enum Encoding {
						None = 0,
						Gzip = 1
					};

					/**
					 * Upload file and commit ref entry to database. id will be the member
					 * by which the ref node is keyed. It can be a std::string or RepoUUID.
					 */
					template<typename IdType>
					bool uploadFileAndCommit(
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const IdType								 &id,
						const std::vector<uint8_t>                   &bin,
						const repo::core::model::RepoRef::Metadata   &metadata = {},
						const Encoding                               &encoding = Encoding::None
					);

					/**
					 * Get the file base on the the ref entry in database
					 */
					template<typename IdType>
					std::vector<uint8_t> getFile(
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const IdType                                 &id
					);

					/**
					 * Get the file base on the the ref entry in database
					 */
					std::ifstream getFileStream(
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &fileName
					);

					/**
					 * Delete file ref and associated file from database.
					 */
					bool deleteFileAndRef(
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const std::string                            &fileName
					);

					repo::core::model::RepoRef getFileRef(
						const std::string& databaseName,
						const std::string& collectionNamePrefix,
						const std::string& fileName);

					repo::core::model::RepoRef getFileRef(
						const std::string& databaseName,
						const std::string& collectionNamePrefix,
						const repo::lib::RepoUUID& id
					);

					/**
					* Get the fully qualified filename from the ref node given
					* the current file handler.
					*/
					std::string getFilePath(
						const repo::core::model::RepoRef& refNode
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

					repo::core::model::RepoRef makeRefNode(
						const repo::lib::RepoUUID& id,
						const std::string& link,
						const repo::core::model::RepoRef::RefType& type,
						const uint32_t& size,
						const repo::core::model::RepoRef::Metadata& metadata);

					repo::core::model::RepoRef makeRefNode(
						const std::string& id,
						const std::string& link,
						const repo::core::model::RepoRef::RefType& type,
						const uint32_t& size,
						const repo::core::model::RepoRef::Metadata& metadata);

					/**
					 * Add ref entry for file to database.
					 */
					template<typename IdType>
					bool upsertFileRef(
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const IdType                                 &id,
						const std::string                            &link,
						const repo::core::model::RepoRef::RefType    &type,
						const uint32_t                               &size,
						const repo::core::model::RepoRef::Metadata   &metadata);

					static FileManager* manager;
					repo::core::handler::AbstractDatabaseHandler *dbHandler;
					std::shared_ptr<AbstractFileHandler> defaultHandler, fsHandler;
				};
			}
		}
	}
}
