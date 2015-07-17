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


using namespace repo::core::model::bson;

RepoProjectSettings::RepoProjectSettings() : RepoBSON()
{
}


RepoProjectSettings* RepoProjectSettings::createRepoProjectSettings(
	const std::string &uniqueProjectName,
	const std::string &owner,
	const std::string &group,
	const std::string &type,
	const std::string &description,
	const uint8_t     &ownerPermissionsOctal,
	const uint8_t     &groupPermissionsOctal,
	const uint8_t     &publicPermissionsOctal)
{
	RepoBSONBuilder builder;

	//--------------------------------------------------------------------------
	// Project name
	if (!uniqueProjectName.empty())
		builder << REPO_LABEL_ID << uniqueProjectName;

	//--------------------------------------------------------------------------
	// Owner
	if (!owner.empty())
		builder << REPO_LABEL_OWNER << owner;

	//--------------------------------------------------------------------------
	// Description
	if (!description.empty())
		builder << REPO_LABEL_DESCRIPTION << description;

	//--------------------------------------------------------------------------
	// Type
	if (!type.empty())
		builder << REPO_LABEL_TYPE << type;

	//--------------------------------------------------------------------------
	// Group
	if (!group.empty())
		builder << REPO_LABEL_GROUP << group;

	//--------------------------------------------------------------------------
	// Permissions
	mongo::BSONArrayBuilder arrayBuilder;
	arrayBuilder << ownerPermissionsOctal;
	arrayBuilder << groupPermissionsOctal;
	arrayBuilder << publicPermissionsOctal;
	builder << REPO_LABEL_PERMISSIONS << arrayBuilder.arr();

	//--------------------------------------------------------------------------
	// Add to the parent object
	return new RepoProjectSettings(builder.obj());
}

RepoProjectSettings::~RepoProjectSettings()
{
}
