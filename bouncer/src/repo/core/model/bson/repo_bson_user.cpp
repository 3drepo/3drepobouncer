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

RepoUser RepoUser::cloneAndMergeUserInfo(
	const RepoUser &newUserInfo) const
{
	RepoBSONBuilder newUserBuilder, customDataBuilder;
	customDataBuilder.appendElementsUnique(newUserInfo.getCustomDataBSON());
	//We need to preserve the fact that if some fields are removed, they are removed.
	//So throw away some fields that should have been updated.
	auto currentCustomData = getCustomDataBSON();
	currentCustomData = currentCustomData.removeField(REPO_LABEL_AVATAR);
	currentCustomData = currentCustomData.removeField(REPO_USER_LABEL_API_KEYS);
	currentCustomData = currentCustomData.removeField(REPO_USER_LABEL_EMAIL);
	currentCustomData = currentCustomData.removeField(REPO_USER_LABEL_FIRST_NAME);
	currentCustomData = currentCustomData.removeField(REPO_USER_LABEL_LAST_NAME);
	customDataBuilder.appendElementsUnique(currentCustomData);
	newUserBuilder.append(REPO_USER_LABEL_CUSTOM_DATA, customDataBuilder.obj());

	newUserBuilder.appendElementsUnique(newUserInfo);
	auto thisWithNoRoles = removeField(REPO_USER_LABEL_ROLES);
	newUserBuilder.appendElementsUnique(thisWithNoRoles);

	return newUserBuilder.obj();
}

RepoBSON RepoUser::createQuotaBSON(QuotaLimit quota) const {
	RepoBSONBuilder quotaBuilder;

	if(quota.unlimitedUsers)
		quotaBuilder << REPO_USER_LABEL_SUB_COLLABORATOR << REPO_USER_LABAL_SUB_UNLIMITED;
	else
		quotaBuilder << REPO_USER_LABEL_SUB_COLLABORATOR << quota.collaborators;

	quotaBuilder << REPO_USER_LABEL_SUB_DATA << quota.data;
	quotaBuilder.appendTime(REPO_USER_LABEL_SUB_EXPIRY_DATE, quota.expiryDate);
	return quotaBuilder.obj();
}

uint64_t RepoUser::getUserCreatedAt() const {
	uint64_t res = 0;
	auto customData = getCustomDataBSON();
	if (!customData.isEmpty()) 
	{
		res = customData.getTimeStampField(REPO_USER_LABEL_CREATED_AT);
	}
	return res;
}

bool RepoUser::quotaEntryHasQuota(const QuotaLimit &quota) const {

	return quota.unlimitedUsers || quota.collaborators || quota.data;
}

RepoUser RepoUser::cloneAndUpdateSubscriptions(
	const QuotaLimit &discretionary,
	const QuotaLimit &enterprise) const
{
	RepoBSONBuilder updatedLicenses, subscriptionsBuilder;

	if (quotaEntryHasQuota(discretionary))
		updatedLicenses << REPO_USER_LABEL_SUB_DISCRETIONARY << createQuotaBSON(discretionary);
	if (quotaEntryHasQuota(enterprise))
		updatedLicenses << REPO_USER_LABEL_SUB_ENTERPRISE << createQuotaBSON(enterprise);

	auto oldSub = getSubscriptionBSON();
	if (oldSub.hasField(REPO_USER_LABEL_SUB_PAYPAL))
		updatedLicenses << REPO_USER_LABEL_SUB_PAYPAL << oldSub.getObjectField(REPO_USER_LABEL_SUB_PAYPAL);
	

	RepoBSONBuilder newCustomDataBuilder, newUserBSON, billingBuilder;
	billingBuilder << REPO_USER_LABEL_SUBS << updatedLicenses.obj();
	newCustomDataBuilder.append(REPO_USER_LABEL_BILLING, billingBuilder.obj());
	newCustomDataBuilder.appendElementsUnique(getObjectField(REPO_USER_LABEL_CUSTOM_DATA));

	newUserBSON << REPO_USER_LABEL_CUSTOM_DATA << newCustomDataBuilder.obj();
	newUserBSON.appendElementsUnique(*this);

	return newUserBSON.obj();
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

RepoBSON RepoUser::getSubscriptionBSON() const
{
	RepoBSON customData = getCustomDataBSON();
	
	auto billing = !customData.isEmpty()? customData.getObjectField(REPO_USER_LABEL_BILLING) : RepoBSON();	
	return !billing.isEmpty()? billing.getObjectField(REPO_USER_LABEL_SUBS) : RepoBSON();
	
}

RepoUser::QuotaLimit RepoUser::convertToQuotaObject(const RepoBSON &bson) const
{
	QuotaLimit res;
	
	if (!bson.isEmpty())
	{
		if (bson.hasField(REPO_USER_LABEL_SUB_COLLABORATOR)) {
			auto collaboratorStr = std::string(bson.getStringField(REPO_USER_LABEL_SUB_COLLABORATOR));
			res.unlimitedUsers = collaboratorStr == REPO_USER_LABAL_SUB_UNLIMITED;
			res.collaborators = res.unlimitedUsers ? -1 : bson.getIntField(REPO_USER_LABEL_SUB_COLLABORATOR);
		}

		res.data = bson.getIntField(REPO_USER_LABEL_SUB_DATA);

		res.expiryDate = bson.getTimeStampField(REPO_USER_LABEL_SUB_EXPIRY_DATE);
	}

	return res;
}

std::vector<RepoUser::PaypalSubscription> RepoUser::convertPayPalSub(const RepoBSON &bson) const
{
	std::vector<PaypalSubscription> subs;
	if (!bson.isEmpty() && bson.hasField(REPO_USER_LABEL_SUB_PAYPAL))
	{
		auto bsonArr = bson.getField(REPO_USER_LABEL_SUB_PAYPAL).Array();
		for (const auto &entry : bsonArr) {
			auto entryBson = entry.embeddedObject();
			if (!entryBson.isEmpty()) {
				if (entryBson.hasField(REPO_USER_LABAL_SUB_PAYPAL_PLAN) &&
					entryBson.hasField(REPO_USER_LABAL_SUB_PAYPAL_QUANTITY) &&
					entryBson.hasField(REPO_USER_LABEL_SUB_EXPIRY_DATE)
				) {
					PaypalSubscription sub;
					sub.plan = entryBson.getStringField(REPO_USER_LABAL_SUB_PAYPAL_PLAN);
					sub.quantity = entryBson.getIntField(REPO_USER_LABAL_SUB_PAYPAL_QUANTITY);
					sub.expiryDate = ((RepoBSON)entryBson).getTimeStampField(REPO_USER_LABEL_SUB_EXPIRY_DATE);
					subs.push_back(sub);
				}		
			}
		}
		
	}

	return subs;
}

RepoUser::SubscriptionInfo RepoUser::getSubscriptionInfo() const
{
	auto subs = getSubscriptionBSON();
	SubscriptionInfo result;
if (!subs.isEmpty())
	{
		result.discretionary = convertToQuotaObject(subs.getObjectField(REPO_USER_LABEL_SUB_DISCRETIONARY));
		result.enterprise = convertToQuotaObject(subs.getObjectField(REPO_USER_LABEL_SUB_ENTERPRISE));

		result.paypal = convertPayPalSub(subs);		
	}

	return result;
}

int32_t RepoUser::getNCollaborators() const
{
	auto subs = getSubscriptionInfo();	
	bool activeDis = RepoBSON::getCurrentTimestamp() > subs.discretionary.expiryDate;
	bool activeEnt = RepoBSON::getCurrentTimestamp() > subs.enterprise.expiryDate;

	if (activeDis && subs.discretionary.unlimitedUsers || 
		activeEnt && subs.enterprise.unlimitedUsers) return -1;

	return activeDis ? subs.discretionary.collaborators : 0
		+ activeEnt? subs.enterprise.collaborators : 0;
}

double RepoUser::getQuota() const
{
	auto subs = getSubscriptionInfo();

	return subs.discretionary.data + subs.enterprise.data;
}

bool RepoUser::isVREnabled() const
{
	auto customData = getCustomDataBSON();

	return customData.isEmpty() ? false : customData.getBoolField(REPO_USER_LABEL_VR_ENABLED);
}

