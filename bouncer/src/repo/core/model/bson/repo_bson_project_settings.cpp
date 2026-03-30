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
*  Project setting BSON
*/

#include "repo_bson_project_settings.h"
#include "repo_bson_builder.h"
#include <repo_log.h>

using namespace repo::core::model;
using namespace repo::lib;

const std::string STATUS_OK = "ok";
const std::string STATUS_ERROR = "error";

RepoProjectSettings::RepoProjectSettings(RepoBSON bson)
{
	id = bson.getStringField(REPO_LABEL_ID);
	status = STATUS_OK;
	if (bson.hasField(REPO_PROJECT_SETTINGS_LABEL_STATUS))
	{
		status = bson.getStringField(REPO_PROJECT_SETTINGS_LABEL_STATUS);
	}
	units = ModelUnits::UNKNOWN;
	if (bson.hasField(REPO_PROJECT_SETTINGS_LABEL_PROPERTIES)) 
	{
		auto properties = bson.getObjectField(REPO_PROJECT_SETTINGS_LABEL_PROPERTIES);
		if (properties.hasField(REPO_PROJECT_SETTINGS_LABEL_UNITS)) 
		{
			units = units::fromString(properties.getStringField(REPO_PROJECT_SETTINGS_LABEL_UNITS));
		}
	}
}

void RepoProjectSettings::setErrorStatus()
{
	status = STATUS_ERROR;
}

void RepoProjectSettings::clearErrorStatus()
{
	status = STATUS_OK;
}

RepoProjectSettings::operator RepoBSON() const
{
	RepoBSONBuilder builder;
	builder.append(REPO_PROJECT_SETTINGS_LABEL_STATUS, status);
	builder.append(REPO_LABEL_ID, id);
	if (status == STATUS_OK) {
		builder.appendTimeStamp(REPO_PROJECT_SETTINGS_LABEL_TIMESTAMP);
	}
	return builder.obj();
}