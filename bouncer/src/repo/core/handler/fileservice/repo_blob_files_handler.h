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
#include <fstream>

#include "./repo_file_manager.h"
#include "./repo_data_ref.h"

namespace repo {
	namespace core {
		namespace handler {
			namespace fileservice {
				const static size_t MAX_FILE_SIZE_BYTES = 104857600; //100MB
				class BlobFilesHandler
				{
				public:
					/**
					 * A Deconstructor
					 */
					~BlobFilesHandler();

					BlobFilesHandler(
						FileManager *fileManager,
						const std::string &database,
						const std::string &collection,
						const repo::core::model::RepoBSON &metadata = repo::core::model::RepoBSON()
					) : manager(fileManager), database(database), collection(collection), metadata(metadata) {};

					void finished() { commitActiveFile(); }

					DataRef insertBinary(const std::vector<uint8_t> &data);
					std::vector<uint8_t> readToBuffer(const DataRef &ref);

				private:
					struct fileEntry
					{
						std::string name;
						std::vector<uint8_t> buffer;
					};

					void commitActiveFile();
					void newActiveFile();

					std::istream fetchStream(const std::string &name);

					FileManager* manager;
					const std::string database, collection;
					std::shared_ptr<fileEntry> activeFile; //mem address we're currently writing to
					const repo::core::model::RepoBSON metadata;

					std::map<std::string, std::ifstream> readStreams;
				};
			}
		}
	}
}
