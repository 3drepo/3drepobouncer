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

#include <repo/core/model/bson/repo_bson_ref.h>
#include <repo/core/model/bson/repo_bson_factory.h>

using namespace repo::core::model;

TEST(RepoRefTest, ConvertTypeAsString)
{
	EXPECT_EQ(RepoRef::convertTypeAsString(RepoRef::RefType::FS), RepoRef::REPO_REF_TYPE_FS);
	EXPECT_EQ(RepoRef::convertTypeAsString(RepoRef::RefType::GRIDFS), RepoRef::REPO_REF_TYPE_GRIDFS);
	EXPECT_EQ(RepoRef::convertTypeAsString(RepoRef::RefType::UNKNOWN), RepoRef::REPO_REF_TYPE_UNKNOWN);
	EXPECT_EQ(RepoRef::convertTypeAsString((RepoRef::RefType)10), RepoRef::REPO_REF_TYPE_UNKNOWN);
}

TEST(RepoRefTest, GeneralTest)
{
	std::string id = "FileNameTest";
	auto type = RepoRef::RefType::GRIDFS;
	std::string link = "linkOfThisFile";
	uint32_t size = std::rand();
	auto ref = RepoBSONFactory::makeRepoRef(id, type, link, size);

	EXPECT_FALSE(ref.isEmpty());
	EXPECT_EQ(id, ref.getID());
	EXPECT_EQ(type, ref.getType());
	EXPECT_EQ(link, ref.getRefLink());
	EXPECT_EQ(size, ref.getIntField(REPO_REF_LABEL_SIZE));
}

TEST(RepoRefTest, UUIDTest)
{
	// Ref nodes can be created outside of makeRepoRef, where the _id
	// field is a UUID, so make sure we can handle these too.

	repo::core::model::RepoBSONBuilder builder;
	auto id = repo::lib::RepoUUID::createUUID();
	auto type = RepoRef::RefType::GRIDFS;
	std::string link = "linkOfThisFile";
	uint32_t size = std::rand();

	builder.append(REPO_LABEL_ID, id);
	builder.append(REPO_REF_LABEL_TYPE, RepoRef::convertTypeAsString(type));
	builder.append(REPO_REF_LABEL_LINK, link);
	builder.append(REPO_REF_LABEL_SIZE, (unsigned int)size);

	auto ref = RepoRef(builder.obj());

	EXPECT_FALSE(ref.isEmpty());
	EXPECT_EQ(id.toString(), ref.getID());
	EXPECT_EQ(type, ref.getType());
	EXPECT_EQ(link, ref.getRefLink());
	EXPECT_EQ(size, ref.getIntField(REPO_REF_LABEL_SIZE));
}

TEST(RepoRefTest, InvalidIdType)
{
	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, 0); // An integer is not a valid Id type
	auto ref = RepoRef(builder.obj());

	EXPECT_THROW({
		ref.getID();
	},
	std::invalid_argument);
}