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
*  User setting BSON
*/

#pragma once
#include "repo_bson.h"

namespace repo {
	namespace core {
		namespace model {

				//------------------------------------------------------------------------------
				//
				// Fields specific to user only
				//
				//------------------------------------------------------------------------------
				#define REPO_USER_LABEL_AVATAR               "avatar"
				#define REPO_USER_LABEL_CREDENTIALS          "credentials"
				#define REPO_USER_LABEL_CUSTOM_DATA          "customData"
				#define REPO_USER_LABEL_EMAIL     			"email"
				#define REPO_USER_LABEL_FIRST_NAME     		"firstName"
				#define REPO_USER_LABEL_LAST_NAME     		"lastName"
				#define REPO_USER_LABEL_ENCRYPTION           "MONGODB-CR"
				#define REPO_USER_LABEL_CLEARTEXT            "cleartext"
				#define REPO_USER_LABEL_LABEL                "label"
				#define REPO_USER_LABEL_OWNER                "account"
				#define REPO_USER_LABEL_KEY                  "key"
				#define REPO_USER_LABEL_PWD           		 "pwd"
				#define REPO_USER_LABEL_USER     			"user"
				#define REPO_USER_LABEL_DB                   "db"
				#define REPO_USER_LABEL_ROLES                 "roles"
				#define REPO_USER_LABEL_ROLE                 "role"
				#define REPO_USER_LABEL_API_KEYS             "apiKeys"

				class REPO_API_EXPORT RepoUser : public RepoBSON
				{
				public:
					RepoUser();

					RepoUser(RepoBSON bson) : RepoBSON(bson){}

					~RepoUser();

					/**
					* --------- Convenience functions -----------
					*/

					/**
					* Get all api keys associated with this user as a list of (label, key) pairs
					* @return returns a list of label,key pairs
					*/
					std::list<std::pair<std::string, std::string> > getAPIKeysList() const;

					/**
					* Get avatar image as a vector of char
					* @return returns a vector of char representing the binary image
					*/
					std::vector<char> getAvatarAsRawData() const;

					/**
					* Get the clear text password of the user
					* @return returns the clear text password
					*/
					std::string getCleartextPassword() const;

					/**
					* Get the custom data within the user bson as a bson
					* @return returns a bson with custom data in it
					*/
					RepoBSON getCustomDataBSON() const;

					/**
					* Get the roles within the user bson as a bson
					* @return returns a bson with roles in it
					*/
					RepoBSON getRolesBSON() const;

					/**
					* Get the first name of this user
					* @return returns first name of the user, empty string if none
					*/
					std::string getFirstName() const
					{
						RepoBSON customData = getCustomDataBSON();
						return customData.isEmpty() ? "" : customData.getStringField(REPO_USER_LABEL_FIRST_NAME);
					}

					/**
					* Get the last name of this user
					* @return returns last name of the user, empty string if none
					*/
					std::string getLastName() const
					{
						RepoBSON customData = getCustomDataBSON();
						return customData.isEmpty() ? "" : customData.getStringField(REPO_USER_LABEL_LAST_NAME);
					}

					/**
					* Get the username from this user
					* @return returns user name of the user, empty string if none
					*/
					std::string getUserName() const
					{
						return getStringField(REPO_USER_LABEL_USER);
					}

					/**
					* Get the email of this user
					* @return returns email of the user, empty string if none
					*/
					std::string getEmail() const
					{
						RepoBSON customData = getCustomDataBSON();
						return customData.isEmpty() ? "" : customData.getStringField(REPO_USER_LABEL_EMAIL);
					}

					/**
					* Get the password of this user
					* @return returns masked password of the user
					*/
					std::string getPassword() const;

					/**
					* Get list of roles
					* @return returns a vector of string pairs (database, roles)
					*/
					std::list<std::pair<std::string, std::string>>
						getRolesList() const;


				};
		}// end namespace model
	} // end namespace core
} // end namespace repo

