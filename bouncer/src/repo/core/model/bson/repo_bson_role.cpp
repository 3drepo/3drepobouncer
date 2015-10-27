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

#include <algorithm>
#include "repo_bson_role.h"


using namespace repo::core::model;

RepoRole::RepoRole()
{
}


RepoRole::~RepoRole()
{
}


std::string RepoRole::dbActionToString(const DBActions &action)
{
	
	switch (action)
	{
	case DBActions::INSERT:
		return "insert";
	case DBActions::UPDATE:
		return "update";
	case DBActions::FIND:
		return "find";
	case DBActions::CREATE_USER:
		return "createUser";
	case DBActions::CREATE_ROLE:
		return "createRole";
	case DBActions::GRANT_ROLE:
		return "grantRole";
	case DBActions::REVOKE_ROLE:
		return "revokeRole";
	case DBActions::VIEW_ROLE:
		return "viewRole";
	default:
		repoError << "Unrecognised action value: " << (uint32_t)action;
	}

	return ""; //only default values will fall through and return empty string.
}

std::vector<DBActions> RepoRole::getActions(RepoBSON actionArr) const
{
	std::vector<DBActions> actions;
	std::set<std::string> fieldNames;
	actionArr.getFieldNames(fieldNames);

	for (const auto &field : fieldNames)
	{
		actions.push_back(stringToDBAction(actionArr.getStringField(field)));
	}

	return actions;
}

std::vector<std::pair<std::string, std::string>> RepoRole::getInheritedRoles() const
{
	std::vector<std::pair<std::string, std::string>> roles;

	RepoBSON parentRoles = getObjectField(REPO_ROLE_LABEL_INHERITED_ROLES); 

	std::set<std::string> fieldNames;
	parentRoles.getFieldNames(fieldNames);

	for (const auto &field : fieldNames)
	{
		RepoBSON roleBSON = parentRoles.getObjectField(field);
		std::string role = roleBSON.getStringField(REPO_ROLE_LABEL_ROLE);
		std::string db = roleBSON.getStringField(REPO_ROLE_LABEL_DATABASE);
		//If there is no database field then it means it's in the same database as this role
		if (db.empty())
			db = getDatabase();
		if (!role.empty())
		{
			std::pair<std::string, std::string> pairing;
			pairing.first = db;
			pairing.second = role; 
			roles.push_back(pairing);
		}
			
	}

	return roles;
}

std::vector<RepoPrivilege> RepoRole::getPrivileges() const
{
	std::vector<RepoPrivilege> privileges;

	RepoBSON pbson = getObjectField(REPO_ROLE_LABEL_PRIVILEGES);
	if (!pbson.isEmpty())
	{
		std::set<std::string> fieldNames;
		pbson.getFieldNames(fieldNames);

		for (const auto &field : fieldNames)
		{
			RepoBSON privilege = pbson.getObjectField(field);
			if (privilege.hasField(REPO_ROLE_LABEL_RESOURCE) 
				&& privilege.hasField(REPO_ROLE_LABEL_ACTIONS))
			{
				std::vector<DBActions> actions = getActions(privilege.getObjectField(REPO_ROLE_LABEL_ACTIONS));
				RepoBSON resource = privilege.getObjectField(REPO_ROLE_LABEL_RESOURCE);
				std::string database = resource.getStringField(REPO_ROLE_LABEL_DATABASE);
				std::string collection = resource.getStringField(REPO_ROLE_LABEL_COLLECTION);
				privileges.push_back({ database, collection, actions });
			}
		}
	}

	return privileges;
}

DBActions RepoRole::stringToDBAction(const std::string &action) const
{
	std::string action_lowerCase = action;

	std::transform(action_lowerCase.begin(), action_lowerCase.end(), action_lowerCase.begin(), ::toupper);

	if (action_lowerCase == "FIND")
	{
		return DBActions::FIND;
	}

	
	if (action_lowerCase == "INSERT")
	{
		return DBActions::INSERT;
	}

	if (action_lowerCase == "UPDATE")
	{
		return DBActions::UPDATE;
	}

	if (action_lowerCase == "CREATEUSER")
	{
		return DBActions::CREATE_USER;
	}

	if (action_lowerCase == "CREATEROLE")
	{
		return DBActions::CREATE_ROLE;
	}

	if (action_lowerCase == "GRANTROLE")
	{
		return DBActions::GRANT_ROLE;
	}

	if (action_lowerCase == "REVOKEROLE")
	{
		return DBActions::REVOKE_ROLE;
	}

	if (action_lowerCase == "VIEWROLE")
	{
		return DBActions::VIEW_ROLE;
	}

	repoWarning << "Unrecognised privileged action: " << action;

	return DBActions::UNKNOWN;

}