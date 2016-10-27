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
#include "../collection/repo_scene.h"
#include "repo_bson_builder.h"
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

using namespace repo::core::model;

RepoRole RepoRole::cloneAndUpdatePermissions(
	const std::vector<RepoPermission> &permissions
	) const
{
	//Remove all project access related privileges, then add the new ones in.
	std::vector<RepoPermission> oldPermissions = getProjectAccessRights();

	auto oldPriv = getPrivilegesMapped(translatePermissions(oldPermissions));

	std::unordered_map<std::string, RepoPrivilege> mapped_privileges = getPrivilegesMapped();

	for (const auto &p : oldPriv)
	{
		mapped_privileges.erase(p.first);
	}

	auto newPriv = getPrivilegesMapped(translatePermissions(permissions));

	mapped_privileges.insert(newPriv.begin(), newPriv.end());

	std::vector<RepoPrivilege> privilegesUpdated;

	boost::copy(
		mapped_privileges | boost::adaptors::map_values,
		std::back_inserter(privilegesUpdated));

	return cloneAndUpdatePrivileges(privilegesUpdated);
}

RepoRole RepoRole::cloneAndUpdatePrivileges(
	const std::vector<RepoPrivilege> &privileges
	) const
{
	RepoBSONBuilder builder;
	if (privileges.size() > 0)
	{
		RepoBSONBuilder privilegesBuilder;
		for (size_t i = 0; i < privileges.size(); ++i)
		{
			const auto &p = privileges[i];
			RepoBSONBuilder innerBsonBuilder, actionBuilder;
			RepoBSON resource = BSON(REPO_ROLE_LABEL_DATABASE << p.database << REPO_ROLE_LABEL_COLLECTION << p.collection);
			innerBsonBuilder << REPO_ROLE_LABEL_RESOURCE << resource;

			for (size_t aCount = 0; aCount < p.actions.size(); ++aCount)
			{
				actionBuilder << std::to_string(aCount) << RepoRole::dbActionToString(p.actions[aCount]);
			}

			innerBsonBuilder.appendArray(REPO_ROLE_LABEL_ACTIONS, actionBuilder.obj());

			privilegesBuilder << std::to_string(i) << innerBsonBuilder.obj();
		}
		builder.appendArray(REPO_ROLE_LABEL_PRIVILEGES, privilegesBuilder.obj());
	}

	auto change = builder.obj();
	return RepoRole(cloneAndAddFields(&change));
}

std::string RepoRole::dbActionToString(const DBActions &action)
{
	switch (action)
	{
	case DBActions::INSERT:
		return "insert";
	case DBActions::UPDATE:
		return "update";
	case DBActions::REMOVE:
		return "remove";
	case DBActions::FIND:
		return "find";
	case DBActions::CREATE_USER:
		return "createUser";
	case DBActions::CREATE_ROLE:
		return "createRole";
	case DBActions::DROP_ROLE:
		return "dropRole";
	case DBActions::GRANT_ROLE:
		return "grantRole";
	case DBActions::REVOKE_ROLE:
		return "revokeRole";
	case DBActions::VIEW_ROLE:
		return "viewRole";
	case DBActions::UNKNOWN:
		//We recognise it as unknown, so we don't need to throw an error here.
		break;
	default:
		//This error is only thrown if someone added the action in enum but didn't add a case here
		//If you see this error you should simply add a case here
		repoError << "Unrecognised action value: " << (uint32_t)action;
	}
	return std::string(); //only default values will fall through and return empty string.
}

std::vector<std::string> RepoRole::dbActionsToStrings(
	const std::vector<DBActions> &actions)
{
	std::vector<std::string> strings(actions.size());
	std::vector<std::string>::size_type i = 0;
	for (DBActions action : actions)
	{
		strings[i++] = dbActionToString(action);
	}
	return strings;
}

DBActions RepoRole::stringToDBAction(const std::string &action)
{
	std::string actionUpperCase = action;

	std::transform(actionUpperCase.begin(), actionUpperCase.end(), actionUpperCase.begin(), ::toupper);

	if (actionUpperCase == "FIND")
	{
		return DBActions::FIND;
	}

	if (actionUpperCase == "REMOVE")
	{
		return DBActions::REMOVE;
	}

	if (actionUpperCase == "INSERT")
	{
		return DBActions::INSERT;
	}

	if (actionUpperCase == "UPDATE")
	{
		return DBActions::UPDATE;
	}

	if (actionUpperCase == "CREATEUSER")
	{
		return DBActions::CREATE_USER;
	}

	if (actionUpperCase == "CREATEROLE")
	{
		return DBActions::CREATE_ROLE;
	}

	if (actionUpperCase == "DROPROLE")
	{
		return DBActions::DROP_ROLE;
	}

	if (actionUpperCase == "GRANTROLE")
	{
		return DBActions::GRANT_ROLE;
	}

	if (actionUpperCase == "REVOKEROLE")
	{
		return DBActions::REVOKE_ROLE;
	}

	if (actionUpperCase == "VIEWROLE")
	{
		return DBActions::VIEW_ROLE;
	}
	repoDebug << "Unrecognised privileged action: " << action << " ignoring...";

	return DBActions::UNKNOWN;
}

std::vector<DBActions> RepoRole::stringsToDBActions(
	const std::vector<std::string> &strings)
{
	std::vector<DBActions> actions(strings.size());
	std::vector<DBActions>::size_type i = 0;
	for (std::string string : strings)
	{
		actions[i++] = stringToDBAction(string);
	}
	return actions;
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

std::unordered_map<std::string, RepoPrivilege> RepoRole::getPrivilegesMapped(const std::vector<RepoPrivilege> &ps)
{
	std::unordered_map<std::string, RepoPrivilege> map;

	for (const auto priv : ps)
	{
		map[priv.database + "." + priv.collection] = priv;
	}

	return map;
}

std::vector<RepoPrivilege> RepoRole::translatePermissions(
	const std::vector<RepoPermission> &permissions)
{
	std::vector<RepoPrivilege> privileges;
	//privileges would be at least the side of permissions
	privileges.reserve(permissions.size());

	/*
		For each project, we need to look at the permission (r/w/rw) allowed and
		translate that into db actions. One project maps to different collections,
		and different collections would require different actions to read/write.
		for e.g.:
		write access to .scene would mean insert but not update,
		as you are not suppose to alter any objects.
		but for .issues, you should have write permission (update AND insert)
		even if you only have read privileges
		For project settings (settings collection), it will be an update and insert operation.
		*/
	for (const RepoPermission &p : permissions)
	{
		if (!p.project.empty())
		{
			for (const std::string &postfix : RepoScene::getProjectExtensions())
			{
				RepoPrivilege privilege;
				privilege.database = p.database;
				privilege.collection = p.project + "." + postfix;
				updateActions(postfix, p.permission, privilege.actions);
				privileges.push_back(privilege);
			}

			//FIXME: If you have write access to a project, do you
			//need write access to settings? or should the project be created by an admin already
			//and people who can upload to the project has no right to alter the settings?
		}
	}

	return privileges;
}

std::vector<RepoPermission> RepoRole::translatePrivileges(
	const std::vector<RepoPrivilege> &privileges)
{
	std::vector<RepoPermission> permissions;

	for (const RepoPrivilege &p : privileges)
	{
		size_t lastDot = p.collection.find_last_of('.');
		if (lastDot == std::string::npos)
		{
			//does not have a .* Regard it as a different project
			RepoPermission perm;
			perm.database = p.database;
			perm.project = p.collection;
			bool hasRead = std::find(p.actions.begin(), p.actions.end(), DBActions::FIND) != p.actions.end();
			bool hasWrite = std::find(p.actions.begin(), p.actions.end(), DBActions::INSERT) != p.actions.end();
			hasWrite |= std::find(p.actions.begin(), p.actions.end(), DBActions::UPDATE) != p.actions.end();

			if (hasRead && hasWrite)
			{
				perm.permission = AccessRight::READ_WRITE;
			}
			else if (hasWrite)
			{
				perm.permission = AccessRight::WRITE;
			}
			else
			{
				perm.permission = AccessRight::READ;
			}
			permissions.push_back(perm);
		}
		else
		{
			std::string postfix = p.collection.substr(lastDot + 1);
			/*
				Making an assumption here that all valid projects has a
				history collection. so we only look for .history when we are looking for a project
				to check for permissions
				*/
			if (postfix == "history")
			{
				RepoPermission perm;
				perm.database = p.database;
				perm.project = p.collection.substr(0, lastDot);
				bool hasRead = std::find(p.actions.begin(), p.actions.end(), DBActions::FIND) != p.actions.end();
				bool hasWrite = std::find(p.actions.begin(), p.actions.end(), DBActions::INSERT) != p.actions.end();

				if (hasRead && hasWrite)
				{
					perm.permission = AccessRight::READ_WRITE;
				}
				else if (hasWrite)
				{
					perm.permission = AccessRight::WRITE;
				}
				else
				{
					perm.permission = AccessRight::READ;
				}
				permissions.push_back(perm);
			}
		}
	}

	return permissions;
}

void RepoRole::updateActions(
	const std::string &collectionType,
	const AccessRight &permission,
	std::vector<DBActions> &vec
	)
{
	//READ PERMISSIONS:
	if (permission == AccessRight::READ || permission == AccessRight::READ_WRITE)
	{
		vec.push_back(DBActions::FIND);

		if (collectionType == "issues" || collectionType == "groups")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::UPDATE);
		}
	}

	//WRITE PERMISSIONS:
	if (permission == AccessRight::WRITE || permission == AccessRight::READ_WRITE)
	{
		if (collectionType == "scene")
		{
			vec.push_back(DBActions::INSERT);
		}
		else if (collectionType == "scene.files")
		{
			vec.push_back(DBActions::INSERT);
		}
		else if (collectionType == "scene.chunks")
		{
			vec.push_back(DBActions::INSERT);
		}
		else if (collectionType == "stash.3drepo")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.3drepo.files")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.3drepo.chunks")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.src")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.src.files")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.src.chunks")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.json_mpc.files")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.json_mpc.chunks")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.x3d")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.x3d.files")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.x3d.chunks")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.gltf")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.gltf.files")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "stash.gltf.chunks")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::REMOVE);
		}
		else if (collectionType == "history")
		{
			vec.push_back(DBActions::INSERT);
		}
		else if (collectionType == "history.files")
		{
			vec.push_back(DBActions::INSERT);
		}
		else if (collectionType == "history.chunks")
		{
			vec.push_back(DBActions::INSERT);
		}
		else if (collectionType == "issues")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::UPDATE);
		}
		else if (collectionType == "groups")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::UPDATE);
		}
		else if (collectionType == "wayfinder")
		{
			vec.push_back(DBActions::INSERT);
			vec.push_back(DBActions::UPDATE);
			vec.push_back(DBActions::REMOVE);
		}
		else

		{
			repoError << "Failed to update privileges: unknown collection type: " << collectionType;
		}
	}

	//Prune possible duplicates
	std::sort(vec.begin(), vec.end());
	vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}