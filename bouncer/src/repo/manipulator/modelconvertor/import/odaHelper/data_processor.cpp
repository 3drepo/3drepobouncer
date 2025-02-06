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

#include <OdaCommon.h>
#include <OdString.h>

#include <toString.h>
#include <DgLevelTableRecord.h>

#include "repo/core/model/bson/repo_bson_factory.h"
#include "data_processor.h"
#include "helper_functions.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

void DataProcessor::triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	GeometryCollector::Face triangle;
	const auto pVertexDataList = vertexDataList();
	for (int i = 0; i < 3; i++)
	{
		auto position = pVertexDataList[p3Vertices[i]];
		triangle.push_back(toRepoVector(position));
	}
	collector->addFace(triangle);
}

void DataProcessor::polylineOut(OdInt32 numPoints, const OdInt32* vertexIndexList)
{
	const auto pVertexDataList = vertexDataList();
	for (int i = 0; i < numPoints - 1; i++)
	{
		collector->addFace({
			toRepoVector(pVertexDataList[vertexIndexList[i + 0]]),
			toRepoVector(pVertexDataList[vertexIndexList[i + 1]])
		});
	}
}

void DataProcessor::polylineOut(OdInt32 numPoints, const OdGePoint3d* vertexList)
{
	for (OdInt32 i = 0; i < (numPoints - 1); i++)
	{
		collector->addFace({
			toRepoVector(vertexList[i]),
			toRepoVector(vertexList[i + 1]),
		});
	}
}

double DataProcessor::deviation(
	const OdGiDeviationType deviationType,
	const OdGePoint3d& pointOnCurve) const {
	return deviationValue;
}

void DataProcessor::convertTo3DRepoMaterial(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData,
	MaterialColours& matColors,
	repo::lib::repo_material_t& material)
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

	matColors.colorDiffuse = diffuseColor.color();
	matColors.colorAmbient = ambientColor.color();
	matColors.colorSpecular = specularColor.color();
	matColors.colorEmissive = emissiveColor.color();

	matColors.colorDiffuseOverride = diffuseColor.method() == OdGiMaterialColor::kOverride;
	matColors.colorAmbientOverride = ambientColor.method() == OdGiMaterialColor::kOverride;
	matColors.colorSpecularOverride = specularColor.method() == OdGiMaterialColor::kOverride;
	matColors.colorEmissiveOverride = emissiveColor.method() == OdGiMaterialColor::kOverride;

	material.shininessStrength = glossFactor;
	material.shininess = materialData.reflectivity();
	material.opacity = opacityPercentage;

	// The caller should handle converting the colours to repo_material_t colours,
	// as these may be overridden by the Gs device
}

OdGiMaterialItemPtr DataProcessor::fillMaterialCache(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData
) {
	MaterialColours colors;
	repo::lib::repo_material_t material;
	bool missingTexture = false;

	convertTo3DRepoMaterial(prevCache, materialId, materialData, colors, material);
	collector->setMaterial(material);

	return OdGiMaterialItemPtr();
}

void DataProcessor::beginViewVectorization()
{
	OdGsBaseMaterialView::beginViewVectorization();
	OdGiGeometrySimplifier::setDrawContext(OdGsBaseMaterialView::drawContext());
	output().setDestGeometry((OdGiGeometrySimplifier&)*this);
	setDrawContextFlags(drawContextFlags(), false);
	setEyeToOutputTransform(getEyeToWorldTransform());
}

void DataProcessor::initialise(GeometryCollector* collector)
{
	this->collector = collector;
}