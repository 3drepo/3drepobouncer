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
#include <Database/Entities/BmRbsSystemType.h>
#include <Base/BmForgeTypeId.h>
#include <Base/BmParameterSet.h>
#include <Base/BmSpecTypeId.h>

using namespace repo::manipulator::modelconvertor::odaHelper;

using ModelUnits = repo::manipulator::modelconvertor::ModelUnits;

static const char* ODA_CSV_LOCATION = "ODA_CSV_LOCATION";
static const std::string REVIT_ELEMENT_ID = "Element ID";

//These metadata params are not of interest to users. Do not read.
const std::set<std::string> IGNORE_PARAMS = {
	"RENDER APPEARANCE",
	"RENDER APPEARANCE PROPERTIES"
};

bool DataProcessorRvt::ignoreParam(const std::string& param)
{
	std::string paramUpper = param; // Copy the string to transform it in place
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
	auto texturePath = boost::filesystem::path(pathStr); // explictly store the value before calling make_preferred().
	texturePath = texturePath.make_preferred();
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
	OdBm::BuiltInParameter::Enum param)
{
	std::string strOut;
	switch (val.type()) {
	case OdVariant::kVoid:
		break;
	case OdVariant::kString:
		strOut = convertToStdString(val.getString());
		break;
	case OdVariant::kBool:
		strOut = std::to_string(val.getBool());
		break;
	case OdVariant::kInt8:
		strOut = std::to_string(val.getInt8());
		break;
	case OdVariant::kInt16:
		strOut = std::to_string(val.getInt16());
		break;
	case OdVariant::kInt32:
		if (paramDef->getParameterTypeId() == OdBmSpecTypeId::Boolean::kYesNo)
			(val.getInt32()) ? strOut = "Yes" : strOut = "No";
		else
			strOut = std::to_string(val.getInt32());
		break;
	case OdVariant::kInt64:
		strOut = std::to_string(val.getInt64());
		break;
	case OdVariant::kDouble:
	{
		auto valWithUnits = convertToStdString(labelUtils->format(database, paramDef->getSpecTypeId(), val.getDouble(), false));
		std::size_t pos = valWithUnits.find(" ");
		strOut = pos == std::string::npos ? valWithUnits : valWithUnits.substr(0, pos);
	}
	break;
	case OdVariant::kAnsiString:
		strOut = std::string(val.getAnsiString().c_str());
		break;
	case OdTfVariant::kDbStubPtr:

		// A stub is effectively a pointer to another database, or built-in, object. We don't recurse these objects, but will try to extract
		// their names or identities where possible (e.g. if a key pointed to a wall object, we would get the name of the wall object).

		OdDbStub* stub = val.getDbStubPtr();
		OdBmObjectId bmId = OdBmObjectId(stub);
		if (stub)
		{
			OdDbHandle hdl = bmId.getHandle();
			if (param == OdBm::BuiltInParameter::ELEM_CATEGORY_PARAM || param == OdBm::BuiltInParameter::ELEM_CATEGORY_PARAM_MT)
			{
				if (OdBmObjectId::isRegularHandle(hdl)) // A regular handle points to a database entry; if it is not regular, it is built-in.
				{
					strOut = std::to_string((OdUInt64)hdl);
				}
				else
				{
					OdBm::BuiltInCategory::Enum builtInValue = static_cast<OdBm::BuiltInCategory::Enum>((OdUInt64)hdl);
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
				OdBmObjectPtr bmPtr = bmId.openObject();
				if (!bmPtr.isNull())
				{
					// The object class is unknown - some superclasses we can handle explicitly.

					auto bmPtrClass = bmPtr->isA();
					if (!bmPtrClass->isKindOf(OdBmElement::desc()))
					{
						OdBmElementPtr elem = bmPtr;
						if (elem->getElementName() == OdString::kEmpty) {
							strOut = std::to_string((OdUInt64)bmId.getHandle());
						}
						else {
							strOut = convertToStdString(elem->getElementName());
						}
					}
					else
					{
						repoError << "Unsupported metadata value type (class) " << convertToStdString(bmPtrClass->name()) << " currently only OdBmElement's are supported.";
					}
				}
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

	geoColl->units = getProjectUnits(database);
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
	if (!matId.isNull() && matId.isValid())
	{
		OdBmObjectPtr objectPtr = matId.safeOpenObject();
		if (!objectPtr.isNull())
			materialElem = OdBmMaterialElem::cast(objectPtr);
	}

	fillMaterial(materialElem, matColors, material);
	fillTexture(materialElem, material, missingTexture);
}

void DataProcessorRvt::convertTo3DRepoTriangle(
	const OdInt32* indices,
	std::vector<repo::lib::RepoVector3D64>& verticesOut,
	repo::lib::RepoVector3D64& normalOut,
	std::vector<repo::lib::RepoVector2D>& uvsOut)
{
	std::vector<OdGePoint3d> odaPoints;
	getVertices(3, indices, odaPoints, verticesOut);

	if (verticesOut.size() != 3) {
		return;
	}

	normalOut = calcNormal(verticesOut[0], verticesOut[1], verticesOut[2]);

	if (isMapperEnabled() && isMapperAvailable()) {

		std::vector<OdGePoint2d> odaUvs;

		if (vertexData() && vertexData()->mappingCoords(OdGiVertexData::kAllChannels))
		{
			// Where Uvs are predefined, we need to get them for each vertex from the
			// dedicated array using the indices again...

			OdGiMapperItemEntryPtr mapper = currentMapper(false)->diffuseMapper();
			if (!mapper.isNull()) {
				odaUvs.resize(verticesOut.size());
				const OdGePoint3d* predefinedUvCoords = vertexData()->mappingCoords(OdGiVertexData::kAllChannels);
				for (OdInt32 i = 0; i < verticesOut.size(); i++)
				{
					mapper->mapPredefinedCoords(predefinedUvCoords + indices[i], odaUvs.data() + i, 1);
				}
			}
		}
		else
		{
			// ...Otherwise we can look up uvs directly from the world positions

			if (!currentMapper(true)->diffuseMapper().isNull())
			{
				odaUvs.resize(verticesOut.size());
				currentMapper()->diffuseMapper()->mapCoords(odaPoints.data(), odaUvs.data());
			}
		}

		uvsOut.clear();
		for (int i = 0; i < odaUvs.size(); ++i) {
			uvsOut.push_back({ (float)odaUvs[i].x, (float)odaUvs[i].y });
		}
	}
}

std::string DataProcessorRvt::getLevel(OdBmElementPtr element, const std::string& name)
{
	auto levelId = element->getAssocLevelId();
	if (levelId.isValid() && !levelId.isNull()) // (Opening a valid, null pointer will result in an exception)
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
	if (owner.isValid() && !owner.isNull())
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

	// In the current version of ODA, each component that is a member of a
	// system is followed by that system as an OdGiDrawable, so we need the
	// geometry to be associated with the previous OdGiDrawable. This is
	// achieved by simply skipping all system drawables (and so not changing
	// the collector settings).

	if (element->isKindOf(OdBmRbsSystemType::desc()))
	{
		return;
	}

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
		// LabelUtils should be used in embedded mode - where the CSV files are built
		// into TB_ExLabelUtils.
		// This is configured with the EMBEDED_CSV environment variable. Make sure this
		// is not set, as Regular mode will result in runtime errors if the CSVs and
		// Checksums are not also maintained.
		OdBmLabelUtilsPEPtr _labelUtils = OdBmObject::desc()->getX(OdBmLabelUtilsPE::desc());
		labelUtils = (OdBmSampleLabelUtilsPE*)_labelUtils.get();
	}
}

void DataProcessorRvt::processParameter(
	OdBmElementPtr element,
	OdBmObjectId paramId,
	std::unordered_map<std::string, std::string> &metadata,
	const OdBm::BuiltInParameter::Enum &buildInEnum
) {
	OdTfVariant value;
	OdResult res = element->getParam(paramId, value);

	if (res == eOk)
	{
		OdBmParamElemPtr pParamElem = paramId.safeOpenObject();
		OdBmParamDefPtr pDescParam = pParamElem->getParamDef();
		OdBmForgeTypeId groupId = pDescParam->getGroupTypeId();

		auto metaKey = convertToStdString(pDescParam->getCaption());

		if (!ignoreParam(metaKey)) {
			auto paramGroup = labelUtils->getLabelForGroup(groupId);
			if (!paramGroup.isEmpty()) {
				metaKey = convertToStdString(paramGroup) + "::" + metaKey;
			}

			// Get the label for the units, using the same mechanism as the OdBmSampleLabelUtilsPE::getParamValueAsString.

			auto pAUnits = getUnits(pParamElem->getDatabase());
			auto pFormatOptions = pAUnits->getFormatOptions(pDescParam->getSpecTypeId());
			if (!pFormatOptions.isNull()) {
				auto symbolTypeId = pFormatOptions->getSymbolTypeId();
				auto units = labelUtils->getLabelForSymbol(symbolTypeId);
				if (!units.isEmpty()) {
					metaKey += " (" + convertToStdString(units) + ")";
				}
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

			auto paramId = element->database()->getObjectId(entry);
			if (paramId.isValid() && !paramId.isNull()) {
				processParameter(element, paramId, metadata, entry);
			}
		}
	}

	for (const auto &entry : aParams.getUserParamsIterator()) {
		processParameter(element, entry, metadata,
			//A dummy entry, as long as it's not ELEM_CATEGORY_PARAM_MT or ELEM_CATEGORY_PARAM it's not utilised.
			OdBm::BuiltInParameter::Enum::ACTUAL_MAX_RIDGE_HEIGHT_PARAM);
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
		repoTrace << "Caught exception whilst trying to fetch metadata by element pointer " << convertToStdString(er.description()) << " Code: " << er.code();
	}

	try
	{
		fillMetadataById(element->getFamId(), metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to get metadata by family ID: " << convertToStdString(er.description()) << " Code: " << er.code();
	}

	try
	{
		fillMetadataById(element->getTypeID(), metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to get metadata by Type ID: " << convertToStdString(er.description()) << " Code: " << er.code();
	}

	try
	{
		fillMetadataById(element->getHeaderCategoryId(), metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to get metadata by category ID: " << convertToStdString(er.description()) << " Code: " << er.code();
	}

	return metadata;
}

void DataProcessorRvt::fillMaterial(OdBmMaterialElemPtr materialPtr, const MaterialColours& matColors, repo_material_t& material)
{
	const float norm = 255.f;

	material.specular = { matColors.colorSpecular.red() / norm, matColors.colorSpecular.green() / norm, matColors.colorSpecular.blue() / norm, 1.0f };
	material.ambient = { matColors.colorAmbient.red() / norm, matColors.colorAmbient.green() / norm, matColors.colorAmbient.blue() / norm, 1.0f };
	material.emissive = { matColors.colorEmissive.red() / norm, matColors.colorEmissive.green() / norm, matColors.colorEmissive.blue() / norm, 1.0f };
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

OdBmAUnitsPtr DataProcessorRvt::getUnits(OdBmDatabasePtr database)
{
	OdBmFormatOptionsPtrArray aFormatOptions;
	OdBmUnitsTrackingPtr pUnitsTracking = database->getAppInfo(OdBm::ManagerType::UnitsTracking);
	OdBmUnitsElemPtr pUnitsElem = pUnitsTracking->getUnitsElemId().safeOpenObject();
	OdBmAUnitsPtr ptrAUnits = pUnitsElem->getUnits().get();
	return ptrAUnits;
}

OdBmForgeTypeId DataProcessorRvt::getLengthUnits(OdBmDatabasePtr database)
{
	OdBmFormatOptionsPtr formatOptionsLength = getUnits(database)->getFormatOptions(OdBmSpecTypeId::kLength);
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

ModelUnits DataProcessorRvt::getProjectUnits(OdBmDatabase* pDb) {
	initLabelUtils();
	auto unitsStr = convertToStdString(labelUtils->getLabelForUnit(getLengthUnits(database)));

	if (unitsStr == "Millimeters") return ModelUnits::MILLIMETRES;
	if (unitsStr == "Centimeters") return ModelUnits::CENTIMETRES;
	if (unitsStr == "Decimeters") return ModelUnits::DECIMETRES;
	if (unitsStr == "Meters") return ModelUnits::METRES;
	if (unitsStr == "Feet") return ModelUnits::FEET;
	if (unitsStr == "Inches") return ModelUnits::INCHES;

	repoWarning << "Unrecognised model units: " << unitsStr;
	return ModelUnits::UNKNOWN;
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
				auto scaleCoef = 1.0 / OdBmUnitUtils::getUnitTypeIdInfo(getLengthUnits(database)).inIntUnitsCoeff;
				convertTo3DRepoWorldCoorindates = [activeOrigin, alignedLocation, scaleCoef](OdGePoint3d point) {
					OdGeVector3d convertedPoint = (point - activeOrigin).transformBy(alignedLocation);
					return repo::lib::RepoVector3D64(convertedPoint.x * scaleCoef, convertedPoint.y * scaleCoef, convertedPoint.z * scaleCoef);
				};
			}
		}
	}
}