/**
*  Copyright (C) 2016 3D Repo Ltd
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

#include <repo/lib/repo_time_zone_converter.h>
#include <gtest/gtest.h>
#include <iostream>

TEST(RepoTimeZoneConverter, ZoneCheck)
{
	auto tzc1 = repo::lib::TimeZoneConverter("Asia/Tokyo");
	EXPECT_EQ(1641523367, tzc1.shiftToTimezone(1641555767));
	auto tzc2 = repo::lib::TimeZoneConverter("America/New_York");
	EXPECT_EQ(1642076100, tzc2.shiftToTimezone(1642058100));
	auto tzc3 = repo::lib::TimeZoneConverter("Europe/London");
	EXPECT_EQ(1652455409, tzc3.shiftToTimezone(1652459009)); // during dst
	EXPECT_EQ(1642091009, tzc3.shiftToTimezone(1642091009)); // outside dst
}

TEST(RepoTimeZoneConverter, UnknownTimezone)
{
	auto tzc1 = repo::lib::TimeZoneConverter("Undsflkdjsf");
	EXPECT_EQ(1641555767, tzc1.shiftToTimezone(1641555767));
	auto tzc2 = repo::lib::TimeZoneConverter("");
	EXPECT_EQ(1641555767, tzc1.shiftToTimezone(1641555767));
}