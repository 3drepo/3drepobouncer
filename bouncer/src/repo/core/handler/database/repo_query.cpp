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

#include "repo_query.h"
#include "repo/core/model/bson/repo_bson.h"
#include "repo/core/model/bson/repo_bson_builder.h"

using namespace repo::core::handler::database;
using namespace repo::core::model;

index::Ascending::Ascending(std::vector<std::string> fields)
	:fields(fields)
{
}

index::Ascending::operator repo::core::model::RepoBSON() const
{
	RepoBSONBuilder builder;
	for (auto& f : fields)
	{
		builder.append(f, 1);
	}
	return builder.obj();
}

index::Descending::Descending(std::vector<std::string> fields)
	:fields(fields)
{
}

index::Descending::operator repo::core::model::RepoBSON() const
{
	RepoBSONBuilder builder;
	for (auto& f : fields)
	{
		builder.append(f, -1);
	}
	return builder.obj();
}