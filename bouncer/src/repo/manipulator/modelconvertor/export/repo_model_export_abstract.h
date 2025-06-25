/**
*  Copyright (C) 2015 3D Repo Ltd
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
* Abstract Model convertor(Export)
*/

#pragma once

#include <string>

#include "../../../core/model/collection/repo_scene.h"
#include <repo/core/model/bson/repo_bson_factory.h>

namespace repo{
	namespace manipulator{
		namespace modelconvertor{

			enum class ExportType { REPO };

			class AbstractModelExport
			{
			public:

				// Type used to pass in the callback for file export.
				// This will be used by the exporter to send data to be
				// written out as a file.
				typedef std::function<void(
					const std::string&,												// databaseName
					const std::string&,												// id
					const std::string&,												// collectionNamePrefix
					const std::vector<uint8_t>&,									// bin
					const std::unordered_map<std::string, repo::lib::RepoVariant>&,	// metadata
					const repo::core::handler::fileservice::FileManager::Encoding&	// encoding
					)> FileCallback;

				// Type used to pass in the callback for database upsertion.
				// This will be used by the exporter for database interactions.
				typedef std::function<void(
					const std::string&,					// database
					const std::string&,					// collection
					const repo::core::model::RepoBSON&,	// obj
					const bool&							// overwrite
					)> UpsertCallback;


				/**
				* Default Constructor, export model with default settings
				*/				
				AbstractModelExport(
					const std::string databaseName,
					const std::string projectName,
					const repo::lib::RepoUUID revId,
					const std::vector<double> worldOffset);

				/**
				* Default Deconstructor
				*/
				virtual ~AbstractModelExport();
								
				/**
				* Exports supermesh to file and adds its information to the ongoing export process.
				* @param a pointer to the supermesh
				*/
				virtual void addSupermesh(repo::core::model::SupermeshNode* supermesh) = 0;

				/**
				* Finalises the export by writing out the metadata and mapping information collected
				* during the ongoing export process.
				*/
				virtual void finalise() = 0;

				/**
				* Sets the callback for file operations used by the exporter.
				* @param A callback to a method for writing out a file.
				*/
				void setFileCallback(FileCallback _fileCallback) {
					fileCallback = _fileCallback;
				}

				/**
				* Sets the callback for upsert operations used by the exporter.
				* @param A callback to a method for upserting a document.
				*/
				void setUpsertCallback(UpsertCallback _upsertCallback) {
					upsertCallback = _upsertCallback;
				}

			protected:

				// Callbacks for file and database operations
				FileCallback fileCallback;
				UpsertCallback upsertCallback;

				// Model info
				std::string databaseName;
				std::string projectName;
				repo::lib::RepoUUID revId;
				std::vector<double> worldOffset;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
