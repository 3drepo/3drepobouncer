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
*  Project setting BSON
*/

#pragma once
#include "repo_bson.h"

namespace repo {
	namespace core {
		namespace model {
			namespace bson {

				class RepoProjectSettings :
					public RepoBSON
				{
				public:
					RepoProjectSettings();

					RepoProjectSettings(RepoBSON bson) : RepoBSON(bson){}

					~RepoProjectSettings();

					/**
					* Static builder for factory use to create a project setting BSON
					* @param uniqueProjectName a unique name for the project
					* @param owner owner ofthis project
					* @param group group with access of this project
					* @param type  type of project
					* @param description Short description of the project
					* @param ownerPermissionsOctal owner's permission in POSIX
					* @param groupPermissionsOctal group's permission in POSIX
					* @param publicPermissionsOctal other's permission in POSIX
					* @return returns a pointer Camera node
					*/
					static RepoProjectSettings* createRepoProjectSettings(
						const std::string &uniqueProjectName,
						const std::string &owner,
						const std::string &group = std::string(),
						const std::string &type = REPO_PROJECT_TYPE_ARCHITECTURAL,
						const std::string &description = std::string(),
						const uint8_t     &ownerPermissionsOctal = 7,
						const uint8_t     &groupPermissionsOctal = 0,
						const uint8_t     &publicPermissionsOctal = 0);

				};
			}// end namespace bson
		}// end namespace model
	} // end namespace core
} // end namespace repo

