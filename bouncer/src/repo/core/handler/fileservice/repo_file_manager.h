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
#include "repo/core/model/bson/repo_bson_ref.h"
#include "repo/lib/repo_config.h"
#include <boost/iostreams/filtering_stream.hpp>
#include "log/repo_log.h"

namespace repo {
	namespace core {
		namespace handler {
			class AbstractDatabaseHandler;
			namespace fileservice {

				template<typename IdType>
				class FileHandle {
				public:

					FileHandle(
						std::string databaseName,
						std::string collectionNamePrefix,
						repo::lib::RepoUUID fileUUID,
						std::string linkName,
						IdType id,
						repo::core::model::RepoRef::Metadata metadata,
						std::unique_ptr<std::ofstream> fileStream,
						std::unique_ptr<boost::iostreams::filtering_stream<boost::iostreams::output>> outStream)
						: databaseName(databaseName), 
						collectionNamePrefix(collectionNamePrefix),
						fileUUID(fileUUID),
						linkName(linkName),
						id(id),
						metadata(metadata),
						fileStream(std::move(fileStream)),
						outStream(std::move(outStream)),
						size(0)
					{
					}

					~FileHandle()
					{
						if (isFileOpen())
						{
							outStream.reset();
							fileStream->close();
						}
					}

					bool isFileOpen()
					{
						return fileStream->is_open();
					}

					void writeData(const std::vector<uint8_t>& bin)
					{
						if (!isFileOpen())
						{
							throw repo::lib::RepoException("Writing to file " + linkName + " attempted but file stream closed unexpectedly.");
						}

						outStream->write((char*)bin.data(), bin.size());
						outStream->flush();

						size += bin.size();
					}

					std::string getLinkName()
					{
						return linkName;
					}

					repo::lib::RepoUUID getFileUUID()
					{
						return fileUUID;
					}

					std::string getDatabaseName()
					{
						return databaseName;
					}

					std::string getCollectionNamePrefix()
					{
						return collectionNamePrefix;
					}

					IdType getId()
					{
						return id;
					}

					size_t getSize()
					{
						return size;
					}

					repo::core::model::RepoRef::Metadata getMetadata()
					{
						return metadata;
					}

					std::ofstream* getFileStream()
					{
						return fileStream.get();
					}

					void closeStreams()
					{
						// Close out stream first. This is important if gzip compression is used.
						outStream.reset();

						// Close the file stream
						fileStream->close();
					}

				private:

					repo::lib::RepoUUID fileUUID;
					std::string linkName;
					IdType id;
					size_t size;
					repo::core::model::RepoRef::Metadata metadata;

					std::string databaseName;
					std::string collectionNamePrefix;

					std::unique_ptr<std::ofstream> fileStream;
					std::unique_ptr<boost::iostreams::filtering_stream<boost::iostreams::output>> outStream;
				};

				// This class is considered thread-safe.
				class FileManager
				{
				public:
					/**
					 * Default constructor
					 */
					FileManager(const repo::lib::RepoConfig& config, std::weak_ptr<AbstractDatabaseHandler> handler);

					/**
					 * A Deconstructor
					 */
					~FileManager() {}

					// The FileManager's definition of Metadata. Consumers of this
					// do not need to know about RepoRef, so it has its own type
					// alias in case they diverge.
					using Metadata = repo::core::model::RepoRef::Metadata;

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
						const std::string& databaseName,
						const std::string& collectionNamePrefix,
						const IdType& id						
					);

					/**
					 * Get the file base on the the ref entry in database
					 */
					template<typename IdType>
					std::vector<uint8_t> getFile(
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix,
						const IdType                                 &id,
						const Encoding								 &encoding
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

					repo::core::model::RepoRefT<std::string> getFileRef(
						const std::string& databaseName,
						const std::string& collectionNamePrefix,
						const std::string& fileName);

					repo::core::model::RepoRefT<repo::lib::RepoUUID> getFileRef(
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

					template<typename IdType>
					std::unique_ptr<FileHandle<IdType>> requestFileHandle(
						const std::string& databaseName,
						const std::string& collectionNamePrefix,
						const IdType& id,
						Metadata metadata,
						const Encoding& encoding = Encoding::None
					);

					template<typename IdType>
					bool turnInFileHandleForUpload(
						std::unique_ptr<FileHandle<IdType>> fileHandle
					);

				private:
					/**
					 * Remove ref entry for file to database.
					 */
					bool dropFileRef(
						const repo::core::model::RepoBSON            bson,
						const std::string                            &databaseName,
						const std::string                            &collectionNamePrefix);

					repo::core::model::RepoRefT<repo::lib::RepoUUID> makeRefNode(
						const repo::lib::RepoUUID& id,
						const std::string& link,
						const repo::core::model::RepoRef::RefType& type,
						const uint32_t& size,
						const repo::core::model::RepoRef::Metadata& metadata);

					repo::core::model::RepoRefT<std::string> makeRefNode(
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

					std::shared_ptr<AbstractDatabaseHandler> getDbHandler();

					std::weak_ptr<AbstractDatabaseHandler> dbHandler;
					std::shared_ptr<AbstractFileHandler> fsHandler;
				};
			}
		}
	}
}
