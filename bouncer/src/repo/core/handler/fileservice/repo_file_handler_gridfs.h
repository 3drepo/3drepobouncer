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

#include "repo_file_handler_abstract.h"
#include "../repo_database_handler_abstract.h"

namespace repo{
	namespace core{
		namespace handler{
			namespace fileservice{
				class GridFSFileHandler : public AbstractFileHandler
				{
				public:
					/*
					 *	=================================== Public Functions ========================================
					 */

					/**
					 * A Deconstructor
					 */
					~GridFSFileHandler();
					GridFSFileHandler(
						repo::core::handler::AbstractDatabaseHandler* handler
						);

					repo::core::model::RepoRef::RefType getType() const {
						return repo::core::model::RepoRef::RefType::GRIDFS;
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
						const std::string		   &keyName);

				private:
					/*
					 *	=================================== Private Fields ========================================
					 */
					repo::core::handler::AbstractDatabaseHandler* handler;
				};
			}
		}
	}
}

