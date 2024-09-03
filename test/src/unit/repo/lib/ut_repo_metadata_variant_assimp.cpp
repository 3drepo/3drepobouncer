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
#include "../../repo_test_utils.h"

#include <repo/lib/datastructure/repo_metadataVariant.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_assimp.h>
#include "assimp/scene.h"

using namespace repo::lib;
using namespace repo::manipulator::modelconvertor;

TEST(RepoMetaVariantConverterAssimpTest, Boolean)
{
	bool data = true;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_BOOL;
	meta.mData = &data;

	MetadataVariant v;
	EXPECT_TRUE(TryConvertMetadataEntry(meta, v));
	EXPECT_EQ(boost::get<bool>(v), true);
}

TEST(RepoMetaVariantConverterAssimpTest, Int32)
{
	// Int32 (positive)
	int dataPos = 2147483647;
	aiMetadataEntry metaPos;
	metaPos.mType = aiMetadataType::AI_INT32;
	metaPos.mData = &dataPos;

	MetadataVariant vPos;
	EXPECT_TRUE(TryConvertMetadataEntry(metaPos, vPos));
	EXPECT_EQ(boost::get<int>(vPos), 2147483647);

	// Int32 (negative)
	int dataNeg = -2147483647;
	aiMetadataEntry metaNeg;
	metaNeg.mType = aiMetadataType::AI_INT32;
	metaNeg.mData = &dataNeg;

	MetadataVariant vNeg;
	EXPECT_TRUE(TryConvertMetadataEntry(metaNeg, vNeg));
	EXPECT_EQ(boost::get<int>(vNeg), -2147483647);
}


TEST(RepoMetaVariantConverterAssimpTest, UInt64)
{
	// UInt64 (min)
	uint64_t dataMin = 0;
	aiMetadataEntry metaMin;
	metaMin.mType = aiMetadataType::AI_UINT64;
	metaMin.mData = &dataMin;

	MetadataVariant vMin;
	EXPECT_TRUE(TryConvertMetadataEntry(metaMin, vMin));
	EXPECT_EQ(boost::get<long long>(vMin), 0);

	// Uint64 (max)
	uint64_t dataMax = 18446744073709551615;
	aiMetadataEntry metaMax;
	metaMax.mType = aiMetadataType::AI_UINT64;
	metaMax.mData = &dataMax;

	MetadataVariant vMax;
	EXPECT_TRUE(TryConvertMetadataEntry(metaMax, vMax));
	EXPECT_EQ(boost::get<long long>(vMax), 18446744073709551615);
}

TEST(RepoMetaVariantConverterAssimpTest, Float)
{
	float data = 0.5;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_FLOAT;
	meta.mData = &data;

	MetadataVariant v;
	EXPECT_TRUE(TryConvertMetadataEntry(meta, v));
	EXPECT_EQ(boost::get<double>(v), 0.5);
}

TEST(RepoMetaVariantConverterAssimpTest, Double)
{
	double data = 0.6;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_DOUBLE;
	meta.mData = &data;

	MetadataVariant v;
	EXPECT_TRUE(TryConvertMetadataEntry(meta, v));
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

	MetadataVariant v;
	EXPECT_TRUE(TryConvertMetadataEntry(meta, v));
	EXPECT_EQ(boost::get<std::string>(v), "[1.00000000000000000, 2.00000000000000000, 3.00000000000000000]");
}

TEST(RepoMetaVariantConverterAssimpTest, AISTRING)
{
	aiString data = aiString("test");
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_AISTRING;
	meta.mData = &data;

	MetadataVariant v;
	EXPECT_FALSE(TryConvertMetadataEntry(meta, v));
}
