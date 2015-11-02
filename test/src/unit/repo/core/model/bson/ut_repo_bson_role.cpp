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

#include <repo/core/model/collection/repo_scene.h>
#include <repo/core/model/bson/repo_bson_role.h>
#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;

static RepoBSON buildRoleExample()
{
	RepoBSONBuilder builder;
	std::string database = "admin";
	std::string roleName = "testRole";
	builder << REPO_LABEL_ID << database + "." + roleName;
	builder << REPO_ROLE_LABEL_ROLE << roleName;
	builder << REPO_ROLE_LABEL_DATABASE << database;

	//====== Add Privileges ========
	RepoBSONBuilder privilegesBuilder;

	RepoBSONBuilder innerBsonBuilder, actionBuilder;
	RepoBSON resource = BSON(REPO_ROLE_LABEL_DATABASE << "testdb" << REPO_ROLE_LABEL_COLLECTION << "testCol");
	innerBsonBuilder << REPO_ROLE_LABEL_RESOURCE << resource;

	std::vector<std::string> actions = { "find", "update" };

	for (size_t aCount = 0; aCount < actions.size(); ++aCount)
	{
		actionBuilder << std::to_string(aCount) << actions[aCount];
	}

	innerBsonBuilder.appendArray(REPO_ROLE_LABEL_ACTIONS, actionBuilder.obj());

	privilegesBuilder << "0" << innerBsonBuilder.obj();

	builder.appendArray(REPO_ROLE_LABEL_PRIVILEGES, privilegesBuilder.obj());


	//====== Add Inherited Roles ========

	RepoBSON inheritedRole = BSON( "role" << "readWrite" << "db" << "canarywharf");
	RepoBSONBuilder inheritedRolesBuilder;

	inheritedRolesBuilder << "0" << inheritedRole;
	builder.appendArray(REPO_ROLE_LABEL_INHERITED_ROLES, inheritedRolesBuilder.obj());
	return builder.obj();
}

TEST(RepoRoleTest, ConstructorTest)
{
	RepoRole role;

	EXPECT_TRUE(role.isEmpty());

	RepoRole role2(buildRoleExample());
	EXPECT_FALSE(role2.isEmpty());
}

TEST(RepoRoleTest, DBActionToStringTest)
{
	EXPECT_EQ("insert", RepoRole::dbActionToString(DBActions::INSERT));
	EXPECT_EQ("update", RepoRole::dbActionToString(DBActions::UPDATE));
	EXPECT_EQ("remove", RepoRole::dbActionToString(DBActions::REMOVE));
	EXPECT_EQ("find", RepoRole::dbActionToString(DBActions::FIND));
	EXPECT_EQ("createUser", RepoRole::dbActionToString(DBActions::CREATE_USER));
	EXPECT_EQ("createRole", RepoRole::dbActionToString(DBActions::CREATE_ROLE));
	EXPECT_EQ("dropRole", RepoRole::dbActionToString(DBActions::DROP_ROLE));
	EXPECT_EQ("grantRole", RepoRole::dbActionToString(DBActions::GRANT_ROLE));
	EXPECT_EQ("revokeRole", RepoRole::dbActionToString(DBActions::REVOKE_ROLE));
	EXPECT_EQ("viewRole", RepoRole::dbActionToString(DBActions::VIEW_ROLE));
	EXPECT_EQ("", RepoRole::dbActionToString(DBActions::UNKNOWN));
	EXPECT_EQ("", RepoRole::dbActionToString((DBActions)100600));

	//This is a bit hacky, but we want to ensure that every enum is catered for.
	//so loop through until we hit UNKNWON and make sure every enum returns a valid string

	uint32_t i = 0;
	for (; i < (uint32_t)DBActions::UNKNOWN-1; ++i)
	{
		EXPECT_NE("", RepoRole::dbActionToString((DBActions)i));
	}

	//Also ensure UNKNOWN is the last member in the enum class
	i += 2;
	EXPECT_EQ("", RepoRole::dbActionToString((DBActions)i));
}

TEST(RepoRoleTest, TranslatePermissionsTest)
{
	std::vector<RepoPermission> permissions;

	permissions.push_back({"test", "project", AccessRight::READ});

	std::vector<RepoPrivilege> privileges = RepoRole::translatePermissions(permissions);

	EXPECT_EQ(RepoScene::getProjectExtensions().size(), privileges.size());


	EXPECT_EQ(0, RepoRole::translatePermissions(std::vector<RepoPermission>()).size());
}