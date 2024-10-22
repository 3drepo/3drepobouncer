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

#include "repo/core/model/bson/repo_bson_project_settings.h"
#include "repo/core/model/bson/repo_bson_builder.h"
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::core::model;
using namespace testing;

// The following tests are written expecting a schema used with upsert and overwrite
// false; that is, the id is expected to be set to identify the document, and only
// the fields that are to be overwritten should be set.

TEST(RepoProjectSettingsTest, Deserialise)
{
	std::string projectId = "project"; // Project Settings Ids are in string format, even though nowadays they are all UUIDs

	{
		RepoBSONBuilder builder;
		builder.append(REPO_LABEL_ID, projectId);
		RepoProjectSettings settings(builder.obj());
		EXPECT_THAT(settings.getProjectId(), Eq(projectId));
	}

	{
		RepoBSONBuilder builder;
		builder.append(REPO_LABEL_ID, projectId);
		RepoProjectSettings settings(builder.obj());
		EXPECT_EQ(settings.getStatus(), "ok");
	}

	{
		RepoBSONBuilder builder;
		builder.append(REPO_LABEL_ID, projectId);
		builder.append(REPO_PROJECT_SETTINGS_LABEL_STATUS, "ok");
		RepoProjectSettings settings(builder.obj());
		EXPECT_EQ(settings.getStatus(), "ok");
	}

	{
		RepoBSONBuilder builder;
		builder.append(REPO_LABEL_ID, projectId);
		builder.append(REPO_PROJECT_SETTINGS_LABEL_STATUS, "error");
		RepoProjectSettings settings(builder.obj());
		EXPECT_EQ(settings.getStatus(), "error");
	}
}

TEST(RepoProjectSettingsTest, Serialise)
{
	auto id = repo::lib::RepoUUID::createUUID().toString();
	RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, id);
	RepoProjectSettings settings(builder.obj());

	// Re-serialising with status OK should update the timestamp

	EXPECT_THAT(((RepoBSON)settings).getStringField(REPO_LABEL_ID), Eq(id));
	EXPECT_THAT(((RepoBSON)settings).getStringField(REPO_PROJECT_SETTINGS_LABEL_STATUS), Eq("ok"));
	EXPECT_THAT(((RepoBSON)settings).getTimeStampField(REPO_PROJECT_SETTINGS_LABEL_TIMESTAMP), IsNow());

	// Serialising with error set should not change the timestamp

	settings.setErrorStatus();

	EXPECT_THAT(((RepoBSON)settings).getStringField(REPO_LABEL_ID), Eq(id));
	EXPECT_THAT(((RepoBSON)settings).getStringField(REPO_PROJECT_SETTINGS_LABEL_STATUS), Eq("error"));
	EXPECT_THAT(((RepoBSON)settings).hasField(REPO_PROJECT_SETTINGS_LABEL_TIMESTAMP), IsFalse());

	// Clearing the status should go back to as if it were deserialised with OK

	settings.clearErrorStatus();

	EXPECT_THAT(((RepoBSON)settings).getStringField(REPO_LABEL_ID), Eq(id));
	EXPECT_THAT(((RepoBSON)settings).getStringField(REPO_PROJECT_SETTINGS_LABEL_STATUS), Eq("ok"));
	EXPECT_THAT(((RepoBSON)settings).getTimeStampField(REPO_PROJECT_SETTINGS_LABEL_TIMESTAMP), IsNow());
}