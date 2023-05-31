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

#include "./repo_file_manager.h"
#include "../../../core/model/bson/repo_bson_builder.h"

namespace repo {
	namespace core {
		namespace handler {
			namespace fileservice {
				const static size_t MAX_FILE_SIZE_BYTES = 104857600; //100MB
				class DataRef {
					const std::string fileName;
					const unsigned int startPos;
					const unsigned int size;
				public:
					DataRef(const std::string &fileName, const unsigned int &startPos, const unsigned int &size)
						: fileName(fileName), startPos(startPos), size(size) {}

					repo::core::model::RepoBSON serialise() const;
				};
				class BlobFilesCreator
				{
				public:
					/**
					 * A Deconstructor
					 */
					~BlobFilesCreator() { commitActiveFile(); }

					BlobFilesCreator(FileManager *fileManager, const std::string &database, const std::string &collection) : manager(fileManager), database(database), collection(collection) {};

					DataRef insertBinary(const std::vector<uint8_t> &data);

				private:
					struct fileEntry
					{
						std::string name;
						std::vector<uint8_t> buffer;
					};

					void commitActiveFile();
					void newActiveFile();

					FileManager* manager;
					const std::string database, collection;
					std::shared_ptr<fileEntry> activeFile; //mem address we're currently writing to
				};
			}
		}
	}
}
