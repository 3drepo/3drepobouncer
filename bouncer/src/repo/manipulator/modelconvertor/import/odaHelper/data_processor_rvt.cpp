/**
*  Copyright (C) 2018 3D Repo Ltd
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

#include <boost/filesystem.hpp>
#include <OdPlatformSettings.h>

#include "vectorise_device_rvt.h"
#include "data_processor_rvt.h"
#include "helper_functions.h"
#include "../../../../lib/repo_utils.h"
#include <Database/BmTransaction.h>
#include <Database/BmUnitUtils.h>
#include <Base/BmForgeTypeId.h>

using namespace repo::manipulator::modelconvertor::odaHelper;

static const char* ODA_CSV_LOCATION = "ODA_CSV_LOCATION";
static const std::string REVIT_ELEMENT_ID = "Element ID";

//These metadata params are not of interest to users. Do not read.
const std::set<std::string> IGNORE_PARAMS = {
	"RENDER APPEARANCE",
	"RENDER APPEARANCE PROPERTIES"
};

bool DataProcessorRvt::ignoreParam(const std::string& param)
{
	auto paramUpper = param;
	std::transform(paramUpper.begin(), paramUpper.end(), paramUpper.begin(), ::toupper);
	return IGNORE_PARAMS.find(paramUpper) != IGNORE_PARAMS.end();
}

std::string DataProcessorRvt::getElementName(OdBmElementPtr element)
{
	std::string elName(convertToStdString(element->getElementName()));
	if (!elName.empty())
		elName.append("_");

	elName.append(std::to_string((OdUInt64)element->objectId().getHandle()));

	return elName;
}

std::string DataProcessorRvt::determineTexturePath(const std::string& inputPath)
{
	// Try to extract one valid paths if multiple paths are provided
	auto pathStr = inputPath.substr(0, inputPath.find("|", 0));
	std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
	auto texturePath = boost::filesystem::path(pathStr).make_preferred();
	if (repo::lib::doesFileExist(texturePath))
		return texturePath.generic_string();

	// Try to apply absolute path
	std::string env = repo::lib::getEnvString(RVT_TEXTURES_ENV_VARIABLE);
	if (env.empty())
		return std::string();

	auto absolutePath = boost::filesystem::absolute(texturePath, env);
	if (repo::lib::doesFileExist(absolutePath))
		return absolutePath.generic_string();

	// Sometimes the texture path has subdirectories like "./mat/1" remove it and see if we can find it.
	auto altPath = boost::filesystem::absolute(texturePath.leaf(), env);
	if (repo::lib::doesFileExist(altPath))
		return altPath.generic_string();

	repoDebug << "Failed to find: " << texturePath;
	return std::string();
}

std::string DataProcessorRvt::translateMetadataValue(
	const OdTfVariant& val,
	OdBmLabelUtilsPEPtr labelUtils,
	OdBmParamDefPtr paramDef,
	OdBmDatabase* database,
	OdBm::BuiltInParameterDefinition::Enum param)
{
	std::string strOut;
	switch (val.type()) {
	case OdTfVariant::kString:
		strOut = convertToStdString(val.getString());
		break;
	case OdTfVariant::kBool:
		strOut = std::to_string(val.getBool());
		break;
	case OdTfVariant::kInt8:
		strOut = std::to_string(val.getInt8());
		break;
	case OdTfVariant::kInt16:
		strOut = std::to_string(val.getInt16());
		break;
	case OdTfVariant::kInt32:
		if (paramDef->getParameterType() == OdBm::ParameterType::YesNo)
			(val.getInt32()) ? strOut = "Yes" : strOut = "No";
		else
			strOut = std::to_string(val.getInt32());
		break;
	case OdTfVariant::kInt64:
		strOut = std::to_string(val.getInt64());
		break;
	case OdTfVariant::kDouble:
	{
		auto valWithUnits = convertToStdString(labelUtils->format(database, paramDef->getUnitType(), val.getDouble(), false));
		std::size_t pos = valWithUnits.find(" ");
		strOut = pos == std::string::npos ? valWithUnits : valWithUnits.substr(0, pos);
	}
	break;
	case OdTfVariant::kDbStubPtr:
		OdDbStub* stub = val.getDbStubPtr();
		if (stub)
		{
			OdBmObjectId rawValue = OdBmObjectId(stub);
			if (param == OdBm::BuiltInParameter::ELEM_CATEGORY_PARAM || param == OdBm::BuiltInParameter::ELEM_CATEGORY_PARAM_MT)
			{
				OdDbHandle hdl = rawValue.getHandle();
				if (OdBmObjectId::isRegularHandle(hdl))
				{
					strOut = std::to_string((OdUInt64)hdl);
				}
				else
				{
					OdBm::BuiltInCategory::Enum builtInValue = static_cast<OdBm::BuiltInCategory::Enum>((OdUInt64)rawValue.getHandle());
					auto category = labelUtils->getLabelFor(OdBm::BuiltInCategory::Enum(builtInValue));
					if (!category.isEmpty()) {
						strOut = convertToStdString(category);
					}
					else {
						strOut = convertToStdString(OdBm::BuiltInCategory(builtInValue).toString());
					}
				}
			}
			else
			{
				OdBmElementPtr elem = database->getObjectId(rawValue.getHandle()).safeOpenObject();
				if (elem->getElementName() == OdString::kEmpty)
					strOut = std::to_string((OdUInt64)rawValue.getHandle());
				else
					strOut = convertToStdString(elem->getElementName());
			}
		}
	}

	return strOut;
}

void DataProcessorRvt::init(GeometryCollector* geoColl, OdBmDatabasePtr database)
{
	this->collector = geoColl;
	this->database = database;
	//getCameras(database);

	establishProjectTranslation(database);
}

DataProcessorRvt::DataProcessorRvt()
{
}

void DataProcessorRvt::beginViewVectorization()
{
	DataProcessor::beginViewVectorization();
	setEyeToOutputTransform(getEyeToWorldTransform());
	initLabelUtils();
}

void DataProcessorRvt::draw(const OdGiDrawable* pDrawable)
{
	fillMeshData(pDrawable);
	OdGsBaseMaterialView::draw(pDrawable);
}

VectoriseDeviceRvt* DataProcessorRvt::device()
{
	return (VectoriseDeviceRvt*)OdGsBaseVectorizeView::device();
}

void DataProcessorRvt::convertTo3DRepoMaterial(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData,
	MaterialColours& matColors,
	repo_material_t& material,
	bool& missingTexture)
{
	DataProcessor::convertTo3DRepoMaterial(prevCache, materialId, materialData, matColors, material, missingTexture);

	OdBmObjectId matId(materialId);
	OdBmMaterialElemPtr materialElem;
	if (matId.isValid())
	{
		OdBmObjectPtr objectPtr = matId.safeOpenObject();
		if (!objectPtr.isNull())
			materialElem = OdBmMaterialElem::cast(objectPtr);
	}

	fillMaterial(materialElem, matColors, material);
	fillTexture(materialElem, material, missingTexture);
}

void DataProcessorRvt::convertTo3DRepoTriangle(
	const OdInt32* p3Vertices,
	std::vector<repo::lib::RepoVector3D64>& verticesOut,
	repo::lib::RepoVector3D64& normalOut,
	std::vector<repo::lib::RepoVector2D>& uvOut)
{
	std::vector<OdGePoint3d> odaPoints;
	getVertices(3, p3Vertices, odaPoints, verticesOut);

	if (verticesOut.size() != 3) {
		return;
	}

	normalOut = calcNormal(verticesOut[0], verticesOut[1], verticesOut[2]);

	OdGiMapperItemEntry::MapInputTriangle trg;

	for (int i = 0; i < odaPoints.size(); ++i)
	{
		trg.inPt[i] = odaPoints[i];
	}

	OdGiMapperItemEntry::MapOutputCoords outTex;

	auto currentMap = currentMapper();
	if (!currentMap.isNull())
	{
		auto diffuseMap = currentMap->diffuseMapper();
		if (!diffuseMap.isNull())
			diffuseMap->mapCoords(trg, outTex);
	}

	uvOut.clear();
	for (int i = 0; i < 3; ++i) {
		uvOut.push_back({ (float)outTex.outCoord[i].x, (float)outTex.outCoord[i].y });
	}
}

std::string DataProcessorRvt::getLevel(OdBmElementPtr element, const std::string& name)
{
	auto levelId = element->getAssocLevelId();
	if (levelId.isValid())
	{
		auto levelObject = levelId.safeOpenObject();
		if (!levelObject.isNull())
		{
			OdBmLevelPtr lptr = OdBmLevel::cast(levelObject);
			if (!lptr.isNull())
				return std::string(convertToStdString(lptr->getElementName()));
		}
	}

	auto owner = element->getOwningElementId();
	if (owner.isValid())
	{
		auto object = owner.openObject();
		if (!object.isNull())
		{
			auto parentElement = OdBmElement::cast(object);
			if (!parentElement.isNull())
				return getLevel(parentElement, name);
		}
	}

	return name;
}

void DataProcessorRvt::fillMeshData(const OdGiDrawable* pDrawable)
{
	OdBmElementPtr element = OdBmElement::cast(pDrawable);
	if (element.isNull())
		return;

	if (!element->isDBRO())
		return;

	collector->stopMeshEntry();

	std::string elementName = getElementName(element);

	collector->setNextMeshName(elementName);
	collector->setMeshGroup(elementName);

	std::string layerName = getLevel(element, "Layer Default");
	// Calling this first time to ensure the layer exists
	collector->setLayer(layerName, layerName);
	// This is the actual layer we want to be on.
	collector->setLayer(elementName, elementName, layerName);

	try
	{
		//some objects material is not set. set default here
		collector->setCurrentMaterial(GetDefaultMaterial());
		if (!collector->hasMeta(elementName)) collector->setMetadata(elementName, fillMetadata(element));
	}
	catch (OdError& er)
	{
		//.. HOTFIX: handle nullPtr exception (reported to ODA)
		repoError << "Caught exception whilst trying to retrieve Material/metadata: " << convertToStdString(er.description());
	}

	collector->startMeshEntry();
}

void DataProcessorRvt::fillMetadataById(
	OdBmObjectId id,
	std::unordered_map<std::string, std::string>& metadata)
{
	if (id.isNull())
		return;

	OdBmObjectPtr ptr = id.safeOpenObject();

	if (ptr.isNull())
		return;

	fillMetadataByElemPtr(ptr, metadata);
}

void DataProcessorRvt::initLabelUtils() {
	if (!labelUtils) {
		OdBmLabelUtilsPEPtr _labelUtils = OdBmObject::desc()->getX(OdBmLabelUtilsPE::desc());
		labelUtils = (OdBmSampleLabelUtilsPE*)_labelUtils.get();
		std::string env = repo::lib::getEnvString(ODA_CSV_LOCATION);
		if (!env.empty()) {
			repoInfo << "Setting root as: " << env;
			labelUtils->setLookupRoot(OdString(env.c_str()));
		}
		else {
			repoWarning << "Cannot find envar ODA_CSV_LOCATION. Metadata may not be processed.";
		}
	}
}

std::string DataProcessorRvt::unitsToString(const OdBm::DisplayUnitType::Enum &units) {
	switch (units) {
	case  OdBm::DisplayUnitType::Enum::DUT_UNDEFINED:
	case  OdBm::DisplayUnitType::Enum::DUT_CUSTOM:
	case  OdBm::DisplayUnitType::Enum::DUT_GENERAL:
	case  OdBm::DisplayUnitType::Enum::DUT_FIXED:
	case OdBm::DisplayUnitType::Enum::DUT_CURRENCY:
		return std::string();
	case  OdBm::DisplayUnitType::Enum::DUT_METERS:
	case  OdBm::DisplayUnitType::Enum::DUT_METERS_CENTIMETERS:
		return "m";
	case  OdBm::DisplayUnitType::Enum::DUT_CENTIMETERS:
		return "cm";
	case  OdBm::DisplayUnitType::Enum::DUT_MILLIMETERS:
		return "mm";
	case  OdBm::DisplayUnitType::Enum::DUT_DECIMAL_FEET:
		return "ft";
	case  OdBm::DisplayUnitType::Enum::DUT_FEET_FRACTIONAL_INCHES:
		return "ft and in";
	case  OdBm::DisplayUnitType::Enum::DUT_FRACTIONAL_INCHES:
	case  OdBm::DisplayUnitType::Enum::DUT_DECIMAL_INCHES:
		return "\"";
	case  OdBm::DisplayUnitType::Enum::DUT_ACRES:
		return "ac";
	case  OdBm::DisplayUnitType::Enum::DUT_HECTARES:
		return "ha";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_YARDS:
		return  u8"yd³";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_FEET:
		return  u8"ft²";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_METERS:
		return  u8"m²";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_METERS:
		return  u8"m³";
	case  OdBm::DisplayUnitType::Enum::DUT_DECIMAL_DEGREES:
		return  u8"°";
	case  OdBm::DisplayUnitType::Enum::DUT_DEGREES_AND_MINUTES:
		return  u8"°,'";
	case  OdBm::DisplayUnitType::Enum::DUT_PERCENTAGE:
		return  "%";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_INCHES:
		return  u8"in²";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_CENTIMETERS:
		return  u8"cm²";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_MILLIMETERS:
		return  u8"mm²";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_INCHES:
		return  u8"in³";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_CENTIMETERS:
		return  u8"cm³";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_MILLIMETERS:
		return  u8"mm³";
	case  OdBm::DisplayUnitType::Enum::DUT_LITERS:
		return  "l";
	case  OdBm::DisplayUnitType::Enum::DUT_GALLONS_US:
		return  "gal US";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOGRAMS_PER_CUBIC_METER:
		return  u8"kg/m³";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_MASS_PER_CUBIC_FOOT:
		return  u8"lb/ft³";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_MASS_PER_CUBIC_INCH:
		return  u8"lb/in³";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNITS:
		return  "Btu";
	case  OdBm::DisplayUnitType::Enum::DUT_CALORIES:
		return  "cal";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOCALORIES:
		return  "kcal";
	case  OdBm::DisplayUnitType::Enum::DUT_JOULES:
		return  "J";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOWATT_HOURS:
		return  "kWh";
	case  OdBm::DisplayUnitType::Enum::DUT_THERMS:
		return  "thm";
	case  OdBm::DisplayUnitType::Enum::DUT_INCHES_OF_WATER_PER_100FT:
		return  "inAq/100ft";
	case  OdBm::DisplayUnitType::Enum::DUT_PASCALS_PER_METER:
		return  "pa/m";
	case  OdBm::DisplayUnitType::Enum::DUT_WATTS:
		return  "W";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOWATTS:
		return  "kW";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNITS_PER_SECOND:
		return  "Btu/s";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNITS_PER_HOUR:
		return  "Btu/hr";
	case  OdBm::DisplayUnitType::Enum::DUT_CALORIES_PER_SECOND:
		return  "cal/s";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOCALORIES_PER_SECOND:
		return  "kcal/s";
	case  OdBm::DisplayUnitType::Enum::DUT_WATTS_PER_SQUARE_FOOT:
		return  u8"W/ft²";
	case  OdBm::DisplayUnitType::Enum::DUT_WATTS_PER_SQUARE_METER:
		return  u8"W/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_INCHES_OF_WATER:
		return  "inAq";
	case  OdBm::DisplayUnitType::Enum::DUT_PASCALS:
		return  "Pa";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOPASCALS:
		return  "kPa";
	case  OdBm::DisplayUnitType::Enum::DUT_MEGAPASCALS:
		return  "MPa";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_FORCE_PER_SQUARE_INCH:
		return  u8"lbf/in²";
	case  OdBm::DisplayUnitType::Enum::DUT_INCHES_OF_MERCURY:
		return  "inHg";
	case  OdBm::DisplayUnitType::Enum::DUT_MILLIMETERS_OF_MERCURY:
		return  "mmHg";
	case  OdBm::DisplayUnitType::Enum::DUT_ATMOSPHERES:
		return  "atm";
	case  OdBm::DisplayUnitType::Enum::DUT_BARS:
		return  "100kPa";
	case  OdBm::DisplayUnitType::Enum::DUT_FAHRENHEIT:
		return  u8"°F";
	case  OdBm::DisplayUnitType::Enum::DUT_CELSIUS:
		return  u8"°C";
	case  OdBm::DisplayUnitType::Enum::DUT_KELVIN:
		return  "K";
	case  OdBm::DisplayUnitType::Enum::DUT_RANKINE:
		return  u8"°R";
	case  OdBm::DisplayUnitType::Enum::DUT_FEET_PER_MINUTE:
		return  "ft/min";
	case  OdBm::DisplayUnitType::Enum::DUT_METERS_PER_SECOND:
		return  "m/s";
	case  OdBm::DisplayUnitType::Enum::DUT_CENTIMETERS_PER_MINUTE:
		return  "cm/min";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_FEET_PER_MINUTE:
		return  u8"ft³/min";
	case  OdBm::DisplayUnitType::Enum::DUT_LITERS_PER_SECOND:
		return  "l/s";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_METERS_PER_SECOND:
		return  u8"m³/s";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_METERS_PER_HOUR:
		return  u8"m³/hr";
	case  OdBm::DisplayUnitType::Enum::DUT_GALLONS_US_PER_MINUTE:
		return  "gal/min";
	case  OdBm::DisplayUnitType::Enum::DUT_GALLONS_US_PER_HOUR:
		return  "gal/hr";
	case  OdBm::DisplayUnitType::Enum::DUT_AMPERES:
		return  "A";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOAMPERES:
		return  "kA";
	case  OdBm::DisplayUnitType::Enum::DUT_MILLIAMPERES:
		return  "mA";
	case  OdBm::DisplayUnitType::Enum::DUT_VOLTS:
		return  "V";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOVOLTS:
		return  "kV";
	case  OdBm::DisplayUnitType::Enum::DUT_MILLIVOLTS:
		return  "mV";
	case  OdBm::DisplayUnitType::Enum::DUT_HERTZ:
		return  "Hz";
	case  OdBm::DisplayUnitType::Enum::DUT_CYCLES_PER_SECOND:
		return  "cycles/s";
	case  OdBm::DisplayUnitType::Enum::DUT_LUX:
		return  "lx";
	case  OdBm::DisplayUnitType::Enum::DUT_FOOTCANDLES:
		return  "fc";
	case  OdBm::DisplayUnitType::Enum::DUT_FOOTLAMBERTS:
		return  "ftL";
	case  OdBm::DisplayUnitType::Enum::DUT_CANDELAS_PER_SQUARE_METER:
		return  u8"cd/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_CANDELAS:
		return  "cd";
	case  OdBm::DisplayUnitType::Enum::DUT_CANDLEPOWER:
		return  "cp";
	case  OdBm::DisplayUnitType::Enum::DUT_LUMENS:
		return  "lm";
	case  OdBm::DisplayUnitType::Enum::DUT_VOLT_AMPERES:
		return  "VA";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOVOLT_AMPERES:
		return  "kVA";
	case  OdBm::DisplayUnitType::Enum::DUT_HORSEPOWER:
		return  "hp";
	case  OdBm::DisplayUnitType::Enum::DUT_NEWTONS:
		return  "N";
	case  OdBm::DisplayUnitType::Enum::DUT_DECANEWTONS:
		return  "dN";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTONS:
		return  "kN";
	case  OdBm::DisplayUnitType::Enum::DUT_MEGANEWTONS:
		return  "MN";
	case  OdBm::DisplayUnitType::Enum::DUT_KIPS:
		return  "kip";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOGRAMS_FORCE:
		return  "kgF";
	case  OdBm::DisplayUnitType::Enum::DUT_TONNES_FORCE:
		return  "tF";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_FORCE:
		return  "lbF";
	case  OdBm::DisplayUnitType::Enum::DUT_NEWTONS_PER_METER:
		return  "N/m";
	case  OdBm::DisplayUnitType::Enum::DUT_DECANEWTONS_PER_METER:
		return  "dN/m";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTONS_PER_METER:
		return  "kN/m";
	case  OdBm::DisplayUnitType::Enum::DUT_MEGANEWTONS_PER_METER:
		return  "MN/m";
	case  OdBm::DisplayUnitType::Enum::DUT_KIPS_PER_FOOT:
		return  "kip/ft";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOGRAMS_FORCE_PER_METER:
		return  "kgF/m";
	case  OdBm::DisplayUnitType::Enum::DUT_TONNES_FORCE_PER_METER:
		return  "tF/m";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_FORCE_PER_FOOT:
		return  "lbF/ft";
	case  OdBm::DisplayUnitType::Enum::DUT_NEWTONS_PER_SQUARE_METER:
		return  u8"N/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_DECANEWTONS_PER_SQUARE_METER:
		return  u8"dN/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTONS_PER_SQUARE_METER:
		return  u8"kN/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_MEGANEWTONS_PER_SQUARE_METER:
		return  u8"MN/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_KIPS_PER_SQUARE_FOOT:
		return  u8"kip/ft²";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOGRAMS_FORCE_PER_SQUARE_METER:
		return  u8"kg/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_TONNES_FORCE_PER_SQUARE_METER:
		return  u8"TF/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_FORCE_PER_SQUARE_FOOT:
		return  u8"lbF/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_NEWTON_METERS:
		return  "Nm";
	case  OdBm::DisplayUnitType::Enum::DUT_DECANEWTON_METERS:
		return  "dNm";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTON_METERS:
		return  "kNm";
	case  OdBm::DisplayUnitType::Enum::DUT_MEGANEWTON_METERS:
		return  "MNm";
	case  OdBm::DisplayUnitType::Enum::DUT_KIP_FEET:
		return  "kipft";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOGRAM_FORCE_METERS:
		return  "kgFm";
	case  OdBm::DisplayUnitType::Enum::DUT_TONNE_FORCE_METERS:
		return  "TFm";
	case  OdBm::DisplayUnitType::Enum::DUT_POUND_FORCE_FEET:
		return  "lbFft";
	case  OdBm::DisplayUnitType::Enum::DUT_METERS_PER_KILONEWTON:
		return  "m/kN";
	case  OdBm::DisplayUnitType::Enum::DUT_FEET_PER_KIP:
		return  "ft/kip";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_METERS_PER_KILONEWTON:
		return  u8"m²/kN";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_FEET_PER_KIP:
		return  "ft/kip";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_METERS_PER_KILONEWTON:
		return  u8"m³/kN";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_FEET_PER_KIP:
		return  u8"ft³/kip";
	case  OdBm::DisplayUnitType::Enum::DUT_INV_KILONEWTONS:
		return  u8"kN⁻¹";
	case  OdBm::DisplayUnitType::Enum::DUT_INV_KIPS:
		return u8"kip⁻¹";
	case  OdBm::DisplayUnitType::Enum::DUT_FEET_OF_WATER_PER_100FT:
		return "ftAq/100ft";
	case  OdBm::DisplayUnitType::Enum::DUT_FEET_OF_WATER:
		return "ftAq";
	case  OdBm::DisplayUnitType::Enum::DUT_PASCAL_SECONDS:
		return "Pas";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_MASS_PER_FOOT_SECOND:
		return "lb/fts";
	case  OdBm::DisplayUnitType::Enum::DUT_CENTIPOISES:
		return "cP";
	case  OdBm::DisplayUnitType::Enum::DUT_FEET_PER_SECOND:
		return "ft/s";
	case  OdBm::DisplayUnitType::Enum::DUT_KIPS_PER_SQUARE_INCH:
		return u8"kips/in²";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTONS_PER_CUBIC_METER:
		return u8"kN/m³";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_FORCE_PER_CUBIC_FOOT:
		return u8"lbF/ft³";
	case  OdBm::DisplayUnitType::Enum::DUT_KIPS_PER_CUBIC_INCH:
		return u8"kip/in³";
	case  OdBm::DisplayUnitType::Enum::DUT_INV_FAHRENHEIT:
		return u8"°F⁻¹";
	case  OdBm::DisplayUnitType::Enum::DUT_INV_CELSIUS:
		return u8"°C⁻¹";
	case  OdBm::DisplayUnitType::Enum::DUT_NEWTON_METERS_PER_METER:
		return "Nm/m";
	case  OdBm::DisplayUnitType::Enum::DUT_DECANEWTON_METERS_PER_METER:
		return "dNm/m";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTON_METERS_PER_METER:
		return "km/m";
	case  OdBm::DisplayUnitType::Enum::DUT_MEGANEWTON_METERS_PER_METER:
		return "Mm/m";
	case  OdBm::DisplayUnitType::Enum::DUT_KIP_FEET_PER_FOOT:
		return "kipft/ft";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOGRAM_FORCE_METERS_PER_METER:
		return "kgFm/m";
	case  OdBm::DisplayUnitType::Enum::DUT_TONNE_FORCE_METERS_PER_METER:
		return "TFm/m";
	case  OdBm::DisplayUnitType::Enum::DUT_POUND_FORCE_FEET_PER_FOOT:
		return "lbFft/ft";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_MASS_PER_FOOT_HOUR:
		return "lb/fth";
	case  OdBm::DisplayUnitType::Enum::DUT_KIPS_PER_INCH:
		return "kip/in";
	case  OdBm::DisplayUnitType::Enum::DUT_KIPS_PER_CUBIC_FOOT:
		return u8"kip/ft³";
	case  OdBm::DisplayUnitType::Enum::DUT_KIP_FEET_PER_DEGREE:
		return u8"kipft/°";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTON_METERS_PER_DEGREE:
		return u8"kNm/°";
	case  OdBm::DisplayUnitType::Enum::DUT_KIP_FEET_PER_DEGREE_PER_FOOT:
		return "kipft/ft";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTON_METERS_PER_DEGREE_PER_METER:
		return u8"kipft/°/ft";
	case  OdBm::DisplayUnitType::Enum::DUT_WATTS_PER_SQUARE_METER_KELVIN:
		return u8"W/m²K";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNITS_PER_HOUR_SQUARE_FOOT_FAHRENHEIT:
		return u8"btu/hft²/°";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_FEET_PER_MINUTE_SQUARE_FOOT:
		return u8"ft³/mft²";
	case  OdBm::DisplayUnitType::Enum::DUT_LITERS_PER_SECOND_SQUARE_METER:
		return u8"l/sm²";
	case  OdBm::DisplayUnitType::Enum::DUT_RATIO_10:
		return "10:x";
	case  OdBm::DisplayUnitType::Enum::DUT_RATIO_12:
		return "12:x";
	case  OdBm::DisplayUnitType::Enum::DUT_SLOPE_DEGREES:
		return "slope degrees";
	case  OdBm::DisplayUnitType::Enum::DUT_RISE_OVER_INCHES:
		return "rise over in";
	case  OdBm::DisplayUnitType::Enum::DUT_RISE_OVER_FOOT:
		return "rise over ft";
	case  OdBm::DisplayUnitType::Enum::DUT_RISE_OVER_MMS:
		return "rise over mms";
	case  OdBm::DisplayUnitType::Enum::DUT_WATTS_PER_CUBIC_FOOT:
		return u8"W/ft³";
	case  OdBm::DisplayUnitType::Enum::DUT_WATTS_PER_CUBIC_METER:
		return "W/m³";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNITS_PER_HOUR_SQUARE_FOOT:
		return u8"btu/hft²";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNITS_PER_HOUR_CUBIC_FOOT:
		return u8"btu/hft³";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_FEET_PER_MINUTE_CUBIC_FOOT:
		return u8"ft³/mft³";
	case  OdBm::DisplayUnitType::Enum::DUT_LITERS_PER_SECOND_CUBIC_METER:
		return u8"l/sm³";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_FEET_PER_MINUTE_TON_OF_REFRIGERATION:
		return u8"ft³/mRT";
	case  OdBm::DisplayUnitType::Enum::DUT_LITERS_PER_SECOND_KILOWATTS:
		return "l/skW";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_FEET_PER_TON_OF_REFRIGERATION:
		return u8"ft²/RT";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_METERS_PER_KILOWATTS:
		return u8"m²/kW";
	case  OdBm::DisplayUnitType::Enum::DUT_LUMENS_PER_WATT:
		return "lum/W";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_FEET_PER_THOUSAND_BRITISH_THERMAL_UNITS_PER_HOUR:
		return u8"ft²/kbtu/h";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTONS_PER_SQUARE_CENTIMETER:
		return u8"kN/cm²";
	case  OdBm::DisplayUnitType::Enum::DUT_NEWTONS_PER_SQUARE_MILLIMETER:
		return u8"N/mm²";
	case  OdBm::DisplayUnitType::Enum::DUT_KILONEWTONS_PER_SQUARE_MILLIMETER:
		return u8"kN/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_RISE_OVER_120_INCHES:
		return u8"rise/120in";
	case  OdBm::DisplayUnitType::Enum::DUT_1_RATIO:
		return u8"1:x";
	case  OdBm::DisplayUnitType::Enum::DUT_RISE_OVER_10_FEET:
		return u8"rise/10ft";
	case  OdBm::DisplayUnitType::Enum::DUT_HOUR_SQUARE_FOOT_FAHRENHEIT_PER_BRITISH_THERMAL_UNIT:
		return u8"hft²°F/btu";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_METER_KELVIN_PER_WATT:
		return u8"m²K/W";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNIT_PER_FAHRENHEIT:
		return u8"btu/°F";
	case  OdBm::DisplayUnitType::Enum::DUT_JOULES_PER_KELVIN:
		return u8"J/K";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOJOULES_PER_KELVIN:
		return u8"kJ/K";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOGRAMS_MASS:
		return u8"kg";
	case  OdBm::DisplayUnitType::Enum::DUT_TONNES_MASS:
		return u8"T";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_MASS:
		return u8"lb";
	case  OdBm::DisplayUnitType::Enum::DUT_METERS_PER_SECOND_SQUARED:
		return u8"m/s²";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOMETERS_PER_SECOND_SQUARED:
		return u8"km/s²";
	case  OdBm::DisplayUnitType::Enum::DUT_INCHES_PER_SECOND_SQUARED:
		return u8"in/s²";
	case  OdBm::DisplayUnitType::Enum::DUT_FEET_PER_SECOND_SQUARED:
		return u8"ft/s²";
	case  OdBm::DisplayUnitType::Enum::DUT_MILES_PER_SECOND_SQUARED:
		return u8"miles/s²";
	case  OdBm::DisplayUnitType::Enum::DUT_FEET_TO_THE_FOURTH_POWER:
		return u8"ft⁴";
	case  OdBm::DisplayUnitType::Enum::DUT_INCHES_TO_THE_FOURTH_POWER:
		return u8"in⁴";
	case  OdBm::DisplayUnitType::Enum::DUT_MILLIMETERS_TO_THE_FOURTH_POWER:
		return u8"mm⁴";
	case  OdBm::DisplayUnitType::Enum::DUT_CENTIMETERS_TO_THE_FOURTH_POWER:
		return u8"cm⁴";
	case  OdBm::DisplayUnitType::Enum::DUT_METERS_TO_THE_FOURTH_POWER:
		return u8"m⁴";
	case  OdBm::DisplayUnitType::Enum::DUT_FEET_TO_THE_SIXTH_POWER:
		return u8"ft⁶";
	case  OdBm::DisplayUnitType::Enum::DUT_INCHES_TO_THE_SIXTH_POWER:
		return u8"in⁶";
	case  OdBm::DisplayUnitType::Enum::DUT_MILLIMETERS_TO_THE_SIXTH_POWER:
		return u8"mm⁶";
	case  OdBm::DisplayUnitType::Enum::DUT_CENTIMETERS_TO_THE_SIXTH_POWER:
		return u8"cm⁶";
	case  OdBm::DisplayUnitType::Enum::DUT_METERS_TO_THE_SIXTH_POWER:
		return u8"m⁶";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_FEET_PER_FOOT:
		return u8"ft²/ft";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_INCHES_PER_FOOT:
		return u8"in²/ft";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_MILLIMETERS_PER_METER:
		return u8"mm²/m";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_CENTIMETERS_PER_METER:
		return u8"cm²/m";
	case  OdBm::DisplayUnitType::Enum::DUT_SQUARE_METERS_PER_METER:
		return u8"m²/m";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOGRAMS_MASS_PER_METER:
		return u8"kg/m";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_MASS_PER_FOOT:
		return u8"lb/ft";
	case  OdBm::DisplayUnitType::Enum::DUT_RADIANS:
		return u8"rad";
	case  OdBm::DisplayUnitType::Enum::DUT_GRADS:
		return u8"grad";
	case  OdBm::DisplayUnitType::Enum::DUT_RADIANS_PER_SECOND:
		return u8"rad/s";
	case  OdBm::DisplayUnitType::Enum::DUT_MILISECONDS:
		return u8"ms";
	case  OdBm::DisplayUnitType::Enum::DUT_SECONDS:
		return u8"s";
	case  OdBm::DisplayUnitType::Enum::DUT_MINUTES:
		return u8"min";
	case  OdBm::DisplayUnitType::Enum::DUT_HOURS:
		return u8"h";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOMETERS_PER_HOUR:
		return u8"km/h";
	case  OdBm::DisplayUnitType::Enum::DUT_MILES_PER_HOUR:
		return u8"mile/h";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOJOULES:
		return u8"kJ";
	case  OdBm::DisplayUnitType::Enum::DUT_KILOGRAMS_MASS_PER_SQUARE_METER:
		return u8"kg/m²";
	case  OdBm::DisplayUnitType::Enum::DUT_POUNDS_MASS_PER_SQUARE_FOOT:
		return u8"lb/ft²";
	case  OdBm::DisplayUnitType::Enum::DUT_WATTS_PER_METER_KELVIN:
		return u8"W/mK";
	case  OdBm::DisplayUnitType::Enum::DUT_JOULES_PER_GRAM_CELSIUS:
		return u8"J/g°C";
	case  OdBm::DisplayUnitType::Enum::DUT_JOULES_PER_GRAM:
		return u8"J/g";
	case  OdBm::DisplayUnitType::Enum::DUT_NANOGRAMS_PER_PASCAL_SECOND_SQUARE_METER:
		return u8"ng/Pasm²";
	case  OdBm::DisplayUnitType::Enum::DUT_OHM_METERS:
		return u8"Ωm";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNITS_PER_HOUR_FOOT_FAHRENHEIT:
		return u8"btu/hft°F";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNITS_PER_POUND_FAHRENHEIT:
		return u8"btu/lb°F";
	case  OdBm::DisplayUnitType::Enum::DUT_BRITISH_THERMAL_UNITS_PER_POUND:
		return u8"btulb";
	case  OdBm::DisplayUnitType::Enum::DUT_GRAINS_PER_HOUR_SQUARE_FOOT_INCH_MERCURY:
		return u8"gr/ft²inHg";
	case  OdBm::DisplayUnitType::Enum::DUT_PER_MILLE:
		return u8"per mille";
	case  OdBm::DisplayUnitType::Enum::DUT_DECIMETERS:
		return u8"dm";
	case  OdBm::DisplayUnitType::Enum::DUT_JOULES_PER_KILOGRAM_CELSIUS:
		return u8"J/kg°C";
	case  OdBm::DisplayUnitType::Enum::DUT_MICROMETERS_PER_METER_CELSIUS:
		return u8" μm/m°C";
	case  OdBm::DisplayUnitType::Enum::DUT_MICROINCHES_PER_INCH_FAHRENHEIT:
		return u8"μin/in°F";
	case  OdBm::DisplayUnitType::Enum::DUT_USTONNES_MASS:
		return u8"T (US)";
	case  OdBm::DisplayUnitType::Enum::DUT_USTONNES_FORCE:
		return u8"stones";
	case  OdBm::DisplayUnitType::Enum::DUT_LITERS_PER_MINUTE:
		return u8"l/min";
	case  OdBm::DisplayUnitType::Enum::DUT_FAHRENHEIT_DIFFERENCE:
		return u8"°F diff";
	case  OdBm::DisplayUnitType::Enum::DUT_CELSIUS_DIFFERENCE:
		return u8"°C diff";
	case  OdBm::DisplayUnitType::Enum::DUT_KELVIN_DIFFERENCE:
		return u8"K diff";
	case  OdBm::DisplayUnitType::Enum::DUT_RANKINE_DIFFERENCE:
		return u8"°R difference";
	case  OdBm::DisplayUnitType::Enum::DUT_STATIONING_METERS:
		return u8"stationing m";
	case  OdBm::DisplayUnitType::Enum::DUT_STATIONING_FEET:
		return u8"stationing ft";
	case  OdBm::DisplayUnitType::Enum::DUT_CUBIC_FEET_PER_HOUR:
		return u8"ft³/hr";
	case  OdBm::DisplayUnitType::Enum::DUT_LITERS_PER_HOUR:
		return u8"l/hr";
	case  OdBm::DisplayUnitType::Enum::DUT_RATIO_TO_1:
		return u8"1:x";
	case  OdBm::DisplayUnitType::Enum::DUT_DECIMAL_US_SURVEY_FEET:
		return u8"ft (US)";
	}
	auto unitsText = convertToStdString(labelUtils->getLabelFor(OdBm::DisplayUnitType::Enum(units)));
	repoError << "Unrecongised unit type : " << unitsText;
	return unitsText;
}

void DataProcessorRvt::processParameter(
	OdBmElementPtr element,
	OdBmObjectId paramId,
	OdBmAUnitsPtr pAUnits,
	std::unordered_map<std::string, std::string> &metadata,
	const OdBm::BuiltInParameterDefinition::Enum &buildInEnum
) {
	OdTfVariant value;
	OdResult res = element->getParam(paramId, value);

	if (res == eOk)
	{
		OdBmParamElemPtr pParamElem = paramId.safeOpenObject();
		OdBmParamDefPtr pDescParam = pParamElem->getParamDef();
		OdInt64 groupId = pDescParam->getGroupElemId();

		auto metaKey = convertToStdString(pDescParam->getCaption());
		if (!ignoreParam(metaKey)) {
			auto paramGroup = labelUtils->getLabelFor(OdBm::BuiltInParameterGroup::Enum(groupId));
			if (!paramGroup.isEmpty()) {
				metaKey = convertToStdString(paramGroup) + "::" + metaKey;
			}

			if (pDescParam->getUnitType() != OdBm::UnitType::UT_Undefined) {
				auto units = unitsToString(OdBmUnitUtils::getDefaultDUT(pAUnits, pDescParam->getUnitType()));
				if (!units.empty()) metaKey += " (" + units + ")";
			}
			std::string variantValue = translateMetadataValue(value, labelUtils, pDescParam, element->getDatabase(), buildInEnum);
			if (!variantValue.empty())
			{
				if (metadata.find(metaKey) != metadata.end() && metadata[metaKey] != variantValue) {
					repoDebug << "FOUND MULTIPLE ENTRY WITH DIFFERENT VALUES: " << metaKey << "value before: " << metadata[metaKey] << " after: " << variantValue;
				}
				metadata[metaKey] = variantValue;
			}
		}
	}
}

void DataProcessorRvt::fillMetadataByElemPtr(
	OdBmElementPtr element,

	std::unordered_map<std::string, std::string>& outputData)
{
	OdBmParameterSet aParams;
	element->getListParams(aParams);

	std::unordered_map<std::string, std::string> metadata;

	OdBmUnitsTrackingPtr pUnitsTracking = database->getAppInfo(OdBm::ManagerType::UnitsTracking);
	OdBmUnitsElemPtr pUnitsElem = pUnitsTracking->getUnitsElemId().safeOpenObject();
	OdBmAUnitsPtr pAUnits = pUnitsElem->getUnits();

	auto id = std::to_string((OdUInt64)element->objectId().getHandle());
	if (collector->metadataCache.find(id) != collector->metadataCache.end()) {
		metadata = collector->metadataCache[id];
	}
	else {
		if (!labelUtils) return;
		for (const auto &entry : aParams.getBuiltInParamsIterator()) {
			std::string builtInName = convertToStdString(OdBm::BuiltInParameter(entry).toString());
			//.. HOTFIX: handle access violation exception (reported to ODA)
			if (ignoreParam(builtInName)) continue;

			processParameter(element, element->database()->getObjectId(entry), pAUnits, metadata, entry);
		}
	}

	for (const auto &entry : aParams.getUserParamsIterator()) {
		processParameter(element, entry, pAUnits, metadata,
			//A dummy entry, as long as it's not ELEM_CATEGORY_PARAM_MT or ELEM_CATEGORY_PARAM it's not utilised.
			OdBm::BuiltInParameterDefinition::Enum::ACTUAL_MAX_RIDGE_HEIGHT_PARAM);
	}

	if (metadata.size()) {
		outputData.insert(metadata.begin(), metadata.end());
	}
}

std::unordered_map<std::string, std::string> DataProcessorRvt::fillMetadata(OdBmElementPtr element)
{
	std::unordered_map<std::string, std::string> metadata;
	metadata[REVIT_ELEMENT_ID] = std::to_string((OdUInt64)element->objectId().getHandle());
	try
	{
		fillMetadataByElemPtr(element, metadata);
	}
	catch (OdError& er)
	{
		repoInfo << "Caught exception whilst trying to fetch metadata by element pointer " << convertToStdString(er.description());

		repoTrace << "Caught exception whilst trying to fetch metadata by element pointer " << convertToStdString(er.description());
	}

	try
	{
		fillMetadataById(element->getFamId(), metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to get metadata by family ID: " << convertToStdString(er.description());
	}

	try
	{
		fillMetadataById(element->getTypeID(), metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to get metadata by Type ID: " << convertToStdString(er.description());
	}
	try
	{
		fillMetadataById(element->getHeaderCategoryId(), metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to get ,etadata nu category ID: " << convertToStdString(er.description());
	}
	return metadata;
}

void DataProcessorRvt::fillMaterial(OdBmMaterialElemPtr materialPtr, const MaterialColours& matColors, repo_material_t& material)
{
	const float norm = 255.f;

	material.specular = { ODGETRED(matColors.colorSpecular) / norm, ODGETGREEN(matColors.colorSpecular) / norm, ODGETBLUE(matColors.colorSpecular) / norm, 1.0f };
	material.ambient = { ODGETRED(matColors.colorAmbient) / norm, ODGETGREEN(matColors.colorAmbient) / norm, ODGETBLUE(matColors.colorAmbient) / norm, 1.0f };
	material.emissive = { ODGETRED(matColors.colorEmissive) / norm, ODGETGREEN(matColors.colorEmissive) / norm, ODGETBLUE(matColors.colorEmissive) / norm, 1.0f };
	material.diffuse = material.emissive;

	if (!materialPtr.isNull())
		material.shininess = (float)materialPtr->getMaterial()->getShininess() / norm;
}

void DataProcessorRvt::fillTexture(OdBmMaterialElemPtr materialPtr, repo_material_t& material, bool& missingTexture)
{
	missingTexture = false;

	if (materialPtr.isNull())
		return;

	OdBmMaterialPtr matPtr = materialPtr->getMaterial();
	OdBmAssetPtr textureAsset = matPtr->getAsset();

	if (textureAsset.isNull())
		return;

	OdBmAssetPtr bitmapAsset;
	OdString textureFileName;
	OdResult es = OdBmAppearanceAssetHelper::getTextureAsset(textureAsset, bitmapAsset);

	if ((es != OdResult::eOk) || (bitmapAsset.isNull()))
		return;

	OdBmUnifiedBitmapSchemaHelper textureHelper(bitmapAsset);
	es = textureHelper.getTextureFileName(textureFileName);

	if (es != OdResult::eOk)
		return;

	std::string textureName(convertToStdString(textureFileName));
	std::string validTextureName = determineTexturePath(textureName);
	if (validTextureName.empty() && !textureName.empty())
		missingTexture = true;

	material.texturePath = validTextureName;
}

OdBm::DisplayUnitType::Enum DataProcessorRvt::getUnits(OdBmDatabasePtr database)
{
	OdBmFormatOptionsPtrArray aFormatOptions;
	OdBmUnitsTrackingPtr pUnitsTracking = database->getAppInfo(OdBm::ManagerType::UnitsTracking);
	OdBmUnitsElemPtr pUnitsElem = pUnitsTracking->getUnitsElemId().safeOpenObject();
	OdBmAUnitsPtr ptrAUnits = pUnitsElem->getUnits().get();
	OdBmFormatOptionsPtr formatOptionsLength = ptrAUnits->getFormatOptions(OdBm::UnitType::Enum::UT_Length);
	return formatOptionsLength->getUnitTypeId();
}

void DataProcessorRvt::getCameras(OdBmDatabasePtr database)
{
	if (collector->hasCameraNodes())
		return;

	forEachBmDBView(database, [&](OdBmDBViewPtr pDBView) { collector->addCameraNode(convertCamera(pDBView)); });
}

void getCameraConfigurations(OdGsViewImpl& view, float& aspectRatio, float& farClipPlane, float& nearClipPlane, float& FOV)
{
	//NOTE : configurations were taken from current 3d view
	aspectRatio = view.windowAspect();
	farClipPlane = view.frontClip();
	nearClipPlane = view.backClip();
	FOV = view.lensLengthToFOV(view.lensLength());
}

camera_t DataProcessorRvt::convertCamera(OdBmDBViewPtr view)
{
	camera_t camera;
	getCameraConfigurations(this->view(), camera.aspectRatio, camera.farClipPlane, camera.nearClipPlane, camera.FOV);
	auto eye = view->getViewDirection();
	auto pos = view->getOrigin();
	auto up = view->getUpDirection();
	camera.eye = repo::lib::RepoVector3D(eye.x, eye.y, eye.z);
	camera.pos = repo::lib::RepoVector3D(pos.x, pos.y, pos.z);
	camera.up = repo::lib::RepoVector3D(up.z, up.y, up.z);
	camera.name = view->getNamed() ? convertToStdString(view->getViewName()) : "camera";
	return camera;
}

//.. TODO: Comment this code for newer version of ODA BIM library
void DataProcessorRvt::establishProjectTranslation(OdBmDatabase* pDb)
{
	OdBmElementTrackingDataPtr pElementTrackingDataMgr = pDb->getAppInfo(OdBm::ManagerType::ElementTrackingData);
	OdBmObjectIdArray aElements;
	OdResult res = pElementTrackingDataMgr->getElementsByType(
		pDb->getObjectId(OdBm::BuiltInCategory::OST_ProjectBasePoint),
		OdBm::TrackingElementType::Elements,
		aElements);

	if (!aElements.isEmpty())
	{
		OdBmBasePointPtr pThis = aElements.first().safeOpenObject();

		if (pThis->getLocationType() == 0)
		{
			if (OdBmGeoLocation::isGeoLocationAllowed(pThis->database()))
			{
				OdBmGeoLocationPtr pActiveLocation = OdBmGeoLocation::getActiveLocationId(pThis->database()).safeOpenObject();
				OdGeMatrix3d activeTransform = pActiveLocation->getTransform();
				OdGePoint3d activeOrigin;
				OdGeVector3d activeX, activeY, activeZ;
				activeTransform.getCoordSystem(activeOrigin, activeX, activeY, activeZ);

				OdBmGeoLocationPtr pProjectLocation = OdBmGeoLocation::getProjectLocationId(pThis->database()).safeOpenObject();
				OdGeMatrix3d projectTransform = pProjectLocation->getTransform();
				OdGePoint3d projectOrigin;
				OdGeVector3d projectX, projectY, projectZ;
				projectTransform.getCoordSystem(projectOrigin, projectX, projectY, projectZ);

				OdGeMatrix3d alignedLocation;
				alignedLocation.setToAlignCoordSys(activeOrigin, activeX, activeY, activeZ, projectOrigin, projectX, projectY, projectZ);

				auto scaleCoef = 1.0 / OdBmUnitUtils::getDisplayUnitTypeInfo(getUnits(database)).inIntUnitsCoeff;
				convertTo3DRepoWorldCoorindates = [activeOrigin, alignedLocation, scaleCoef](OdGePoint3d point) {
					auto convertedPoint = (point - activeOrigin).transformBy(alignedLocation);
					return repo::lib::RepoVector3D64(convertedPoint.x * scaleCoef, convertedPoint.y * scaleCoef, convertedPoint.z * scaleCoef);
				};
			}
		}
	}
}