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


using namespace repo::core::model::bson;

RepoUser::RepoUser() : RepoBSON()
{
}


RepoUser::~RepoUser()
{
}

RepoUser RepoUser::createRepoUser(
	const std::string                                      &userName,
	const std::string									   &cleartextPassword,
	const std::string                                      &firstName,
	const std::string                                      &lastName,
	const std::string                                      &email,
	const std::list<std::pair<std::string, std::string>>   &projects,
	const std::list<std::pair<std::string, std::string>>   &roles,
	const std::list<std::pair<std::string, std::string> >  &groups,
	const std::list<std::pair<std::string, std::string> >  &apiKeys,
	const std::vector<char>                                &avatar)
{
	RepoBSONBuilder builder;
	RepoBSONBuilder customDataBuilder;

	builder.append(REPO_LABEL_ID, generateUUID());
	if (!userName.empty())
		builder << REPO_USER_LABEL_USER << userName;

	if (!cleartextPassword.empty())
	{
		RepoBSONBuilder credentialsBuilder;
		credentialsBuilder << REPO_USER_LABEL_CLEARTEXT << cleartextPassword;
		builder << REPO_USER_LABEL_CREDENTIALS << credentialsBuilder.obj();
	}

	if (!firstName.empty())
		customDataBuilder << REPO_USER_LABEL_FIRST_NAME << firstName;

	if (!lastName.empty())
		customDataBuilder << REPO_USER_LABEL_LAST_NAME << lastName;

	if (!email.empty())
		customDataBuilder << REPO_USER_LABEL_EMAIL << email;

	if (projects.size())
		customDataBuilder.appendArrayPair(REPO_USER_LABEL_PROJECTS, projects, REPO_USER_LABEL_OWNER, REPO_USER_LABEL_PROJECT);

	if (groups.size())
		customDataBuilder.appendArrayPair(REPO_USER_LABEL_GROUPS, groups, REPO_USER_LABEL_OWNER, REPO_USER_LABEL_GROUP);

	if (!apiKeys.empty())
		customDataBuilder.appendArrayPair(REPO_USER_LABEL_API_KEYS, apiKeys, REPO_USER_LABEL_LABEL, REPO_USER_LABEL_KEY);
	
	if (avatar.size())
	{
		RepoBSONBuilder avatarBuilder;
		//FIXME: use repo image?
		avatarBuilder.appendBinary(REPO_LABEL_DATA, &avatar.at(0), sizeof(avatar.at(0))*avatar.size());
		customDataBuilder << REPO_LABEL_AVATAR << avatarBuilder.obj();
	}
		

	builder << REPO_USER_LABEL_CUSTOM_DATA << customDataBuilder.obj();

	if (roles.size())
		builder.appendArrayPair(REPO_USER_LABEL_ROLES, roles, REPO_USER_LABEL_DB, REPO_USER_LABEL_ROLE);

	return RepoUser(builder.obj());
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
		getBinaryFieldAsVector(customData.getObjectField(REPO_USER_LABEL_AVATAR).getField("data"), &image);

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
/*	RepoBSON roleData = getRolesBSON();
	if (!roleData.isEmpty())
		result = roleData.*/
	result = getListStringPairField(REPO_USER_LABEL_ROLES, REPO_USER_LABEL_DB, REPO_USER_LABEL_ROLE);

	return result;
}

std::list<std::pair<std::string, std::string>>
	RepoUser::getGroupsList() const
{
	std::list<std::pair<std::string, std::string>> result;
	RepoBSON customData = getCustomDataBSON();
	if (!customData.isEmpty())
		result = customData.getListStringPairField(REPO_USER_LABEL_GROUPS, REPO_USER_LABEL_OWNER, REPO_USER_LABEL_GROUP);

	return result;
}

std::list<std::pair<std::string, std::string>>
	RepoUser::getProjectsList() const 
{
	std::list<std::pair<std::string, std::string>> result;
	RepoBSON customData = getCustomDataBSON();
	if (!customData.isEmpty())
		result = customData.getListStringPairField(REPO_USER_LABEL_PROJECTS, REPO_USER_LABEL_OWNER, REPO_USER_LABEL_PROJECT);

	return result;
}

