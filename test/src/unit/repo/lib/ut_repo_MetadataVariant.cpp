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
#include <repo/lib/datastructure/repo_metadataVariant.h>
#include <repo/lib/datastructure/repo_metadataVariantHelper.h>
#include <gtest/gtest.h>

#include "../../repo_test_utils.h"

#include <Attribute/NwPropertyAttribute.h>
#include <NwVariant.h>
#include <NwDatabase.h>
#include <StaticRxObject.h>
#include <ExSystemServices.h>
#include <NwHostAppServices.h>
#include "DynamicLinker.h"
#include "Gs/GsBaseModule.h"

#include "StaticRxObject.h"
#include "ExBimHostAppServices.h"
#include <Database/BmParamDefHelper.h>
#include "HostObj/Entities/BmSWall.h"
#include "Database/BmDatabase.h"
//#include <Database/BmTransaction.h>
#include "TB_Commands/TBCommandsStdAfx.h"

#include "Essential/Entities/BmExtrusionElem.h"

using namespace repo::lib;

// Test of basic assignments and retreival.
TEST(RepoMetaVariantTest, AssignmentTest)
{
	MetadataVariant v0 = true;
	bool value0 = boost::get<bool>(v0);
	EXPECT_TRUE(value0);

	MetadataVariant v1 = 24;
	int value1 = boost::get<int>(v1);
	EXPECT_EQ(value1, 24);

	MetadataVariant v2 = 9223372036854775806;
	long long value2 = boost::get<long long>(v2);
	EXPECT_EQ(value2, 9223372036854775806);

	MetadataVariant v3 = 24.24;
	double value3 = boost::get<double>(v3);
	EXPECT_EQ(value3, 24.24);

	MetadataVariant v4 = std::string("3d Repo");
	std::string value4 = boost::get<std::string>(v4);
	EXPECT_EQ(value4, "3d Repo");

	tm tmPre;
	tmPre.tm_sec = 1;
	tmPre.tm_min = 2;
	tmPre.tm_hour = 3;
	tmPre.tm_mday = 4;
	tmPre.tm_mon = 5;
	tmPre.tm_year = 76;
	tmPre.tm_wday = 6;
	tmPre.tm_yday = 7;
	tmPre.tm_isdst = 1;
	MetadataVariant v5 = tmPre;
	tm value5 = boost::get<tm>(v5);
	EXPECT_EQ(value5.tm_sec, tmPre.tm_sec);
	EXPECT_EQ(value5.tm_min, tmPre.tm_min);
	EXPECT_EQ(value5.tm_hour, tmPre.tm_hour);
	EXPECT_EQ(value5.tm_mday, tmPre.tm_mday);
	EXPECT_EQ(value5.tm_mon, tmPre.tm_mon);
	EXPECT_EQ(value5.tm_year, tmPre.tm_year);
	EXPECT_EQ(value5.tm_wday, tmPre.tm_wday);
	EXPECT_EQ(value5.tm_yday, tmPre.tm_yday);
	EXPECT_EQ(value5.tm_isdst, tmPre.tm_isdst);
}

TEST(RepoMetaVariantTest, StringVisitor) {
	MetadataVariant v0 = true;
	std::string value0 = boost::apply_visitor(StringConversionVisitor(), v0);
	EXPECT_EQ(value0, "1");

	MetadataVariant v1 = 24;
	std::string value1 = boost::apply_visitor(StringConversionVisitor(), v1);
	EXPECT_EQ(value1, "24");

	MetadataVariant v2 = 9223372036854775806;
	std::string value2 = boost::apply_visitor(StringConversionVisitor(), v2);
	EXPECT_EQ(value2, "9223372036854775806");

	MetadataVariant v3 = 19.02;
	std::string value3 = boost::apply_visitor(StringConversionVisitor(), v3);
	EXPECT_EQ(value3, "19.020000");

	MetadataVariant v4 = std::string("3d Repo");
	std::string value4 = boost::apply_visitor(StringConversionVisitor(), v4);
	EXPECT_EQ(value4, "3d Repo");

	tm tmPre;
	tmPre.tm_sec = 1;
	tmPre.tm_min = 2;
	tmPre.tm_hour = 3;
	tmPre.tm_mday = 4;
	tmPre.tm_mon = 5;
	tmPre.tm_year = 76;
	tmPre.tm_wday = 6;
	tmPre.tm_yday = 7;
	tmPre.tm_isdst = 1;
	MetadataVariant v5 = tmPre;
	std::string value5 = boost::apply_visitor(StringConversionVisitor(), v5);
	EXPECT_EQ(value5, "04-06-1976 03-02-01");
}

TEST(RepoMetaVariantTest, CompareVisitor) {

	// Same native type, same value
	MetadataVariant v0a = true;
	MetadataVariant v0b = true;
	EXPECT_TRUE(boost::apply_visitor(DuplicationVisitor(), v0a, v0b));

	// Same native type, different value
	MetadataVariant v1a = true;
	MetadataVariant v1b = false;
	EXPECT_FALSE(boost::apply_visitor(DuplicationVisitor(), v1a, v1b));

	// Different native type
	MetadataVariant v2a = true;
	MetadataVariant v2b = 5;
	EXPECT_FALSE(boost::apply_visitor(DuplicationVisitor(), v2a, v2b));

	// Same standard class type, same value
	MetadataVariant v3a = std::string("Test");
	MetadataVariant v3b = std::string("Test");
	EXPECT_TRUE(boost::apply_visitor(DuplicationVisitor(), v3a, v3b));

	// Same standard class type, different value
	MetadataVariant v4a = std::string("Test");
	MetadataVariant v4b = std::string("Testing");
	EXPECT_FALSE(boost::apply_visitor(DuplicationVisitor(), v4a, v4b));

	// Same time type, same value
	tm tm5a;
	tm5a.tm_sec = 1;
	tm5a.tm_min = 2;
	tm5a.tm_hour = 3;
	tm5a.tm_mday = 4;
	tm5a.tm_mon = 5;
	tm5a.tm_year = 70;
	tm5a.tm_wday = 6;
	tm5a.tm_yday = 7;
	tm5a.tm_isdst = 1;
	tm tm5b = tm5a;
	MetadataVariant v5a = tm5a;
	MetadataVariant v5b = tm5b;
	EXPECT_TRUE(boost::apply_visitor(DuplicationVisitor(), v5a, v5b));

	// Same time type, different value
	tm tm6a;
	tm6a.tm_sec = 1;
	tm6a.tm_min = 2;
	tm6a.tm_hour = 3;
	tm6a.tm_mday = 4;
	tm6a.tm_mon = 5;
	tm6a.tm_year = 70;
	tm6a.tm_wday = 6;
	tm6a.tm_yday = 7;
	tm6a.tm_isdst = 1;
	tm tm6b;
	tm6b.tm_sec = 2;
	tm6b.tm_min = 3;
	tm6b.tm_hour = 4;
	tm6b.tm_mday = 5;
	tm6b.tm_mon = 6;
	tm6b.tm_year = 75;
	tm6b.tm_wday = 3;
	tm6b.tm_yday = 12;
	tm6b.tm_isdst = 0;
	MetadataVariant v6a = tm6a;
	MetadataVariant v6b = tm6b;

	EXPECT_FALSE(boost::apply_visitor(DuplicationVisitor(), v6a, v6b));
}

TEST(RepoMetaVariantConverterAssimpTest, Boolean)
{
	bool data = true;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_BOOL;
	meta.mData = &data;

	MetadataVariant v;
	EXPECT_TRUE(MetadataVariantHelper::TryConvert(meta, v));
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
	EXPECT_TRUE(MetadataVariantHelper::TryConvert(metaPos, vPos));
	EXPECT_EQ(boost::get<int>(vPos), 2147483647);

	// Int32 (negative)
	int dataNeg = -2147483647;
	aiMetadataEntry metaNeg;
	metaNeg.mType = aiMetadataType::AI_INT32;
	metaNeg.mData = &dataNeg;

	MetadataVariant vNeg;
	EXPECT_TRUE(MetadataVariantHelper::TryConvert(metaNeg, vNeg));
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
	EXPECT_TRUE(MetadataVariantHelper::TryConvert(metaMin, vMin));
	EXPECT_EQ(boost::get<long long>(vMin), 0);

	// Uint64 (max)
	uint64_t dataMax = 18446744073709551615;
	aiMetadataEntry metaMax;
	metaMax.mType = aiMetadataType::AI_UINT64;
	metaMax.mData = &dataMax;

	MetadataVariant vMax;
	EXPECT_TRUE(MetadataVariantHelper::TryConvert(metaMax, vMax));
	EXPECT_EQ(boost::get<long long>(vMax), 18446744073709551615);
}

TEST(RepoMetaVariantConverterAssimpTest, Float)
{
	float data = 0.5;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_FLOAT;
	meta.mData = &data;

	MetadataVariant v;
	EXPECT_TRUE(MetadataVariantHelper::TryConvert(meta, v));
	EXPECT_EQ(boost::get<double>(v), 0.5);
}

TEST(RepoMetaVariantConverterAssimpTest, Double)
{
	double data = 0.6;
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_DOUBLE;
	meta.mData = &data;

	MetadataVariant v;
	EXPECT_TRUE(MetadataVariantHelper::TryConvert(meta, v));
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
	EXPECT_TRUE(MetadataVariantHelper::TryConvert(meta, v));
	EXPECT_EQ(boost::get<std::string>(v), "[1.00000000000000000, 2.00000000000000000, 3.00000000000000000]");
}

TEST(RepoMetaVariantConverterAssimpTest, AISTRING)
{
	aiString data = aiString("test");
	aiMetadataEntry meta;
	meta.mType = aiMetadataType::AI_AISTRING;
	meta.mData = &data;

	MetadataVariant v;
	EXPECT_FALSE(MetadataVariantHelper::TryConvert(meta, v));
}


// First helper class for the test of the NWD Converter
class OdExNwSystemServices : public ExSystemServices
{
public:
	OdExNwSystemServices() {}
};


// Second helper class for the test of the NWD Converter
class MyNwServices : public OdExNwSystemServices, public OdNwHostAppServices {
protected:
	ODRX_USING_HEAP_OPERATORS(OdExNwSystemServices);
};

void SetupOdEnvForNWD(OdStaticRxObject<MyNwServices> &svcs, OdNwDatabasePtr &pDB) {
	odrxInitialize(&svcs);
	odgsInitialize();
	::odrxDynamicLinker()->loadModule(OdNwDbModuleName, false);

	pDB = svcs.createDatabase(true);
}

void TeardownOdEnvForNWD() {
	::odrxDynamicLinker()->unloadUnreferenced();
	odgsUninitialize();
	::odrxUninitialize();
}

TEST(RepoMetaVariantConverterNWDTest, DoubleTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	double value = 1.0;
	OdNwVariant var = OdNwVariant(value);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("double"), OdString("double"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<double>(v), 1.0);

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, Int32Test)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	OdInt32 value = -2147483648ll;
	OdNwVariant var = OdNwVariant(value);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("int32"), OdString("int32"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<long long>(v), -2147483648ll);

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, BoolTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	bool value = true;
	OdNwVariant var = OdNwVariant(value);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("bool"), OdString("bool"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<bool>(v), true);

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, DisplayStringTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	OdString value = OdString("Test");
	OdNwVariant var = OdNwVariant(value);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("dispString"), OdString("dispString"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("Test"));

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, TimeTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	OdUInt64 value = 781747200ll;
	OdNwVariant var = OdNwVariant(value);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("time"), OdString("time"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(mktime(&boost::get<tm>(v)), value);

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, LengthTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	double value = 2.0;
	OdNwVariant var = OdNwVariant(value, OdNwVariant::Type::kLength);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("length"), OdString("length"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<double>(v), 2.0);

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, AngleTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	double value = 3.0;
	OdNwVariant var = OdNwVariant(value, OdNwVariant::Type::kAngle);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("angle"), OdString("angle"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<double>(v), 3.0);

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, NameTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	OdNwNamePtr value = OdNwName::createObject();
	OdNwVariant var = OdNwVariant(OdRxObject::cast(value));

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("name"), OdString("name"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("")); // Not ideal, but I have not found a way to set the OdNwName object yet (FT).

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, IDStringTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	OdString value = OdString("ID");
	OdNwVariant var = OdNwVariant(value);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("idStr"), OdString("idStr"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("ID"));

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, AreaTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	double value = 4.0;
	OdNwVariant var = OdNwVariant(value, OdNwVariant::Type::kArea);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("area"), OdString("area"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<double>(v), 4.0);

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, VolumeTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	double value = 5.0;
	OdNwVariant var = OdNwVariant(value, OdNwVariant::Type::kVolume);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("volume"), OdString("volume"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<double>(v), 5.0);

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, Point2DTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	OdGePoint2d value = OdGePoint2d(1.0, 2.0);
	OdNwVariant var = OdNwVariant(value);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("point2D"), OdString("point2D"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("1.000000, 2.000000"));

	// Teardown
	TeardownOdEnvForNWD();
}

TEST(RepoMetaVariantConverterNWDTest, Point3DTest)
{
	// Setup of the OD environment
	OdStaticRxObject<MyNwServices> svcs;
	OdNwDatabasePtr pDB;
	SetupOdEnvForNWD(svcs, pDB);

	// Create data
	OdGePoint3d value = OdGePoint3d(1.0, 2.0, 3.0);
	OdNwVariant var = OdNwVariant(value);

	// Convert to property
	OdNwPropertyAttributePtr attribute = OdNwPropertyAttribute::createObject();
	attribute->addProperty(OdString("point3D"), OdString("point3D"), var);

	// Retrieve property
	OdArray<OdNwDataPropertyPtr> props;
	attribute->getProperties(props);

	// Convert to MetadataVariant
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("1.000000, 2.000000, 3.000000"));

	// Teardown
	TeardownOdEnvForNWD();
}

// First helper class for the test of the RVT Converter
class OdExBimSystemServices : public ExSystemServices
{
public:
	OdExBimSystemServices() {}
};


// Second helper class for the test of the RVT Converter
class MyRvServices : public OdExBimSystemServices, public OdExBimHostAppServices {
protected:
	ODRX_USING_HEAP_OPERATORS(OdExBimSystemServices);
};

void SetupOdEnvForRvt(OdStaticRxObject<MyRvServices>& svcs, OdBmDatabasePtr& pDB) {
	odrxInitialize(&svcs);
	odgsInitialize();
	::odrxDynamicLinker()->loadModule(OdBmLoaderModuleName, false);

	pDB = svcs.createDatabase(true);
}

void TeardownOdEnvForRvt() {
	::odrxDynamicLinker()->unloadUnreferenced();
	odgsUninitialize();
	::odrxUninitialize();
}

TEST(RepoMetaVariantConverterRevitTest, EmptyTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBmDatabase* database;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdTfVariant variant;	

	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, database, param, v);

	// Check result
	EXPECT_FALSE(success);
}

TEST(RepoMetaVariantConverterRevitTest, StringTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBmDatabase* database;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdTfVariant variant = OdString("Test");

	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, database, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("Test"));
}

TEST(RepoMetaVariantConverterRevitTest, BoolTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBmDatabase* database;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdTfVariant variant = true;

	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, database, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<bool>(v), true);
}

TEST(RepoMetaVariantConverterRevitTest, Int8Test) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBmDatabase* database;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdInt8 data = -128;
	OdTfVariant variant = data;

	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, database, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<int>(v), -128);
}

TEST(RepoMetaVariantConverterRevitTest, Int16Test) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBmDatabase* database;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdInt16 data = -32768;
	OdTfVariant variant = data;

	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, database, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<int>(v), -32768);
}


TEST(RepoMetaVariantConverterRevitTest, Int32AsBoolTest) {

	// Setup of the OD environment
	OdStaticRxObject<MyRvServices> svcs;
	OdBmDatabasePtr pDB;
	SetupOdEnvForRvt(svcs, pDB);


	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Configure paramDef
	paramDef = OdBmParamDefHelper::createParamDef(OdBmSpecTypeId::Boolean::kYesNo, OdBm::StorageType::Integer);

	// Create data
	OdInt32 data = 0;
	OdTfVariant variant = data;

	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, pDB, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<bool>(v), false);

	// Teardown
	TeardownOdEnvForRvt();
}

TEST(RepoMetaVariantConverterRevitTest, Int32asIntTest) {

	// Setup of the OD environment
	OdStaticRxObject<MyRvServices> svcs;
	OdBmDatabasePtr pDB;
	SetupOdEnvForRvt(svcs, pDB);

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdInt32 data = -2147483648ll;
	OdTfVariant variant = data;

	// Configure paramDef (to anything but kYesNo)
	paramDef = OdBmParamDefHelper::createParamDef(OdBmSpecTypeId::Int::kInteger, OdBm::StorageType::Integer);


	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, pDB, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<long long>(v), -2147483648ll);

	// Teardown
	TeardownOdEnvForRvt();
}

TEST(RepoMetaVariantConverterRevitTest, Int64Test) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBmDatabase* database;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdInt64 data = -9223372036854775808ll;
	OdTfVariant variant = data;

	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, database, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<long long>(v), -9223372036854775808ll);
}

TEST(RepoMetaVariantConverterRevitTest, DoubleTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBmDatabase* database;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdDouble data = 1.0;
	OdTfVariant variant = data;

	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, database, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<double>(v), 1.0);
}

TEST(RepoMetaVariantConverterRevitTest, AnsiStringTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBmDatabase* database;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdAnsiString data = "Test";
	OdTfVariant variant = data;

	// Convert
	MetadataVariant v;
	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, database, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("Test"));
}

//TEST(RepoMetaVariantRevitConverterTest, StubPtrDataTest) {
//
//	// Setup of the OD environment
//	OdStaticRxObject<MyRvServices> svcs;
//
//	odrxInitialize(&svcs);
//	odgsInitialize();
//	::odrxDynamicLinker()->loadModule(OdBmLoaderModuleName, false);
//
//
//
//	OdBmDatabasePtr pDB;
//	pDB = svcs.createDatabase(true);
//
//
//
//	// Set up
//	OdBmLabelUtilsPEPtr labelUtils;
//	OdBmParamDefPtr paramDef;
//	OdBm::BuiltInParameter::Enum param;
//
//	// Create data
//
//	OdBmElementPtr elementPtr = OdBmElement::createObject();
//	elementPtr->setElementName("Test");
//	
//	OdBmObjectPtr objectPtr = elementPtr;
//
//	auto objectId = elementPtr->objectId();
//
//	OdDbStub* stub(objectId);
//	
//
//	OdTfVariant variant = stub;
//
//	// Configure paramDef (to anything but kYesNo)
//	//paramDef = OdBmParamDefHelper::createParamDef(OdBmSpecTypeId::Int::kInteger, OdBm::StorageType::Integer);
//
//
//	// Convert
//	MetadataVariant v;
//	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, pDB, param, v);
//
//	// Check result
//	EXPECT_TRUE(success);
//	EXPECT_EQ(boost::get<std::string>(v), std::string("test"));
//
//	// Teardown
//	::odrxDynamicLinker()->unloadUnreferenced();
//	odgsUninitialize();
//	::odrxUninitialize();
//}

//TEST(RepoMetaVariantRevitConverterTest, StubPtrDataDBTest) {
//
//	// Setup of the OD environment
//	OdStaticRxObject<MyRvServices> svcs;
//
//	odrxInitialize(&svcs);
//	odgsInitialize();
//	::odrxDynamicLinker()->loadModule(OdBmLoaderModuleName, false);
//
//	OdBmDatabasePtr pDB;
//	pDB = svcs.createDatabase(true);
//
//	// Set up
//	OdBmLabelUtilsPEPtr labelUtils;
//	OdBmParamDefPtr paramDef;
//	OdBm::BuiltInParameter::Enum param;
//
//	// Create data
//	OdBmTransaction tr(pDB, OdBm::TransactionType::Regular);
//	tr.start();
//	OdBmObjectId id;
//	OdBmSWallPtr pSWall = OdBmSWall::createObject();
//	OdResult result = pDB->addElement(pSWall, id);
//	EXPECT_EQ(result, OdResult::eOk);
//	tr.commit();
//
//
//	OdTfVariant variant;// = stub;
//
//
//	// Convert
//	MetadataVariant v;
//	bool success = MetadataVariantHelper::TryConvert(variant, labelUtils, paramDef, pDB, param, v);
//
//	// Check result
//	EXPECT_TRUE(success);
//	EXPECT_EQ(boost::get<std::string>(v), std::string("test"));
//
//	// Teardown
//	::odrxDynamicLinker()->unloadUnreferenced();
//	odgsUninitialize();
//	::odrxUninitialize();
//}