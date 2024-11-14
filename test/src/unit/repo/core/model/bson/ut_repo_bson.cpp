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
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_matchers.h"
#include "../../../../repo_test_mesh_utils.h"

#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_element.h>
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_project_settings.h>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/json.hpp>


using namespace repo::core::model;
using namespace testing;

bsoncxx::document::value makeTestBSON()
{
	bsoncxx::builder::stream::document builder;
	builder << "ice" << "lolly" << "amount" << 100.0;
	return builder.extract();
}

static auto mongoTestBSON = makeTestBSON();
static const RepoBSON testBson = RepoBSON(makeTestBSON());
static const RepoBSON emptyBson;

// This is the same definition as in winnt.h, defined here for gcc. There is also
// LLONG_MAX definition in climits - however we do *not* want any compiler
// specifics here. Anything in and out of the database should be bit-equivalent
// regardless of platform, so a concrete value is defined for testing.
#define MAXLONGLONG (0x7fffffffffffffff)

template<class T>
bsoncxx::array::value makeBsonArray(std::vector<T> a)
{
	bsoncxx::builder::basic::array arrbuilder;
	for (auto i = 0; i < a.size(); i++)
	{
		arrbuilder.append(a[i]);
	}
	return arrbuilder.extract();
}

template<>
bsoncxx::array::value makeBsonArray(std::vector<repo::lib::RepoUUID> uuids)
{
	bsoncxx::builder::basic::array arrbuilder;
	for (auto i = 0; i < uuids.size(); i++)
	{
		arrbuilder.append(
			bsoncxx::types::b_binary{
				bsoncxx::binary_sub_type::k_uuid_deprecated, // This is the same as mongo::bdtUUID
				(uint32_t)uuids[i].data().size(),
				uuids[i].data().data()
			}
		);
	}
	return arrbuilder.extract();
}

bsoncxx::document::value makeRepoVectorObj(const repo::lib::RepoVector3D& v)
{
	bsoncxx::builder::basic::document builder;
	builder.append(bsoncxx::builder::basic::kvp("x", v.x));
	builder.append(bsoncxx::builder::basic::kvp("y", v.y));
	builder.append(bsoncxx::builder::basic::kvp("z", v.z));
	return builder.extract();
}

bsoncxx::document::value makeBoundsObj(const repo::lib::RepoVector3D64& min, const repo::lib::RepoVector3D64& max)
{
	bsoncxx::builder::basic::document builder;
	builder.append(bsoncxx::builder::basic::kvp("0", makeBsonArray(min.toStdVector())));
	builder.append(bsoncxx::builder::basic::kvp("1", makeBsonArray(max.toStdVector())));
	return builder.extract();
}

bsoncxx::document::value makeMatrixObj(const repo::lib::RepoMatrix& m)
{
	auto d = m.getData();
	bsoncxx::builder::basic::document builder;
	builder.append(bsoncxx::builder::basic::kvp("0", makeBsonArray(std::vector<float>({ d[0], d[1], d[2], d[3]}))));
	builder.append(bsoncxx::builder::basic::kvp("1", makeBsonArray(std::vector<float>({ d[4], d[5], d[6], d[7] }))));
	builder.append(bsoncxx::builder::basic::kvp("2", makeBsonArray(std::vector<float>({ d[8], d[9], d[10], d[11] }))));
	builder.append(bsoncxx::builder::basic::kvp("3", makeBsonArray(std::vector<float>({ d[12], d[13], d[14], d[15] }))));
	return builder.extract();
}

/**
* Construct from mongo builder and mongo bson should give me the same bson
*/
TEST(RepoBSONTest, ConstructFromMongo)
{
	bsoncxx::document::value mongoObj = makeTestBSON();
	bsoncxx::builder::stream::document builder;
	builder << "ice" << "lolly";
	builder << "amount" << 100;

	bsoncxx::builder::stream::document different;
	different << "something" << "different";

	RepoBSON bson1(mongoObj);
	RepoBSON bson2(builder);
	RepoBSON bsonDiff(different);

	EXPECT_EQ(bson1, bson2);
	EXPECT_EQ(bson1.toString(), bson2.toString());
	EXPECT_NE(bson1, bsonDiff);
}

TEST(RepoBSONTest, ConstructFromMongoSizeExceeds) {
	std::string msgData;
	msgData.resize(1024 * 1024 * 65);

	ASSERT_ANY_THROW({
		bsoncxx::builder::stream::document builder;
		builder << "message" << msgData;
		auto bson = builder.extract();
	});

	try {
		bsoncxx::builder::stream::document builder;
		builder << "message" << msgData;
		auto bson = builder.extract();
	}
	catch (const std::exception &e) {
		EXPECT_NE(std::string(e.what()).find("BufBuilder"), std::string::npos);
	}
}

TEST(RepoBSONTest, Fields)
{
	// For historical reasons, hasField should return only the 'true'
	// BSON fields and not bin mapped fields

	RepoBSON::BinMapping map;
	map["bin1"] = makeRandomBinary();

	bsoncxx::builder::basic::document builder;

	builder.append(bsoncxx::builder::basic::kvp("stringField", "string"));
	builder.append(bsoncxx::builder::basic::kvp("doubleField", (double)0.123));

	RepoBSON bson(builder.extract(), map);

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

	EXPECT_THROW({
		emptyBson.getField("hello");
	},
	repo::lib::RepoFieldNotFoundException);
}

TEST(RepoBSONTest, AssignOperator)
{
	// Simple object with no mappings

	EXPECT_THAT(testBson.hasOversizeFiles(), IsFalse()); // Sanity check the source test bson is as we think it is

	RepoBSON test = testBson;
	EXPECT_THAT(test, Eq(testBson));

	RepoBSON::BinMapping map;
	map["field1"] = makeRandomBinary();
	map["field2"] = makeRandomBinary();

	RepoBSON test2(testBson, map);

	// With mappings

	EXPECT_THAT(test2.toString(), Eq(testBson.toString()));
	EXPECT_THAT(test2.getFilesMapping(), Eq(map));

	RepoBSON test3 = test2;
	EXPECT_THAT(test3, Eq(test2));

	// The references are not the same (though, both being immutable, they might
	// as well be...)

	EXPECT_THAT(&test3 == &test2, IsFalse());
}

TEST(RepoBSONTest, GetUUIDField)
{
	repo::lib::RepoUUID uuid = repo::lib::RepoUUID::createUUID();

	// Test legacy uuid
	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("uuid", bsoncxx::types::b_binary{
				bsoncxx::binary_sub_type::k_uuid_deprecated, // This is the same as mongo::bdtUUID
				(uint32_t)uuid.data().size(),
				(uint8_t*)uuid.data().data()
			}
		));
		RepoBSON bson = RepoBSON(builder.extract());
		EXPECT_EQ(uuid, bson.getUUIDField("uuid"));
		EXPECT_THROW({ bson.getUUIDField("empty"); }, repo::lib::RepoFieldNotFoundException);
	}

	// Test new UUID
	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("uuid", bsoncxx::types::b_binary{
				bsoncxx::binary_sub_type::k_uuid, // This is the same as mongo::newUUID
				(uint32_t)uuid.data().size(),
				(uint8_t*)uuid.data().data()
			}
		));

		RepoBSON bson = RepoBSON(builder.extract());
		EXPECT_EQ(uuid, bson.getUUIDField("uuid"));
		EXPECT_THROW({ bson.getUUIDField("empty"); }, repo::lib::RepoFieldNotFoundException);
	}

	// Test type checking
	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("uuid", bsoncxx::types::b_binary{
				bsoncxx::binary_sub_type::k_user, // This is the same as mongo::bdtCustom
				(uint32_t)uuid.data().size(),
				(uint8_t*)uuid.data().data()
			}
		));
		RepoBSON bson = RepoBSON(builder.extract());
		EXPECT_THROW({ bson.getUUIDField("uuid"); }, repo::lib::RepoFieldTypeException);
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("uuid", uuid.toString()));

		RepoBSON bson = RepoBSON(builder.extract());
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

	bsoncxx::builder::basic::document builder;
	builder.append(bsoncxx::builder::basic::kvp("uuids", makeBsonArray(uuids)));
	RepoBSON bson = RepoBSON(builder.extract());
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

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("array", makeBsonArray(arr)));

		RepoBSON bson = RepoBSON(builder.extract());

		EXPECT_THAT(bson.getFloatArray("array"), ElementsAreArray(arr));
		EXPECT_THAT(bson.getFloatArray("none"), IsEmpty());
	}

	// We expect a type error if the field is an array, but of the wrong type

	{
		std::vector<std::string> arr;
		bsoncxx::builder::basic::document builder;

		arr.reserve(size);
		for (size_t i = 0; i < size; ++i)
		{
			arr.push_back(std::to_string((float)rand() / 100.));
		}

		builder.append(bsoncxx::builder::basic::kvp("array", makeBsonArray(arr)));

		RepoBSON bson = RepoBSON(builder.extract());

		EXPECT_THROW({ bson.getFloatArray("array"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THAT(bson.getFloatArray("none"), IsEmpty());
	}
}

TEST(RepoBSONTest, GetTimeStampField)
{
	auto now = std::chrono::system_clock::now();
	bsoncxx::types::b_date date(now);

	// For comparison, get the date in posix ticks (seconds since the unix epoch)
	auto posixTimestamp = std::chrono::system_clock::to_time_t(now);

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("ts", date));
		RepoBSON bson = RepoBSON(builder.extract());
		EXPECT_THAT(bson.getTimeStampField("ts"), Eq(posixTimestamp));
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("ts", "hello"));
		RepoBSON bson = RepoBSON(builder.extract());
		EXPECT_THROW({ bson.getTimeStampField("ts"); }, repo::lib::RepoFieldTypeException);
	}

	{
		bsoncxx::builder::basic::document builder;
		RepoBSON bson = RepoBSON(builder.extract());
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

		EXPECT_THAT(bson.getBinary("blah"), Eq(mapping["blah"]));
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
		EXPECT_THROW({ bson.getBinaryFieldAsVector("ice", out); }, repo::lib::RepoFieldTypeException);
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
	// big files array will join the two, and prefer the *new* mapping

	bsoncxx::builder::basic::document builder;
	builder.append(bsoncxx::builder::basic::kvp("field", "value"));

	RepoBSON::BinMapping existing;
	existing["bin1"] = makeRandomBinary();
	existing["bin2"] = makeRandomBinary();

	RepoBSON::BinMapping additional;
	additional["bin2"] = makeRandomBinary();
	additional["bin3"] = makeRandomBinary();

	RepoBSON bson(builder.extract(), existing);

	EXPECT_THAT(bson.getBinary("bin1"), Eq(existing["bin1"]));
	EXPECT_THAT(bson.getBinary("bin2"), Eq(existing["bin2"]));
	EXPECT_THROW({ bson.getBinary("bin3"); }, repo::lib::RepoFieldNotFoundException);

	RepoBSON bson2(bson, additional);

	EXPECT_THAT(bson2.getBinary("bin1"), Eq(existing["bin1"]));
	EXPECT_THAT(bson2.getBinary("bin2"), Eq(additional["bin2"]));
	EXPECT_THAT(bson2.getBinary("bin3"), Eq(additional["bin3"]));
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
	builder.append("u", u);
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
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("bool", true));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getBoolField("bool"), Eq(true));
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("bool", false));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getBoolField("bool"), Eq(false)); // Get the value false - not throwing based on not finding the field
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("bool", "string"));
		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getBoolField("bool"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getBoolField("none"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, GetStringField)
{
	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("string", "value"));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getStringField("string"), Eq("value"));
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("string", ""));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getStringField("string"), Eq("")); // Get the 'empty' value - not throwing based on an empty field
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("string", true));
		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getStringField("string"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getStringField("none"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, GetObjectField)
{
	// A single object
	{
		bsoncxx::builder::basic::document sub;
		sub.append(bsoncxx::builder::basic::kvp("string", "value"));
		sub.append(bsoncxx::builder::basic::kvp("int", 0));
		auto subObj = sub.extract();
		auto subObjString = bsoncxx::to_json(subObj);

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("object", subObj));

		RepoBSON bson(builder.extract());

		EXPECT_THAT(bson.getObjectField("object").toString(), Eq(subObjString));
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

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("object", makeBsonArray(strings)));

		RepoBSON bson(builder.extract());
		auto o = bson.getObjectField("object");

		for (int i = 0; i < strings.size(); i++)
		{
			EXPECT_THAT(o.getStringField(std::to_string(i)), Eq(strings[i]));
		}
	}

	// A complex array of objects
	{
		bsoncxx::builder::basic::document obj1;
		std::vector<std::string> strings = {
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
		};
		obj1.append(bsoncxx::builder::basic::kvp("array", makeBsonArray(strings)));
		obj1.append(bsoncxx::builder::basic::kvp("field", "value"));

		bsoncxx::builder::basic::document obj2;
		std::vector<repo::lib::RepoUUID> uuids = {
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
		};
		obj2.append(bsoncxx::builder::basic::kvp("array", makeBsonArray(uuids)));
		obj2.append(bsoncxx::builder::basic::kvp("field", "value"));

		bsoncxx::builder::basic::document obj3;
		obj3.append(bsoncxx::builder::basic::kvp("field", "value"));
		obj3.append(bsoncxx::builder::basic::kvp("field2", 0));

		std::vector<bsoncxx::document::value> objs = {
			obj1.extract(),
			obj2.extract(),
			obj3.extract()
		};

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("object", makeBsonArray(objs)));

		RepoBSON bson(builder.extract());
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
		bsoncxx::builder::basic::document obj1;
		std::vector<std::string> strings = {
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
			repo::lib::RepoUUID::createUUID().toString(),
		};
		obj1.append(bsoncxx::builder::basic::kvp("array", makeBsonArray(strings)));
		obj1.append(bsoncxx::builder::basic::kvp("field", "value"));

		bsoncxx::builder::basic::document obj2;
		std::vector<repo::lib::RepoUUID> uuids = {
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
		};
		obj2.append(bsoncxx::builder::basic::kvp("array", makeBsonArray(uuids)));
		obj2.append(bsoncxx::builder::basic::kvp("field", "value"));
		obj2.append(bsoncxx::builder::basic::kvp("obj1", obj1.extract()));

		bsoncxx::builder::basic::document obj3;
		obj3.append(bsoncxx::builder::basic::kvp("field", "value"));
		obj3.append(bsoncxx::builder::basic::kvp("obj2", obj2.extract()));

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("object", obj3.extract()));

		RepoBSON bson(builder.extract());
		auto o3 = bson.getObjectField("object");

		EXPECT_THAT(o3.getStringField("field"), Eq("value"));
		EXPECT_THAT(o3.getObjectField("obj2").getStringField("field"), Eq("value"));
		EXPECT_THAT(o3.getObjectField("obj2").getUUIDFieldArray("array"), Eq(uuids));
		EXPECT_THAT(o3.getObjectField("obj2").getObjectField("obj1").getStringField("field"), Eq("value"));
		EXPECT_THAT(o3.getObjectField("obj2").getObjectField("obj1").getStringArray("array"), Eq(strings));
	}

	{
		bsoncxx::builder::basic::document builder;
		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getObjectField("object"); }, repo::lib::RepoFieldNotFoundException);
	}

	// Should throw if this is a primitive type - objects are explicitly BSON
	// structures; if a caller is looking for something that encapsulates both
	// primitive and object types, they should use getField to return a
	// RepoBSONElement.
	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("object", "string"));
		RepoBSON bson(builder.extract());
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

	bsoncxx::builder::basic::document builder;
	builder.append(bsoncxx::builder::basic::kvp("bounds", bounds));

	RepoBSON bson(builder.extract());

	auto expected = std::vector<repo::lib::RepoVector3D>({
		repo::lib::RepoVector3D((float)min.x, (float)min.y, (float)min.z),
		repo::lib::RepoVector3D((float)max.x, (float)max.y, (float)max.z),
	});

	EXPECT_THAT(bson.getBounds3D("bounds"), Eq(expected));
}

TEST(RepoBSONTest, GetVector3DField)
{
	// RepoVectors are encoded by component names instead of indices

	bsoncxx::builder::basic::document builder;
	auto v = makeRepoVector();
	builder.append(bsoncxx::builder::basic::kvp("vector", makeRepoVectorObj(v)));
	RepoBSON bson(builder.extract());
	EXPECT_THAT(bson.getVector3DField("vector"), Eq(v));
}

TEST(RepoBSONTest, GetMatrixField)
{
	// Matrices are encoded as row-major, nested arrays
	auto m = repo::test::utils::mesh::makeTransform(true, true);

	bsoncxx::builder::basic::document builder;
	builder.append(bsoncxx::builder::basic::kvp("matrix", makeMatrixObj(m)));
	RepoBSON bson(builder.extract());
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

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("vector", makeBsonArray(arr)));
		RepoBSON bson(builder.extract());

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

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("vector", makeBsonArray(arr)));
		RepoBSON bson(builder.extract());

		EXPECT_THAT(bson.getFloatVectorField("vector"), ElementsAreArray(arr));
	}

	// The cast means it will silently fail if the doubles are outside the range
	// of a float however - this is accepted and it is expected the caller is aware
	// of this and will use the correct method
	{
		std::vector<double> arr;
		arr.push_back(DBL_MAX);
		arr.push_back(DBL_MIN);

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("vector", makeBsonArray(arr)));
		RepoBSON bson(builder.extract());

		EXPECT_THAT(bson.getFloatVectorField("vector"), ElementsAreArray(arr));
	}

	// As these are intended to be used with vectors, they differ in the usual way
	// from the 'Array' methods, in that if the field is missing it will throw,
	// in addition to throwing if the type is wrong
	{
		std::vector<std::string> arr;
		arr.push_back(repo::lib::RepoUUID::createUUID().toString());

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("vector", makeBsonArray(arr)));
		RepoBSON bson(builder.extract());

		EXPECT_THROW({ bson.getFloatVectorField("vector"); }, repo::lib::RepoFieldTypeException);
	}

	{
		bsoncxx::builder::basic::document builder;
		RepoBSON bson(builder.extract());
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

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("vector", makeBsonArray(arr)));
		RepoBSON bson(builder.extract());

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

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("vector", makeBsonArray(arr)));
		RepoBSON bson(builder.extract());

		EXPECT_THAT(bson.getDoubleVectorField("vector"), Eq(arr));
	}

	// Currently, reinterpreting an integer array as a double array is not supported
	// and will throw. It is possible to store 32 bit integers in a double losslessly,
	// (though not 64 bit ones) so this could be changed in the future.
	{
		std::vector<int> arr;
		for (int i = 0; i < 10; i++)
		{
			arr.push_back(rand());
		}

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("vector", makeBsonArray(arr)));
		RepoBSON bson(builder.extract());

		EXPECT_THROW({ bson.getDoubleVectorField("vector"); }, repo::lib::RepoFieldTypeException);
	}

	// As these are intended to be used with vectors, they differ in the usual way
	// from the 'Array' methods, in that if the field is missing it will throw,
	// in addition to throwing if the type is wrong
	{
		std::vector<std::string> arr;
		arr.push_back(repo::lib::RepoUUID::createUUID().toString());

		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("vector", makeBsonArray(arr)));
		RepoBSON bson(builder.extract());

		EXPECT_THROW({ bson.getDoubleVectorField("vector"); }, repo::lib::RepoFieldTypeException);
	}

	{
		bsoncxx::builder::basic::document builder;
		RepoBSON bson(builder.extract());
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
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("rFile", makeBsonArray(files)));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getFileList("rFile"), Eq(files));
	}

	// If there is no field, this method should throw an exception
	{
		bsoncxx::builder::basic::document builder;
		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getFileList("rFile"); }, repo::lib::RepoFieldNotFoundException);
	}

	// Or if it is the wrong type
	{
		bsoncxx::builder::basic::document builder;

		builder.append(bsoncxx::builder::basic::kvp("rFile", makeBsonArray(std::vector<int>({
			1,
			2,
			3
		}))));

		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getFileList("rFile"); }, repo::lib::RepoFieldTypeException);
	}
}

TEST(RepoBSONTest, GetDoubleField)
{
	{
		double d = 1.229348;
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("double", d));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getDoubleField("double"), Eq(d));
	}

	// Double field should also return floats (but not integers).
	{
		float d = 1.229348;
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("double", d));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getDoubleField("double"), Eq(d));
	}

	// This is because doubles can contain floats losslessly, but not do the same
	// for integers.
	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("double", (long long)1));
		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getDoubleField("double"); }, repo::lib::RepoFieldTypeException);
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("double", (double)0));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getDoubleField("double"), Eq(0));
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("double", true));
		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getDoubleField("double"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getDoubleField("none"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, GetLongField)
{
	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("long", (long long)MAXLONGLONG));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getLongField("long"), Eq(MAXLONGLONG));
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("long", 0));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getLongField("long"), Eq(0));
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("long", true));
		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getLongField("long"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getLongField("none"); }, repo::lib::RepoFieldNotFoundException);
	}
}

TEST(RepoBSONTest, IsEmpty)
{
	{
		bsoncxx::builder::basic::document builder;
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.isEmpty(), IsTrue());
	}

	// Only regular fields
	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("field", "value"));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.isEmpty(), IsFalse());
	}

	// Only big files
	{
		bsoncxx::builder::basic::document builder;
		RepoBSON::BinMapping map;
		map["bin"] = makeRandomBinary();
		RepoBSON bson(builder.extract(), map);
		EXPECT_THAT(bson.isEmpty(), IsFalse());
	}
}

TEST(RepoBSONTest, GetIntField)
{
	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("int", (int)INT_MIN));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getIntField("int"), Eq(INT_MIN));
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("int", 0));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getIntField("int"), Eq(0));
	}

	{
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("int", true));
		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getIntField("int"); }, repo::lib::RepoFieldTypeException);
		EXPECT_THROW({ bson.getIntField("none"); }, repo::lib::RepoFieldNotFoundException);
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
		bsoncxx::builder::basic::document builder;
		builder.append(bsoncxx::builder::basic::kvp("strings", makeBsonArray(files)));
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getStringArray("strings"), Eq(files));
	}

	// In contrast to the other array methods, if there is no field, this method
	// should return an empty array and *not throw*
	{
		bsoncxx::builder::basic::document builder;
		RepoBSON bson(builder.extract());
		EXPECT_THAT(bson.getStringArray("strings"), IsEmpty());
	}

	// Or if it is the wrong type
	{
		bsoncxx::builder::basic::document builder;

		builder.append(bsoncxx::builder::basic::kvp("strings", makeBsonArray(std::vector<int>({
			1,
			2,
			3
			}))));

		RepoBSON bson(builder.extract());
		EXPECT_THROW({ bson.getStringArray("strings"); }, repo::lib::RepoFieldTypeException);
	}
}

TEST(RepoBSONTest, HasBinField)
{
	RepoBSON::BinMapping map;
	map["bin1"] = makeRandomBinary();

	bsoncxx::builder::basic::document builder;
	builder.append(bsoncxx::builder::basic::kvp("bin2", 1000));

	RepoBSON bson(builder.extract(), map);

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
		RepoBSONBuilder _a;
		_a.append("f1", "1");
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.append("f1", "1");
		EXPECT_THAT(a, Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", 1);
		EXPECT_THAT(a, Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", "1");
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}

	{
		RepoBSONBuilder _a;
		_a.append("f1", (int)1);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.append("f1", (int)1);
		EXPECT_THAT(a, Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", "1");
		EXPECT_THAT(a, Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", (int)1);
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}

	{
		RepoBSONBuilder _a;
		_a.append("f1", (double)1);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.append("f1", (double)1);
		EXPECT_THAT(a, Eq(b.obj()));

		// This case is commented out for the moment. The Mongo Driver is sensitive
		// to type when reading, but in this case the in-built equality operator
		// is not considering a difference when the cast is lossless.
		// Wait and see what the new driver will do.
		/*
		RepoBSONBuilder c;
		c.append("f1", (int)1); // Equality operator should be sensitive to the type, even if the values would be interpreted identically
		EXPECT_THAT(a, Not(Eq(c.extract())));
		*/

		RepoBSONBuilder d;
		d.append("f2", (double)1);
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}

	{
		RepoBSONBuilder _a;
		_a.append("f1", (long long)1);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.append("f1", (long long)1);
		EXPECT_THAT(a, Eq(b.obj()));

		// This case is commented out for the moment. The Mongo Driver is sensitive
		// to type when reading, but in this case the in-built equality operator
		// is not considering a difference when the cast is lossless.
		// Wait and see what the new driver will do.
		/*
		RepoBSONBuilder c;
		c.append("f1", (int)1); // Equality operator should be sensitive to the type, even if the values would be interpreted identically
		EXPECT_THAT(a, Not(Eq(c.extract())));
		*/

		RepoBSONBuilder d;
		d.append("f2", (long long)1);
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}

	{
		auto uuid = repo::lib::RepoUUID::createUUID();
		RepoBSONBuilder _a;
		_a.append("f1", uuid);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.append("f1", uuid);
		EXPECT_THAT(a, Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", repo::lib::RepoUUID::createUUID());
		EXPECT_THAT(a, Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", uuid);
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}

	{
		auto v = makeRepoVector();
		RepoBSONBuilder _a;
		_a.append("f1", v);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.append("f1", v);
		EXPECT_THAT(a, Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", makeRepoVector());
		EXPECT_THAT(a, Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", v);
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}

	{
		auto v = makeRepoVectorObj(makeRepoVector()); // This tests with sub-documents
		RepoBSONBuilder _a;
		_a.append("f1", v);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.append("f1", v);
		EXPECT_THAT(a, Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", makeRepoVectorObj(makeRepoVector()));
		EXPECT_THAT(a, Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", v);
		EXPECT_THAT(a, Not(Eq(d.obj())));
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
		RepoBSONBuilder _a;
		_a.appendArray("f1", arr);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.appendArray("f1", arr);
		EXPECT_THAT(a, Eq(b.obj()));

		RepoBSONBuilder c;
		c.appendArray("f1", std::vector<int>({ rand(), rand() }));
		EXPECT_THAT(a, Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.appendArray("f2", arr);
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}

	{
		RepoBSONBuilder _a;
		_a.append("f1", true);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.append("f1", true);
		EXPECT_THAT(a, Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", false);
		EXPECT_THAT(a, Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", true);
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}

	{
		auto t = getRandomTm();
		RepoBSONBuilder _a;
		_a.append("f1", t);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.append("f1", t);
		EXPECT_THAT(a, Eq(b.obj()));

		RepoBSONBuilder c;
		c.append("f1", getRandomTm());
		EXPECT_THAT(a, Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.append("f2", t);
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}

	{
		auto bin = makeRandomBinary();
		RepoBSONBuilder _a;
		_a.appendLargeArray("f1", bin);
		auto a = _a.obj();
		EXPECT_THAT(a.isEmpty(), IsFalse());

		RepoBSONBuilder b;
		b.appendLargeArray("f1", bin);
		EXPECT_THAT(a, Eq(b.obj()));

		RepoBSONBuilder c;
		c.appendLargeArray("f1", makeRandomBinary());
		EXPECT_THAT(a, Not(Eq(c.obj())));

		RepoBSONBuilder d;
		d.appendLargeArray("f2", bin);
		EXPECT_THAT(a, Not(Eq(d.obj())));
	}
}