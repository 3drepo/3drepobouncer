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
#define REPO_USER_LABEL_SUBS				 "subscriptions"
#define REPO_USER_LABEL_SUBS_PLAN			 "plan"
#define REPO_USER_LABEL_SUBS_ID				 "_id"
#define REPO_USER_LABEL_SUBS_ACTIVE			 "active"
#define REPO_USER_LABEL_SUBS_CURRENT_AGREE   "inCurrentAgreement"
#define REPO_USER_LABEL_SUBS_PENDING_DEL     "pendingDelete"
#define REPO_USER_LABEL_SUBS_TOKEN           "token"
#define REPO_USER_LABEL_SUBS_CREATED_AT      "createdAt"
#define REPO_USER_LABEL_SUBS_UPDATED_AT      "updatedAt"
#define REPO_USER_LABEL_SUBS_EXPIRES_AT      "expiredAt"
#define REPO_USER_LABEL_SUBS_BILLING_USER	 "billingUser"
#define REPO_USER_LABEL_SUBS_ASSIGNED_USER	 "assignedUser"
#define REPO_USER_LABEL_SUBS_LIMITS			 "limits"
#define REPO_USER_LABEL_SUBS_LIMITS_COLLAB	 "collaboratorLimit"
#define REPO_USER_LABEL_SUBS_LIMITS_SPACE	 "spaceLimit"

			//name of free user plan
#define REPO_USER_SUBS_STANDARD_FREE   "BASIC"

			class REPO_API_EXPORT RepoUser : public RepoBSON
			{
			public:
				struct SubscriptionInfo
				{
					std::string id;
					bool active;
					bool inCurrentAgreement;
					bool pendingDelete;
					int64_t updatedAt;
					int64_t createdAt;
					int64_t expiresAt;
					std::string billingUser;
					std::string assignedUser;
					std::string token;
					std::string planName;
					int collaboratorLimit;
					double spaceLimit;
				};

				RepoUser();

				RepoUser(RepoBSON bson) : RepoBSON(bson){}

				~RepoUser();

				/**
				* Generate a default subscription
				*/
				static SubscriptionInfo createDefaultSub(
					const int64_t &expireTS = -1);

				/**
				* Clone and add roles into this use bson
				* @param dbName name of the database for the role
				* @param role name of the role
				* @return returns a cloned repoUser bson with role added
				*/

				RepoUser cloneAndAddRole(
					const std::string &dbName,
					const std::string &role) const;

				/**
				* Clone and merge new user info into the existing info
				* @newUserInfo new info that needs ot be added/updated
				* @return returns a new user with fields updated
				*/
				RepoUser cloneAndMergeUserInfo(
					const RepoUser &newUserInfo) const;

				/**
				* Clone and increase/decrease license count
				* if diff is positive, it will add new default licenses
				* If we are trying to decrease n licenses, there has to be n unassigned, active licenses.
				* @param diff the increase or decrease of license (-1 to remove 1, 1 to add 1)
				* @param expDate specify the expiry date of the license (default is -1, which never expires)
				* @returns a cloned version of repoUser with subscription information updated
				*/
				RepoUser cloneAndUpdateLicenseCount(
					const int &diff,
					const int64_t expDate = -1
					) const;

				RepoUser cloneAndUpdateSubscriptions(
					const std::vector<SubscriptionInfo> &subs) const;

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
				* Get information on how the licenses are assigned
				* If the license isn't assigned, returns an empty string
				* @return returns a list of pair {license plan name, assignment}
				*/
				std::list<std::pair<std::string, std::string>> getLicenseAssignment() const;

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

				/**
				* Get list of subscription info
				* @return returns a vector of subscription info
				*/
				std::vector<SubscriptionInfo> getSubscriptionInfo() const;

				/**
				* Get the current collaborator limit of the user
				* @return returns the number of collaborators available to the user
				*/
				uint32_t getNCollaborators() const;

				/**
				* Get the current quota of the user
				* @return returns the quota available to the user
				*/
				uint64_t getQuota() const;

			private:
				/**
				* determine if the subscription is active
				* @param sub subscription to examine
				* @return returns true if it is active, false otherwise
				*/
				bool isSubActive(const RepoUser::SubscriptionInfo &sub) const;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
