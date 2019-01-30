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
#include "vectorise_view_rvt.h"
#include "helper_functions.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

const char* RVT_TEXTURES_ENV_VARIABLE = "RVT_TEXTURES";

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

	//..try to extract one valid paths if multiple paths are provided
	outputFilePath = outputFilePath.substr(0, outputFilePath.find("|", 0));

	if (isFileExist(outputFilePath))
		return outputFilePath;

	//..try to apply absolute path
	char* env = std::getenv(RVT_TEXTURES_ENV_VARIABLE);
	if (env == nullptr)
		return std::string();

	auto absolutePath = boost::filesystem::absolute(outputFilePath, env);
	outputFilePath = absolutePath.generic_string();

	if (isFileExist(outputFilePath))
		return outputFilePath;

	return std::string();
}

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

OdGsViewPtr VectorizeView::createObject(GeometryCollector* geoColl)
{
	auto ptr = OdRxObjectImpl<VectorizeView, OdGsView>::createObject();
	static_cast<VectorizeView*>(ptr.get())->collector = geoColl;
	return ptr;
}

VectorizeView::VectorizeView()
{
	meshesCount = 0;
}

void VectorizeView::draw(const OdGiDrawable* pDrawable)
{
	fillMeshData(pDrawable);
	OdGsBaseMaterialView::draw(pDrawable);
}

VectoriseDeviceRvt* VectorizeView::device()
{
	return (VectoriseDeviceRvt*)OdGsBaseVectorizeView::device();
}

void VectorizeView::OnFillMaterialCache(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData,
	const MaterialColors& matColors,
	repo_material_t& material)
{
	bool missingTexture = false;
	fillMaterial(matColors, material);
	fillTexture(materialId, material, missingTexture);

	collector->setCurrentMaterial(material, missingTexture);
	collector->stopMeshEntry();
	collector->startMeshEntry();
}

void VectorizeView::OnTriangleOut(const std::vector<repo::lib::RepoVector3D64>& vertices)
{
	const int numVertices = 3;
	if (vertices.size() != numVertices)
		return;

	OdGiMapperItemEntry::MapInputTriangle trg;

	for (int i = 0; i < vertices.size(); ++i)
	{
		trg.inPt[i].x = vertices[i].x;
		trg.inPt[i].y = vertices[i].y;
		trg.inPt[i].z = vertices[i].z;
	}

	OdGiMapperItemEntry::MapOutputCoords outTex;

	auto currentMap = currentMapper();
	if (!currentMap.isNull())
	{
		auto diffuseMap = currentMap->diffuseMapper();
		if (!diffuseMap.isNull())
			diffuseMap->mapCoords(trg, outTex);
	}

	std::vector<repo::lib::RepoVector2D> uvc;
	for (int i = 0; i < numVertices; ++i)
		uvc.push_back({ (float)outTex.outCoord[i].x, (float)outTex.outCoord[i].y });

	collector->addFace(vertices, uvc);
}

std::string VectorizeView::getLevel(OdBmElementPtr element, const std::string& name)
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

void VectorizeView::fillMeshData(const OdGiDrawable* pDrawable)
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
	}
	catch(OdError& er)
	{
		//..Hotfix: handle nullPtr exception (need to check updated library)
	}

	meshesCount++;

	std::string layerName = getLevel(element, "Layer Default");
	collector->setLayer(layerName);
}

std::pair<std::vector<std::string>, std::vector<std::string>> VectorizeView::fillMetadata(OdBmElementPtr element)
{
	std::pair<std::vector<std::string>, std::vector<std::string>> metadata;

	OdBuiltInParamArray aParams;
	element->getListParams(aParams);

	OdBmLabelUtilsPEPtr labelUtils = OdBmObject::desc()->getX(OdBmLabelUtilsPE::desc());
	if (labelUtils.isNull())
		return metadata;

	for (OdBuiltInParamArray::iterator it = aParams.begin(); it != aParams.end(); it++)
	{
		std::string builtInName = convertToStdString(OdBm::BuiltInParameter(*it).toString());

		//..Hotfix: handle access violation exception (need to check updated library)
		if (builtInName == std::string("ROOF_SLOPE"))
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

	return metadata;
}

void VectorizeView::fillMaterial(const MaterialColors& matColors, repo_material_t& material)
{
	const float norm = 255.f;

	material.diffuse = { ODGETRED(matColors.colorDiffuse) / norm, ODGETGREEN(matColors.colorDiffuse) / norm, ODGETBLUE(matColors.colorDiffuse) / norm, 1.0f };
	material.specular = { ODGETRED(matColors.colorSpecular) / norm, ODGETGREEN(matColors.colorSpecular) / norm, ODGETBLUE(matColors.colorSpecular) / norm, 1.0f };
	material.ambient = { ODGETRED(matColors.colorAmbient) / norm, ODGETGREEN(matColors.colorAmbient) / norm, ODGETBLUE(matColors.colorAmbient) / norm, 1.0f };
	material.emissive = { ODGETRED(matColors.colorEmissive) / norm, ODGETGREEN(matColors.colorEmissive) / norm, ODGETBLUE(matColors.colorEmissive) / norm, 1.0f };
}

void VectorizeView::fillTexture(OdDbStub* materialId, repo_material_t& material, bool& missingTexture)
{
	missingTexture = false;

	OdBmObjectId matId(materialId);
	if (!matId.isValid())
		return;

	OdBmObjectPtr materialPtr = matId.safeOpenObject(OdBm::kForRead);
	OdBmMaterialElemPtr materialElem = (OdBmMaterialElem*)(materialPtr.get());
	OdBmMaterialPtr matPtr = materialElem->getMaterial();
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