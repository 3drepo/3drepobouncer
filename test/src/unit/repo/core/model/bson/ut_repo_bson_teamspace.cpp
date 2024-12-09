/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include "repo/core/model/bson/repo_bson_teamspace.h"
#include "repo/core/model/bson/repo_bson_builder.h"
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::core::model;
using namespace testing;

TEST(RepoTeamspaceTest, Deserialise)
{
	{
		RepoBSONBuilder builder;
		RepoBSONBuilder addOnsBuilder;
		builder.append("addOns", addOnsBuilder.obj());

		RepoTeamspace teamspace(builder.obj());

		EXPECT_THAT(teamspace.isAddOnEnabled(REPO_USER_LABEL_VR_ENABLED), IsFalse());
		EXPECT_THAT(teamspace.isAddOnEnabled(REPO_USER_LABEL_SRC_ENABLED), IsFalse());
	}

	{
		RepoBSONBuilder builder;
		RepoBSONBuilder addOnsBuilder;
		addOnsBuilder.append("srcEnabled", false);
		addOnsBuilder.append("vrEnabled", false);
		builder.append("addOns", addOnsBuilder.obj());

		RepoTeamspace teamspace(builder.obj());

		EXPECT_THAT(teamspace.isAddOnEnabled(REPO_USER_LABEL_SRC_ENABLED), IsFalse());
		EXPECT_THAT(teamspace.isAddOnEnabled(REPO_USER_LABEL_VR_ENABLED), IsFalse());
	}

	{
		RepoBSONBuilder builder;
		RepoBSONBuilder addOnsBuilder;
		addOnsBuilder.append("srcEnabled", false);
		addOnsBuilder.append("vrEnabled", true);
		builder.append("addOns", addOnsBuilder.obj());

		RepoTeamspace teamspace(builder.obj());

		EXPECT_THAT(teamspace.isAddOnEnabled(REPO_USER_LABEL_SRC_ENABLED), IsFalse());
		EXPECT_THAT(teamspace.isAddOnEnabled(REPO_USER_LABEL_VR_ENABLED), IsTrue());
	}

	{
		RepoBSONBuilder builder;
		RepoBSONBuilder addOnsBuilder;
		addOnsBuilder.append("srcEnabled", true);
		addOnsBuilder.append("vrEnabled", true);
		builder.append("addOns", addOnsBuilder.obj());

		RepoTeamspace teamspace(builder.obj());

		EXPECT_THAT(teamspace.isAddOnEnabled(REPO_USER_LABEL_SRC_ENABLED), IsTrue());
		EXPECT_THAT(teamspace.isAddOnEnabled(REPO_USER_LABEL_VR_ENABLED), IsTrue());
	}
}