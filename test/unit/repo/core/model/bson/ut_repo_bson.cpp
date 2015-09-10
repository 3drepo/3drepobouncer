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

#include <gtest/gtest.h>

#include <repo/core/model/bson/repo_bson.h>

using namespace repo::core::model;

static const RepoBSON testBson = RepoBSON(BSON("ice" << "lolly" << "amount" << 100));

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(RepoBSONTest, ConstructFromMongo)
{
	mongo::BSONObj mongoObj = BSON("ice" << "lolly" << "amount" << 100);
	mongo::BSONObjBuilder builder;
	builder << "ice" << "lolly";
	builder << "amount" << 100;

	RepoBSON bson1(mongoObj);
	RepoBSON bson2(builder);
	RepoBSON bsonDiff(BSON("something" << "different"));

	EXPECT_EQ(bson1, bson2);
	EXPECT_EQ(bson1.toString(), bson2.toString());
	EXPECT_NE(bson1, bsonDiff);

}

TEST(RepoBSONTest, GetField)
{
	EXPECT_EQ(testBson.getField("ice"), testBson.getField("ice"));
	EXPECT_NE(testBson.getField("ice"), testBson.getField("amount"));

	EXPECT_EQ("lolly", testBson.getField("ice").str());
	EXPECT_EQ(100, testBson.getField("amount").Int());
}

TEST(RepoBSONTest, GetBinaryAsVectorEmbedded)
{
	mongo::BSONObjBuilder builder;

	std::vector < uint8_t > in, out;

	size_t size = 100;

	for (size_t i = 0; i < size; ++i)
		in.push_back(i);

	builder << "stringTest" << "hello";
	builder << "numTest" << 1.35;
	builder.appendBinData("binDataTest", in.size(), mongo::BinDataGeneral, &in[0]);

	RepoBSON bson(builder);


	EXPECT_TRUE(bson.getBinaryFieldAsVector(bson.getField("binDataTest"), in.size(), &out));

	ASSERT_EQ(out.size(), in.size());
	for (size_t i = 0; i < size; ++i)
	{
		EXPECT_EQ(in[i], out[i]);
	}


	std::vector<char> *null = nullptr;

	//Invalid retrieval, but they shouldn't throw exception
	EXPECT_FALSE(bson.getBinaryFieldAsVector(bson.getField("binDataTest"), in.size(), null));
	EXPECT_FALSE(bson.getBinaryFieldAsVector(bson.getField("numTest"), &out));
	EXPECT_FALSE(bson.getBinaryFieldAsVector(bson.getField("stringTest"), &out));
	EXPECT_FALSE(bson.getBinaryFieldAsVector(bson.getField("doesn'tExist"), &out));
}

TEST(RepoBSONTest, GetBinaryAsVectorReferenced)
{
	mongo::BSONObjBuilder builder;

	std::vector < uint8_t > in, out;

	size_t size = 100;

	for (size_t i = 0; i < size; ++i)
		in.push_back(i);

	std::unordered_map<std::string, std::vector<uint8_t>> map;
	std::string fname = "testingfile";
	map[fname] = in;



	RepoBSON bson(BSON("binDataTest" << fname), map);


	bson.getBinaryFieldAsVector(bson.getField("binDataTest"), in.size(), &out);

	ASSERT_EQ(out.size(), in.size());
	for (size_t i = 0; i < size; ++i)
	{
		EXPECT_EQ(in[i], out[i]);
	}

}