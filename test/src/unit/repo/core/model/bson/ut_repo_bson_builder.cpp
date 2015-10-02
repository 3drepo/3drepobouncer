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

TEST(RepoBSONBuilderTest, appendArray)
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
	testBuilder.appendArray("emptyTest", std::vector<std::string>());
	
}