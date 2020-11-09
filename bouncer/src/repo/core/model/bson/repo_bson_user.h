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
#define REPO_USER_LABEL_AVATAR						"avatar"
#define REPO_USER_LABEL_CREDENTIALS					"credentials"
#define REPO_USER_LABEL_CUSTOM_DATA					"customData"
#define REPO_USER_LABEL_EMAIL     					"email"
#define REPO_USER_LABEL_FIRST_NAME     				"firstName"
#define REPO_USER_LABEL_LAST_NAME     				"lastName"
#define REPO_USER_LABEL_ENCRYPTION					"MONGODB-CR"
#define REPO_USER_LABEL_CLEARTEXT					"cleartext"
#define REPO_USER_LABEL_LABEL						"label"
#define REPO_USER_LABEL_OWNER						"account"
#define REPO_USER_LABEL_KEY							"key"
#define REPO_USER_LABEL_PWD           				"pwd"
#define REPO_USER_LABEL_USER     					"user"
#define REPO_USER_LABEL_DB							"db"
#define REPO_USER_LABEL_ROLES						"roles"
#define REPO_USER_LABEL_ROLE						"role"
#define REPO_USER_LABEL_API_KEYS					"apiKeys"
#define REPO_USER_LABEL_BILLING						"billing"
#define REPO_USER_LABEL_SUBS						"subscriptions"
#define REPO_USER_LABEL_SUBS_BILLING_USER			"billingUser"
#define REPO_USER_LABEL_VR_ENABLED					"vrEnabled"
#define REPO_USER_LABEL_SRC_ENABLED					"srcEnabled"
#define REPO_USER_LABEL_CREATED_AT					"createdAt"
#define REPO_USER_LABEL_SUB_PAYPAL					"paypal"
#define REPO_USER_LABEL_SUB_DISCRETIONARY			"discretionary"
#define REPO_USER_LABEL_SUB_ENTERPRISE				"enterprise"
#define REPO_USER_LABEL_SUB_DATA					"data"
#define REPO_USER_LABEL_SUB_COLLABORATOR			"collaborators"
#define REPO_USER_LABEL_SUB_EXPIRY_DATE				"expiryDate"
#define REPO_USER_LABAL_SUB_UNLIMITED				"unlimited"			
#define REPO_USER_LABAL_SUB_PAYPAL_QUANTITY			"quantity"
#define REPO_USER_LABAL_SUB_PAYPAL_PLAN				"plan"


			class REPO_API_EXPORT RepoUser : public RepoBSON
			{
			public:
				struct PaypalSubscription
				{
					int32_t quantity;
					int64_t expiryDate;
					std::string plan;					
				};

				struct QuotaLimit
				{
					bool unlimitedUsers = false;
					int32_t collaborators = 0;
					int32_t data = 0 ;
					int64_t expiryDate = 0;
				};

				struct SubscriptionInfo
				{
					std::vector<PaypalSubscription> paypal;
					QuotaLimit enterprise;
					QuotaLimit discretionary;
				};

				RepoUser();

				RepoUser(RepoBSON bson) : RepoBSON(bson){}

				~RepoUser();

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
	

				RepoUser cloneAndUpdateSubscriptions(
					const QuotaLimit &discretionary,
					const QuotaLimit &enterprise) const;

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
				* Get the timestamp of when the user was created
				* @return returns timestamp value of when the user is created, 0 if unknown
				*/
				uint64_t getUserCreatedAt() const;
				

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

				/**
				* Get subscription info
				* @return returns subscriptionInfo struct containing all licenses this user owns
				*/
				SubscriptionInfo getSubscriptionInfo() const;

				/**
				* Get the current collaborator limit of the user (without paypal)
				* @return returns the number of collaborators available to the user, -1 denotes unlimited
				*/
				int32_t getNCollaborators() const;

				/**
				* Get the current quota of the user (without paypal)
				* @return returns the quota available to the user, in MiB
				*/
				double getQuota() const;

				/**
				* Check if VR is enabled for this teamspace
				*/
				bool isVREnabled() const;

				/**
				* Check if SRC stashes are enabled for this teamspace
				*/
				bool isSrcEnabled() const;

			private:
				/**
				* Converts a RepoBSON object into a PaypalSubscription Object
				* @params bson
				*/
				std::vector<PaypalSubscription> convertPayPalSub(const RepoBSON &bson) const;

				/**
				* Converts a RepoBSON object into a QuotaLimit Object
				* @params bson
				*/
				QuotaLimit convertToQuotaObject(const RepoBSON &bson) const;

				/**
				* Given a quota limit, construct a bson object
				* @param quota quota struct that contains quota information
				* @return 
				*/
				RepoBSON createQuotaBSON(QuotaLimit quota) const;

				/**
				* Return subscription bson object
				*/
				RepoBSON getSubscriptionBSON() const;
				
				/**
				* Determine if the given entry has any quota
				* @return returns true if there's quota
				*/
				bool quotaEntryHasQuota(const QuotaLimit &quota) const;

			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
