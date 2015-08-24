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


RepoProjectSettings RepoProjectSettings::createRepoProjectSettings(
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
	return RepoProjectSettings(builder.obj());
}

RepoProjectSettings::~RepoProjectSettings()
{
}

std::vector<bool> RepoProjectSettings::getPermissionsBoolean() const
{

	std::vector<bool> perm;
	perm.resize(12);

	std::vector<uint8_t> permInt = getPermissionsOctal();
	uint8_t mask[3] = { READVAL, WRITEVAL, EXECUTEVAL };


	perm[0] = perm[1] = perm[2] = false; //uid/gid/sticky all false.

	for (uint32_t i = 0; i < 3; i++)
	{
		for (uint32_t j = 0; j < 3; j++)
		{
			uint32_t ind = (i+1) * 3 + j;
			perm[ind] = permInt[i] & mask[j];
		}

		BOOST_LOG_TRIVIAL(trace) << "POSIX value : " << permInt[i] << "Permission = " 
			<< perm[(i + 1) * 3] << ", " << perm[(i + 1) * 3 + 1] << "," << perm[(i + 1) * 3 + 2];
	}



	return perm;
}

std::string RepoProjectSettings::getPermissionsString() const
{
	std::stringstream sstream;
	std::vector<uint8_t> permVec = getPermissionsOctal();

	std::string mapping[8] = {"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};

	if (permVec.size() >= 3)
	{
		//only take the first 3 values as the rest wouldn't make sense anyway
		for (int i = 0; i < 3; i++)
		{
			int value = permVec[i];

			if (value >= 0 && value <= 7)
			{
				//ensure the value is in range to prevent index out of range
				sstream << mapping[value];
			}
			else
			{
				BOOST_LOG_TRIVIAL(error) << "Invalid POSIX permission value : " << value;
				sstream << "eee"; //let's use "eee" to denote error.
			}
		}
	}

	return sstream.str();
}


std::vector<uint8_t> RepoProjectSettings::getPermissionsOctal() const
{

	std::vector<uint8_t> perm;
	if (hasField(REPO_LABEL_PERMISSIONS))
	{
		std::vector<RepoBSONElement> arr = getField(REPO_LABEL_PERMISSIONS).Array();
		for (const auto &ele : arr)
			perm.push_back(ele.Int());
	}

	return perm;
}

std::vector<bool> RepoProjectSettings::stringToPermissionsBool(std::string octal)
{
	std::vector<bool> perm;
	perm.resize(12);
	uint8_t mask[3] = { READVAL, WRITEVAL, EXECUTEVAL };

	//FIXME: a lot of repetition with getPermissionBoolean().
	if (octal.size() >= 4)
	{
		for (int i = 0; i < 4; i++)
		{
			uint32_t p = std::stoi(octal.substr(i, 1));

			for (int j = 0; j < 3; j++)
			{
				uint32_t ind = i * 3 + j;
				perm[ind] = p & mask[j];
			}
				
		}
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "ProjectSettings - stringToPermissionsBool : size of parameter is smaller than 4.";
	}

	return perm;
}

std::vector<std::string> RepoProjectSettings::getUsers() const
{
	std::vector<std::string> users;
	if (hasField(REPO_LABEL_USERS))
	{
		std::vector<RepoBSONElement> arr = getField(REPO_LABEL_USERS).Array();
		users.reserve(arr.size());
		for (const auto &ele : arr)
			users.push_back(ele.String());
	}
	return users;
}