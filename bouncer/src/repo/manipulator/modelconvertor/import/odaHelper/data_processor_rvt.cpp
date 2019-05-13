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

using namespace repo::manipulator::modelconvertor::odaHelper;


bool DataProcessorRvt::ignoreParam(const std::string& param)
{
	auto paramUpper = param;
	std::transform(paramUpper.begin(), paramUpper.end(), paramUpper.begin(), ::toupper);
	return PROBLEMATIC_PARAMS.find(param) != PROBLEMATIC_PARAMS.end() || IGNORE_PARAMS.find(paramUpper) != IGNORE_PARAMS.end();
}

std::string DataProcessorRvt::getElementName(OdBmElementPtr element, uint64_t id)
{
	std::string elName(convertToStdString(element->getElementName()));
	if (!elName.empty())
		elName.append("_");

	elName.append(std::to_string(id));

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
	char* env = std::getenv(RVT_TEXTURES_ENV_VARIABLE);
	if (env == nullptr)
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
		strOut = convertToStdString(labelUtils->format(*database->getUnits(), paramDef->getUnitType(), val.getDouble(), false));
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
						strOut = convertToStdString(OdBm::BuiltInCategory(builtInValue).toString());
					}
				}
				else
				{
					strOut = std::to_string((OdUInt64)rawValue.getHandle());
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
	forEachBmDBView(database, [&](OdBmDBViewPtr pDBView) { hiddenElementsViewRejection(pDBView); });

	establishProjectTranslation(database);
}

DataProcessorRvt::DataProcessorRvt() :
	meshesCount(0)
{
}

void DataProcessorRvt::beginViewVectorization()
{
	DataProcessor::beginViewVectorization();
	setEyeToOutputTransform(getEyeToWorldTransform());
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
		OdBmObjectPtr objectPtr = matId.safeOpenObject(OdBm::kForRead);
		if (!objectPtr.isNull())
			materialElem = OdBmMaterialElem::cast(objectPtr);
	}

	fillMaterial(materialElem, matColors, material);
	fillTexture(materialElem, material, missingTexture);
}

void DataProcessorRvt::convertTo3DRepoVertices(
	const OdInt32* p3Vertices,
	std::vector<repo::lib::RepoVector3D64>& verticesOut,
	repo::lib::RepoVector3D64& normalOut,
	std::vector<repo::lib::RepoVector2D>& uvOut)
{
	std::vector<OdGePoint3d> odaPoints;
	getVertices(p3Vertices, odaPoints, verticesOut);

	const int numVertices = 3;
	if (verticesOut.size() != numVertices)
		return;

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
	for (int i = 0; i < numVertices; ++i)
		uvOut.push_back({ (float)outTex.outCoord[i].x, (float)outTex.outCoord[i].y });
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

	std::string elementName = getElementName(element, meshesCount++);

	collector->setNextMeshName(elementName);
	collector->setMeshGroup(elementName);

	try
	{
		collector->setCurrentMeta(fillMetadata(element));
		//some objects material is not set. set default here
		collector->setCurrentMaterial(GetDefaultMaterial());
	}
	catch(OdError& er)
	{
		//.. HOTFIX: handle nullPtr exception (reported to ODA)
		repoDebug << "Caught exception whilst: " << convertToStdString(er.description());
	}

	std::string layerName = getLevel(element, "Layer Default");
	collector->setLayer(layerName);
	collector->stopMeshEntry();
	collector->startMeshEntry();
}

void DataProcessorRvt::fillMetadataById(
	OdBmObjectId id,
	std::pair<std::vector<std::string>, std::vector<std::string>>& metadata)
{
	if (id.isNull())
		return;

	OdBmObjectPtr ptr = id.safeOpenObject();

	if (ptr.isNull())
		return;

	fillMetadataByElemPtr(ptr, metadata);
}

void DataProcessorRvt::fillMetadataByElemPtr(
	OdBmElementPtr element,
	std::pair<std::vector<std::string>, std::vector<std::string>>& metadata)
{
	OdBuiltInParamArray aParams;
	element->getListParams(aParams);

	OdBmLabelUtilsPEPtr labelUtils = OdBmObject::desc()->getX(OdBmLabelUtilsPE::desc());
	if (labelUtils.isNull())
		return;

	for (OdBuiltInParamArray::iterator it = aParams.begin(); it != aParams.end(); it++)
	{
		std::string builtInName = convertToStdString(OdBm::BuiltInParameter(*it).toString());

		//.. HOTFIX: handle access violation exception (reported to ODA)
		if (ignoreParam(builtInName)) continue;

		std::string paramName;
		if (!labelUtils->getLabelFor(*it).isEmpty())
			paramName = convertToStdString(labelUtils->getLabelFor(*it));
		else
			paramName = builtInName;

		OdTfVariant value;
		OdResult res = element->getParam(*it, value);
		if (res == eOk)
		{
			OdBmParamElemPtr pParamElem = element->database()->getObjectId(*it).safeOpenObject();
			OdBmParamDefPtr pDescParam = pParamElem->getParamDef();

			auto metaKey = convertToStdString(pDescParam->getCaption());
			if (!ignoreParam(metaKey)) {
				std::string variantValue = translateMetadataValue(value, labelUtils, pDescParam, element->getDatabase(), *it);
				if (!variantValue.empty())
				{
				
					metadata.first.push_back(metaKey);
					metadata.second.push_back(variantValue);
				}
			}
		}
	}
}

std::pair<std::vector<std::string>, std::vector<std::string>> DataProcessorRvt::fillMetadata(OdBmElementPtr element)
{
	std::pair<std::vector<std::string>, std::vector<std::string>> metadata;
	try
	{
		fillMetadataByElemPtr(element, metadata);
	}
	catch (OdError& er)
	{
		repoDebug << "Caught exception whilst: " << convertToStdString(er.description());
	}

	try
	{
		fillMetadataById(element->getFamId(), metadata);
	}
	catch (OdError& er)
	{
		repoDebug << "Caught exception whilst: " << convertToStdString(er.description());
	}

	try
	{
		fillMetadataById(element->getTypeID(), metadata);
	}
	catch (OdError& er)
	{
		repoDebug << "Caught exception whilst: " << convertToStdString(er.description());
	}

	try
	{
		fillMetadataById(element->getCategroryId(), metadata);
	}
	catch (OdError& er)
	{
		repoDebug << "Caught exception whilst: " << convertToStdString(er.description());
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
	return formatOptionsLength->getDisplayUnits();
}

void DataProcessorRvt::hiddenElementsViewRejection(OdBmDBViewPtr pDBView)
{
	//.. NOTE: exclude hidden elements from drawing
	auto setts = pDBView->getHiddenElementsViewSettings();
	OdBmObjectIdArray arr;
	setts->getHiddenElements(arr);
	for (uint32_t i = 0; i < arr.size(); i++)
	{
		OdBmElementPtr hidden = arr[i].safeOpenObject(OdBm::OpenMode::kForWrite);
		if (hidden.isNull())
			continue;

		OdBmRejectedViewRules rules;
		rules.rejectAllViewTypes();
		hidden->setViewRules(rules);
	}
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

				auto scaleCoef = 1.0 / BmUnitUtils::getDisplayUnitTypeInfo(getUnits(database))->inIntUnitsCoeff;
				convertTo3DRepoWorldCoorindates = [activeOrigin, alignedLocation, scaleCoef](OdGePoint3d point) {
					auto convertedPoint = (point - activeOrigin).transformBy(alignedLocation);
					return repo::lib::RepoVector3D64(convertedPoint.x * scaleCoef, convertedPoint.y * scaleCoef, convertedPoint.z * scaleCoef);
				};

			}

		}
	}
}
