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

#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;

RepoBSON emptyBSON;

TEST(RepoBSONBuilderTest, ConstructBuilder)
{
	RepoBSONBuilder b;
	RepoBSONBuilder *b_ptr = new RepoBSONBuilder();
	delete b_ptr;
}

TEST(RepoBSONBuilderTest, AppendArray)
{
	std::vector<std::string> arr({"hello", "bye"});
	RepoBSON arrBson = BSON("1" << arr[0] << "2" << arr[1]);
	mongo::BSONObjBuilder mbuilder;
	mbuilder.appendArray("arrayTest", arrBson);
	RepoBSON b(mbuilder.obj());

	RepoBSONBuilder builder, builder2;
	builder.appendArray("arrayTest", arr);
	builder2.appendArray("arrayTest", RepoBSON(arrBson));

	RepoBSON arrayRepoBson = builder.obj();
	RepoBSON arrayRepoBson2 = builder2.obj();

	EXPECT_EQ(arrayRepoBson.toString(), b.toString());
	EXPECT_EQ(arrayRepoBson2.toString(), b.toString());

	//Sanity check: make sure everything is not equal to empty bson.
	EXPECT_NE(emptyBSON.toString(), arrayRepoBson.toString());
	//Sanity check: make sure appending an empty bson/vector doesn't crash
	RepoBSONBuilder testBuilder;

	testBuilder.appendArray("emptyTest", emptyBSON);
	testBuilder.appendArray("emptyTest2", std::vector<std::string>());
	
}

TEST(RepoBSONBuilderTest, AppendGeneric)
{
	RepoBSONBuilder builder;

	std::string stringT = "string";
	int intT = 64;
	long long longT = 123412452141L;
	float floatT = 1.2345678;
	repo::lib::RepoUUID idT = repo::lib::RepoUUID::createUUID();
	repo::lib::RepoVector3D rvecT = { 0.1234f, 1.2345f, 2.34567f };
	//TODO: add more if more comes up!


	builder.append("string", stringT);
	builder.append("int", intT);
	builder.append("long", longT);
	builder.append("float", floatT);
	builder.append("id", idT);
	builder.append("repoVector", rvecT);

	RepoBSON bson = builder.obj();

	EXPECT_EQ(bson.getStringField("string"), stringT);
	EXPECT_EQ(bson.getField("int").Int(), intT);
	EXPECT_EQ(bson.getField("long").Long(), longT);
	EXPECT_EQ(bson.getField("float").Double(), floatT);
	EXPECT_EQ(bson.getUUIDField("id"), idT);

	std::vector<float> rvecOut = bson.getFloatArray("repoVector");
	EXPECT_EQ(3, rvecOut.size());
	EXPECT_EQ(rvecT.x, rvecOut[0]);
	EXPECT_EQ(rvecT.y, rvecOut[1]);
	EXPECT_EQ(rvecT.z, rvecOut[2]);
	
}

TEST(RepoBSONBuilderTest, AppendUUID)
{
	repo::lib::RepoUUID uuid = repo::lib::RepoUUID::createUUID();
	RepoBSONBuilder builder;
	builder.append("uuidTest", uuid);
	RepoBSON bson = builder.obj();

	EXPECT_EQ(uuid, bson.getUUIDField("uuidTest"));

}

TEST(RepoBSONBuilderTest, AppendRepoVectorT)
{
	repo::lib::RepoVector3D vector3d = { 1.0, 2.0, 3.0 };
	RepoBSONBuilder builder;
	builder.append("vectorTest", vector3d);
	RepoBSON bson = builder.obj();

	std::vector<float> vectorOut = bson.getFloatArray("vectorTest");

	EXPECT_EQ(3, vectorOut.size());

	EXPECT_EQ(vectorOut[0], vector3d.x);
	EXPECT_EQ(vectorOut[1], vector3d.y);
	EXPECT_EQ(vectorOut[2], vector3d.z);


}

TEST(RepoBSONBuilderTest, AppendArrayPair)
{
	std::list<std::pair<std::string, std::string>> list;
	int size = 10;
	for (int i = 0; i < size; ++i)
	{
		std::pair<std::string, std::string> p(std::to_string(std::rand()), std::to_string(std::rand()));
		list.push_back(p);
	}
	
	RepoBSONBuilder builder;
	builder.appendArrayPair("arrayPairTest", list, "first", "second");

	RepoBSON testBson = builder.obj();

	std::list<std::pair<std::string, std::string> > outList = testBson.getListStringPairField("arrayPairTest", "first", "second");

	EXPECT_EQ(outList.size(), list.size());

	auto inIt = list.begin();
	auto outIt = outList.begin();

	for (; inIt != list.end(); ++inIt, ++outIt)
	{
		EXPECT_EQ(outIt->first, inIt->first);
		EXPECT_EQ(outIt->second, inIt->second);
	}

	//Ensure this doesn't crash and die.
	builder.appendArrayPair("blah", std::list<std::pair<std::string, std::string> >(), "first", "second");

}

TEST(RepoBSONBuilderTest, appendBinary)
{
	size_t binSize = 1024 * 1024;
	uint8_t *binaryData = (uint8_t*) malloc(binSize);

	RepoBSONBuilder builder;

	builder.appendBinary("binaryData", binaryData, binSize);
	builder.appendBinary("binaryData2", binaryData, 0);

	uint8_t *nothing = nullptr;
	builder.appendBinary("binaryData3", nothing, 0);

	RepoBSON bson = builder.obj();

	//only "binaryData" should've been appended
	EXPECT_TRUE(bson.hasField("binaryData"));
	EXPECT_FALSE(bson.hasField("binaryData2"));
	EXPECT_FALSE(bson.hasField("binaryData3"));

	std::vector<uint8_t> binOut;
	EXPECT_TRUE(bson.getBinaryFieldAsVector("binaryData", binOut));
	EXPECT_EQ(binOut.size(), binSize);

	for (size_t i = 0; i < binSize; ++i)
	{
		EXPECT_EQ(binOut[i], binaryData[i]);
	}
}

TEST(RepoBSONBuilderTest, appendTimeStamp)
{

	RepoBSONBuilder builder;
	builder.appendTimeStamp("ts");

	RepoBSON bson = builder.obj();

	EXPECT_TRUE(bson.hasField("ts"));

	EXPECT_EQ(bson.getField("ts").type(), ElementType::DATE );
}