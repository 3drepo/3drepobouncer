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

#include "repo_bson_builder.h"
#include "repo_bson_user.h"

using namespace repo::core::model;

RepoUser::RepoUser() : RepoBSON()
{
}

RepoUser::~RepoUser()
{
}

RepoUser RepoUser::cloneAndAddRole(
	const std::string &dbName,
	const std::string &role) const
{
	std::list<std::pair<std::string, std::string>> currentRoles = getRolesList();
	std::pair<std::string, std::string> newEntry = { dbName, role };
	currentRoles.push_back(newEntry);

	RepoBSONBuilder builder;
	builder.appendArrayPair(REPO_USER_LABEL_ROLES, currentRoles, REPO_USER_LABEL_DB, REPO_USER_LABEL_ROLE);
	builder.appendElementsUnique(*this);

	return RepoUser(builder.obj());
}

RepoUser RepoUser::cloneAndUpdateLicenseCount(
	const int &diff,
	const int64_t expDate
	) const
{
	auto subs = getSubscriptionInfo();
	int orgCount = subs.size();
	if (diff < 0)
	{
		//Removing license
		for (int i = 0; i < subs.size(); ++i)
		{
			if (subs[i].planName != REPO_USER_SUBS_STANDARD_FREE
				&& isSubActive(subs[i]) && subs[i].assignedUser.empty())
			{
				//Can only delete it if it is not the free account, it's active and it's not assigned.
				subs.erase(subs.begin() + i);
				--i; //decrease the counter as we have just removed the current item.
				if (orgCount + diff == subs.size()) break;
			}
		}

		if (orgCount + diff == subs.size() - 1)
		{
			// We're missing one license - the self assigned license
			// go round again and removed the last license that is assigned to yourself
			//FIXME: Probably a better way to do this..
			for (int i = 0; i < subs.size(); ++i)
			{
				if (subs[i].planName != REPO_USER_SUBS_STANDARD_FREE
					&& isSubActive(subs[i]) && subs[i].assignedUser == getUserName())
				{
					//Can only delete it if it is not the free account, it's active and it's not assigned.
					subs.erase(subs.begin() + i);
					--i; //decrease the counter as we have just removed the current item.
				}
			}
		}

		if (orgCount + diff != subs.size())
		{
			repoError << "Failed to remove " << diff << " licenses. Licenses cannot be removed if assigned";
		}
	}
	else
	{
		bool firstLicense = (!getLicenseAssignment().size());

		for (int i = 0; i < diff; ++i)
		{
			subs.push_back(createDefaultSub(expDate));
			if (firstLicense)
			{
				//We're adding a first license, make sure it's assigned to the user
				subs.back().assignedUser = getUserName();
				firstLicense = false;
			}
		}
	}

	return cloneAndUpdateSubscriptions(subs);
}

RepoUser RepoUser::cloneAndUpdateSubscriptions(
	const std::vector<RepoUser::SubscriptionInfo> &subs) const
{
	//turn the subscription struct into a bsonArray
	std::vector<RepoBSON> subArrayBson;
	for (const auto &sub : subs)
	{
		RepoBSONBuilder builder;
		if (sub.id.empty())
			builder << REPO_USER_LABEL_SUBS_ID << mongo::OID::gen();
		else
			builder << REPO_USER_LABEL_SUBS_ID << mongo::OID(sub.id);

		if (!sub.planName.empty())
			builder << REPO_USER_LABEL_SUBS_PLAN << sub.planName;

		if (!sub.token.empty())
			builder << REPO_USER_LABEL_SUBS_TOKEN << sub.token;

		if (!sub.billingUser.empty())
			builder << REPO_USER_LABEL_SUBS_BILLING_USER << sub.billingUser;

		if (!sub.assignedUser.empty())
			builder << REPO_USER_LABEL_SUBS_ASSIGNED_USER << sub.assignedUser;

		if (sub.active)
			builder << REPO_USER_LABEL_SUBS_ACTIVE << sub.active;

		if (sub.inCurrentAgreement)
			builder << REPO_USER_LABEL_SUBS_CURRENT_AGREE << sub.inCurrentAgreement;

		if (sub.pendingDelete)
			builder << REPO_USER_LABEL_SUBS_PENDING_DEL << sub.pendingDelete;

		if (sub.createdAt != -1)
			builder.appendTime(REPO_USER_LABEL_SUBS_CREATED_AT, sub.createdAt);
		else
			builder << REPO_USER_LABEL_SUBS_CREATED_AT << mongo::BSONNULL;

		if (sub.updatedAt != -1)
			builder.appendTime(REPO_USER_LABEL_SUBS_UPDATED_AT, sub.updatedAt);
		else
			builder << REPO_USER_LABEL_SUBS_UPDATED_AT << mongo::BSONNULL;

		if (sub.expiresAt != -1)
			builder.appendTime(REPO_USER_LABEL_SUBS_EXPIRES_AT, sub.expiresAt);
		else
			builder << REPO_USER_LABEL_SUBS_EXPIRES_AT << mongo::BSONNULL;

		RepoBSONBuilder limitsBuilder;
		limitsBuilder << REPO_USER_LABEL_SUBS_LIMITS_COLLAB << sub.collaboratorLimit;
		limitsBuilder << REPO_USER_LABEL_SUBS_LIMITS_SPACE << sub.spaceLimit;
		builder.append(REPO_USER_LABEL_SUBS_LIMITS, limitsBuilder.obj());

		auto subBson = builder.obj();
		subArrayBson.push_back(subBson);
	}

	RepoBSONBuilder newCustomDataBuilder, newUserBSON;
	newCustomDataBuilder.appendArray(REPO_USER_LABEL_SUBS, subArrayBson);
	newCustomDataBuilder.appendElementsUnique(getObjectField(REPO_USER_LABEL_CUSTOM_DATA));

	newUserBSON << REPO_USER_LABEL_CUSTOM_DATA << newCustomDataBuilder.obj();
	newUserBSON.appendElementsUnique(*this);

	return newUserBSON.obj();
}

RepoUser::SubscriptionInfo RepoUser::createDefaultSub(
	const int64_t &expireTS)
{
	SubscriptionInfo sub;

	auto ts = RepoBSON::getCurrentTimestamp();
	sub.planName = "THE-100-QUID-PLAN";
	sub.createdAt = sub.updatedAt = ts;
	sub.expiresAt = expireTS;
	sub.inCurrentAgreement = true;
	sub.id = mongo::OID::gen().toString();
	sub.active = true;
	sub.pendingDelete = false;
	sub.collaboratorLimit = 1;
	sub.spaceLimit = 10737418240l;//10GB

	return sub;
}

std::list<std::pair<std::string, std::string> > RepoUser::getAPIKeysList() const
{
	RepoBSON customData = getCustomDataBSON();

	return customData.getListStringPairField(REPO_USER_LABEL_API_KEYS, REPO_USER_LABEL_LABEL, REPO_USER_LABEL_KEY);
}

std::vector<char> RepoUser::getAvatarAsRawData() const
{
	std::vector<char> image;
	RepoBSON customData = getCustomDataBSON();
	if (customData.hasField(REPO_USER_LABEL_AVATAR))
		RepoBSON(customData.getObjectField(REPO_USER_LABEL_AVATAR)).getBinaryFieldAsVector("data", image);

	return image;
}

std::string RepoUser::getCleartextPassword() const
{
	std::string password;
	if (hasField(REPO_USER_LABEL_CREDENTIALS))
	{
		RepoBSON cred = getField(REPO_USER_LABEL_CREDENTIALS).embeddedObject();

		password = cred.getField(REPO_USER_LABEL_CLEARTEXT).str();
	}
	return password;
}

RepoBSON RepoUser::getCustomDataBSON() const
{
	RepoBSON customData;
	if (hasField(REPO_USER_LABEL_CUSTOM_DATA))
	{
		customData = getField(REPO_USER_LABEL_CUSTOM_DATA).embeddedObject();
	}
	return customData;
}

std::list<std::pair<std::string, std::string>> RepoUser::getLicenseAssignment() const
{
	std::list<std::pair<std::string, std::string>> results;

	auto subs = getSubscriptionInfo();
	for (const auto &sub : subs)
	{
		if (sub.planName != REPO_USER_SUBS_STANDARD_FREE && isSubActive(sub))
		{
			//skip the free plan as it's not a license
			results.push_back({ sub.planName, sub.assignedUser });
		}
	}
	return results;
}

RepoBSON RepoUser::getRolesBSON() const
{
	RepoBSON roles;
	if (hasField(REPO_USER_LABEL_ROLES))
	{
		roles = getField(REPO_USER_LABEL_ROLES).embeddedObject();
	}
	return roles;
}

std::string RepoUser::getPassword() const
{
	std::string password;
	if (hasField(REPO_USER_LABEL_CREDENTIALS))
	{
		RepoBSON cred = getField(REPO_USER_LABEL_CREDENTIALS).embeddedObject();

		password = cred.getField(REPO_USER_LABEL_ENCRYPTION).str();
	}
	return password;
}

std::list<std::pair<std::string, std::string>>
RepoUser::getRolesList() const
{
	std::list<std::pair<std::string, std::string>> result;
	result = getListStringPairField(REPO_USER_LABEL_ROLES, REPO_USER_LABEL_DB, REPO_USER_LABEL_ROLE);

	return result;
}

std::vector<RepoUser::SubscriptionInfo> RepoUser::getSubscriptionInfo() const
{
	std::vector<RepoUser::SubscriptionInfo> result;
	RepoBSON customData = getCustomDataBSON();
	if (!customData.isEmpty())
	{
		auto subs = customData.getObjectField(REPO_USER_LABEL_SUBS);
		if (!subs.isEmpty())
		{
			for (int i = 0; i < subs.nFields(); ++i)
			{
				RepoBSON singleSub = subs.getObjectField(std::to_string(i));
				if (!singleSub.isEmpty())
				{
					result.resize(result.size() + 1);
					result.back().id = singleSub.hasField(REPO_USER_LABEL_SUBS_ID) ? singleSub.getField(REPO_USER_LABEL_SUBS_ID).OID().toString() : "";
					result.back().planName = singleSub.getStringField(REPO_USER_LABEL_SUBS_PLAN);
					result.back().token = singleSub.getStringField(REPO_USER_LABEL_SUBS_TOKEN);
					result.back().billingUser = singleSub.getStringField(REPO_USER_LABEL_SUBS_BILLING_USER);
					result.back().assignedUser = singleSub.getStringField(REPO_USER_LABEL_SUBS_ASSIGNED_USER);
					result.back().active = singleSub.getBoolField(REPO_USER_LABEL_SUBS_ACTIVE);
					result.back().inCurrentAgreement = singleSub.getBoolField(REPO_USER_LABEL_SUBS_CURRENT_AGREE);
					result.back().pendingDelete = singleSub.getBoolField(REPO_USER_LABEL_SUBS_PENDING_DEL);
					result.back().createdAt = singleSub.getTimeStampField(REPO_USER_LABEL_SUBS_CREATED_AT);
					result.back().updatedAt = singleSub.getTimeStampField(REPO_USER_LABEL_SUBS_UPDATED_AT);
					result.back().expiresAt = singleSub.getTimeStampField(REPO_USER_LABEL_SUBS_EXPIRES_AT);

					auto limitsBson = singleSub.getObjectField(REPO_USER_LABEL_SUBS_LIMITS);
					if (limitsBson.isEmpty())
					{
						result.back().collaboratorLimit = 0;
						result.back().spaceLimit = 0;
					}
					else
					{
						auto nCollab = limitsBson.getIntField(REPO_USER_LABEL_SUBS_LIMITS_COLLAB);
						auto nSpace = limitsBson.hasField(REPO_USER_LABEL_SUBS_LIMITS_SPACE) ? limitsBson.getField(REPO_USER_LABEL_SUBS_LIMITS_SPACE).Long() : 0;
						result.back().collaboratorLimit = nCollab == INT_MIN ? 0 : nCollab;
						result.back().spaceLimit = nSpace == INT_MIN ? 0 : nSpace;
					}
				}
			}
		}
	}

	return result;
}

uint32_t RepoUser::getNCollaborators() const
{
	uint32_t nCollab = 0;
	auto subs = getSubscriptionInfo();
	for (const auto &sub : subs)
	{
		if (isSubActive(sub))
			nCollab += sub.collaboratorLimit;
	}

	return nCollab;
}

uint64_t RepoUser::getQuota() const
{
	uint64_t quota = 0;
	auto subs = getSubscriptionInfo();
	for (const auto &sub : subs)
	{
		if (isSubActive(sub))
		{
			quota += sub.spaceLimit;
		}
	}

	return quota;
}

bool RepoUser::isSubActive(const RepoUser::SubscriptionInfo &sub) const
{
	return sub.expiresAt >= RepoBSON::getCurrentTimestamp() || sub.expiresAt == -1;
}