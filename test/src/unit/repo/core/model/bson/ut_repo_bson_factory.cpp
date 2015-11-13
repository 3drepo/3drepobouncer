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
	std::string group = "repoGroup";
	std::string type = "Structural";
	std::string description = "testing project";
	std::vector<uint8_t> perm = { 7, 4, 0 };

	RepoProjectSettings settings = RepoBSONFactory::makeRepoProjectSettings(projectName, owner, group, type,
		description, perm[0], perm[1], perm[2]);

	EXPECT_EQ(projectName, settings.getProjectName());
	EXPECT_EQ(description, settings.getDescription());
	EXPECT_EQ(owner, settings.getOwner());
	EXPECT_EQ(group, settings.getGroup());
	std::vector<uint8_t> permOct = settings.getPermissionsOctal();

	std::vector<uint8_t> mask = { 4, 2, 1 };

	for (int i = 0; i < 3; ++i)
	{
		EXPECT_EQ(perm[i], permOct[i]);
	}

	EXPECT_EQ(type, settings.getType());

}

