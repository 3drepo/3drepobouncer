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

#include <repo/lib/time_zone_converter.h>
#include <gtest/gtest.h>


TEST(RepoTimeZoneConverter, ZoneCheck)
{
	
	using namespace std::literals;
	// time zone is Asia/Tokyo (UTC+9)
	// a given timestamp in the sequence is 1641555767 (Friday, 7 January 2022 11:42:47 GMT+00:00)
	// the timestamp should be converted to 1641523367 (Friday, 7 January 2022 02:42:47 GMT+00:00)
	auto tzc1 = repo::lib::TimeZoneConverter("Asia/Tokyo");
	std::cout << "test result: " << (1641523367s == tzc1.timeZoneEpochToUtcEpoch(1641555767s) ? "true" : "false") << "\n";
	// 1642076100 converts to Thursday January 13, 2022 07:15 : 00 (am)in time zone America / New York(EST)
	// The offset(difference to Greenwich Time / GMT) is - 05 : 00 or in seconds - 18000.	
	//runTest(1642076100s, 1642058100s, tz2);
	auto tzc2 = repo::lib::TimeZoneConverter("America/New_York");
	std::cout << "test result: " << (1642076100s == tzc2.timeZoneEpochToUtcEpoch(1642058100s) ? "true" : "false") << "\n";
	

}

