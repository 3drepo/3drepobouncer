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

#include <cstdlib>

#include <gtest/gtest.h>

#include <repo/core/model/bson/repo_bson_factory.h>


using namespace repo::core::model;


TEST(RepoBSONTest, MakeRepoProjectSettingsTest)
{
	std::string projectName = "project";
	std::string owner = "repo";
	std::string type = "Structural";
	std::string description = "testing project";
    // TODO: add test values for pinSize, avatarHeight, visibilityLimit, speed, zNear, zFar

    RepoProjectSettings settings = RepoBSONFactory::makeRepoProjectSettings(projectName, owner, type,
        description);

	EXPECT_EQ(projectName, settings.getProjectName());
	EXPECT_EQ(description, settings.getDescription());
	EXPECT_EQ(owner, settings.getOwner());
	EXPECT_EQ(type, settings.getType());
}

TEST(RepoBSONTest, MakeRepoRoleTest)
{
	std::string roleName = "repoRole";
	std::string databaseName = "admin";
	std::vector<RepoPermission> permissions;

	std::string proDB1 = "database";
	std::string proName1 = "project1";
	AccessRight proAccess1 = AccessRight::READ;

	std::string proDB2 = "databaseb";
	std::string proName2 = "project2";
	AccessRight proAccess2 = AccessRight::WRITE;

	std::string proDB3 = "databasec";
	std::string proName3 = "project3";
	AccessRight proAccess3 = AccessRight::READ_WRITE;

	permissions.push_back({ proDB1, proName1, proAccess1 });
	permissions.push_back({ proDB2, proName2, proAccess2 });
	permissions.push_back({ proDB3, proName3, proAccess3 });

	RepoRole role = RepoBSONFactory::makeRepoRole(roleName, databaseName, permissions);

	EXPECT_EQ(databaseName, role.getDatabase());
	EXPECT_EQ(roleName, role.getName());
	EXPECT_EQ(0, role.getInheritedRoles().size()); 

	std::vector<RepoPermission> accessRights = role.getProjectAccessRights();

	ASSERT_EQ(permissions.size(), accessRights.size());

	for (int i = 0; i < accessRights.size(); ++i)
	{
		//Order is not guaranteed, this is a long winded way to find the same permission again
		//but it should be fine as it's only 3 members
		bool found = false; 
		for (int j = 0; j < permissions.size(); ++j)
		{
			found |=  permissions[j].database == accessRights[i].database
						&& permissions[j].project == accessRights[i].project
						&& permissions[j].permission == accessRights[i].permission;
		}
		EXPECT_TRUE(found);
	}
}

TEST(RepoBSONTest, MakeRepoRoleTest2)
{
	std::string roleName = "repoRole";
	std::string databaseName = "admin";
	std::vector<RepoPrivilege> privileges;
	std::vector <std::pair<std::string, std::string>> inheritedRoles;

	privileges.push_back({ "db1", "col1", {DBActions::FIND} });
	privileges.push_back({ "db2", "col2", { DBActions::INSERT, DBActions::CREATE_USER } });
	privileges.push_back({ "db1", "col2", { DBActions::FIND, DBActions::DROP_ROLE } });

	inheritedRoles.push_back(std::pair < std::string, std::string > {"orange", "superUser"});

	RepoRole role = RepoBSONFactory::_makeRepoRole(roleName, databaseName, privileges, inheritedRoles);


	EXPECT_EQ(databaseName, role.getDatabase());
	EXPECT_EQ(roleName, role.getName());
	
	auto inheritedRolesOut = role.getInheritedRoles();

	ASSERT_EQ(inheritedRoles.size(), inheritedRolesOut.size());

	for (int i = 0; i < inheritedRolesOut.size(); ++i)
	{
		EXPECT_EQ(inheritedRolesOut[i].first, inheritedRoles[i].first);
		EXPECT_EQ(inheritedRolesOut[i].second, inheritedRoles[i].second);
	}


	auto privOut = role.getPrivileges();

	ASSERT_EQ(privOut.size(), privileges.size());
	for (int i = 0; i < privOut.size(); ++i)
	{
		EXPECT_EQ(privOut[i].database, privileges[i].database);
		EXPECT_EQ(privOut[i].collection, privileges[i].collection);
		EXPECT_EQ(privOut[i].actions.size(), privileges[i].actions.size());
	}
}

TEST(RepoBSONTest, MakeRepoUserTest)
{
	std::string username = "user";
	std::string password = "password";
	std::string firstName = "firstname";
	std::string lastName = "lastName";
	std::string email = "email";

	std::list<std::pair<std::string, std::string>>   projects;
	std::list<std::pair<std::string, std::string>>   roles;
	std::list<std::pair<std::string, std::string>>   groups;
	std::list<std::pair<std::string, std::string>>   apiKeys;
	std::vector<char>                                avatar;


	projects.push_back(std::pair<std::string, std::string >("database", "projectName"));
	roles.push_back(std::pair<std::string, std::string >("database", "roleName"));
	groups.push_back(std::pair<std::string, std::string >("database", "groupName"));
	apiKeys.push_back(std::pair<std::string, std::string >("database", "apiKey"));
	avatar.resize(10);

	RepoUser user = RepoBSONFactory::makeRepoUser(username, password, firstName, lastName, email, projects, roles, groups, apiKeys, avatar);

	EXPECT_EQ(username, user.getUserName());
	EXPECT_EQ(firstName, user.getFirstName());
	EXPECT_EQ(lastName, user.getLastName());
	EXPECT_EQ(email, user.getEmail());

	auto proOut = user.getProjectsList();
	auto rolesOut = user.getRolesList();
	auto groupsOut = user.getGroupsList();
	auto apiOut = user.getAPIKeysList();
	auto avatarOut = user.getAvatarAsRawData();


	EXPECT_EQ(projects.size(), proOut.size());
	EXPECT_EQ(roles.size(), rolesOut.size());
	EXPECT_EQ(groups.size(), groupsOut.size());
	EXPECT_EQ(apiKeys.size(), apiOut.size());
	EXPECT_EQ(avatar.size(), avatarOut.size());

	EXPECT_EQ(proOut.begin()->first, projects.begin()->first);
	EXPECT_EQ(proOut.begin()->second, projects.begin()->second);

	EXPECT_EQ(rolesOut.begin()->first, roles.begin()->first);
	EXPECT_EQ(rolesOut.begin()->second, roles.begin()->second);

	EXPECT_EQ(groupsOut.begin()->first, groups.begin()->first);
	EXPECT_EQ(groupsOut.begin()->second, groups.begin()->second);

	EXPECT_EQ(apiOut.begin()->first, apiKeys.begin()->first);
	EXPECT_EQ(apiOut.begin()->second, apiKeys.begin()->second);

	EXPECT_EQ(std::string(avatar.data()), std::string(avatarOut.data()));
}

TEST(RepoBSONTest, AppendDefaultsTest)
{
	RepoBSONBuilder builder;

	RepoBSONFactory::appendDefaults(
		builder, "test");

	RepoNode n = builder.obj();

	EXPECT_FALSE(n.isEmpty());

	EXPECT_EQ(4, n.nFields()); 

	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_ID));
	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_SHARED_ID));
	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_API));
	EXPECT_TRUE(n.hasField(REPO_NODE_LABEL_TYPE));

	//Ensure existing fields doesnt' disappear

	RepoBSONBuilder builderWithFields;

	builderWithFields << "Number" << 1023;
	builderWithFields << "doll" << "Kitty";

	RepoBSONFactory::appendDefaults(
		builderWithFields, "test");

	RepoNode nWithExists = builderWithFields.obj();
	EXPECT_FALSE(nWithExists.isEmpty());

	EXPECT_EQ(6, nWithExists.nFields());

	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_ID));
	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_SHARED_ID));
	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_API));
	EXPECT_TRUE(nWithExists.hasField(REPO_NODE_LABEL_TYPE));

	EXPECT_TRUE(nWithExists.hasField("doll"));
	EXPECT_EQ("Kitty", std::string(nWithExists.getStringField("doll")));
	EXPECT_TRUE(nWithExists.hasField("Number"));
	EXPECT_EQ(nWithExists.getField("Number").Int(), 1023);

}
