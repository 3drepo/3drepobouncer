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

#include <repo/lib/datastructure/repo_variant.h>
#include <repo/manipulator/modelconvertor/import/odaHelper/data_processor_nwd.h>

#include <Attribute/NwPropertyAttribute.h>
#include <NwVariant.h>
#include <NwDatabase.h>
#include <StaticRxObject.h>
#include <ExSystemServices.h>
#include <NwHostAppServices.h>
#include "DynamicLinker.h"
#include "Gs/GsBaseModule.h"

using namespace repo::lib;
using namespace repo::manipulator::modelconvertor::odaHelper;

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

void SetupOdEnvForNWD(OdStaticRxObject<MyNwServices>& svcs, OdNwDatabasePtr& pDB) {
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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

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

	// Convert to RepoVariant
	RepoVariant v;
	bool success = TryConvertMetadataProperty("test", props[0], v);

	// Check results
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("1.000000, 2.000000, 3.000000"));

	// Teardown
	TeardownOdEnvForNWD();
}