/**
*  Copyright (C) 2019 3D Repo Ltd
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

#include <repo/core/model/bson/repo_bson_ref.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <repo/core/model/bson/repo_bson_builder.h>

using namespace repo::core::model;
using namespace testing;

TEST(RepoRefTest, ConvertTypeAsString)
{
	EXPECT_EQ(RepoRef::convertTypeAsString(RepoRef::RefType::FS), RepoRef::REPO_REF_TYPE_FS);
	EXPECT_EQ(RepoRef::convertTypeAsString(RepoRef::RefType::UNKNOWN), RepoRef::REPO_REF_TYPE_UNKNOWN);
	EXPECT_EQ(RepoRef::convertTypeAsString((RepoRef::RefType)10), RepoRef::REPO_REF_TYPE_UNKNOWN);
}

TEST(RepoRefTest, Constructor)
{
	auto ref = RepoRef();

	EXPECT_THAT(ref.isEmpty(), IsTrue());
	EXPECT_THAT(ref.getRefLink(), IsEmpty());
}

TEST(RepoRefTest, Factory)
{
	// Create a RepoRef using a String as the index

	auto type = RepoRef::RefType::FS;
	std::string link = "linkOfThisFile";
	uint32_t size = std::rand();

	std::string stringid = "FileNameTest";

	auto a = RepoBSONFactory::makeRepoRef(stringid, type, link, size);

	EXPECT_THAT(a.getType(), Eq(type));
	EXPECT_THAT(a.getRefLink(), Eq(link));
	EXPECT_THAT(a.getRefLink(), Eq(link));
	EXPECT_THAT(a.getFileSize(), Eq(size));

	EXPECT_THAT(a.getId(), Eq(stringid));

	// Create a RepoRef using a LUUID as the index

	auto uuidid = repo::lib::RepoUUID::createUUID();

	auto b = RepoBSONFactory::makeRepoRef(uuidid, type, link, size);

	EXPECT_THAT(b.getType(), Eq(type));
	EXPECT_THAT(b.getRefLink(), Eq(link));
	EXPECT_THAT(b.getRefLink(), Eq(link));
	EXPECT_THAT(b.getFileSize(), Eq(size));

	EXPECT_THAT(b.getId(), Eq(uuidid));
}

TEST(RepoRefTest, Serialise)
{
	auto a = RepoBSONFactory::makeRepoRef(
		std::string("stringid"),
		RepoRef::RefType::FS,
		std::string("linkOfThisFile"),
		1024);

	auto bson = (RepoBSON)a;

	EXPECT_THAT(bson.getStringField(REPO_LABEL_ID), a.getId());
	EXPECT_THAT(bson.getStringField(REPO_REF_LABEL_TYPE), Eq(RepoRef::convertTypeAsString(a.getType())));
	EXPECT_THAT(bson.getStringField(REPO_REF_LABEL_LINK), Eq(a.getRefLink()));
	EXPECT_THAT(bson.getIntField(REPO_REF_LABEL_SIZE), Eq(a.getFileSize()));

	auto b = RepoBSONFactory::makeRepoRef(
		repo::lib::RepoUUID::createUUID(),
		RepoRef::RefType::FS,
		std::string("linkOfThisFile"),
		1024);

	bson = (RepoBSON)b;

	EXPECT_THAT(bson.getUUIDField(REPO_LABEL_ID), b.getId());
	EXPECT_THAT(bson.getStringField(REPO_REF_LABEL_TYPE), Eq(RepoRef::convertTypeAsString(b.getType())));
	EXPECT_THAT(bson.getStringField(REPO_REF_LABEL_LINK), Eq(b.getRefLink()));
	EXPECT_THAT(bson.getIntField(REPO_REF_LABEL_SIZE), Eq(b.getFileSize()));

	// Metadata

	auto m = RepoRef::Metadata();
	m["1"] = std::string("X8D%QptFu*s#wn");
	m["2"] = (int)100;
	m["3"] = (double)100;
	m["4"] = (bool)true;
	auto uuid = repo::lib::RepoUUID::createUUID();
	m["5"] = uuid;

	auto c = RepoBSONFactory::makeRepoRef(
		std::string("stringid"),
		RepoRef::RefType::FS,
		std::string("linkOfThisFile"),
		1024,
		m
	);

	bson = (RepoBSON)c;

	EXPECT_THAT(bson.getStringField("1"), Eq("X8D%QptFu*s#wn"));
	EXPECT_THAT(bson.getIntField("2"), Eq(100));
	EXPECT_THAT(bson.getDoubleField("3"), Eq(100));
	EXPECT_THAT(bson.getBoolField("4"), Eq(true));
	EXPECT_THAT(bson.getUUIDField("5"), Eq(uuid));
}

TEST(RepoRefTest, DeserialiseString)
{
	auto id = repo::lib::RepoUUID::createUUID().toString();
	auto type = RepoRef::RefType::FS;
	std::string link = "linkOfThisFile";
	uint32_t size = std::rand();

	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, id);
	builder.append(REPO_REF_LABEL_TYPE, RepoRef::convertTypeAsString(type));
	builder.append(REPO_REF_LABEL_LINK, link);
	builder.append(REPO_REF_LABEL_SIZE, (unsigned int)size);

	auto ref = RepoRef(builder.obj());

	EXPECT_THAT(ref.getType(), Eq(RepoRef::RefType::FS));
	EXPECT_THAT(ref.getRefLink(), Eq(link));
	EXPECT_THAT(ref.getFileSize(), Eq(size));
}

TEST(RepoRefTest, DeserialiseUUID)
{
	auto id = repo::lib::RepoUUID::createUUID().toString();
	auto type = RepoRef::RefType::FS;
	std::string link = "linkOfThisFile";
	uint32_t size = std::rand();

	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, id);
	builder.append(REPO_REF_LABEL_TYPE, RepoRef::convertTypeAsString(type));
	builder.append(REPO_REF_LABEL_LINK, link);
	builder.append(REPO_REF_LABEL_SIZE, (unsigned int)size);

	auto ref = RepoRef(builder.obj());

	EXPECT_THAT(ref.getType(), Eq(RepoRef::RefType::FS));
	EXPECT_THAT(ref.getRefLink(), Eq(link));
	EXPECT_THAT(ref.getFileSize(), Eq(size));
}

TEST(RepoRefTest, DeserialiseEmpty)
{
	repo::core::model::RepoBSONBuilder builder;
	auto ref = RepoRef(builder.obj()); // building this should not throw an exception
	EXPECT_THAT(ref.isEmpty(), IsTrue());
}

TEST(RepoRefTest, Name)
{
	// The name field is not currently supported by the factory as it is not used.
	// However, RefNodes can have the field set, either in schema sourced elsewhere,
	// or via the metadata options if the field names coincide...

	// Names should be empty by default.

	auto a = RepoRef();
	EXPECT_THAT(a.getFileName(), IsEmpty());

	RepoBSONBuilder B;
	B.append(REPO_REF_LABEL_SIZE, 1); // RepoRef expects all of these otherwise it will initialise to empty
	B.append(REPO_REF_LABEL_LINK, "link");
	B.append(REPO_REF_LABEL_TYPE, "fs");
	auto b = RepoRef(B.obj());
	EXPECT_THAT(b.isEmpty(), IsFalse());
	EXPECT_THAT(b.getFileName(), IsEmpty());

	// Another process has set the name field
	
	RepoBSONBuilder C;
	C.append(REPO_REF_LABEL_SIZE, 1); // RepoRef expects all of these otherwise it will initialise to empty
	C.append(REPO_REF_LABEL_LINK, "link");
	C.append(REPO_REF_LABEL_TYPE, "fs");
	C.append(REPO_NODE_LABEL_NAME, "myName");
	auto c = RepoRef(C.obj());
	EXPECT_THAT(c.isEmpty(), IsFalse());
	EXPECT_THAT(c.getFileName(), "myName");

	// It is also possible to set a name if the metadata key matches the field
	// name, and for this to go round robin

	auto d = RepoBSONFactory::makeRepoRef(
		repo::lib::RepoUUID::createUUID(),
		RepoRef::RefType::FS,
		"",
		0,
		{ 
			{ REPO_NODE_LABEL_NAME, repo::lib::RepoVariant(std::string("metadataName")) } 
		}
	);

	auto e = (RepoBSON)d;
	EXPECT_THAT(e.getStringField(REPO_NODE_LABEL_NAME), "metadataName");

	auto f = RepoRef(e);
	EXPECT_THAT(f.getFileName(), Eq("metadataName"));
}

TEST(RepoRefTest, InvalidIdType)
{
	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_REF_LABEL_TYPE, RepoRef::convertTypeAsString(RepoRef::RefType::FS));
	builder.append(REPO_REF_LABEL_LINK, "link");
	builder.append(REPO_REF_LABEL_SIZE, (unsigned int)0);

	builder.append(REPO_LABEL_ID, 0); // An integer is not a valid Id type

	// Attempting to read a node with broken schema should not fail silently

	EXPECT_THROW({
		auto ref = RepoRefT<repo::lib::RepoUUID>(builder.obj());
	},
	repo::lib::RepoBSONException);

	EXPECT_THROW({
		auto ref = RepoRefT<std::string>(builder.obj());
	},
	repo::lib::RepoBSONException);
}

TEST(RepoRefTest, MismatchedIdTypeUUID)
{
	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_REF_LABEL_TYPE, RepoRef::convertTypeAsString(RepoRef::RefType::FS));
	builder.append(REPO_REF_LABEL_LINK, "link");
	builder.append(REPO_REF_LABEL_SIZE, (unsigned int)0);
	builder.append(REPO_LABEL_ID, repo::lib::RepoUUID::createUUID());

	// Reading a string indexed document as a UUID indexed document is not
	// supported, as a string will not always be able to be parsed into a
	// UUID.

	EXPECT_THROW({
		auto ref = RepoRefT<std::string>(builder.obj());
		},
		repo::lib::RepoBSONException);
}

TEST(RepoRefTest, MismatchedIdTypeString)
{
	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_REF_LABEL_TYPE, RepoRef::convertTypeAsString(RepoRef::RefType::FS));
	builder.append(REPO_REF_LABEL_LINK, "link");
	builder.append(REPO_REF_LABEL_SIZE, (unsigned int)0);
	builder.append(REPO_LABEL_ID, repo::lib::RepoUUID::createUUID().toString());

	// Reading a UUID indexed document, as a string indexed document, while
	// theoretically possible, should throw so its symmetric with the above

	EXPECT_THROW({
		auto ref = RepoRefT<repo::lib::RepoUUID>(builder.obj());
		},
		repo::lib::RepoBSONException);
}
