#include "boost/filesystem.hpp"
#include "OdPlatformSettings.h"

#include "vectorise_device_rvt.h"
#include "vectorise_view_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

const char* RVT_TEXTURES_ENV_VARIABLE = "RVT_TEXTURES";

std::string getElementName(OdBmElementPtr element, uint64_t id)
{
	std::string elName((const char*)element->getElementName());
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

std::string variantToString(const OdTfVariant& val)
{
	std::string strOut;
	switch (val.type()) {
	case OdTfVariant::kString: 
		strOut = (const char*)val.getString();
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
		strOut = std::to_string(val.getInt32());
		break;
	case OdTfVariant::kInt64:  
		strOut = std::to_string(val.getInt64());
		break;
	case OdTfVariant::kDouble: 
		strOut = std::to_string(val.getDouble());
		break;
	}

	return strOut;
}

OdGsViewPtr VectorizeView::createObject(GeometryCollector* geoColl)
{
	auto ptr = OdRxObjectImpl<VectorizeView, OdGsView>::createObject();
	static_cast<VectorizeView*>(ptr.get())->geoColl = geoColl;
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

void VectorizeView::beginViewVectorization()
{
	OdGsBaseVectorizer::beginViewVectorization();
	OdGiGeometrySimplifier::setDrawContext(OdGsBaseMaterialView::drawContext());
	output().setDestGeometry((OdGiGeometrySimplifier&)*this);
	setDrawContextFlags(drawContextFlags(), false);
	setEyeToOutputTransform(getEyeToWorldTransform());
}

VectoriseDeviceRvt* VectorizeView::device()
{
	return (VectoriseDeviceRvt*)OdGsBaseVectorizeView::device();
}

void VectorizeView::triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	const OdGePoint3d*  pVertexDataList = vertexDataList();
	const int numVertices = 3;

	OdGePoint3d trgPoints[numVertices] =
	{
		vertexDataList()[p3Vertices[0]],
		vertexDataList()[p3Vertices[1]],
		vertexDataList()[p3Vertices[2]]
	};

	std::vector<repo::lib::RepoVector3D64> vertices;

	if ((pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[1]) &&
		(pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[2]) &&
		(pVertexDataList + p3Vertices[1]) != (pVertexDataList + p3Vertices[2]))
	{
		for (int i = 0; i < numVertices; ++i)
		{
			vertices.push_back({ trgPoints[i].x , trgPoints[i].y, trgPoints[i].z });
		}
	}
	else
	{
		return;
	}

	OdGiMapperItemEntry::MapInputTriangle trg{ { trgPoints[0], trgPoints[1], trgPoints[2] } };
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

	geoColl->addFace(vertices, uvc);
}

OdGiMaterialItemPtr VectorizeView::fillMaterialCache(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData)
{
	repo_material_t material;

	bool missingTexture = false;
	fillMaterial(materialData, material);
	fillTexture(materialId, material, missingTexture);

	geoColl->setCurrentMaterial(material, missingTexture);
	geoColl->stopMeshEntry();
	geoColl->startMeshEntry();

	return OdGiMaterialItemPtr();
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
				return std::string((const char*)lptr->getElementName());
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

	geoColl->setNextMeshName(elementName);
	geoColl->setMeshGroup(elementName);
	geoColl->setCurrentMeta(fillMetadata(element));
	meshesCount++;

	std::string layerName = getLevel(element, "Layer Default");
	geoColl->setLayer(layerName);
}

repo::core::model::MetadataNode* VectorizeView::fillMetadata(OdBmElementPtr element)
{
	repo::core::model::MetadataNode *metaNode;
	repo::core::model::RepoBSONBuilder builder;

	OdBuiltInParamArray aParams;
	element->getListParams(aParams);

	for (OdBuiltInParamArray::iterator it = aParams.begin(); it != aParams.end(); it++)
	{
		std::string paramName = std::string((const char*)OdBm::BuiltInParameter(*it).toString());

		OdTfVariant value;
		OdResult res = element->getParam(*it, value);
		if (res == eOk)
		{
			std::string variantValue = variantToString(value);
			if (!variantValue.empty())
				builder << paramName << variantValue;
		}
	}

	repo::core::model::RepoBSON metaBSON = builder.obj();
	std::string handleStr = (const char*)(element->objectId().getHandle().ascii());
	std::string metaName = std::string("meta[" + handleStr + "]");
	return new repo::core::model::MetadataNode(repo::core::model::RepoBSONFactory::makeMetaDataNode(metaBSON, "", metaName));
}

void VectorizeView::fillMaterial(const OdGiMaterialTraitsData & materialData, repo_material_t& material)
{
	OdGiMaterialColor diffuseColor; OdGiMaterialMap diffuseMap;
	OdGiMaterialColor ambientColor;
	OdGiMaterialColor specularColor; OdGiMaterialMap specularMap; double glossFactor;
	OdGiMaterialColor emissiveColor; OdGiMaterialMap emissiveMap;

	double opacityPercentage; OdGiMaterialMap opacityMap;

	materialData.diffuse(diffuseColor, diffuseMap);
	materialData.ambient(ambientColor);
	materialData.specular(specularColor, specularMap, glossFactor);
	materialData.opacity(opacityPercentage, opacityMap);
	materialData.emission(emissiveColor, emissiveMap);

	ODCOLORREF colorDiffuse(0), colorAmbient(0), colorSpecular(0), colorEmissive(0);
	if (diffuseColor.color().colorMethod() == OdCmEntityColor::kByColor)
	{
		colorDiffuse = ODTOCOLORREF(diffuseColor.color());
	}
	else if (diffuseColor.color().colorMethod() == OdCmEntityColor::kByACI)
	{
		colorDiffuse = OdCmEntityColor::lookUpRGB((OdUInt8)diffuseColor.color().colorIndex());
	}
	if (ambientColor.color().colorMethod() == OdCmEntityColor::kByColor)
	{
		colorAmbient = ODTOCOLORREF(ambientColor.color());
	}
	else if (ambientColor.color().colorMethod() == OdCmEntityColor::kByACI)
	{
		colorAmbient = OdCmEntityColor::lookUpRGB((OdUInt8)ambientColor.color().colorIndex());
	}
	if (specularColor.color().colorMethod() == OdCmEntityColor::kByColor)
	{
		colorSpecular = ODTOCOLORREF(specularColor.color());
	}
	else if (specularColor.color().colorMethod() == OdCmEntityColor::kByACI)
	{
		colorSpecular = OdCmEntityColor::lookUpRGB((OdUInt8)specularColor.color().colorIndex());
	}
	if (emissiveColor.color().colorMethod() == OdCmEntityColor::kByColor)
	{
		colorEmissive = ODTOCOLORREF(emissiveColor.color());
	}
	else if (emissiveColor.color().colorMethod() == OdCmEntityColor::kByACI)
	{
		colorEmissive = OdCmEntityColor::lookUpRGB((OdUInt8)emissiveColor.color().colorIndex());
	}

	const float norm = 255.f;

	material.diffuse = { ODGETRED(colorDiffuse) / norm, ODGETGREEN(colorDiffuse) / norm, ODGETBLUE(colorDiffuse) / norm, 1.0f };
	material.specular = { ODGETRED(colorSpecular) / norm, ODGETGREEN(colorSpecular) / norm, ODGETBLUE(colorSpecular) / norm, 1.0f };
	material.ambient = { ODGETRED(colorAmbient) / norm, ODGETGREEN(colorAmbient) / norm, ODGETBLUE(colorAmbient) / norm, 1.0f };
	material.emissive = { ODGETRED(colorEmissive) / norm, ODGETGREEN(colorEmissive) / norm, ODGETBLUE(colorEmissive) / norm, 1.0f };

	material.shininessStrength = glossFactor;
	material.shininess = materialData.reflectivity();

	material.opacity = opacityPercentage;
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

	std::string textureName((const char*)textureFileName);
	std::string validTextureName = extractValidTexturePath(textureName);

	if (validTextureName.empty() && !textureName.empty())
		missingTexture = true;

	material.texturePath = validTextureName;
}