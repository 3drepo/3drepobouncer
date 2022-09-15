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

namespace repo {
	namespace core {
		namespace handler {
			namespace fileservice {
				class AbstractFileHandler
				{
				public:
					/**
					 * A Deconstructor
					 */
					virtual ~AbstractFileHandler() {}

					/**
					* Delete file.
					*/
					virtual bool deleteFile(
						const std::string          &database,
						const std::string          &collection,
						const std::string &fileName) = 0;

					/**
					* Get file.
					*/
					virtual std::vector<uint8_t> getFile(
						const std::string          &database,
						const std::string          &collection,
						const std::string &fileName) = 0;

					/**
					* Upload file.
					*/
					virtual std::string uploadFile(
						const std::string          &database,
						const std::string          &collection,
						const std::string          &fileName,
						const std::vector<uint8_t> &bin
					) = 0;

					virtual repo::core::model::RepoRef::RefType getType() const = 0;

				protected:
					/**
					 * Default constructor
					 */
					AbstractFileHandler() {};
				};
			}
		}
	}
}
