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

#include "repo_bson_teamspace.h"
#include "repo/core/model/bson/repo_bson.h"
#include "repo/lib/repo_log.h"

using namespace repo::core::model;

#define REPO_TEAMSPACE_LEVEL_ADDONS "addOns"

RepoTeamspace::RepoTeamspace(RepoBSON bson)
{
	if (bson.hasField(REPO_TEAMSPACE_LEVEL_ADDONS))
	{
		auto addons = bson.getObjectField(REPO_TEAMSPACE_LEVEL_ADDONS);
		for (const auto& name : addons.getFieldNames())
		{
			auto addon = addons.getField(name);
			if (addon.type() == repo::core::model::ElementType::BOOL) {
				if (addon.Bool()) {
					addOns.insert(name);
				}
			}
		}
	}
}

bool RepoTeamspace::isAddOnEnabled(const std::string& addOnName)
{
	return addOns.find(addOnName) != addOns.end();
}