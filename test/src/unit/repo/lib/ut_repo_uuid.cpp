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

#include <cstdlib>
#include <sstream>
#include <repo/lib/datastructure/repo_uuid.h>
#include <gtest/gtest.h>

#include <repo/core/model/bson/repo_bson_builder.h>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


using namespace repo::lib;

static boost::uuids::random_generator gen;

TEST(RepoUUIDTest, constructorTest)
{
	auto generatedID = RepoUUID::createUUID();
	mongo::BSONObjBuilder builder;

	boost::uuids::uuid id = gen();
	builder.appendBinData("uuid", id.size(), mongo::bdtUUID, (char*)id.data);
	auto bson = builder.obj();
	auto fromBsonEle(bson.getField("uuid"));
	
	RepoUUID fromDefault;

	RepoUUID fromBoost(id);
	RepoUUID fromAnotherRepoUUID(generatedID);
}

TEST(RepoUUIDTest, dataTest)
{
	boost::uuids::uuid id = gen();

	RepoUUID fromBoost(id);	
	auto data = fromBoost.data();
	EXPECT_EQ(0, memcmp(data.data(), id.data, data.size() * sizeof(*data.data())));

	RepoUUID fromAnotherRepoUUID(fromBoost);
	auto data2 = fromAnotherRepoUUID.data();
	EXPECT_EQ(0, memcmp(data2.data(), data.data(), data.size() * sizeof(*data.data())));

	repo::core::model::RepoBSONBuilder builder;
	builder.append("id", fromBoost);
	auto bson = builder.obj();
	RepoUUID fromEle = RepoUUID::fromBSONElement(bson.getField("id"));
	auto data3 = fromEle.data();
	EXPECT_EQ(0, memcmp(data3.data(), data.data(), data.size() * sizeof(*data.data())));

	RepoUUID fromString(fromBoost.toString());
	auto datas = fromString.data();
	EXPECT_EQ(0, memcmp(datas.data(), data.data(), data.size() * sizeof(*data.data())));

	RepoUUID fromDefault;
	auto data4 = fromDefault.data();
	EXPECT_NE(0, memcmp(data4.data(), data.data(), data.size() * sizeof(*data.data())));

	auto random = RepoUUID::createUUID();
	auto data5 = random.data();
	EXPECT_NE(0, memcmp(data5.data(), data.data(), data.size() * sizeof(*data.data())));
}

TEST(RepoUUIDTest, hashTest)
{
	boost::uuids::uuid id = gen();

	RepoUUID fromBoost(id);	
	EXPECT_EQ(fromBoost.getHash(), fromBoost.getHash());

	RepoUUID fromAnotherRepoUUID(fromBoost);
	EXPECT_EQ(fromBoost.getHash(), fromAnotherRepoUUID.getHash());

	repo::core::model::RepoBSONBuilder builder;
	builder.append("id", fromBoost);
	auto bson = builder.obj();
	RepoUUID fromEle = RepoUUID::fromBSONElement(bson.getField("id"));
	EXPECT_EQ(fromBoost.getHash(), fromEle.getHash());

	RepoUUID fromString(fromBoost.toString());
	EXPECT_EQ(fromBoost.getHash(), fromString.getHash());

	RepoUUID fromDefault;
	EXPECT_NE(fromBoost.getHash(), fromDefault.getHash());

	auto random = RepoUUID::createUUID();
	EXPECT_NE(fromBoost.getHash(), random.getHash());
}

TEST(RepoUUIDTest, toStringTest)
{
	boost::uuids::uuid id = gen();

	std::stringstream ss;
	ss << id;
	std::string idString = ss.str();

	RepoUUID fromBoost(id);
	EXPECT_EQ(idString, fromBoost.toString());

	RepoUUID fromAnotherRepoUUID(fromBoost);
	EXPECT_EQ(idString, fromAnotherRepoUUID.toString());

	repo::core::model::RepoBSONBuilder builder;
	builder.append("id", fromBoost);
	auto bson = builder.obj();
	RepoUUID fromEle = RepoUUID::fromBSONElement(bson.getField("id"));
	EXPECT_EQ(idString, fromEle.toString());

	RepoUUID fromString(fromBoost.toString());
	EXPECT_EQ(idString, fromString.toString());

	RepoUUID fromDefault;
	EXPECT_NE(idString, fromDefault.toString());
	EXPECT_EQ(RepoUUID::defaultValue, fromDefault.toString());

	auto random = RepoUUID::createUUID();
	EXPECT_NE(idString, random.toString());
}

TEST(RepoUUIDTest, assignmentOpTest)
{
	RepoUUID fromBoost(gen());
	RepoUUID equal = fromBoost;
	EXPECT_EQ(equal.toString(), fromBoost.toString());

	RepoUUID fromDefault;
	equal = fromDefault;
	EXPECT_NE(equal.toString(), fromBoost.toString());
	EXPECT_EQ(equal.toString(), fromDefault.toString());
}

TEST(RepoUUIDTest, streamOperatorTest)
{
	RepoUUID fromBoost(gen()), other(gen());
	std::stringstream ss;
	ss << fromBoost;
	std::string stringRep = ss.str();
	EXPECT_EQ(stringRep, fromBoost.toString());
	EXPECT_NE(stringRep, other.toString());
}

TEST(RepoUUIDTest, equalsOperatorTest)
{
	RepoUUID defaultA, defaultB, fromGenA(gen()), fromGenB(gen());

	EXPECT_TRUE(defaultA == defaultB);
	EXPECT_TRUE(defaultB == defaultA);
	EXPECT_TRUE(defaultA == defaultA);
	EXPECT_FALSE(defaultA == fromGenA);
	EXPECT_FALSE(fromGenA == defaultA);
	EXPECT_TRUE(fromGenA == fromGenA);
	EXPECT_FALSE(fromGenA == fromGenB);
	EXPECT_FALSE(fromGenB == fromGenA);
}

TEST(RepoUUIDTest, notEqualsOperatorTest)
{
	RepoUUID defaultA, defaultB, fromGenA(gen()), fromGenB(gen());

	EXPECT_FALSE(defaultA != defaultB);
	EXPECT_FALSE(defaultB != defaultA);
	EXPECT_FALSE(defaultA != defaultA);
	EXPECT_TRUE(defaultA != fromGenA);
	EXPECT_TRUE(fromGenA != defaultA);
	EXPECT_FALSE(fromGenA != fromGenA);
	EXPECT_TRUE(fromGenA != fromGenB);
	EXPECT_TRUE(fromGenB != fromGenA);
}

TEST(RepoUUIDTest, ltOperatorTest)
{
	RepoUUID defaultA, defaultB, fromGenA(gen()), fromGenB(gen());

	EXPECT_FALSE(defaultA < defaultB);
	EXPECT_FALSE(defaultB < defaultA);

	EXPECT_FALSE(defaultA < defaultA);

	EXPECT_TRUE(defaultA < fromGenA);
	EXPECT_FALSE(fromGenA < defaultA);

	EXPECT_TRUE(defaultA < fromGenB);
	EXPECT_FALSE(fromGenB < defaultA);

	if (fromGenA < fromGenB)
	{
		EXPECT_FALSE(fromGenB < fromGenA);
	}
	else
	{
		EXPECT_TRUE(fromGenB < fromGenA);
	}

}

TEST(RepoUUIDTest, gtOperatorTest)
{
	RepoUUID defaultA, defaultB, fromGenA(gen()), fromGenB(gen());

	EXPECT_FALSE(defaultA > defaultB);
	EXPECT_FALSE(defaultB > defaultA);

	EXPECT_FALSE(defaultA > defaultA);

	EXPECT_FALSE(defaultA > fromGenA);
	EXPECT_TRUE(fromGenA > defaultA);

	EXPECT_FALSE(defaultA > fromGenB);
	EXPECT_TRUE(fromGenB > defaultA);

	if (fromGenA > fromGenB)
	{
		EXPECT_FALSE(fromGenB > fromGenA);
	}
	else
	{
		EXPECT_TRUE(fromGenB > fromGenA);
	}

}

TEST(RepoUUIDTest, lteOperatorTest)
{
	RepoUUID defaultA, defaultB, fromGenA(gen()), fromGenB(gen());

	EXPECT_TRUE(defaultA <= defaultB);
	EXPECT_TRUE(defaultB <= defaultA);

	EXPECT_TRUE(defaultA <= defaultA);
	EXPECT_TRUE(fromGenA <= fromGenA);
	EXPECT_TRUE(fromGenB <= fromGenB);

	EXPECT_TRUE(defaultA <= fromGenA);
	EXPECT_FALSE(fromGenA <= defaultA);

	EXPECT_TRUE(defaultA <= fromGenB);
	EXPECT_FALSE(fromGenB <= defaultA);

	if (fromGenA <= fromGenB)
	{
		EXPECT_FALSE(fromGenB <= fromGenA);
	}
	else
	{
		EXPECT_TRUE(fromGenB <= fromGenA);
	}

}

TEST(RepoUUIDTest, gteOperatorTest)
{
	RepoUUID defaultA, defaultB, fromGenA(gen()), fromGenB(gen());

	EXPECT_TRUE(defaultA >= defaultB);
	EXPECT_TRUE(defaultB >= defaultA);

	EXPECT_TRUE(defaultA >= defaultA);
	EXPECT_TRUE(fromGenA <= fromGenA);
	EXPECT_TRUE(fromGenB <= fromGenB);

	EXPECT_FALSE(defaultA >= fromGenA);
	EXPECT_TRUE(fromGenA >= defaultA);

	EXPECT_FALSE(defaultA >= fromGenB);
	EXPECT_TRUE(fromGenB >= defaultA);

	if (fromGenA >= fromGenB)
	{
		EXPECT_FALSE(fromGenB >= fromGenA);
	}
	else
	{
		EXPECT_TRUE(fromGenB >= fromGenA);
	}

}