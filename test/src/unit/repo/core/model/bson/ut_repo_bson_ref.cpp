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
	EXPECT_EQ(RepoRef::convertTypeAsString(RepoRef::RefType::S3), RepoRef::REPO_REF_TYPE_S3);
	EXPECT_EQ(RepoRef::convertTypeAsString(RepoRef::RefType::FS), RepoRef::REPO_REF_TYPE_FS);
	EXPECT_EQ(RepoRef::convertTypeAsString(RepoRef::RefType::GRIDFS), RepoRef::REPO_REF_TYPE_GRIDFS);
	EXPECT_EQ(RepoRef::convertTypeAsString(RepoRef::RefType::UNKNOWN), RepoRef::REPO_REF_TYPE_UNKNOWN);
	EXPECT_EQ(RepoRef::convertTypeAsString((RepoRef::RefType)10), RepoRef::REPO_REF_TYPE_UNKNOWN);
}

TEST(RepoRefTest, GeneralTest)
{
	std::string fileName = "FileNameTest";
	auto type = RepoRef::RefType::S3;
	std::string link = "linkOfThisFile";
	uint32_t size = std::rand();
	auto ref = RepoBSONFactory::makeRepoRef(fileName, type, link, size);

	EXPECT_FALSE(ref.isEmpty());
	EXPECT_EQ(fileName, ref.getFileName());
	EXPECT_EQ(type, ref.getType());
	EXPECT_EQ(link, ref.getRefLink());
	EXPECT_EQ(size, ref.getIntField(REPO_REF_LABEL_SIZE));
	
}