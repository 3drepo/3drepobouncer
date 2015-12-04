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

enum class DBActions { INSERT, UPDATE, REMOVE, FIND, CREATE_USER, CREATE_ROLE,
                       DROP_ROLE, GRANT_ROLE, REVOKE_ROLE, VIEW_ROLE, UNKNOWN };
enum class AccessRight { READ, WRITE, READ_WRITE};

struct RepoPrivilege
{
    std::string database;
    std::string collection;
    std::vector<DBActions> actions;

};

struct RepoPermission
{
    std::string database;
    //Project name (not collection! put "model" instead of "model.{scene,history,stash,...}" in here.)
    std::string project;
    AccessRight permission;

};

class REPO_API_EXPORT RepoRole : public RepoBSON
{

#define REPO_ROLE_LABEL_ROLE            "role"
#define REPO_ROLE_LABEL_DATABASE        "db"
#define REPO_ROLE_LABEL_COLLECTION      "collection"
#define REPO_ROLE_LABEL_RESOURCE        "resource"
#define REPO_ROLE_LABEL_ACTIONS         "actions"
#define REPO_ROLE_LABEL_PRIVILEGES      "privileges"
#define REPO_ROLE_LABEL_INHERITED_ROLES "roles"

public:

	RepoRole() {}

	RepoRole(RepoBSON bson) : RepoBSON(bson){}

	~RepoRole() {}

public:

	/**
	* Convert a given DBAction to a string command
	* @param action the DBAction enum to convert from
	* @return a string that represents the DBAction, empty string if unknown action
	*/
	static std::string dbActionToString(const DBActions &action);

	/**
	* Convert a given DBAction vector to a vector of string commands
	* @param actions the vector of DBAction enums to convert from
	* @return vector of strings that represents the DBActions
	*/
	static std::vector<std::string> dbActionsToStrings(
		const std::vector<DBActions> &actions);

	/**
	* Given a string representing a dbAction, returns the enumType
	* @param action action in string
	* @return returns Action type in DBAction
	*/
	static DBActions stringToDBAction(const std::string &action);

	/**
	* Given a vector of strings representing dbActions, returns a vector of
	* corresponding enumTypes
	* @param actions vector of actions in strings
	* @return returns vector of action types in DBActions
	*/
	static std::vector<DBActions> stringsToDBActions(
		const std::vector<std::string> &strings);

	/**
	* Translate RepoPermission of projects into RepoPrivileges for collections
	* @param permission a vector of permissions to translate
	* @return returns a vector of privileges for the corresponding collections
	*/
	static std::vector<RepoPrivilege> translatePermissions(
		const std::vector<RepoPermission> &permissions);

	/**
	* Translate RepoPermission of projects into RepoPrivileges for collections
	* @param permission a vector of permissions to translate
	* @return returns a vector of privileges for the corresponding collections
	*/
	static std::vector<RepoPermission> translatePrivileges(
		const std::vector<RepoPrivilege> &permissions);

	/**
	* Update the vector of actions with the appropriate action
	* gien the access information
	* @param collectionType the type of collection in question (e.g "history", "scene", "issues")
	* @param permission the access right for this collection type
	* @param vec the vector to update
	*/
	static void updateActions(
		const std::string &collectionType,
		const AccessRight &permission,
		std::vector<DBActions> &vec
		);

	/**
	* --------- Pretentious Edit functions -----------
	*/


	/**
	* Make a copy of this role and add the update access rights into
	* the new role
	* NOTE1: this function does NOT alter the existing RepoRole
	* NOTE2: if the role already has privilege on the project in question
	*       the privileges will be overwritten!
	* NOTE3: Access rights that used to exist but not specified in this
	*        list will be moved
	* @param permissions new permissions list
	*/
	RepoRole cloneAndUpdatePermissions(
		const std::vector<RepoPermission> &permissions
		);

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


	/**
	* Get the list of privileges as a map of database.collection, privileges
	* @return returns a map of privileges
	*/
	std::unordered_map<std::string, RepoPrivilege> getPrivilegesMapped() const
	{
		return getPrivilegesMapped(getPrivileges());
	}

	static std::unordered_map<std::string, RepoPrivilege> 
		getPrivilegesMapped(const std::vector<RepoPrivilege> &ps);


	/**
	* Get the list of project access rights as a vector
	* @return returns a vector of permissions
	*/
	std::vector<RepoPermission> getProjectAccessRights() const
	{
		return translatePrivileges(getPrivileges());
	}

private:

	/**
	* Given a RepoBSON that is an array of database actions,
	* return a vector of DBActions
	* @param actionArr a BSON that contains an array of actions
	* @return returns a vector of DBActions
	*/
	std::vector<DBActions> getActions(RepoBSON actionArr) const;


	/**
	* Make a copy of the role and alter privileges to the set provided
	* @param privileges new privileges 
	*/
	RepoRole cloneAndUpdatePrivileges(
		const std::vector<RepoPrivilege> &privileges
		);



};

}// end namespace model
} // end namespace core
} // end namespace repo


