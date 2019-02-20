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

#include "boost/filesystem.hpp"
#include "OdPlatformSettings.h"

#include "vectorise_device_rvt.h"
#include "data_processor_rvt.h"
#include "helper_functions.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

//.. NOTE: Environment variable name for Revit textures
const char* RVT_TEXTURES_ENV_VARIABLE = "RVT_TEXTURES";

//.. NOTE: These metadata params are crashing application when we're trying to get them; Report to ODA
const std::vector<std::string> PROBLEMATIC_PARAMS = { 
	"ROOF_SLOPE", 
	"RBS_PIPE_SIZE_MAXIMUM",
	"RBS_PIPE_SIZE_MINIMUM", 
	"RBS_SYSTEM_CLASSIFICATION_PARAM" 
};

bool isProblematicParam(const std::string& param)
{
	return std::find(PROBLEMATIC_PARAMS.begin(), PROBLEMATIC_PARAMS.end(), param) != PROBLEMATIC_PARAMS.end();
}

repo_material_t GetDefaultMaterial()
{
	repo_material_t material;
	material.shininess = 0.5;
	material.shininessStrength = 0.5;
	material.opacity = 1;
	material.specular = { 0, 0, 0, 0 };
	material.diffuse = { 0.2f, 0.2f, 0.2f, 0 };
	material.emissive = material.diffuse;

	return material;
}

std::string getElementName(OdBmElementPtr element, uint64_t id)
{
	std::string elName(convertToStdString(element->getElementName()));
	if (!elName.empty())
		elName.append("_");

	elName.append(std::to_string(id));

	return elName;
}

bool isFileExist(const std::string& inputPath)
{
	if (boost::filesystem::exists(inputPath))
		if (boost::filesystem::is_regular_file(inputPath))
			return true;

	return false;
}

std::string extractValidTexturePath(const std::string& inputPath)
{
	std::string outputFilePath = inputPath;

	//.. NOTE: try to extract one valid paths if multiple paths are provided
	outputFilePath = outputFilePath.substr(0, outputFilePath.find("|", 0));

	if (isFileExist(outputFilePath))
		return outputFilePath;

	//.. NOTE: try to apply absolute path
	char* env = std::getenv(RVT_TEXTURES_ENV_VARIABLE);
	if (env == nullptr)
		return std::string();

	auto absolutePath = boost::filesystem::absolute(outputFilePath, env);
	outputFilePath = absolutePath.generic_string();

	if (isFileExist(outputFilePath))
		return outputFilePath;

	return std::string();
}

//.. NOTE: Converts metadata object to a valid std::string that represents metadata value
std::string variantToString(const OdTfVariant& val, OdBmLabelUtilsPEPtr labelUtils, OdBmParamDefPtr paramDef, OdBmDatabase* database, OdBm::BuiltInParameterDefinition::Enum param)
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
					OdBmElementPtr elem = database->getObjectId(rawValue.getHandle()).safeOpenObject();
					strOut = (elem->getElementName() == OdString::kEmpty) ? std::to_string((OdUInt64)rawValue.getHandle()) : convertToStdString(elem->getElementName());
				}
			}
	}

	return strOut;
}

void DataProcessorRvt::init(GeometryCollector* geoColl, OdBmDatabasePtr database)
{
	this->collector = geoColl;
	this->database = database;
	getCameras(database);
	forEachBmDBView(database, [&](OdBmDBViewPtr pDBView) { hiddenElementsViewRejection(pDBView); });
}

DataProcessorRvt::DataProcessorRvt()
{
	meshesCount = 0;
}

void DataProcessorRvt::beginViewVectorization()
{
	DataProcessor::beginViewVectorization();
	OdBm::DisplayUnitType::Enum lengthUnits = getUnits(database);
	double scale = 1.0 / getUnitsCoef(lengthUnits);
	repo::lib::RepoMatrix mat({ (float)scale,0,0,0,
		0,(float)scale,0,0,
		0,0,(float)scale,0,
		0,0,0,1 });
	collector->setRootMatrix(mat);
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
	MaterialColors& matColors,
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
	std::vector<repo::lib::RepoVector2D>& uvOut)
{
	DataProcessor::convertTo3DRepoVertices(p3Vertices, verticesOut, uvOut);

	const int numVertices = 3;
	if (verticesOut.size() != numVertices)
		return;

	OdGiMapperItemEntry::MapInputTriangle trg;

	for (int i = 0; i < verticesOut.size(); ++i)
	{
		trg.inPt[i].x = verticesOut[i].x;
		trg.inPt[i].y = verticesOut[i].y;
		trg.inPt[i].z = verticesOut[i].z;
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

	std::string elementName = getElementName(element, meshesCount);

	collector->setNextMeshName(elementName);
	collector->setMeshGroup(elementName);

	try
	{
		collector->setCurrentMeta(fillMetadata(element));

		//.. NOTE: for some objects material is not set. set default here
		collector->setCurrentMaterial(GetDefaultMaterial());
	}
	catch(OdError& er)
	{
		//.. HOTFIX: handle nullPtr exception (reported to ODA)
	}

	meshesCount++;

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
		if (isProblematicParam(builtInName))
			continue;

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

			std::string variantValue = variantToString(value, labelUtils, pDescParam, element->getDatabase(), *it);
			if (!variantValue.empty())
			{
				metadata.first.push_back(convertToStdString(pDescParam->getCaption()));
				metadata.second.push_back(variantValue);
			}
		}
	}
}

std::pair<std::vector<std::string>, std::vector<std::string>> DataProcessorRvt::fillMetadata(OdBmElementPtr element)
{
	std::pair<std::vector<std::string>, std::vector<std::string>> metadata;
	fillMetadataByElemPtr(element, metadata);
	fillMetadataById(element->getFamId(), metadata);
	fillMetadataById(element->getTypeID(), metadata);
	fillMetadataById(element->getCategroryId(), metadata);
	return metadata;
}

void DataProcessorRvt::fillMaterial(OdBmMaterialElemPtr materialPtr, const MaterialColors& matColors, repo_material_t& material)
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
	std::string validTextureName = extractValidTexturePath(textureName);

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

double repo::manipulator::modelconvertor::odaHelper::DataProcessorRvt::getUnitsCoef(OdBm::DisplayUnitType::Enum unitType)
{
	return BmUnitUtils::getDisplayUnitTypeInfo(unitType)->inIntUnitsCoeff;
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
	if (collector->hasCemaraNodes())
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
