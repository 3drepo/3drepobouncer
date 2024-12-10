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
#include "repo/lib/datastructure/repo_variant.h"
#include "repo/lib/datastructure/repo_variant_utils.h"

#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;
using namespace testing;

repo::lib::RepoMatrix makeRepoMatrix()
{
	std::vector<float> d;
	for (int i = 0; i < 16; i++) {
		d.push_back(((float)rand() - (float)rand()) / (float)rand());
	}
	return repo::lib::RepoMatrix(d);
}

TEST(RepoBSONBuilderTest, ConstructBuilder)
{
	RepoBSONBuilder b;
	RepoBSONBuilder *b_ptr = new RepoBSONBuilder();
	delete b_ptr;
}

TEST(RepoBSONBuilderTest, AppendArray)
{
	{
		// Test empty array.

		std::vector<std::string> empty;
		RepoBSONBuilder builder;
		builder.appendArray("array", empty);
		RepoBSON bson(builder.obj());
		EXPECT_THAT(bson.isEmpty(), IsFalse());

		// Array should be empty, so any cast will work
		EXPECT_THAT(bson.getStringArray("array"), IsEmpty());
	}

	{
		// Test an array with one entry - should still get an indexed object

		std::vector<std::string> one({ "one" });
		RepoBSONBuilder builder;
		builder.appendArray("array", one);
		RepoBSON bson(builder.obj());

		auto a = bson.getStringArray("array");
		EXPECT_THAT(a.size(), Eq(1));
		EXPECT_THAT(a[0], Eq(std::string("one")));
	}

	{
		// A regular array

		std::vector<std::string> arr({ "hello", "bye" });

		RepoBSONBuilder builder;
		builder.appendArray("array", arr);
		RepoBSON bson(builder.obj());

		auto a = bson.getStringArray("array");
		EXPECT_THAT(a, Eq(arr));
	}

	{
		// A regular array of a different type

		std::vector<float> arr({ 1, 7, 10, 12382, 1238, 98981 });

		RepoBSONBuilder builder;
		builder.appendArray("array", arr);
		RepoBSON bson(builder.obj());

		auto a = bson.getFloatArray("array");
		EXPECT_THAT(a, Eq(arr));
	}

	{
		// A regular array of complex types - i.e. not basic primitive types, but
		// ones we do have overloads for in RepoBSONBuilder. For example, UUIDs
		// or matrices.

		std::vector<repo::lib::RepoUUID> arr({
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
			repo::lib::RepoUUID::createUUID(),
		});

		RepoBSONBuilder builder;
		builder.appendArray("array", arr);
		RepoBSON bson(builder.obj());

		auto a = bson.getUUIDFieldArray("array");
		EXPECT_THAT(a, Eq(arr));
	}

	{
		// An array of mixed documents

		RepoBSONBuilder a;
		a.append("a", "a");

		RepoBSONBuilder b;
		b.append("b", 1);

		RepoBSONBuilder c;
		c.append("c", repo::lib::RepoUUID::createUUID());

		RepoBSONBuilder d;
		d.append("d", repo::lib::RepoMatrix());

		std::vector<RepoBSON> arr({
			a.obj(),
			b.obj(),
			c.obj(),
			d.obj()
		});

		RepoBSONBuilder builder;
		builder.appendArray("array", arr);
		RepoBSON bson(builder.obj());

		auto documents = bson.getObjectArray("array");

		EXPECT_THAT(documents[0].getStringField("a"), Eq("a"));
		EXPECT_THAT(documents[1].getIntField("b"), Eq(1));
		EXPECT_THAT(documents[2].getUUIDField("c").isDefaultValue(), IsFalse());
		EXPECT_THAT(documents[3].getMatrixField("d").isIdentity(), IsTrue());
	}
}

TEST(RepoBSONBuilderTest, AppendGeneric)
{
	RepoBSONBuilder builder;

	std::string stringT = "string";
	bool boolT = false;
	double doubleT = (double)rand() - (double)rand() / (double)rand();
	float floatT = (float)rand() - (float)rand() / (float)rand();
	int intT = 64;
	int64_t longT = 123412452141L;
	repo::lib::RepoUUID uuidT = repo::lib::RepoUUID::createUUID();
	repo::lib::RepoVector3D repoVectorT = { 0.1234f, 1.2345f, 2.34567f };
	repo::lib::RepoMatrix repoMatrixT = makeRepoMatrix();
	tm tmT = getRandomTm();

	builder.append("string", stringT);
	builder.append("bool", boolT);
	builder.append("double", doubleT);
	builder.append("float", floatT);
	builder.append("int", intT);
	builder.append("long", longT);
	builder.append("uuid", uuidT);
	builder.append("repoVector", repoVectorT);
	builder.append("repoMatrix", repoMatrixT);
	builder.append("tm", tmT);

	RepoBSON bson = builder.obj();

	EXPECT_THAT(bson.getStringField("string"), Eq(stringT));
	EXPECT_THAT(bson.getBoolField("bool"), Eq(boolT));
	EXPECT_THAT(bson.getDoubleField("double"), Eq(doubleT));
	EXPECT_THAT(bson.getDoubleField("float"), Eq(floatT)); // Floats should be able to be read as doubles
	EXPECT_THAT(bson.getIntField("int"), Eq(intT));
	EXPECT_THAT(bson.getLongField("long"), Eq(longT));
	EXPECT_THAT(bson.getUUIDField("uuid"), Eq(uuidT));
	EXPECT_THAT(bson.getDoubleVectorField("repoVector"), ElementsAreArray(repoVectorT.toStdVector())); // RepoVectors through append should appear as arrays; use RepoVectorObject to get the document version
	EXPECT_THAT(bson.getMatrixField("repoMatrix"), Eq(repoMatrixT));
	EXPECT_THAT(bson.getTimeStampField("tm"), Eq(mktime(&tmT)));
}

TEST(RepoBSONBuilderTest, AppendRepoVectorObject)
{
	// Tests where the builder can assemble a Vector Object (i.e. a document with
	// x, y, z as field names instead of array indices).

	repo::lib::RepoVector3D vector = makeRepoVector();
	RepoBSONBuilder builder;
	builder.appendVector3DObject("vector", vector);
	RepoBSON bson = builder.obj();
	EXPECT_THAT(bson.getVector3DField("vector"), Eq(vector));
}

TEST(RepoBSONBuilderTest, AppendTimeStamp)
{
	RepoBSONBuilder builder;
	builder.appendTimeStamp("ts");
	RepoBSON bson = builder.obj();
	EXPECT_THAT(bson.getTimeStampField("ts"), IsNow());
}

TEST(RepoBSONBuilderTest, AppendTm)
{
	// Append time from a tm struct

	auto tm = getRandomTm();

	RepoBSONBuilder builder;
	builder.appendTime("ts", tm);
	RepoBSON bson = builder.obj();
	EXPECT_THAT(bson.getTimeStampField("ts"), mktime(&tm));
}

TEST(RepoBSONBuilderTest, AppendTime)
{
	// Append time from a unix epoch

	auto tm = getRandomTm();
	auto t = mktime(&tm);

	RepoBSONBuilder builder;
	builder.appendTime("ts", t);
	RepoBSON bson = builder.obj();
	EXPECT_THAT(bson.getTimeStampField("ts"), t);
}

TEST(RepoBSONBuilderTest, AppendRepoVariant)
{
	RepoBSONBuilder builder;

	repo::lib::RepoVariant vBool = false;
	repo::lib::RepoVariant vInt = 10;
	repo::lib::RepoVariant vLong = (int64_t)101LL;
	repo::lib::RepoVariant vDouble = 99.99;
	repo::lib::RepoVariant vString = std::string("string");
	repo::lib::RepoVariant vEmptyString = std::string("");
	repo::lib::RepoVariant vTime = getRandomTm();
	repo::lib::RepoVariant vUUID = repo::lib::RepoUUID::createUUID();

	builder.appendRepoVariant("vBool", vBool);
	builder.appendRepoVariant("vInt", vInt);
	builder.appendRepoVariant("vLong", vLong);
	builder.appendRepoVariant("vDouble", vDouble);
	builder.appendRepoVariant("vString", vString);
	builder.appendRepoVariant("vEmptyString", vEmptyString);
	builder.appendRepoVariant("vTime", vTime);
	builder.appendRepoVariant("vUUID", vUUID);

	RepoBSON bson(builder.obj());

	EXPECT_THAT(bson.getBoolField("vBool"), Eq(boost::get<bool>(vBool)));
	EXPECT_THAT(bson.getIntField("vInt"), Eq(boost::get<int>(vInt)));
	EXPECT_THAT(bson.getLongField("vLong"), Eq(boost::get<int64_t>(vLong)));
	EXPECT_THAT(bson.getDoubleField("vDouble"), Eq(boost::get<double>(vDouble)));
	EXPECT_THAT(bson.getStringField("vString"), Eq(boost::get<std::string>(vString)));
	EXPECT_THAT(bson.getStringField("vEmptyString"), Eq(std::string("")));
	EXPECT_THAT(bson.getTimeStampField("vTime"), Eq(mktime(&boost::get<tm>(vTime))));
	EXPECT_THAT(bson.getUUIDField("vUUID"), Eq(boost::get<repo::lib::RepoUUID>(vUUID)));
}

TEST(RepoBSONBuilderTest, AppendLargeArray)
{
	RepoBSONBuilder builder;
	auto bin = makeRandomBinary();
	builder.appendLargeArray("bin", bin);
	RepoBSON bson = builder.obj();
	EXPECT_THAT(bson.getBinary("bin"), Eq(bin));
}