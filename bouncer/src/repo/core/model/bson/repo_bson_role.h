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

#pragma once
#include "repo_bson.h"

namespace repo {
	namespace core {
		namespace model {
			enum class DBActions {INSERT, UPDATE, FIND, CREATE_USER, CREATE_ROLE, GRANT_ROLE, REVOKE_ROLE, VIEW_ROLE, UNKNOWN};

			struct RepoPrivilege{
				std::string database;
				std::string collection;
				std::vector<DBActions> actions;
				
			};

			class REPO_API_EXPORT RepoRole :
				public RepoBSON
			{

				#define REPO_ROLE_LABEL_ROLE       "role"
				#define REPO_ROLE_LABEL_DATABASE   "db"
				#define REPO_ROLE_LABEL_COLLECTION "collection"
				#define REPO_ROLE_LABEL_RESOURCE   "resource"
				#define REPO_ROLE_LABEL_ACTIONS    "actions"
				#define REPO_ROLE_LABEL_PRIVILEGES "privileges"
				#define REPO_ROLE_LABEL_INHERITED_ROLES "roles"
			public:
				RepoRole();

				RepoRole(RepoBSON bson) : RepoBSON(bson){}

				~RepoRole();

				static std::string dbActionToString(const DBActions &action);


				/**
				* --------- Convenience functions -----------
				*/

				/**
				* Get the name of the database which this role belongs to
				* @return returns the name of the database
				*/
				std::string getDatabase() const
				{
					return getStringField(REPO_ROLE_LABEL_DATABASE);
				}

				/**
				* Get the list of roles this role inherited as a vector of {database, role}
				* @return returns a vector of pairs {database, role}
				*/
				std::vector<std::pair<std::string, std::string>> getInheritedRoles() const;

				/**
				* Get the name of the role
				* @return returns the name of the role
				*/
				std::string getName() const
				{
					return getStringField(REPO_ROLE_LABEL_ROLE);
				}

				/**
				* Get the list of privileges as a vector
				* @return returns a vector of privileges
				*/
				std::vector<RepoPrivilege> getPrivileges() const;				

			private:
				/**
				* Given a RepoBSON that is an array of database actions, 
				* return a vector of DBActions
				* @param actionArr a BSON that contains an array of actions
				* @return returns a vector of DBActions
				*/
				std::vector<DBActions> RepoRole::getActions(RepoBSON actionArr) const;

				/**
				* Given a string representing a dbAction, returns the enumType
				* @param action action in string
				* @return returns Action type in DBAction
				*/
				DBActions RepoRole::stringToDBAction(const std::string &action) const;
			};

		}// end namespace model
	} // end namespace core
} // end namespace repo


