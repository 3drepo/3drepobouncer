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

#define NOMINMAX

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include "repo_test_database_info.h"
#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;
using namespace testing;

 std::pair<std::pair<std::string, std::string>, repo::core::model::RepoBSON> getDataForDropCase()
{
	std::pair<std::pair<std::string, std::string>, repo::core::model::RepoBSON> result;
	result.first = { "sampleDataRW", "collectionToDrop" };
	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", repo::lib::RepoUUID("0ab45528-9258-421a-927c-c51bf40fc478"));
	result.second = builder.obj();

	return result;
}