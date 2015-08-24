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
				class REPO_API_EXPORT RepoProjectSettings :
					public RepoBSON
				{
				public:

					static const uint8_t READVAL = 4;
					static const uint8_t WRITEVAL = 2;
					static const uint8_t EXECUTEVAL = 1;

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
					static RepoProjectSettings createRepoProjectSettings(
						const std::string &uniqueProjectName,
						const std::string &owner,
						const std::string &group = std::string(),
						const std::string &type = REPO_PROJECT_TYPE_ARCHITECTURAL,
						const std::string &description = std::string(),
						const uint8_t     &ownerPermissionsOctal = 7,
						const uint8_t     &groupPermissionsOctal = 0,
						const uint8_t     &publicPermissionsOctal = 0);

					/**
					* --------- Convenience functions -----------
					*/

					/**
					* Get the name of the project for this settings
					* @return returns project name as string
					*/
					std::string getProjectName() const
					{
						return getStringField(REPO_LABEL_ID);
					}

					/**
					* Get the description of the project for this settings
					* @return returns project description as string
					*/
					std::string getDescription() const
					{
						return getStringField(REPO_LABEL_DESCRIPTION);
					}

					/**
					* Get the owner of the project for this settings
					* @return returns owner name as string
					*/
					std::string getOwner() const
					{
						return getStringField(REPO_LABEL_OWNER);
					}

					/**
					* Get the group of the project for this settings
					* @return returns group name as string
					*/
					std::string getGroup() const
					{
						return getStringField(REPO_LABEL_GROUP);
					}

					/**
					* Get permissions as a vector of 9 booleans
					* the booleans represents permissions in this order:
					* owner read, owner write, owner exe,
					* group read, group write, group exe,
					* public read, public write, public exe
					* @return returns a vector of size 9 containing the permissions
					*/
					std::vector<bool> RepoProjectSettings::getPermissionsBoolean() const;

					/**
					* Get permissions as octal form (777, 660 etc)
					* @return returns a vector of size 3 containing the permissions
					*/
					std::vector<uint8_t> getPermissionsOctal() const;

					/**
					* Get the rwx permissions of the project as string
					* @return returns permissions as string (eee denotes error reading that permission value)
					*/
					std::string getPermissionsString() const;

					/**
					* given a string representation of a 4 digit octal, return the
					* permissions in boolean form
					* @param octal octal permission number in string form(4digit)
					* @return returns a vector of 12 booleans representing the relative permissions
					*/
					static std::vector<bool> stringToPermissionsBool(std::string octal);

					/**
					* Get the type of the project for this settings
					* @return returns project type as string
					*/
					std::string getType() const
					{
						return getStringField(REPO_LABEL_TYPE);
					}

					std::vector<std::string> getUsers() const;


				};
		}// end namespace model
	} // end namespace core
} // end namespace repo

