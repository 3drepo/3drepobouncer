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

#include <cstdlib>

#include <gtest/gtest.h>

#include <repo/core/model/bson/repo_bson_element.h>
#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;


/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(RepoBSONElementTest, ConstructorTest)
{

	RepoBSONElement empty;

	EXPECT_TRUE(empty.eoo());

	auto mongoBson = BSON("stringField" << "String" << "IntField" << std::rand() << "DoubleField" << (double)std::rand() / 100. << "BoolField" << true);

	EXPECT_EQ(mongoBson.getField("stringField").String(), RepoBSONElement(mongoBson.getField("stringField")).String());
	EXPECT_EQ(mongoBson.getField("IntField").Int(), RepoBSONElement(mongoBson.getField("IntField")).Int());
	EXPECT_EQ(mongoBson.getField("DoubleField").Double(), RepoBSONElement(mongoBson.getField("DoubleField")).Double());
	EXPECT_EQ(mongoBson.getField("BoolField").Bool(), RepoBSONElement(mongoBson.getField("BoolField")).Bool());
}

TEST(RepoBSONElementTest, ConstructorTest)
{
	RepoBSONElement empty;

	EXPECT_EQ(ElementType::UNKNOWN);

	RepoBSONBuilder builder;
	builder << "string" << "stringField";
	builder << "string" << "stringField";
}