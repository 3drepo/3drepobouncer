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
#include <repo/manipulator/modelconvertor/import/odaHelper/data_processor_rvt.h>
#include <repo/manipulator/modelconvertor/import/odaHelper/repo_oda_system_services.h>

#include "Gs/GsBaseModule.h"
#include "DynamicLinker.h"
#include "StaticRxObject.h"
#include "ExBimHostAppServices.h"
#include <Database/BmParamDefHelper.h>
#include "HostObj/Entities/BmSWall.h"
#include "Database/BmDatabase.h"
#include "Database/BmTransaction.h"

#include "Essential/Entities/BmExtrusionElem.h"
#include "Database/Entities/BmDirectShape.h"

#include "Essential/Entities/BmBlendElem.h"

using namespace repo::lib;
using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace testing;

// First helper class for the test of the RVT Converter
class OdExBimSystemServices : public RepoSystemServices
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

	pDB = svcs.createTemplate(OdBm::ProjectTemplate::Structural, OdBm::UnitSystem::Metric);
}

void TeardownOdEnvForRvt(OdBmDatabasePtr& pDB) {
	pDB.release();

	::odrxDynamicLinker()->unloadUnreferenced();
	odgsUninitialize();
	::odrxUninitialize();
}

TEST(RepoMetaVariantConverterRevitTest, EmptyTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdTfVariant variant;

	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_FALSE(success);
}

TEST(RepoMetaVariantConverterRevitTest, StringTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdString data = OdString("Test");
	OdTfVariant variant = OdTfVariant(data);

	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("Test"));
}

TEST(RepoMetaVariantConverterRevitTest, BoolTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	bool data = true;
	OdTfVariant variant = OdTfVariant(data);

	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<bool>(v), true);
}

TEST(RepoMetaVariantConverterRevitTest, Int8Test) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdInt8 data = -128;
	OdTfVariant variant = OdTfVariant(data);

	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<int>(v), -128);
}

TEST(RepoMetaVariantConverterRevitTest, Int16Test) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdInt16 data = -32768;
	OdTfVariant variant = OdTfVariant(data);

	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

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
	paramDef = OdBmParamDefHelper::createParamDef(OdBmSpecTypeId::Boolean::kYesNo);

	// Create data
	OdInt32 data = 0;
	OdTfVariant variant = OdTfVariant(data);

	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<bool>(v), false);

	// Teardown
	TeardownOdEnvForRvt(pDB);
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
	OdTfVariant variant = OdTfVariant(data);

	// Configure paramDef (to anything but kYesNo)
	paramDef = OdBmParamDefHelper::createParamDef(OdBmSpecTypeId::Int::kInteger);


	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<int64_t>(v), -2147483648ll);

	// Teardown
	TeardownOdEnvForRvt(pDB);
}

TEST(RepoMetaVariantConverterRevitTest, Int64Test) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdInt64 data = -9223372036854775808ll;
	OdTfVariant variant = OdTfVariant(data);

	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<int64_t>(v), -9223372036854775808ll);
}

TEST(RepoMetaVariantConverterRevitTest, DoubleTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdDouble data = 1.0;
	OdTfVariant variant = OdTfVariant(data);

	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<double>(v), 1.0);
}

TEST(RepoMetaVariantConverterRevitTest, AnsiStringTest) {

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdAnsiString data = "Test";
	OdTfVariant variant = OdTfVariant(data);

	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	EXPECT_EQ(boost::get<std::string>(v), std::string("Test"));
}

TEST(RepoMetaVariantConverterRevitTest, StubPtrDataTestNoParamNoName) {

	// Setup of the OD environment
	OdStaticRxObject<MyRvServices> svcs;
	OdBmDatabasePtr pDB;
	SetupOdEnvForRvt(svcs, pDB);

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param;

	// Create data
	OdBmObjectId id;

	// Note, the transaction gets its own scope
	// Don't ask me why, but this is the only way this does not crashes and burns on teardown.
	{
		OdBmTransaction tr(pDB, OdBm::TransactionType::Regular);
		tr.start();

		OdResult result;
		OdBmDirectShapePtr pDirectShape = OdBmDirectShape::createObject();

		result = pDB->addElement(pDirectShape, id);

		tr.commit();
	}

	OdDbStub* stub(id);
	OdTfVariant variant = OdTfVariant(stub);


	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	std::string variantResult = boost::get<std::string>(v);
	EXPECT_EQ(variantResult, std::to_string((OdUInt64)id.getHandle()));

	// Teardown
	TeardownOdEnvForRvt(pDB);

}

TEST(RepoMetaVariantConverterRevitTest, StubPtrDataTestParamSetRegularHdl) {

	// Setup of the OD environment
	OdStaticRxObject<MyRvServices> svcs;
	OdBmDatabasePtr pDB;
	SetupOdEnvForRvt(svcs, pDB);

	// Set up
	OdBmLabelUtilsPEPtr labelUtils;
	OdBmParamDefPtr paramDef;
	OdBm::BuiltInParameter::Enum param = OdBm::BuiltInParameter::ELEM_CATEGORY_PARAM;

	// Create data
	OdBmObjectId id;

	// Note, the transaction gets its own scope
	// Don't ask me why, but this is the only way this does not crashes and burns on teardown.
	{
		OdBmTransaction tr(pDB, OdBm::TransactionType::Regular);
		tr.start();

		OdResult result;
		OdBmDirectShapePtr pDirectShape = OdBmDirectShape::createObject();

		result = pDB->addElement(pDirectShape, id);

		tr.commit();
	}

	OdDbStub* stub(id);
	OdTfVariant variant = OdTfVariant(stub);


	// Convert
	RepoVariant v;
	bool success = DataProcessorRvt::tryConvertMetadataEntry(variant, labelUtils, paramDef, param, v);

	// Check result
	EXPECT_TRUE(success);
	std::string variantResult = boost::get<std::string>(v);
	EXPECT_EQ(variantResult, std::to_string((OdUInt64)id.getHandle()));

	TeardownOdEnvForRvt(pDB);
}
