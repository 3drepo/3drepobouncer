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

#include "repo_bson_builder.h"
#include "repo_bson_project_settings.h"
#include "../../../lib/repo_log.h"

using namespace repo::core::model;

RepoProjectSettings RepoProjectSettings::cloneAndClearStatus() const
{
	RepoBSONBuilder builder;
	builder.appendTimeStamp(REPO_PROJECT_SETTINGS_LABEL_TIMESTAMP);
	builder.append(REPO_PROJECT_SETTINGS_LABEL_STATUS, "ok");
	builder.appendElementsUnique(*this);
	return builder.obj();
}


RepoProjectSettings RepoProjectSettings::cloneAndAddErrorStatus() const
{
	RepoBSONBuilder builder;
	builder.append(REPO_PROJECT_SETTINGS_LABEL_STATUS, "error");
	builder.appendElementsUnique(*this);
	return builder.obj();
}

RepoProjectSettings RepoProjectSettings::cloneAndMergeProjectSettings
(const RepoProjectSettings &proj) const
{
	RepoBSONBuilder newProjBuilder, propertiesBuilder;

	auto currentProperties = getObjectField(REPO_LABEL_PROPERTIES);
	propertiesBuilder.appendElements(proj.getObjectField(REPO_LABEL_PROPERTIES));

	currentProperties = currentProperties.removeField(REPO_LABEL_PIN_SIZE);
	currentProperties = currentProperties.removeField(REPO_LABEL_AVATAR_HEIGHT);
	currentProperties = currentProperties.removeField(REPO_LABEL_VISIBILITY_LIMIT);
	currentProperties = currentProperties.removeField(REPO_LABEL_SPEED);
	currentProperties = currentProperties.removeField(REPO_LABEL_ZNEAR);
	currentProperties = currentProperties.removeField(REPO_LABEL_ZFAR);

	propertiesBuilder.appendElementsUnique(currentProperties);
	newProjBuilder.append(REPO_LABEL_PROPERTIES, propertiesBuilder.obj());

	newProjBuilder.appendElementsUnique(proj);

	auto currentProjectSettings = removeField(REPO_LABEL_OWNER);
	currentProjectSettings = currentProjectSettings.removeField(REPO_LABEL_TYPE);
	currentProjectSettings = currentProjectSettings.removeField(REPO_LABEL_DESCRIPTION);
	newProjBuilder.appendElementsUnique(currentProjectSettings);

	return newProjBuilder.obj();
}