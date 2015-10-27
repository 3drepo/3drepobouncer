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
			enum class DBActions {INSERT, UPDATE, FIND, CREATE_USER, CREATE_ROLE, GRANT_ROLE, REVOKE_ROLE, VIEW_ROLE};

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

			};

		}// end namespace model
	} // end namespace core
} // end namespace repo


