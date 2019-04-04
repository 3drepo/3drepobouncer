/**
*  Copyright (C) 2016 3D Repo Ltd
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
#include <repo/lib/repo_config.h>
#include <gtest/gtest.h>

using namespace repo::lib;

TEST(RepoConfigTest, constructorTest)
{
	std::string db = "database", user = "username", password = "password";
	int port = 10000;
	bool pwDigested = true;
	RepoConfig config = {db, port, user, password, pwDigested };
	auto dbData = config.getDatabaseConfig();
	EXPECT_EQ(dbData.addr, db);
	EXPECT_EQ(dbData.port, port);
	EXPECT_EQ(dbData.username, user);
	EXPECT_EQ(dbData.password, password);
	EXPECT_EQ(dbData.pwDigested, pwDigested);
}
