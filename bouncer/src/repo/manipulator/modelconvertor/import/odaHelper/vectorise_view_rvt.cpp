#include "vectorise_view_rvt.h"

#include "boost/filesystem.hpp"
#include "OdPlatformSettings.h"
#include <BimCommon.h>
#include <Database/BmElement.h>
#include <RxObjectImpl.h>
#include <ColorMapping.h>
#include <toString.h>
#include "Database/Entities/BmMaterialElem.h"
#include "Geometry/Entities/BmMaterial.h"
#include "Database/BmAssetHelpers.h"

#include "Main/Entities/BmDirectShape.h"
#include "Main/Entities/BmDirectShapeCell.h"
#include "Main/Entities/BmDirectShapeDataCell.h"

#include "vectorise_device_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

const char* RVT_TEXTURES_ENV_VARIABLE = "RVT_TEXTURES";

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
	geoColl->stopMeshEntry();
	geoColl->setMeshGroup(std::to_string(meshesCount));
	geoColl->setNextMeshName(std::to_string(meshesCount));
	geoColl->startMeshEntry();
	meshesCount++;

	OdString sClassName = toString(pDrawable->isA());
	bool bDBRO = false;
	OdBmElementPtr pElem = OdBmElement::cast(pDrawable);
	if (!pElem.isNull())
		bDBRO = pElem->isDBRO();
	OdString sHandle = bDBRO ? toString(pElem->objectId().getHandle()) : toString(L"non-DbResident");
	OdGsBaseVectorizer::draw(pDrawable);
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