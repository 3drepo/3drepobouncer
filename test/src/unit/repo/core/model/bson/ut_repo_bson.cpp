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

#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_builder.h>

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


	EXPECT_TRUE(bson.getBinaryFieldAsVector(bson.getField("binDataTest"), in.size(), &out));

	ASSERT_EQ(out.size(), in.size());
	for (size_t i = 0; i < size; ++i)
	{
		EXPECT_EQ(in[i], out[i]);
	}

}

TEST(RepoBSONTest, AssignOperator)
{
	RepoBSON test = testBson;

	EXPECT_TRUE(test.toString() == testBson.toString());

	EXPECT_EQ(test.getFilesMapping().size(), testBson.getFilesMapping().size());
	
	//Test with bigfile mapping
	std::vector < uint8_t > in;

	in.resize(100);

	std::unordered_map<std::string, std::vector<uint8_t>> map, mapout;
	map["testingfile"] = in;

	RepoBSON test2(testBson, map);


	mapout = test2.getFilesMapping();
	ASSERT_EQ(mapout.size(), map.size());
	
	auto mapIt = map.begin();
	auto mapoutIt = mapout.begin();

	for (; mapIt != map.end(); ++mapIt, ++mapoutIt)
	{
		EXPECT_EQ(mapIt->first, mapIt->first);
		std::vector<uint8_t> dataOut = mapoutIt->second;
		std::vector<uint8_t> dataIn = mapIt->second;
		EXPECT_EQ(dataOut.size(), dataIn.size());
		if (dataIn.size()>0)
			EXPECT_EQ(strncmp((char*)&dataOut[0], (char*)&dataIn[0], dataIn.size()), 0);
	}

}

TEST(RepoBSONTest, Swap)
{
	RepoBSON test = testBson;

	//Test with bigfile mapping
	std::vector < uint8_t > in;

	in.resize(100);

	std::unordered_map<std::string, std::vector<uint8_t>> map, mapout;
	map["testingfile"] = in;

	RepoBSON testDiff_org(BSON("entirely" << "different"), map);
	RepoBSON testDiff = testDiff_org;

	EXPECT_TRUE(testDiff_org.toString() == testDiff.toString());
	EXPECT_TRUE(testDiff_org.getFilesMapping().size() == testDiff.getFilesMapping().size());


	test.swap(testDiff);
	EXPECT_EQ(test.toString(), testDiff_org.toString());


	mapout = test.getFilesMapping();
	ASSERT_EQ(mapout.size(), map.size());

	auto mapIt = map.begin();
	auto mapoutIt = mapout.begin();

	for (; mapIt != map.end(); ++mapIt, ++mapoutIt)
	{
		EXPECT_EQ(mapIt->first, mapIt->first);
		std::vector<uint8_t> dataOut = mapoutIt->second;
		std::vector<uint8_t> dataIn = mapIt->second;
		EXPECT_EQ(dataOut.size(), dataIn.size());
		if (dataIn.size()>0)
			EXPECT_EQ(strncmp((char*)dataOut.data(), (char*)dataIn.data(), dataIn.size()), 0);
	}
}

TEST(RepoBSONTest, GetUUIDField)
{

	repoUUID uuid = generateUUID();
	mongo::BSONObjBuilder builder;
	builder.appendBinData("uuid", uuid.size(), mongo::bdtUUID, (char*)uuid.data);

	RepoBSON test = RepoBSON(builder.obj());
	EXPECT_EQ(test.getUUIDField("uuid"), uuid);

	//Shouldn't fail if trying to get a uuid field that doesn't exist
	EXPECT_NE(test.getUUIDField("hello"), uuid);
	EXPECT_NE(testBson.getUUIDField("ice"), uuid);

}


TEST(RepoBSONTest, GetUUIDFieldArray)
{
	std::vector<repoUUID> uuids;

	size_t size = 10;
	mongo::BSONObjBuilder builder, arrbuilder;

	uuids.reserve(size);
	for (size_t i = 0; i < size; ++i)
	{
		uuids.push_back(generateUUID());
		arrbuilder.appendBinData(std::to_string(i), uuids[i].size(), mongo::bdtUUID, (char*)uuids[i].data);
	}

	builder.appendArray("uuid", arrbuilder.obj());

	RepoBSON bson = builder.obj();

	std::vector<repoUUID> outUUIDS = bson.getUUIDFieldArray("uuid");

	EXPECT_EQ(outUUIDS.size(), uuids.size());
	for (size_t i = 0; i < size; i++)
	{
		EXPECT_EQ(outUUIDS[i], uuids[i]);
	}

	//Shouldn't fail if trying to get a uuid field that doesn't exist
	EXPECT_EQ(bson.getUUIDFieldArray("hello").size(), 0);
	EXPECT_EQ(testBson.getUUIDFieldArray("ice").size(), 0);

}

TEST(RepoBSONTest, GetFloatArray)
{
	std::vector<float> floatArrIn;

	size_t size = 10;
	mongo::BSONObjBuilder builder, arrbuilder;

	floatArrIn.reserve(size);
	for (size_t i = 0; i < size; ++i)
	{
		floatArrIn.push_back((float)rand()/100.);
		arrbuilder << std::to_string(i) << floatArrIn[i];
	}

	builder.appendArray("floatarr", arrbuilder.obj());

	RepoBSON bson = builder.obj();

	std::vector<float> floatArrOut = bson.getFloatArray("floatarr");

	EXPECT_EQ(floatArrOut.size(), floatArrIn.size());

	for (size_t i = 0; i < size; i++)
	{
		EXPECT_EQ(floatArrIn[i], floatArrOut[i]);
	}

	//Shouldn't fail if trying to get a uuid field that doesn't exist
	EXPECT_EQ(bson.getFloatArray("hello").size(), 0);
	EXPECT_EQ(testBson.getFloatArray("ice").size(), 0);
}