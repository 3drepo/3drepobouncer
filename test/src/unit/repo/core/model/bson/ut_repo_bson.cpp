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

#define NOMINMAX

#include <cstdlib>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_matchers.h"
#include "../../../../repo_test_mesh_utils.h"

#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_element.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_project_settings.h>

using namespace repo::core::model;
using namespace testing;

static auto mongoTestBSON = BSON("ice" << "lolly" << "amount" << 100.0);
static const RepoBSON testBson = RepoBSON(BSON("ice" << "lolly" << "amount" << 100));
static const RepoBSON emptyBson;

template<class T>
mongo::BSONObj makeBsonArray(std::vector<T> a)
{
	mongo::BSONObjBuilder arrbuilder;
	for (auto i = 0; i < a.size(); i++)
	{
		arrbuilder.append(std::to_string(i), a[i]);
	}
	return arrbuilder.obj();
}

template<>
mongo::BSONObj makeBsonArray(std::vector<repo::lib::RepoUUID> uuids)
{
	mongo::BSONObjBuilder arrbuilder;
	for (auto i = 0; i < uuids.size(); i++)
	{
		arrbuilder.appendBinData(std::to_string(i), uuids[i].data().size(), mongo::bdtUUID, (char*)uuids[i].data().data());
	}
	return arrbuilder.obj();
}

mongo::BSONObj makeRepoVectorObj(const repo::lib::RepoVector3D& v)
{
	mongo::BSONObjBuilder builder;
	builder.append("x", v.x);
	builder.append("y", v.y);
	builder.append("z", v.z);
	return builder.obj();
}

mongo::BSONObj makeBoundsObj(const repo::lib::RepoVector3D64& min, const repo::lib::RepoVector3D64& max)
{
	mongo::BSONObjBuilder builder;
	builder.append("0", makeBsonArray(min.toStdVector()));
	builder.append("1", makeBsonArray(max.toStdVector()));
	return builder.obj();
}

mongo::BSONObj makeMatrixObj(const repo::lib::RepoMatrix& m)
{
	auto d = m.getData();
	mongo::BSONObjBuilder builder;
	builder.append("0", makeBsonArray(std::vector<float>({ d[0], d[1], d[2], d[3]})));
	builder.append("1", makeBsonArray(std::vector<float>({ d[4], d[5], d[6], d[7] })));
	builder.append("2", makeBsonArray(std::vector<float>({ d[8], d[9], d[10], d[11] })));
	builder.append("3", makeBsonArray(std::vector<float>({ d[12], d[13], d[14], d[15] })));
	return builder.obj();
}

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

TEST(RepoBSONTest, ConstructFromMongoSizeExceeds) {
	std::string msgData;
	msgData.resize(1024 * 1024 * 65);

	ASSERT_ANY_THROW(BSON("message" << msgData));

	try {
		BSON("message" << msgData);
	}
	catch (const std::exception &e) {
		EXPECT_NE(std::string(e.what()).find("BufBuilder"), std::string::npos);
	}
}

TEST(RepoBSONTest, Fields)
{
	// For historical reasons, nFields and hasField should return only the 'true'
	// BSON fields and not bin mapped fields

	RepoBSON::BinMapping map;
	map["bin1"] = makeRandomBinary();

	mongo::BSONObjBuilder builder;

	builder.append("stringField", "string");
	builder.append("doubleField", (double)0.123);

	RepoBSON bson(builder.obj(), map);

	EXPECT_THAT(bson.nFields(), Eq(2));

	EXPECT_THAT(bson.hasField("stringField"), IsTrue());
	EXPECT_THAT(bson.hasField("doubleField"), IsTrue());
	EXPECT_THAT(bson.hasField("bin1"), IsFalse());
	EXPECT_THAT(bson.hasBinField("bin1"), IsTrue());
}

TEST(RepoBSONTest, GetField)
{
	EXPECT_EQ(testBson.getField("ice"), testBson.getField("ice"));
	EXPECT_NE(testBson.getField("ice"), testBson.getField("amount"));

	EXPECT_EQ("lolly", testBson.getStringField("ice"));
	EXPECT_EQ(100, testBson.getIntField("amount"));
	EXPECT_TRUE(emptyBson.getField("hello").eoo());
}

TEST(RepoBSONTest, GetBinDataGeneralAsVectorEmbedded)
{
	// RepoBSONBuilder no longer supports adding BinDataGeneral fields, however
	// legacy documents may still have them, therefore RepoBSON should be able
	// to return these fields if they exist.

	std::vector <uint8_t> in, out;

	in = makeRandomBinary();

	mongo::BSONObjBuilder builder;
	builder << "stringTest" << "hello";
	builder << "numTest" << 1.35;
	builder.appendBinData("binDataTest", in.size(), mongo::BinDataGeneral, &in[0]);

	RepoBSON bson(builder.obj());

	EXPECT_TRUE(bson.getBinaryFieldAsVector("binDataTest", out));
	EXPECT_THAT(in, Eq(out));

	EXPECT_THROW({ bson.getBinaryFieldAsVector("numTest", out); }, repo::lib::RepoFieldTypeException);
	EXPECT_THROW({ bson.getBinaryFieldAsVector("stringTest", out); }, repo::lib::RepoFieldTypeException);
	EXPECT_THROW({ bson.getBinaryFieldAsVector("doesn'tExist", out); }, repo::lib::RepoFieldNotFoundException);
}

TEST(RepoBSONTest, AssignOperator)
{
	// Simple object with no mappings

	RepoBSON test = testBson;

	EXPECT_TRUE(test.toString() == testBson.toString());
	EXPECT_EQ(test.getFilesMapping().size(), testBson.getFilesMapping().size());

	RepoBSON::BinMapping map;
	map["field1"] = makeRandomBinary();
	map["field2"] = makeRandomBinary();

	RepoBSON test2(testBson, map);

	// With mappings
	EXPECT_THAT(test2.toString(), Eq(testBson.toString()));
	EXPECT_THAT(test2.getFilesMapping(), Eq(map));

	RepoBSON test3 = test2;
	EXPECT_THAT(test3.toString(), Eq(test2.toString()));
	EXPECT_THAT(test3.getFilesMapping(), Eq(test2.getFilesMapping()));

	// Original instance should be independent

	test2.removeField("ice");
	EXPECT_THAT(test3.toString(), Not(Eq(test2.toString())));
}

TEST(RepoBSONTest, GetUUIDField)
{
	repo::lib::RepoUUID uuid = repo::lib::RepoUUID::createUUID();

	// Test legacy uuid
	{
		mongo::BSONObjBuilder builder;
		builder.appendBinData("uuid", uuid.data().size(), mongo::bdtUUID, (char*)uuid.data().data());

		RepoBSON bson = RepoBSON(builder.obj());
		EXPECT_EQ(uuid, bson.getUUIDField("uuid"));
		EXPECT_THROW({ bson.getUUIDField("empty"); }, repo::lib::RepoFieldNotFoundException);
	}

	// Test new UUID
	{
		mongo::BSONObjBuilder builder;
		builder.appendBinData("uuid", uuid.data().size(), mongo::newUUID, (char*)uuid.data().data());

		RepoBSON bson = RepoBSON(builder.obj());
		EXPECT_EQ(uuid, bson.getUUIDField("uuid"));
		EXPECT_THROW({ bson.getUUIDField("empty"); }, repo::lib::RepoFieldNotFoundException);
	}

	// Test type checking
	{
		mongo::BSONObjBuilder builder;
		builder.appendBinData("uuid", uuid.data().size(), mongo::bdtCustom, (char*)uuid.data().data());

		RepoBSON bson = RepoBSON(builder.obj());
		EXPECT_THROW({ bson.getUUIDField("uuid"); }, repo::lib::RepoFieldTypeException);
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("uuid", uuid.toString());

		RepoBSON bson = RepoBSON(builder.obj());
		EXPECT_THROW({ bson.getUUIDField("uuid"); }, repo::lib::RepoFieldTypeException);
	}
}

TEST(RepoBSONTest, GetUUIDFieldArray)
{
	std::vector<repo::lib::RepoUUID> uuids = {
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID(),
		repo::lib::RepoUUID::createUUID()
	};

	mongo::BSONObjBuilder builder;
	builder.appendArray("uuids", makeBsonArray(uuids));
	RepoBSON bson = builder.obj();
	EXPECT_THAT(bson.getUUIDFieldArray("uuids"), ElementsAreArray(uuids)); // We expect the order to be preserved here

	// The getUUIDFieldArray doesn't throw, but returns an empty array if the
	// field does not exist.

	EXPECT_THAT(bson.getUUIDFieldArray("none"), IsEmpty());
}

TEST(RepoBSONTest, GetFloatArray)
{
	size_t size = 10;

	{
		std::vector<float> arr;
		for (size_t i = 0; i < size; ++i)
		{
			arr.push_back((float)rand() / 100.);
		}

		mongo::BSONObjBuilder builder;
		builder.appendArray("array", makeBsonArray(arr));

		RepoBSON bson = builder.obj();

		EXPECT_THAT(bson.getFloatArray("array"), ElementsAreArray(arr));
		EXPECT_THAT(bson.getFloatArray("none"), IsEmpty());
	}

	// We expect a type error if the field is an array, but of the wrong type

	{
		std::vector<std::string> arr;
		mongo::BSONObjBuilder builder;

		arr.reserve(size);
		for (size_t i = 0; i < size; ++i)
		{
			arr.push_back(std::to_string((float)rand() / 100.));
		}

		builder.appendArray("array", makeBsonArray(arr));

		RepoBSON bson = builder.obj();

		EXPECT_THROW({ bson.getFloatArray("array"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THAT(bson.getFloatArray("none"), IsEmpty());
	}
}

TEST(RepoBSONTest, GetTimeStampField)
{
	auto posixTimestamp = time(NULL);

	// By convention, Date_t takes a posix timestamp in milliseconds
	mongo::Date_t date = mongo::Date_t(posixTimestamp * 1000);

	{
		mongo::BSONObjBuilder builder;
		builder.append("ts", date);
		RepoBSON bson = builder.obj();
		EXPECT_THAT(bson.getTimeStampField("ts"), Eq(posixTimestamp));
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("ts", "hello");
		RepoBSON bson = builder.obj();
		EXPECT_THROW({ bson.getTimeStampField("ts"); }, repo::lib::RepoFieldTypeException);
	}

	{
		mongo::BSONObjBuilder builder;
		RepoBSON bson = builder.obj();
		EXPECT_THROW({ bson.getTimeStampField("ts"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, GetBinary)
{
	{
		auto in = makeRandomBinary();
		RepoBSON::BinMapping mapping;
		mapping["blah"] = in;

		RepoBSON bson(testBson, mapping);

		EXPECT_THAT(testBson.getBinary("blah"), Eq(mapping["blah"]));
	}

	// Check that if we pass in formatted data such as strings, it doesn't mess
	// with them
	{
		std::string s = "Ut velit leo, lobortis eget nibh quis, imperdiet \0consectetur orci. Donec accumsan\0 tortor odio, vel imperdiet metus viverra id. Maecenas efficitur vulputate diam id molestie. Cras molestie, orci eget consectetur auctor, leo velit auctor lacus, nec dictum velit nibh et diam. Donec ac sapien euismod, vulputate odio quis, bibendum nulla. \"Sed aliquet \"rhoncus pulvinar\0. Donec consectetur tristiq\"ue nibh non pulvinar.";
		auto data = std::vector<uint8_t>(s.c_str(), s.c_str() + s.size());

		RepoBSON::BinMapping mapping;
		mapping["string"] = data;

		RepoBSON bson(testBson, mapping);

		auto out = bson.getBinary("string");
		EXPECT_THAT(std::string((const char*)out.data(), out.size()), Eq(s));
	}
}

struct randomType
{
	int a;
	char b;
	double c;
};

static bool operator== (randomType a, randomType b)
{
	return a.a == b.a && a.b == b.b && a.c == b.c;
}

TEST(RepoBSONTest, GetBinaryAsVector)
{
	{
		auto in = makeRandomBinary();
		RepoBSON::BinMapping mapping;
		mapping["blah"] = in;

		RepoBSON bson(testBson, mapping);

		std::vector<uint8_t> out;
		bson.getBinaryFieldAsVector("blah", out);

		EXPECT_THAT(out, Eq(in));
		EXPECT_THROW({ bson.getBinaryFieldAsVector("hello", out); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getBinaryFieldAsVector("nofield", out); }, repo::lib::RepoFieldNotFoundException);
	}

	// Check that the typecasting works
	{
		// Write this test using std::vectors to handle the buffers, because that
		// is most likely what will be used for real

		std::vector<randomType> in;

		for (int i = 0; i < 100; i++)
		{
			randomType t;
			t.a = rand();
			t.b = rand() % 256;
			t.c = rand() * 10000 / 10843.7;
			in.push_back(t);
		}

		RepoBSON::BinMapping mapping;
		mapping["randomType"] = std::vector<uint8_t>(
			(const char*)in.data(),
			(const char*)in.data() + (in.size() * sizeof(randomType)));

		RepoBSON bson(testBson, mapping);

		std::vector<randomType> out;
		bson.getBinaryFieldAsVector("randomType", out);

		EXPECT_THAT(out, Eq(in));
	}
}

TEST(RepoBSONTest, BinaryFilesUpdated)
{
	// Checks that providing a mapping along with a RepoBSON that already has a
	// big files array will join the two, and prefer the *original* mapping

	mongo::BSONObjBuilder builder;
	builder.append("field", "value");

	RepoBSON::BinMapping existing;
	existing["bin1"] = makeRandomBinary();
	existing["bin2"] = makeRandomBinary();

	RepoBSON::BinMapping additional;
	additional["bin1"] = makeRandomBinary();
	additional["bin2"] = makeRandomBinary();
	additional["bin3"] = makeRandomBinary();

	RepoBSON bson(builder.obj(), existing);

	EXPECT_THAT(bson.getBinary("bin1"), Eq(existing["bin1"]));
	EXPECT_THAT(bson.getBinary("bin2"), Eq(existing["bin2"]));
	EXPECT_THROW({ bson.getBinary("bin3"); }, repo::lib::RepoFieldNotFoundException);

	RepoBSON bson2(bson, additional);

	EXPECT_THAT(bson2.getBinary("bin1"), Eq(existing["bin1"]));
	EXPECT_THAT(bson2.getBinary("bin2"), Eq(existing["bin2"]));
	EXPECT_THAT(bson2.getBinary("bin3"), Eq(additional["bin3"]));
	EXPECT_THAT(bson2.getBinary("bin1"), Not(Eq(additional["bin1"])));
	EXPECT_THAT(bson2.getBinary("bin2"), Not(Eq(additional["bin2"])));
}

TEST(RepoBSONTest, GetBinariesAsBuffer)
{
	RepoBSON::BinMapping map;
	map["file1"] = makeRandomBinary();
	map["file2"] = makeRandomBinary();
	map["file3"] = makeRandomBinary();

	RepoBSONBuilder builder;
	RepoBSON bson(builder.obj(), map);

	auto buf = bson.getBinariesAsBuffer();

	// The first element of the pair should be a document describing the position
	// in the buffer of each file.

	auto elements = buf.first;

	// And the second the concatenated buffer

	auto buffer = buf.second;

	for (auto f : map) {
		auto originalName = f.first;
		auto originalData = f.second;

		auto range = elements.getObjectField(originalName);

		auto start = range.getIntField(REPO_LABEL_BINARY_START);
		auto size = range.getIntField(REPO_LABEL_BINARY_SIZE);

		auto actualData = std::vector<uint8_t>(buffer.data() + start, buffer.data() + start + size);

		EXPECT_THAT(actualData, Eq(originalData));
	}
}

TEST(RepoBSONTest, ReplaceBinaryWithReference)
{
	// Create mockups of the Elements and Buffer ref documents

	RepoBSONBuilder elementsBuilder;
	int32_t counter = 0;
	for (int i = 0; i < 10; i++)
	{
		RepoBSONBuilder rangeBuilder;
		rangeBuilder.append(REPO_LABEL_BINARY_START, (int32_t)counter);
		auto size = rand();
		rangeBuilder.append(REPO_LABEL_BINARY_SIZE, (int32_t)size);
		counter += size;
		elementsBuilder.append(repo::lib::RepoUUID::createUUID().toString(), rangeBuilder.obj());
	}
	auto elemRef = elementsBuilder.obj();

	RepoBSONBuilder fileRefBuilder;
	fileRefBuilder.append(REPO_LABEL_BINARY_START, (int32_t)0);
	fileRefBuilder.append(REPO_LABEL_BINARY_SIZE, (int32_t)rand());
	fileRefBuilder.append(REPO_LABEL_BINARY_FILENAME, repo::lib::RepoUUID::createUUID().toString()); // File references use strings, even though they are all typically UUIDs now
	auto fileRef = fileRefBuilder.obj();

	RepoBSONBuilder builder;

	auto s = "myString";
	auto d = 123.456;
	auto u = repo::lib::RepoUUID::createUUID();
	auto v = std::vector<float>({ 1, 2, 3, 4, 5, 6, 7 });

	builder.append("s", s);
	builder.append("d", d);
	builder.appendTimeStamp("now");
	builder.append("uuid", u);
	builder.appendArray("v", v);

	RepoBSON::BinMapping map;
	map["b1"] = makeRandomBinary();
	map["b2"] = makeRandomBinary();

	RepoBSON bson = RepoBSON(builder.obj(), map);

	// To start with, the BSON should not have the reference entry. This is
	// true whether the BSON has binary mappings or not.

	EXPECT_THAT(bson.hasFileReference(), IsFalse());

	// (Note that replaceBinaryWithReference doesn't actually use the mappings,
	// which from this point are ignored.)

	bson.replaceBinaryWithReference(fileRef, elemRef);

	// Should now say that it has a _blobRef

	EXPECT_THAT(bson.hasFileReference(), IsTrue());

	// Replacing the binary reference should in practice add the _blobRef entry
	// in place.

	auto blobRef = bson.getObjectField(REPO_LABEL_BINARY_REFERENCE);
	EXPECT_THAT(blobRef.getObjectField(REPO_LABEL_BINARY_ELEMENTS), Eq(elemRef));
	EXPECT_THAT(blobRef.getObjectField(REPO_LABEL_BINARY_BUFFER), Eq(fileRef));

	// All existing fields should be preserved.

	EXPECT_THAT(bson.getStringField("s"), Eq(s));
	EXPECT_THAT(bson.getDoubleField("d"), Eq(d));
	EXPECT_THAT(bson.getTimeStampField("now"), IsNow());
	EXPECT_THAT(bson.getUUIDField("u"), Eq(u));
	EXPECT_THAT(bson.getFloatVectorField("v"), Eq(v));

	// It should be possible to get the buffer via getBinaryReference too

	EXPECT_THAT(bson.getBinaryReference(), Eq(fileRef));
}


TEST(RepoBSONTest, InitBinaryBuffer)
{
	// Create a mockup again, but this time based on an actual bin mapping

	RepoBSON::BinMapping map;

	map[repo::lib::RepoUUID::createUUID().toString()] = makeRandomBinary();
	map[repo::lib::RepoUUID::createUUID().toString()] = makeRandomBinary();
	map[repo::lib::RepoUUID::createUUID().toString()] = makeRandomBinary();
	map[repo::lib::RepoUUID::createUUID().toString()] = makeRandomBinary();

	RepoBSONBuilder originalBuilder;
	RepoBSON original(originalBuilder.obj(), map);

	auto buf = original.getBinariesAsBuffer();
	auto elems = buf.first;
	auto file = buf.second;

	// Make a fileref out of the return buffer (though this won't actually be used
	// because we will pass the buffer in directly).

	RepoBSONBuilder fileRefBuilder;
	fileRefBuilder.append(REPO_LABEL_BINARY_START, (int32_t)0);
	fileRefBuilder.append(REPO_LABEL_BINARY_SIZE, (int32_t)file.size());
	fileRefBuilder.append(REPO_LABEL_BINARY_FILENAME, repo::lib::RepoUUID::createUUID().toString()); // File references use strings, even though they are all typically UUIDs now
	auto fileRef = fileRefBuilder.obj();

	// Create a new BSON without the mappings, but give it the _blobRef

	RepoBSONBuilder bsonBuilder;
	RepoBSON bson(bsonBuilder.obj());

	bson.replaceBinaryWithReference(fileRef, elems);

	// Does not yet have the actual buffers, but now has the ability to read them

	for(auto f : map)
	{
		EXPECT_THAT(bson.hasBinField(f.first), IsFalse());
	}

	// Read the file into the binary mapping

	bson.initBinaryBuffer(file);

	// Now the fields should be available

	for (auto f : map)
	{
		EXPECT_THAT(bson.getBinary(f.first), Eq(f.second));
	}
}

TEST(RepoBSONTest, GetFilesMapping)
{
	RepoBSON::BinMapping mapping, outMapping;
	mapping["orgRef"] = makeRandomBinary();

	RepoBSON binBson(testBson, mapping);
	outMapping = binBson.getFilesMapping();

	EXPECT_EQ(1, outMapping.size());
	EXPECT_FALSE(outMapping.find("orgRef") == outMapping.end());

	EXPECT_EQ(0, testBson.getFilesMapping().size());
	EXPECT_EQ(0, emptyBson.getFilesMapping().size());
}

TEST(RepoBSONTest, HasOversizeFiles)
{
	RepoBSON::BinMapping mapping;
	mapping["orgRef"] = makeRandomBinary();

	RepoBSON binBson(testBson, mapping);

	EXPECT_TRUE(binBson.hasOversizeFiles());
	EXPECT_FALSE(testBson.hasOversizeFiles());
	EXPECT_FALSE(emptyBson.hasOversizeFiles());
}

TEST(RepoBSONTest, GetBoolField)
{
	{
		mongo::BSONObjBuilder builder;
		builder.append("bool", true);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getBoolField("bool"), Eq(true));
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("bool", false);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getBoolField("bool"), Eq(false)); // Get the value false - not throwing based on not finding the field
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("bool", "string");
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getBoolField("bool"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getBoolField("none"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, GetStringField)
{
	{
		mongo::BSONObjBuilder builder;
		builder.append("string", "value");
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getStringField("string"), Eq("value"));
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("string", "");
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getStringField("string"), Eq("")); // Get the 'empty' value - not throwing based on an empty field
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("string", true);
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getStringField("string"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getStringField("string"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, GetObjectField)
{
	// A single object
	{
		mongo::BSONObjBuilder sub;
		sub.append("string", "value");
		sub.append("int", 0);
		auto subObj = sub.obj();

		mongo::BSONObjBuilder builder;
		builder.append("object", subObj);

		RepoBSON bson(builder.obj());

		EXPECT_THAT(bson.getObjectField("object").toString(), Eq(subObj.toString()));
	}

	// A simple array (as an object)
	{
		std::vector<std::string> strings = {
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
		};

		mongo::BSONObjBuilder builder;
		builder.append("object", makeBsonArray(strings));

		RepoBSON bson(builder.obj());

		auto o = bson.getObjectField("object");

		for (int i = 0; i < strings.size(); i++)
		{
			EXPECT_THAT(o.getStringField(std::to_string(i)), Eq(strings[i]));
		}
	}

	// A complex array of objects
	{
		mongo::BSONObjBuilder obj1;
		std::vector<std::string> strings = {
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
		};
		obj1.append("array", makeBsonArray(strings));
		obj1.append("field", "value");

		mongo::BSONObjBuilder obj2;
		std::vector<repo::lib::RepoUUID> uuids = {
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
		};
		obj2.append("array", makeBsonArray(uuids));
		obj2.append("field", "value");

		mongo::BSONObjBuilder obj3;
		obj3.append("field", "value");
		obj3.append("field2", 0);

		std::vector<mongo::BSONObj> objs = {
			obj1.obj(),
			obj2.obj(),
			obj3.obj()
		};

		mongo::BSONObjBuilder builder;
		builder.append("object", makeBsonArray(objs));

		RepoBSON bson(builder.obj());

		auto o = bson.getObjectField("object");

		EXPECT_THAT(o.getObjectField("0").getStringArray("array"), Eq(strings));
		EXPECT_THAT(o.getObjectField("0").getStringField("field"), Eq("value"));
		EXPECT_THAT(o.getObjectField("1").getUUIDFieldArray("array"), Eq(uuids));
		EXPECT_THAT(o.getObjectField("1").getStringField("field"), Eq("value"));
		EXPECT_THAT(o.getObjectField("2").getStringField("field"), Eq("value"));
		EXPECT_THAT(o.getObjectField("2").getIntField("field2"), Eq(0));
	}

	// A nested hierarchy of objects
	{
		mongo::BSONObjBuilder obj1;
		std::vector<std::string> strings = {
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
		};
		obj1.append("array", makeBsonArray(strings));
		obj1.append("field", "value");

		mongo::BSONObjBuilder obj2;
		std::vector<repo::lib::RepoUUID> uuids = {
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
		};
		obj2.append("array", makeBsonArray(uuids));
		obj2.append("field", "value");
		obj2.append("obj1", obj1.obj());

		mongo::BSONObjBuilder obj3;
		obj3.append("field", "value");
		obj3.append("obj2", obj2.obj());

		mongo::BSONObjBuilder builder;
		builder.append("object", obj3.obj());

		RepoBSON bson(builder.obj());

		auto o = bson.getObjectField("object");

		EXPECT_THAT(o.getObjectField("field").getStringField("field"), Eq("value"));
		EXPECT_THAT(o.getObjectField("obj2").getStringField("field"), Eq("value"));
		EXPECT_THAT(o.getObjectField("obj2").getUUIDFieldArray("array"), Eq(uuids));
		EXPECT_THAT(o.getObjectField("obj2").getObjectField("obj1").getStringField("field"), Eq("value"));
		EXPECT_THAT(o.getObjectField("obj2").getObjectField("obj1").getStringArray("array"), Eq(strings));
	}

	{
		mongo::BSONObjBuilder builder;
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getObjectField("object"); }, repo::lib::RepoFieldNotFoundException);
	}

	// Should throw if this is a primitive type - objects are explicitly BSON
	// structures; if looking for something encapsulating both primitives and
	// these, the caller should be looking at a BSON element.
	// Only arrays are polymorphic with objects.
	{
		mongo::BSONObjBuilder builder;
		builder.append("object", "string");
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getObjectField("object"); }, repo::lib::RepoFieldTypeException);
	}
}

TEST(RepoBSONTest, GetBounds3D)
{
	// Bounds are encoded as two nested arrays, and parse to two (float) RepoVectors

	repo::lib::RepoVector3D64 a(rand() - rand(), rand() - rand(), rand() - rand());
	repo::lib::RepoVector3D64 b(rand() - rand(), rand() - rand(), rand() - rand());
	auto min = repo::lib::RepoVector3D64::min(a, b);
	auto max = repo::lib::RepoVector3D64::max(a, b);

	auto bounds = makeBoundsObj(min, max);

	mongo::BSONObjBuilder builder;
	builder.append("bounds", bounds);

	RepoBSON bson(builder.obj());

	auto expected = std::vector<repo::lib::RepoVector3D>({
		repo::lib::RepoVector3D((float)min.x, (float)min.y, (float)min.z),
		repo::lib::RepoVector3D((float)max.x, (float)max.y, (float)max.z),
	});

	EXPECT_THAT(bson.getBounds3D("bounds"), Eq(expected));
}

TEST(RepoBSONTest, GetVector3DField)
{
	// RepoVectors are encoded by component names instead of indices

	mongo::BSONObjBuilder builder;
	auto v = makeRepoVector();
	builder.append("vector", makeRepoVectorObj(v));
	RepoBSON bson(builder.obj());
	EXPECT_THAT(bson.getVector3DField("vector"), Eq(v));
}

TEST(RepoBSONTest, GetMatrixField)
{
	// Matrices are encoded as row-major, nested arrays
	auto m = repo::test::utils::mesh::makeTransform(true, true);

	mongo::BSONObjBuilder builder;
	builder.append("matrix", makeMatrixObj(m));
	RepoBSON bson(builder.obj());
	EXPECT_THAT(bson.getMatrixField("matrix"), Eq(m));
}

TEST(RepoBSONTest, GetFloatVector)
{
	// This is typically used to intiialise a 2d or 3d vector, though for now
	// this is not enforced and it will return fields of arbitrary length

	{
		std::vector<float> arr;
		for (int i = 0; i < 10; i++)
		{
			arr.push_back(rand() / 1.23);
		}

		mongo::BSONObjBuilder builder;
		builder.append("vector", makeBsonArray(arr));
		RepoBSON bson(builder.obj());

		EXPECT_THAT(bson.getFloatVectorField("vector"), Eq(arr));
	}

	// Undernath, this retrieves values as doubles, so it should also work for
	// double arrays with ranges inside those of floats

	{
		std::vector<double> arr;
		for (int i = 0; i < 10; i++)
		{
			arr.push_back(rand() / 1.23);
		}

		mongo::BSONObjBuilder builder;
		builder.append("vector", makeBsonArray(arr));
		RepoBSON bson(builder.obj());

		EXPECT_THAT(bson.getFloatVectorField("vector"), ElementsAreArray(arr));
	}

	// The cast means it will silently fail if the doubles are outside the range
	// of a float however - this is accepted and it is expected the caller is aware
	// of this and will use the correct method
	{
		std::vector<double> arr;
		arr.push_back(DBL_MAX);
		arr.push_back(DBL_MIN);

		mongo::BSONObjBuilder builder;
		builder.append("vector", makeBsonArray(arr));
		RepoBSON bson(builder.obj());

		EXPECT_THAT(bson.getFloatVectorField("vector"), ElementsAreArray(arr));
	}

	// As these are intended to be used with vectors, they differ in the usual way
	// from the 'Array' methods, in that if the field is missing it will throw,
	// in addition to throwing if the type is wrong
	{
		std::vector<std::string> arr;
		arr.push_back(repo::lib::RepoUUID::createUUID().toString());

		mongo::BSONObjBuilder builder;
		builder.append("vector", makeBsonArray(arr));
		RepoBSON bson(builder.obj());

		EXPECT_THROW({ bson.getFloatVectorField("vector"); }, repo::lib::RepoFieldTypeException);
	}

	{
		mongo::BSONObjBuilder builder;
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getFloatVectorField("vector"); }, repo::lib::RepoFieldNotFoundException);
	}

}

TEST(RepoBSONTest, GetDoubleVectorField)
{
	// This is typically used to intialise a 2d or 3d vector, though for now
	// this is not enforced and it will return fields of arbitrary length

	// Should return floats as doubles
	{
		std::vector<float> arr;
		for (int i = 0; i < 10; i++)
		{
			arr.push_back(rand() / 1.23);
		}
		arr.push_back(FLT_MIN);
		arr.push_back(FLT_MAX);

		mongo::BSONObjBuilder builder;
		builder.append("vector", makeBsonArray(arr));
		RepoBSON bson(builder.obj());

		EXPECT_THAT(bson.getDoubleVectorField("vector"), ElementsAreArray(arr));
	}

	{
		std::vector<double> arr;
		for (int i = 0; i < 10; i++)
		{
			arr.push_back(rand() / 1.23);
		}
		arr.push_back(DBL_MIN);
		arr.push_back(DBL_MAX);

		mongo::BSONObjBuilder builder;
		builder.append("vector", makeBsonArray(arr));
		RepoBSON bson(builder.obj());

		EXPECT_THAT(bson.getDoubleVectorField("vector"), Eq(arr));
	}

	{
		std::vector<int> arr;
		for (int i = 0; i < 10; i++)
		{
			arr.push_back(rand());
		}

		mongo::BSONObjBuilder builder;
		builder.append("vector", makeBsonArray(arr));
		RepoBSON bson(builder.obj());

		EXPECT_THAT(bson.getDoubleVectorField("vector"), ElementsAreArray(arr));
	}

	// As these are intended to be used with vectors, they differ in the usual way
	// from the 'Array' methods, in that if the field is missing it will throw,
	// in addition to throwing if the type is wrong
	{
		std::vector<std::string> arr;
		arr.push_back(repo::lib::RepoUUID::createUUID().toString());

		mongo::BSONObjBuilder builder;
		builder.append("vector", makeBsonArray(arr));
		RepoBSON bson(builder.obj());

		EXPECT_THROW({ bson.getDoubleVectorField("vector"); }, repo::lib::RepoFieldTypeException);
	}

	{
		mongo::BSONObjBuilder builder;
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getDoubleVectorField("vector"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, GetFileList)
{
	// This is effectively thes same as getStringArray - it is intended to be used with the rFile field.

	std::vector<std::string> files = {
		repo::lib::RepoUUID::createUUID().toString(),
		repo::lib::RepoUUID::createUUID().toString(),
		repo::lib::RepoUUID::createUUID().toString() + "_rvt",
	};

	{
		mongo::BSONObjBuilder builder;
		builder.append("rFile", makeBsonArray(files));
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getFileList("rFile"), Eq(files));
	}

	// If there is no field, this method should throw an exception
	{
		mongo::BSONObjBuilder builder;
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getFileList("rFile"); }, repo::lib::RepoFieldNotFoundException);
	}

	// Or if it is the wrong type
	{
		mongo::BSONObjBuilder builder;

		builder.append("rFile", makeBsonArray(std::vector<int>({
			1,
			2,
			3
		})));

		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getFileList("rFile"); }, repo::lib::RepoFieldTypeException);
	}
}

TEST(RepoBSONTest, GetDoubleField)
{
	{
		mongo::BSONObjBuilder builder;
		builder.append("double", 1.29348);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getDoubleField("double"), Eq(1.29348));
	}

	// Double field should also return floats (but not integers).
	{
		mongo::BSONObjBuilder builder;
		builder.append("double", (float)1.29348);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getDoubleField("double"), Eq(1.29348));
	}

	// This is because doubles can contain floats losslessly, but not do the same
	// for integers.
	{
		mongo::BSONObjBuilder builder;
		builder.append("double", (long long)1);
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getDoubleField("double"); }, repo::lib::RepoFieldTypeException);
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("double", 0);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getDoubleField("double"), Eq(0)); // Get the 'empty' value - not throwing based on an empty field
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("double", true);
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getDoubleField("double"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getDoubleField("double"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, GetLongField)
{
	{
		mongo::BSONObjBuilder builder;
		builder.append("long", (long long)LONG_MAX);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getLongField("long"), Eq(LONG_MAX));
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("long", 0);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getLongField("long"), Eq(0)); // Get the 'empty' value - not throwing based on an empty field
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("long", true);
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getLongField("long"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getLongField("long"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, IsEmpty)
{
	{
		mongo::BSONObjBuilder builder;
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.isEmpty(), IsTrue());
	}

	// Only regular fields
	{
		mongo::BSONObjBuilder builder;
		builder.append("field", "value");
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.isEmpty(), IsFalse());
	}

	// Only big files
	{
		mongo::BSONObjBuilder builder;
		RepoBSON::BinMapping map;
		map["bin"] = makeRandomBinary();
		RepoBSON bson(builder.obj(), map);
		EXPECT_THAT(bson.isEmpty(), IsFalse());
	}
}

TEST(RepoBSONTest, GetIntField)
{
	{
		mongo::BSONObjBuilder builder;
		builder.append("int", (int)INT_MIN);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getLongField("int"), Eq(INT_MIN));
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("int", 0);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getLongField("int"), Eq(0)); // Get the 'empty' value - not throwing based on an empty field
	}

	{
		mongo::BSONObjBuilder builder;
		builder.append("int", true);
		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getLongField("int"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getLongField("int"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, GetStringArray)
{
	std::vector<std::string> files = {
		repo::lib::RepoUUID::createUUID().toString(),
		repo::lib::RepoUUID::createUUID().toString(),
		repo::lib::RepoUUID::createUUID().toString(),
	};

	{
		mongo::BSONObjBuilder builder;
		builder.append("strings", makeBsonArray(files));
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getStringArray("strings"), Eq(files));
	}

	// If there is no field, this method should return an empty array
	{
		mongo::BSONObjBuilder builder;
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.getFileList("strings"), IsEmpty());
	}

	// Or if it is the wrong type
	{
		mongo::BSONObjBuilder builder;

		builder.append("strings", makeBsonArray(std::vector<int>({
			1,
			2,
			3
			})));

		RepoBSON bson(builder.obj());
		EXPECT_THROW({ bson.getFileList("strings"); }, repo::lib::RepoFieldTypeException);
	}
}

TEST(RepoBSONTest, HasBinField)
{
	RepoBSON::BinMapping map;
	map["bin1"] = makeRandomBinary();

	mongo::BSONObjBuilder builder;
	builder.append("bin2", 1000);

	RepoBSON bson(builder.obj(), map);

	EXPECT_THAT(bson.hasBinField("bin1"), IsTrue());
	EXPECT_THAT(bson.hasBinField("bin2"), IsFalse());
	EXPECT_THAT(bson.hasBinField("bin3"), IsFalse());
}

TEST(RepoBSONTest, EqualityOperator)
{
	// The equality operator should compare the serialised state,
	// including the binary mappings

	{
		RepoBSON empty1;
		RepoBSON empty2;
		EXPECT_THAT(empty1, Eq(empty2));
	}

	{
		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.append("f1", "1");
		b.append("f1", "1");
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", 1);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", "1");
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.append("f1", (int)1);
		b.append("f1", (int)1);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", "1");
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", (int)1);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.append("f1", (double)1);
		b.append("f1", (double)1);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", (int)1); // Equality operator should be sensitive to the type, even if the values would be interpreted identically
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", (double)1);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.append("f1", (long long)1);
		b.append("f1", (long long)1);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", (int)1); // Equality operator should be sensitive to the type, even if the values would be interpreted identically
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", (long long)1);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		auto uuid = repo::lib::RepoUUID::createUUID();
		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.append("f1", uuid);
		b.append("f1", uuid);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", repo::lib::RepoUUID::createUUID());
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", uuid);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		auto v = makeRepoVector();
		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.append("f1", v);
		b.append("f1", v);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", makeRepoVector());
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", v);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		auto v = makeRepoVectorObj(makeRepoVector()); // This tests with sub-documents
		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.append("f1", v);
		b.append("f1", v);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", makeRepoVectorObj(makeRepoVector()));
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", v);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		std::vector<int> arr({
			rand(),
			rand(),
			rand(),
			rand(),
			rand(),
			rand(),
		});

		auto v = makeRepoVectorObj(makeRepoVector()); // This tests with sub-documents
		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.appendArray("f1", arr);
		b.appendArray("f1", arr);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.appendArray("f1", std::vector<int>({ rand(), rand() }));
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.appendArray("f2", arr);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.append("f1", true);
		b.append("f1", true);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", false);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", true);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		auto t = getRandomTm();

		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.append("f1", t);
		b.append("f1", t);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", getRandomTm());
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", t);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}

	{
		auto bin = makeRandomBinary();

		RepoBSONBuilder a;
		RepoBSONBuilder b;
		a.appendLargeArray("f1", bin);
		b.appendLargeArray("f1", bin);
		EXPECT_THAT(a.obj(), Eq(b.obj()));

		RepoBSONBuilder c;
		c.appendLargeArray("f1", makeRandomBinary());
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.appendLargeArray("f2", bin);
		EXPECT_THAT(a.obj(), Not(Eq(c.obj())));
	}
}