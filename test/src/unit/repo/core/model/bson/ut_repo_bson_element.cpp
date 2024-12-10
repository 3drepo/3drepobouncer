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

#include <ctime>
#include "repo/core/model/bson/repo_bson_element.h"
#include "repo/core/model/bson/repo_bson_builder.h"
#include "repo/lib/datastructure/repo_variant.h"
#include "repo/lib/datastructure/repo_variant_utils.h"

using namespace repo::core::model;
using namespace testing;

TEST(RepoBSONElementTest, TypeTest)
{
	RepoBSONBuilder objectBuilder;
	objectBuilder.append("name", "subobject");
	auto object = objectBuilder.obj();

	// The following test the currently used types, which are those supported by
	// RepoBSONBuilder.

	RepoBSONBuilder builder;
	auto uuid = repo::lib::RepoUUID::createUUID();
	auto time_tm = getRandomTm();
	auto time_t = mktime(&time_tm);
	builder.append("array", std::vector<float>{1});
	builder.append("uuid", uuid);
	builder.append("bool", true);
	builder.appendTime("date", time_t); // Must use appendTime here because time_t will be seen first as a int64_t unless explicitly converted to a mongo::Date_t
	builder.append("double", (double)1);
	builder.append("int", (int)1);
	builder.append("long", (int64_t)1);
	builder.append("object", object);
	builder.append("string", "a string");

	RepoBSON bson(builder.obj());

	EXPECT_THAT(bson.getField("array").type(), Eq(ElementType::ARRAY));
	EXPECT_THAT(bson.getField("uuid").type(), Eq(ElementType::UUID));
	EXPECT_THAT(bson.getField("bool").type(), Eq(ElementType::BOOL));
	EXPECT_THAT(bson.getField("date").type(), Eq(ElementType::DATE));
	EXPECT_THAT(bson.getField("double").type(), Eq(ElementType::DOUBLE));
	EXPECT_THAT(bson.getField("int").type(), Eq(ElementType::INT));
	EXPECT_THAT(bson.getField("long").type(), Eq(ElementType::LONG));
	EXPECT_THAT(bson.getField("object").type(), Eq(ElementType::OBJECT));
	EXPECT_THAT(bson.getField("string").type(), Eq(ElementType::STRING));

	EXPECT_THAT(bson.getField("bool").Bool(), Eq(true));
	EXPECT_THAT(bson.getField("date").TimeT(), Eq(time_t));
	EXPECT_THAT(bson.getField("date").Tm(), Eq(time_tm));
	EXPECT_THAT(bson.getField("double").Double(), Eq((double)1));
	EXPECT_THAT(bson.getField("int").Int(), Eq((int)1));
	EXPECT_THAT(bson.getField("long").Long(), Eq((int64_t)1));
	EXPECT_THAT(bson.getField("object").Object(), Eq(object));
	EXPECT_THAT(bson.getField("string").String(), Eq("a string"));

	// Below we look for standard exception (not repo exception) as the
	// RepoBSONElement does not override this yet.

	EXPECT_THROW({ bson.getField("bool").String(); }, std::exception);
	EXPECT_THROW({ bson.getField("date").String(); }, std::exception);
	EXPECT_THROW({ bson.getField("date").String(); }, std::exception);
	EXPECT_THROW({ bson.getField("double").String(); }, std::exception);
	EXPECT_THROW({ bson.getField("int").String(); }, std::exception);
	EXPECT_THROW({ bson.getField("long").String(); }, std::exception);
	EXPECT_THROW({ bson.getField("object").String(); }, std::exception);
	EXPECT_THROW({ bson.getField("string").Bool(); }, std::exception);

	// Finally as RepoVariant

	EXPECT_THAT(boost::get<bool>(bson.getField("bool").repoVariant()), Eq(true));
	EXPECT_THAT(boost::get<tm>(bson.getField("date").repoVariant()), Eq(time_tm));
	EXPECT_THAT(boost::get<double>(bson.getField("double").repoVariant()), Eq((double)1));
	EXPECT_THAT(boost::get<int>(bson.getField("int").repoVariant()), Eq((int)1));
	EXPECT_THAT(boost::get<int64_t>(bson.getField("long").repoVariant()), Eq((int64_t)1));
	EXPECT_THAT(boost::get<std::string>(bson.getField("string").repoVariant()), Eq("a string"));
}