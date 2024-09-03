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

#include "repo/manipulator/modelconvertor/import/odaHelper/helper_functions.h"
#include <cstdlib>
#include <repo/lib/datastructure/repo_metadataVariant.h>
#include <repo/lib/datastructure/repo_metadataVariantHelper.h>
#include <gtest/gtest.h>

#include "../../repo_test_utils.h"


using namespace repo::lib;

// Test of basic assignments and retreival.
TEST(RepoMetaVariantTest, AssignmentTest)
{
	MetadataVariant v0 = true;
	bool value0 = boost::get<bool>(v0);
	EXPECT_TRUE(value0);

	MetadataVariant v1 = 24;
	int value1 = boost::get<int>(v1);
	EXPECT_EQ(value1, 24);

	MetadataVariant v2 = 9223372036854775806;
	long long value2 = boost::get<long long>(v2);
	EXPECT_EQ(value2, 9223372036854775806);

	MetadataVariant v3 = 24.24;
	double value3 = boost::get<double>(v3);
	EXPECT_EQ(value3, 24.24);

	MetadataVariant v4 = std::string("3d Repo");
	std::string value4 = boost::get<std::string>(v4);
	EXPECT_EQ(value4, "3d Repo");

	tm tmPre;
	tmPre.tm_sec = 1;
	tmPre.tm_min = 2;
	tmPre.tm_hour = 3;
	tmPre.tm_mday = 4;
	tmPre.tm_mon = 5;
	tmPre.tm_year = 76;
	tmPre.tm_wday = 6;
	tmPre.tm_yday = 7;
	tmPre.tm_isdst = 1;
	MetadataVariant v5 = tmPre;
	tm value5 = boost::get<tm>(v5);
	EXPECT_EQ(value5.tm_sec, tmPre.tm_sec);
	EXPECT_EQ(value5.tm_min, tmPre.tm_min);
	EXPECT_EQ(value5.tm_hour, tmPre.tm_hour);
	EXPECT_EQ(value5.tm_mday, tmPre.tm_mday);
	EXPECT_EQ(value5.tm_mon, tmPre.tm_mon);
	EXPECT_EQ(value5.tm_year, tmPre.tm_year);
	EXPECT_EQ(value5.tm_wday, tmPre.tm_wday);
	EXPECT_EQ(value5.tm_yday, tmPre.tm_yday);
	EXPECT_EQ(value5.tm_isdst, tmPre.tm_isdst);
}

TEST(RepoMetaVariantTest, StringVisitor) {
	MetadataVariant v0 = true;
	std::string value0 = boost::apply_visitor(StringConversionVisitor(), v0);
	EXPECT_EQ(value0, "1");

	MetadataVariant v1 = 24;
	std::string value1 = boost::apply_visitor(StringConversionVisitor(), v1);
	EXPECT_EQ(value1, "24");

	MetadataVariant v2 = 9223372036854775806;
	std::string value2 = boost::apply_visitor(StringConversionVisitor(), v2);
	EXPECT_EQ(value2, "9223372036854775806");

	MetadataVariant v3 = 19.02;
	std::string value3 = boost::apply_visitor(StringConversionVisitor(), v3);
	EXPECT_EQ(value3, "19.020000");

	MetadataVariant v4 = std::string("3d Repo");
	std::string value4 = boost::apply_visitor(StringConversionVisitor(), v4);
	EXPECT_EQ(value4, "3d Repo");

	tm tmPre;
	tmPre.tm_sec = 1;
	tmPre.tm_min = 2;
	tmPre.tm_hour = 3;
	tmPre.tm_mday = 4;
	tmPre.tm_mon = 5;
	tmPre.tm_year = 76;
	tmPre.tm_wday = 6;
	tmPre.tm_yday = 7;
	tmPre.tm_isdst = 1;
	MetadataVariant v5 = tmPre;
	std::string value5 = boost::apply_visitor(StringConversionVisitor(), v5);
	EXPECT_EQ(value5, "04-06-1976 03-02-01");
}

TEST(RepoMetaVariantTest, CompareVisitor) {

	// Same native type, same value
	MetadataVariant v0a = true;
	MetadataVariant v0b = true;
	EXPECT_TRUE(boost::apply_visitor(DuplicationVisitor(), v0a, v0b));

	// Same native type, different value
	MetadataVariant v1a = true;
	MetadataVariant v1b = false;
	EXPECT_FALSE(boost::apply_visitor(DuplicationVisitor(), v1a, v1b));

	// Different native type
	MetadataVariant v2a = true;
	MetadataVariant v2b = 5;
	EXPECT_FALSE(boost::apply_visitor(DuplicationVisitor(), v2a, v2b));

	// Same standard class type, same value
	MetadataVariant v3a = std::string("Test");
	MetadataVariant v3b = std::string("Test");
	EXPECT_TRUE(boost::apply_visitor(DuplicationVisitor(), v3a, v3b));

	// Same standard class type, different value
	MetadataVariant v4a = std::string("Test");
	MetadataVariant v4b = std::string("Testing");
	EXPECT_FALSE(boost::apply_visitor(DuplicationVisitor(), v4a, v4b));

	// Same time type, same value
	tm tm5a;
	tm5a.tm_sec = 1;
	tm5a.tm_min = 2;
	tm5a.tm_hour = 3;
	tm5a.tm_mday = 4;
	tm5a.tm_mon = 5;
	tm5a.tm_year = 70;
	tm5a.tm_wday = 6;
	tm5a.tm_yday = 7;
	tm5a.tm_isdst = 1;
	tm tm5b = tm5a;
	MetadataVariant v5a = tm5a;
	MetadataVariant v5b = tm5b;
	EXPECT_TRUE(boost::apply_visitor(DuplicationVisitor(), v5a, v5b));

	// Same time type, different value
	tm tm6a;
	tm6a.tm_sec = 1;
	tm6a.tm_min = 2;
	tm6a.tm_hour = 3;
	tm6a.tm_mday = 4;
	tm6a.tm_mon = 5;
	tm6a.tm_year = 70;
	tm6a.tm_wday = 6;
	tm6a.tm_yday = 7;
	tm6a.tm_isdst = 1;
	tm tm6b;
	tm6b.tm_sec = 2;
	tm6b.tm_min = 3;
	tm6b.tm_hour = 4;
	tm6b.tm_mday = 5;
	tm6b.tm_mon = 6;
	tm6b.tm_year = 75;
	tm6b.tm_wday = 3;
	tm6b.tm_yday = 12;
	tm6b.tm_isdst = 0;
	MetadataVariant v6a = tm6a;
	MetadataVariant v6b = tm6b;

	EXPECT_FALSE(boost::apply_visitor(DuplicationVisitor(), v6a, v6b));
}