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
static const RepoBSON emptyBson;

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
	EXPECT_TRUE(emptyBson.getField("hello").eoo());
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

	EXPECT_TRUE(bson.getBinaryFieldAsVector("binDataTest", out));

	EXPECT_EQ(in.size(), out.size());
	for (size_t i = 0; i < size; ++i)
	{
		EXPECT_EQ(in[i], out[i]);
	}

	//Invalid retrieval, but they shouldn't throw exception
	EXPECT_FALSE(bson.getBinaryFieldAsVector("numTest", out));
	EXPECT_FALSE(bson.getBinaryFieldAsVector("stringTest", out));
	EXPECT_FALSE(bson.getBinaryFieldAsVector("doesn'tExist", out));
}

TEST(RepoBSONTest, GetBinaryAsVectorReferenced)
{
	mongo::BSONObjBuilder builder;

	std::vector < uint8_t > in, out;

	size_t size = 100;

	for (size_t i = 0; i < size; ++i)
		in.push_back(i);

	std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> map;
	std::string fname = "testingfile";
	map["binDataTest"] = std::pair<std::string, std::vector<uint8_t>>(fname, in);

	//FIXME: This needs to work until we stop supporting it
	RepoBSON bson(BSON("binDataTest" << fname), map);

	RepoBSON bson2(RepoBSON(), map);

	EXPECT_TRUE(bson.getBinaryFieldAsVector("binDataTest", out));
	EXPECT_FALSE(bson.getBinaryFieldAsVector(fname, out)); //make sure fieldname/filename are not mixed up.

	ASSERT_EQ(out.size(), in.size());
	for (size_t i = 0; i < size; ++i)
	{
		EXPECT_EQ(in[i], out[i]);
	}

	EXPECT_TRUE(bson2.getBinaryFieldAsVector("binDataTest", out));
	EXPECT_FALSE(bson2.getBinaryFieldAsVector(fname, out)); //make sure fieldname/filename are not mixed up.

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

	std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> map, mapout;
	map["testingfile"] = std::pair<std::string, std::vector<uint8_t>>("field", in);

	RepoBSON test2(testBson, map);

	mapout = test2.getFilesMapping();
	ASSERT_EQ(mapout.size(), map.size());

	auto mapIt = map.begin();
	auto mapoutIt = mapout.begin();

	for (; mapIt != map.end(); ++mapIt, ++mapoutIt)
	{
		EXPECT_EQ(mapIt->first, mapIt->first);
		EXPECT_EQ(mapIt->second.first, mapIt->second.first);
		std::vector<uint8_t> dataOut = mapoutIt->second.second;
		std::vector<uint8_t> dataIn = mapIt->second.second;
		EXPECT_EQ(dataOut.size(), dataIn.size());
		if (dataIn.size() > 0)
			EXPECT_EQ(0, strncmp((char*)&dataOut[0], (char*)&dataIn[0], dataIn.size()));
	}
}

TEST(RepoBSONTest, Swap)
{
	RepoBSON test = testBson;

	//Test with bigfile mapping
	std::vector < uint8_t > in;

	in.resize(100);

	std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> map, mapout;
	map["testingfile"] = std::pair<std::string, std::vector<uint8_t>>("blah", in);

	RepoBSON testDiff_org(BSON("entirely" << "different"), map);
	RepoBSON testDiff = testDiff_org;

	EXPECT_TRUE(testDiff_org.toString() == testDiff.toString());
	EXPECT_TRUE(testDiff_org.getFilesMapping().size() == testDiff.getFilesMapping().size());

	test.swap(testDiff);
	EXPECT_EQ(testDiff_org.toString(), test.toString());

	mapout = test.getFilesMapping();
	ASSERT_EQ(map.size(), mapout.size());

	auto mapIt = map.begin();
	auto mapoutIt = mapout.begin();

	for (; mapIt != map.end(); ++mapIt, ++mapoutIt)
	{
		EXPECT_EQ(mapIt->first, mapIt->first);
		EXPECT_EQ(mapIt->second.first, mapIt->second.first);
		std::vector<uint8_t> dataOut = mapoutIt->second.second;
		std::vector<uint8_t> dataIn = mapIt->second.second;
		EXPECT_EQ(dataIn.size(), dataOut.size());
		if (dataIn.size() > 0)
			EXPECT_EQ(0, strncmp((char*)dataOut.data(), (char*)dataIn.data(), dataIn.size()));
	}
}

TEST(RepoBSONTest, GetUUIDField)
{
	repo::lib::RepoUUID uuid = repo::lib::RepoUUID::createUUID();
	mongo::BSONObjBuilder builder;
	builder.appendBinData("uuid", uuid.data().size(), mongo::bdtUUID, (char*)uuid.data().data());

	RepoBSON test = RepoBSON(builder.obj());
	EXPECT_EQ(uuid, test.getUUIDField("uuid"));

	//Shouldn't fail if trying to get a uuid field that doesn't exist
	EXPECT_NE(uuid, test.getUUIDField("hello"));
	EXPECT_NE(uuid, testBson.getUUIDField("ice"));
	EXPECT_NE(uuid, emptyBson.getUUIDField("ice"));

	//Test new UUID
	mongo::BSONObjBuilder builder2;
	builder2.appendBinData("uuid", uuid.data().size(), mongo::newUUID, (char*)uuid.data().data());
	RepoBSON test2 = RepoBSON(builder2.obj());
	EXPECT_EQ(uuid, test2.getUUIDField("uuid"));
}

TEST(RepoBSONTest, GetUUIDFieldArray)
{
	std::vector<repo::lib::RepoUUID> uuids;

	size_t size = 10;
	mongo::BSONObjBuilder builder, arrbuilder;

	uuids.reserve(size);
	for (size_t i = 0; i < size; ++i)
	{
		uuids.push_back(repo::lib::RepoUUID::createUUID());
		arrbuilder.appendBinData(std::to_string(i), uuids[i].data().size(), mongo::bdtUUID, (char*)uuids[i].data().data());
	}

	builder.appendArray("uuid", arrbuilder.obj());

	RepoBSON bson = builder.obj();

	std::vector<repo::lib::RepoUUID> outUUIDS = bson.getUUIDFieldArray("uuid");

	EXPECT_EQ(outUUIDS.size(), uuids.size());
	for (size_t i = 0; i < size; i++)
	{
		EXPECT_EQ(uuids[i], outUUIDS[i]);
	}

	//Shouldn't fail if trying to get a uuid field that doesn't exist
	EXPECT_EQ(0, bson.getUUIDFieldArray("hello").size());
	EXPECT_EQ(0, testBson.getUUIDFieldArray("ice").size());
	EXPECT_EQ(0, emptyBson.getUUIDFieldArray("ice").size());
}

TEST(RepoBSONTest, GetFloatArray)
{
	std::vector<float> floatArrIn;

	size_t size = 10;
	mongo::BSONObjBuilder builder, arrbuilder;

	floatArrIn.reserve(size);
	for (size_t i = 0; i < size; ++i)
	{
		floatArrIn.push_back((float)rand() / 100.);
		arrbuilder << std::to_string(i) << floatArrIn[i];
	}

	builder.appendArray("floatarr", arrbuilder.obj());

	RepoBSON bson = builder.obj();

	std::vector<float> floatArrOut = bson.getFloatArray("floatarr");

	EXPECT_EQ(floatArrIn.size(), floatArrOut.size());

	for (size_t i = 0; i < size; i++)
	{
		EXPECT_EQ(floatArrIn[i], floatArrOut[i]);
	}

	//Shouldn't fail if trying to get a uuid field that doesn't exist
	EXPECT_EQ(0, bson.getFloatArray("hello").size());
	EXPECT_EQ(0, testBson.getFloatArray("ice").size());
	EXPECT_EQ(0, emptyBson.getFloatArray("ice").size());
}

TEST(RepoBSONTest, GetTimeStampField)
{
	mongo::BSONObjBuilder builder;

	mongo::Date_t date = mongo::Date_t(time(NULL) * 1000);

	builder.append("ts", date);

	RepoBSON tsBson = builder.obj();

	EXPECT_EQ(date.asInt64(), tsBson.getTimeStampField("ts"));

	//Shouldn't fail if trying to get a uuid field that doesn't exist
	EXPECT_EQ(-1, tsBson.getTimeStampField("hello"));
	EXPECT_EQ(-1, testBson.getTimeStampField("ice"));
	EXPECT_EQ(-1, emptyBson.getTimeStampField("ice"));
}

TEST(RepoBSONTest, GetListStringPairField)
{
	std::vector<std::vector<std::string>> vecIn;

	size_t size = 10;
	mongo::BSONObjBuilder builder, arrbuilder;

	vecIn.reserve(size);
	for (size_t i = 0; i < size; ++i)
	{
		int n1 = rand();
		int n2 = rand();
		vecIn.push_back({ std::to_string(n1), std::to_string(n2) });

		arrbuilder << std::to_string(i) << BSON("first" << std::to_string(n1) << "second" << std::to_string(n2) << "third" << 1);
	}

	builder.appendArray("bsonArr", arrbuilder.obj());

	RepoBSON bson = builder.obj();

	std::list<std::pair<std::string, std::string>> vecOut =
		bson.getListStringPairField("bsonArr", "first", "second");

	EXPECT_EQ(vecIn.size(), vecOut.size());
	auto listIt = vecOut.begin();

	for (size_t i = 0; i < size; i++)
	{
		EXPECT_EQ(vecIn[i][0], listIt->first);
		EXPECT_EQ(vecIn[i][1], listIt->second);
		++listIt;
	}

	//Shouldn't fail if trying to get a uuid field that doesn't exist
	EXPECT_EQ(0, bson.getListStringPairField("hello", "first", "third").size());
	EXPECT_EQ(0, bson.getListStringPairField("hello", "hi", "bye").size());
	EXPECT_EQ(0, bson.getListStringPairField("hello", "first", "second").size());
	EXPECT_EQ(0, testBson.getListStringPairField("ice", "first", "second").size());
	EXPECT_EQ(0, emptyBson.getListStringPairField("ice", "first", "second").size());
}

TEST(RepoBSONTest, CloneAndShrink)
{
	//shrinking a bson without any binary fields should yield an identical bson
	RepoBSON shrunkBson = testBson.cloneAndShrink();

	EXPECT_EQ(testBson, shrunkBson);
	EXPECT_EQ(testBson.getFilesMapping().size(), shrunkBson.getFilesMapping().size());

	mongo::BSONObjBuilder builder;
	std::vector < uint8_t > in, out, ref;

	size_t size = 100;

	in.resize(size);
	ref.resize(size);

	builder << "stringTest" << "hello";
	builder << "numTest" << 1.35;
	builder.appendBinData("binDataTest", in.size(), mongo::BinDataGeneral, in.data());

	std::unordered_map < std::string, std::pair<std::string, std::vector<uint8_t>>> mapping, outMapping;
	mapping["orgRef"] = std::pair<std::string, std::vector<uint8_t>>("blah", ref);

	RepoBSON binBson(builder.obj(), mapping);

	shrunkBson = binBson.cloneAndShrink();
	outMapping = shrunkBson.getFilesMapping();

	EXPECT_NE(shrunkBson, binBson);
	EXPECT_FALSE(shrunkBson.hasField("binDataTest"));
	EXPECT_EQ(2, outMapping.size());
	EXPECT_TRUE(outMapping.find("orgRef") != outMapping.end());

	//Check the binary still obtainable
	EXPECT_TRUE(shrunkBson.getBinaryFieldAsVector("binDataTest", out));

	ASSERT_EQ(in.size(), out.size());
	for (size_t i = 0; i < out.size(); ++i)
	{
		EXPECT_EQ(in[i], out[i]);
	}

	//Check the out referenced bigfile is still sane

	EXPECT_EQ(ref.size(), outMapping["orgRef"].second.size());
	for (size_t i = 0; i < ref.size(); ++i)
	{
		EXPECT_EQ(ref[i], outMapping["orgRef"].second[i]);
	}
}

TEST(RepoBSONTest, GetBigBinary)
{
	std::vector < uint8_t > in, out;

	size_t size = 100;

	in.resize(size);

	std::unordered_map < std::string, std::pair<std::string, std::vector<uint8_t>>> mapping;
	mapping["blah"] = std::pair<std::string, std::vector<uint8_t>>("orgRef", in);

	RepoBSON binBson(testBson, mapping);

	out = binBson.getBigBinary("blah");
	EXPECT_EQ(in.size(), out.size());
	for (size_t i = 0; i < out.size(); ++i)
	{
		EXPECT_EQ(in[i], out[i]);
	}

	EXPECT_EQ(0, binBson.getBigBinary("hello").size());
	EXPECT_EQ(0, binBson.getBigBinary("ice").size());
	EXPECT_EQ(0, emptyBson.getBigBinary("ice").size());
}

TEST(RepoBSONTest, GetFileList)
{
	std::vector < uint8_t > in;

	size_t size = 100;

	in.resize(size);

	std::unordered_map < std::string, std::pair<std::string, std::vector<uint8_t>>> mapping;
	mapping["orgRef"] = std::pair<std::string, std::vector<uint8_t>>("blah", in);

	RepoBSON binBson(testBson, mapping);
	auto fileList = binBson.getFileList();

	EXPECT_EQ(1, fileList.size());
	EXPECT_TRUE(fileList[0].first == "orgRef");
	EXPECT_EQ(0, testBson.getFileList().size());
	EXPECT_EQ(0, emptyBson.getFileList().size());
}

TEST(RepoBSONTest, GetFilesMapping)
{
	std::vector < uint8_t > in;

	size_t size = 100;

	in.resize(size);

	std::unordered_map < std::string, std::pair<std::string, std::vector<uint8_t>>> mapping, outMapping;
	mapping["orgRef"] = std::pair<std::string, std::vector<uint8_t>>("blah", in);

	RepoBSON binBson(testBson, mapping);
	outMapping = binBson.getFilesMapping();

	EXPECT_EQ(1, outMapping.size());
	EXPECT_FALSE(outMapping.find("orgRef") == outMapping.end());

	EXPECT_EQ(0, testBson.getFilesMapping().size());
	EXPECT_EQ(0, emptyBson.getFilesMapping().size());
}

TEST(RepoBSONTest, HasOversizeFiles)
{
	std::vector < uint8_t > in;

	size_t size = 100;

	in.resize(size);

	std::unordered_map < std::string, std::pair<std::string, std::vector<uint8_t>>> mapping;
	mapping["orgRef"] = std::pair<std::string, std::vector<uint8_t>>("blah", in);

	RepoBSON binBson(testBson, mapping);

	EXPECT_TRUE(binBson.hasOversizeFiles());
	EXPECT_FALSE(testBson.hasOversizeFiles());
	EXPECT_FALSE(emptyBson.hasOversizeFiles());
}

TEST(RepoBSONTest, GetEmbeddedDoubleTest)
{
	RepoBSON empty;

	//Shouldn't fail.
	EXPECT_EQ(empty.getEmbeddedDouble("something", "somethingElse"), 0);
	EXPECT_EQ(empty.getEmbeddedDouble("something", "somethingElse", 10), 10);

	RepoBSON hasFieldWrongTypeBson(BSON("field" << 1));
	EXPECT_EQ(hasFieldWrongTypeBson.getEmbeddedDouble("field", "somethingElse"), 0);

	RepoBSON hasFieldNoEmbeddedField(BSON("field" << testBson));
	EXPECT_EQ(hasFieldNoEmbeddedField.getEmbeddedDouble("field", "somethingElse"), 0);

	RepoBSON hasEmbeddedFieldWrongType(BSON("field" << testBson));
	EXPECT_EQ(hasEmbeddedFieldWrongType.getEmbeddedDouble("field", "ice"), 0);

	RepoBSON expectNumber(BSON("field" << testBson));
	EXPECT_EQ(expectNumber.getEmbeddedDouble("field", "amount"), 100);

	auto innerBson = BSON("amount" << 1.10101);
	RepoBSON expectNumber2(BSON("field" << innerBson));
	EXPECT_EQ(expectNumber2.getEmbeddedDouble("field", "amount"), 1.10101);
}

TEST(RepoBSONTest, HasEmbeddedFieldTest)
{
	EXPECT_FALSE(emptyBson.hasEmbeddedField("hi", "bye"));

	RepoBSON hasFieldWrongTypeBson(BSON("field" << 1));
	EXPECT_FALSE(hasFieldWrongTypeBson.hasEmbeddedField("field", "bye"));

	RepoBSON expectTrue(BSON("field" << testBson));
	EXPECT_TRUE(expectTrue.hasEmbeddedField("field", "ice"));
	EXPECT_FALSE(expectTrue.hasEmbeddedField("field", "NonExistent"));
}