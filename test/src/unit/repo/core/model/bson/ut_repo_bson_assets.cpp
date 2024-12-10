/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/model/bson/repo_bson_factory.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::core::model;
using namespace testing;

namespace repo {
	namespace core {
		namespace model {
			static bool operator== (const RepoSupermeshMetadata& a, const RepoSupermeshMetadata& b)
			{
				return memcmp(&a, &b, sizeof(a)) == 0;
			}
		}
	}
}

TEST(RepoAssetsTest, Serialise)
{
	auto rid = repo::lib::RepoUUID::createUUID();
	auto files = std::vector<std::string>();
	auto jsons = std::vector<std::string>();
	auto database = "database";
	auto model = "model";
	auto offset = repo::lib::RepoVector3D64(rand() - rand(), rand() - rand(), rand() - rand());
	std::vector<RepoSupermeshMetadata> metadata;
	for (int i = 0; i < 5; i++)
	{
		files.push_back(repo::lib::RepoUUID::createUUID().toString());
		jsons.push_back(repo::lib::RepoUUID::createUUID().toString());
		RepoSupermeshMetadata md;
		md.min = rand() - rand();
		md.max = rand() - rand();
		md.numFaces = rand();
		md.numUVChannels = rand() % 2;
		md.numVertices = rand();
		md.primitive = 2 + rand() % 2;
		metadata.push_back(md);
	}

	auto bson = (RepoBSON)RepoBSONFactory::makeRepoBundleAssets(
		rid,
		files,
		database,
		model,
		offset.toStdVector(),
		jsons,
		metadata
	);

	EXPECT_THAT(bson.getUUIDField(REPO_LABEL_ID), rid);
	EXPECT_THAT(bson.getStringArray(REPO_ASSETS_LABEL_ASSETS), ElementsAreArray(files)); // The array order must match because the file names must coincide with the json file names
	EXPECT_THAT(bson.getStringArray(REPO_ASSETS_LABEL_JSONFILES), ElementsAreArray(jsons)); // The array order must match because the file names must coincide with the json file names
	EXPECT_THAT(bson.getStringField(REPO_LABEL_DATABASE), Eq(database));
	EXPECT_THAT(bson.getStringField(REPO_LABEL_MODEL), Eq(model));
	EXPECT_THAT(bson.getDoubleVectorField(REPO_ASSETS_LABEL_OFFSET), Eq(offset.toStdVector()));

	std::vector<RepoSupermeshMetadata> actual;
	auto md = bson.getObjectArray(REPO_ASSETS_LABEL_METADATA);
	for (auto& o : md)
	{
		RepoSupermeshMetadata m;
		m.max = o.getVector3DField(REPO_ASSETS_LABEL_MAX);
		m.min = o.getVector3DField(REPO_ASSETS_LABEL_MIN);
		m.primitive = o.getIntField(REPO_ASSETS_LABEL_PRIMITIVE);
		m.numUVChannels = o.getIntField(REPO_ASSETS_LABEL_NUMUVCHANNELS);
		m.numFaces = o.getIntField(REPO_ASSETS_LABEL_NUMFACES);
		m.numVertices = o.getIntField(REPO_ASSETS_LABEL_NUMVERTICES);
		actual.push_back(m);
	}
	EXPECT_THAT(actual, Eq(metadata));
}

TEST(RepoAssetsTest, SerialiseEmpty)
{
	auto bson = (RepoBSON)RepoBSONFactory::makeRepoBundleAssets(
		repo::lib::RepoUUID::defaultValue,
		{},
		"",
		"",
		{},
		{},
		{}
	);

	EXPECT_THAT(bson.getUUIDField(REPO_LABEL_ID), Eq(repo::lib::RepoUUID::defaultValue));
	EXPECT_THAT(bson.hasField(REPO_ASSETS_LABEL_ASSETS), IsFalse());
	EXPECT_THAT(bson.hasField(REPO_ASSETS_LABEL_JSONFILES), IsFalse());
	EXPECT_THAT(bson.hasField(REPO_LABEL_DATABASE), IsFalse());
	EXPECT_THAT(bson.hasField(REPO_LABEL_MODEL), IsFalse());
	EXPECT_THAT(bson.getDoubleVectorField(REPO_ASSETS_LABEL_OFFSET), ElementsAre(0,0,0));
}