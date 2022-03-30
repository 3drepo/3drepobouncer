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
 *  filesystem based handler
 */

#pragma once

#include <string>

#include <iostream>
#include <fstream>
#include <boost/interprocess/streams/bufferstream.hpp>

#include "repo_file_handler_abstract.h"

namespace repo {
	namespace core {
		namespace handler {
			namespace fileservice {
				class FSFileHandler : public AbstractFileHandler
				{
				public:
					/*
					 *	=================================== Public Functions ========================================
					 */

					 /**
					  * A Deconstructor
					  */
					~FSFileHandler();

					FSFileHandler(
						const std::string &dir,
						const int &nLevel
					);

					repo::core::model::RepoRef::RefType getType() const {
						return repo::core::model::RepoRef::RefType::FS;
					}

					/**
					 * Upload file to FS
					 * upon success, returns the link information for the file, empty otherwise.
					 */
					std::string uploadFile(
						const std::string          &database,
						const std::string          &collection,
						const std::string          &keyName,
						const std::vector<uint8_t> &bin
					);

					/**
					 * Delete file from FS.
					 */
					bool deleteFile(
						const std::string          &database,
						const std::string          &collection,
						const std::string &keyName);
					/**
					 * Get file from FS.
					 */
					std::vector<uint8_t> getFile(
						const std::string          &database,
						const std::string          &collection,
						const std::string &keyName);

				private:
					/*
					 *	=================================== Private Fields ========================================
					 */
					std::vector<std::string> determineHierachy(const std::string &name) const;

					const std::string dirPath;
					const int level;
					const static int minChunkLength = 4;
				};
			}
		}
	}
}
