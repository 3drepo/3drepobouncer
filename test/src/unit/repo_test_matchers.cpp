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
#include <iomanip>
#include <ctime>

#include "repo_test_matchers.h"
#include "repo/core/model/bson/repo_bson.h"

using namespace repo::core::model;
using namespace testing;

void repo::core::model::PrintTo(const repo::core::model::RepoBSON& point, std::ostream* os)
{
	*os << point.toString();
}

bool operator== (tm a, tm b)
{
	return difftime(std::mktime(&a), std::mktime(&b)) == 0;
}

void operator<< (std::basic_ostream<char, std::char_traits<char>>& out, tm a)
{
	out << std::put_time(&a, "%d-%m-%Y %H-%M-%S");
}