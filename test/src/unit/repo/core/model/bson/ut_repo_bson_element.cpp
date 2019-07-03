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

	EXPECT_TRUE(empty.isNull());

	auto mongoBson = BSON("stringField" << "String" << "IntField" << std::rand() << "DoubleField" << (double)std::rand() / 100. << "BoolField" << true);

	EXPECT_EQ(mongoBson.getField("stringField").String(), RepoBSONElement(mongoBson.getField("stringField")).String());
	EXPECT_EQ(mongoBson.getField("IntField").Int(), RepoBSONElement(mongoBson.getField("IntField")).Int());
	EXPECT_EQ(mongoBson.getField("DoubleField").Double(), RepoBSONElement(mongoBson.getField("DoubleField")).Double());
	EXPECT_EQ(mongoBson.getBoolField("BoolField"), RepoBSONElement(mongoBson.getField("BoolField")).Bool());
}

TEST(RepoBSONElementTest, TypeTest)
{
	RepoBSONElement empty;
	EXPECT_EQ(ElementType::UNKNOWN, empty.type());

	RepoBSON bson = BSON("stringField" << "String" << "IntField" << std::rand() << "DoubleField" << (double)std::rand() / 100. << "BoolField" << true);

	EXPECT_EQ(ElementType::STRING, bson.getField("stringField").type());
	EXPECT_EQ(ElementType::INT, bson.getField("IntField").type());
	EXPECT_EQ(ElementType::DOUBLE, bson.getField("DoubleField").type());
	EXPECT_EQ(ElementType::BOOL, bson.getField("BoolField").type());
}

TEST(RepoBSONElementTest, ArrayTest)
{
	RepoBSONElement empty;

	auto elementArrEmpty = empty.Array();

	EXPECT_EQ(0, elementArrEmpty.size());

	std::vector<int> intVect = { 1, 2, 3, 4, 5, 6, 7, 8 };

	RepoBSONBuilder builder;
	builder.appendArray("intArr", intVect);

	auto bson = builder.obj();
	auto intEleArr = bson.getField("intArr").Array();
	ASSERT_EQ(intEleArr.size(), intVect.size());
	for (int i = 0; i < intEleArr.size(); ++i)
	{
		EXPECT_EQ(ElementType::INT, intEleArr[i].type());
		EXPECT_EQ(intEleArr[i].Int(), intVect[i]);
	}
}