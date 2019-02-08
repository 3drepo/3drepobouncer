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

#include "../../../../core/model/bson/repo_bson_factory.h"
#include "data_processor.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

void DataProcessor::convertTo3DRepoVertices(
	const OdInt32* p3Vertices, 
	std::vector<repo::lib::RepoVector3D64>& verticesOut,
	std::vector<repo::lib::RepoVector2D>& uvOut)
{
	const OdGePoint3d*  pVertexDataList = vertexDataList();
	const OdGeVector3d* pNormals = NULL;

	if ((pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[1]) &&
		(pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[2]) &&
		(pVertexDataList + p3Vertices[1]) != (pVertexDataList + p3Vertices[2]))
	{
		for (int i = 0; i < 3; ++i)
		{
			verticesOut.push_back({ pVertexDataList[p3Vertices[i]].x , pVertexDataList[p3Vertices[i]].y, pVertexDataList[p3Vertices[i]].z });
		}
	}
}

void DataProcessor::triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	std::vector<repo::lib::RepoVector3D64> vertices;
	std::vector<repo::lib::RepoVector2D> uv;

	convertTo3DRepoVertices(p3Vertices, vertices, uv);

	if (vertices.size())
		collector->addFace(vertices, uv);
}

double DataProcessor::deviation(
	const OdGiDeviationType deviationType,
	const OdGePoint3d& pointOnCurve) const {
	return 0;
}

void DataProcessor::convertTo3DRepoMaterial(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData,
	MaterialColors& matColors,
	repo_material_t& material,
	bool& missingTexture)
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

	if (diffuseColor.color().colorMethod() == OdCmEntityColor::kByColor)
	{
		matColors.colorDiffuse = ODTOCOLORREF(diffuseColor.color());
	}
	else if (diffuseColor.color().colorMethod() == OdCmEntityColor::kByACI)
	{
		matColors.colorDiffuse = OdCmEntityColor::lookUpRGB((OdUInt8)diffuseColor.color().colorIndex());
	}
	if (ambientColor.color().colorMethod() == OdCmEntityColor::kByColor)
	{
		matColors.colorAmbient = ODTOCOLORREF(ambientColor.color());
	}
	else if (ambientColor.color().colorMethod() == OdCmEntityColor::kByACI)
	{
		matColors.colorAmbient = OdCmEntityColor::lookUpRGB((OdUInt8)ambientColor.color().colorIndex());
	}
	if (specularColor.color().colorMethod() == OdCmEntityColor::kByColor)
	{
		matColors.colorSpecular = ODTOCOLORREF(specularColor.color());
	}
	else if (specularColor.color().colorMethod() == OdCmEntityColor::kByACI)
	{
		matColors.colorSpecular = OdCmEntityColor::lookUpRGB((OdUInt8)specularColor.color().colorIndex());
	}
	if (emissiveColor.color().colorMethod() == OdCmEntityColor::kByColor)
	{
		matColors.colorEmissive = ODTOCOLORREF(emissiveColor.color());
	}
	else if (emissiveColor.color().colorMethod() == OdCmEntityColor::kByACI)
	{
		matColors.colorEmissive = OdCmEntityColor::lookUpRGB((OdUInt8)emissiveColor.color().colorIndex());
	}

	matColors.colorDiffuseOverride = diffuseColor.method() == OdGiMaterialColor::kOverride ? true : false;
	matColors.colorAmbientOverride = ambientColor.method() == OdGiMaterialColor::kOverride ? true : false;
	matColors.colorSpecularOverride = specularColor.method() == OdGiMaterialColor::kOverride ? true : false;
	matColors.colorEmissiveOverride = emissiveColor.method() == OdGiMaterialColor::kOverride ? true : false;

	material.shininessStrength = glossFactor;
	material.shininess = materialData.reflectivity();
	material.opacity = opacityPercentage;
}

OdGiMaterialItemPtr DataProcessor::fillMaterialCache(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData
) {
	MaterialColors colors;
	repo_material_t material;
	bool missingTexture = false;

	convertTo3DRepoMaterial(prevCache, materialId, materialData, colors, material, missingTexture);

	collector->setCurrentMaterial(material, missingTexture);
	collector->stopMeshEntry();
	collector->startMeshEntry();

	return OdGiMaterialItemPtr();
}

void DataProcessor::beginViewVectorization()
{
	OdGsBaseVectorizer::beginViewVectorization();
	OdGiGeometrySimplifier::setDrawContext(OdGsBaseMaterialView::drawContext());
	output().setDestGeometry((OdGiGeometrySimplifier&)*this);
	setDrawContextFlags(drawContextFlags(), false);
	setEyeToOutputTransform(getEyeToWorldTransform());
}