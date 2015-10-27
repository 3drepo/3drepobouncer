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


#include "repo_bson_role.h"

using namespace repo::core::model;

RepoRole::RepoRole()
{
}


RepoRole::~RepoRole()
{
}


std::string RepoRole::dbActionToString(const DBActions &action)
{
	
	switch (action)
	{
	case DBActions::INSERT:
		return "insert";
	case DBActions::UPDATE:
		return "update";
	case DBActions::FIND:
		return "find";
	case DBActions::CREATE_USER:
		return "createUser";
	case DBActions::CREATE_ROLE:
		return "createRole";
	case DBActions::GRANT_ROLE:
		return "grantRole";
	case DBActions::REVOKE_ROLE:
		return "revokeRole";
	case DBActions::VIEW_ROLE:
		return "viewRole";
	default:
		repoError << "Unrecognised action value: " << (uint32_t)action;
	}

	return "";
}