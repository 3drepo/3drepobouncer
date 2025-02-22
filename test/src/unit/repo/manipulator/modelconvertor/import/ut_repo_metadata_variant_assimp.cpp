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
#include <unit/repo_test_utils.h>

#include <repo/lib/datastructure/repo_variant.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_assimp.h>
#include "assimp/scene.h"

using namespace repo::lib;
using namespace repo::manipulator::modelconvertor;
using namespace testing;

TEST(RepoMetaVariantConverterAssimpTest, Boolean)
{
	bool data = true;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_BOOL;
	meta.mData = &data;

	RepoVariant v;
	EXPECT_TRUE(AssimpModelImport::tryConvertMetadataEntry(meta, v));
	EXPECT_EQ(boost::get<bool>(v), true);
}

TEST(RepoMetaVariantConverterAssimpTest, Int32)
{
	// Int32 (positive)
	int dataPos = 2147483647;
	aiMetadataEntry metaPos;
	metaPos.mType = aiMetadataType::AI_INT32;
	metaPos.mData = &dataPos;

	RepoVariant vPos;
	EXPECT_TRUE(AssimpModelImport::tryConvertMetadataEntry(metaPos, vPos));
	EXPECT_EQ(boost::get<int>(vPos), 2147483647);

	// Int32 (negative)
	int dataNeg = -2147483647;
	aiMetadataEntry metaNeg;
	metaNeg.mType = aiMetadataType::AI_INT32;
	metaNeg.mData = &dataNeg;

	RepoVariant vNeg;
	EXPECT_TRUE(AssimpModelImport::tryConvertMetadataEntry(metaNeg, vNeg));
	EXPECT_EQ(boost::get<int>(vNeg), -2147483647);
}


TEST(RepoMetaVariantConverterAssimpTest, UInt64)
{
	// UInt64 (min)
	uint64_t dataMin = 0;
	aiMetadataEntry metaMin;
	metaMin.mType = aiMetadataType::AI_UINT64;
	metaMin.mData = &dataMin;

	RepoVariant vMin;
	EXPECT_TRUE(AssimpModelImport::tryConvertMetadataEntry(metaMin, vMin));
	EXPECT_EQ(boost::get<int64_t>(vMin), 0);

	// Uint64 (max)
	uint64_t dataMax = 9223372036854775807ll;
	aiMetadataEntry metaMax;
	metaMax.mType = aiMetadataType::AI_UINT64;
	metaMax.mData = &dataMax;

	RepoVariant vMax;
	EXPECT_TRUE(AssimpModelImport::tryConvertMetadataEntry(metaMax, vMax));
	EXPECT_EQ(boost::get<int64_t>(vMax), 9223372036854775807ll);
}

TEST(RepoMetaVariantConverterAssimpTest, Float)
{
	float data = 0.5;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_FLOAT;
	meta.mData = &data;

	RepoVariant v;
	EXPECT_TRUE(AssimpModelImport::tryConvertMetadataEntry(meta, v));
	EXPECT_EQ(boost::get<double>(v), 0.5);
}

TEST(RepoMetaVariantConverterAssimpTest, Double)
{
	double data = 0.6;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_DOUBLE;
	meta.mData = &data;

	RepoVariant v;
	EXPECT_TRUE(AssimpModelImport::tryConvertMetadataEntry(meta, v));
	EXPECT_EQ(boost::get<double>(v), 0.6);
}

TEST(RepoMetaVariantConverterAssimpTest, Vector)
{
	aiVector3D data;
	data.x = 1.00;
	data.y = 2.00;
	data.z = 3.00;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_AIVECTOR3D;
	meta.mData = &data;

	RepoVariant v;
	EXPECT_TRUE(AssimpModelImport::tryConvertMetadataEntry(meta, v));
	EXPECT_EQ(boost::get<std::string>(v), "[1.00000000000000000, 2.00000000000000000, 3.00000000000000000]");
}

TEST(RepoMetaVariantConverterAssimpTest, AISTRING)
{
	aiString data = aiString("test");
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_AISTRING;
	meta.mData = &data;

	RepoVariant v;
	EXPECT_FALSE(AssimpModelImport::tryConvertMetadataEntry(meta, v));
}
