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

void DataProcessor::convertTo3DRepoTriangle(
	const OdInt32* p3Vertices,
	std::vector<repo::lib::RepoVector3D64>& verticesOut,
	repo::lib::RepoVector3D64& normalOut,
	std::vector<repo::lib::RepoVector2D>& uvOut)
{
	const auto pVertexDataList = vertexDataList();
	for (int i = 0; i < 3; i++)
	{
		auto position = pVertexDataList[p3Vertices[i]];
		verticesOut.push_back(toRepoVector(position));
	}
	normalOut = calcNormal(verticesOut[0], verticesOut[1], verticesOut[2]);
}

void DataProcessor::getVertices(
	int numVertices,
	const OdInt32* p3Vertices,
	std::vector<OdGePoint3d> &odaPoint,
	std::vector<repo::lib::RepoVector3D64> &repoPoint
){
	const OdGePoint3d*  pVertexDataList = vertexDataList();

	if ((pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[1]) &&
		(pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[2]) &&
		(pVertexDataList + p3Vertices[1]) != (pVertexDataList + p3Vertices[2]))
	{
		for (int i = 0; i < numVertices; ++i)
		{
			auto point = pVertexDataList[p3Vertices[i]];
			odaPoint.push_back(point);
			repoPoint.push_back(toRepoVector(point));
		}
	}
}

void DataProcessor::triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	std::vector<repo::lib::RepoVector3D64> vertices;
	std::vector<repo::lib::RepoVector2D> uv;
	repo::lib::RepoVector3D64 normal;

	convertTo3DRepoTriangle(p3Vertices, vertices, normal, uv);

	if (vertices.size()) {
		collector->addFace(vertices, normal, uv);
	}
}

void DataProcessor::polylineOut(OdInt32 numPoints, const OdInt32* vertexIndexList)
{
	std::vector<OdGePoint3d> vertices;
	const auto pVertexDataList = vertexDataList();
	for (int i = 0; i < numPoints; i++)
	{
		vertices.push_back(pVertexDataList[vertexIndexList[i]]);
	}

	polylineOut(numPoints, vertices.data());
}

void DataProcessor::polylineOut(OdInt32 numPoints, const OdGePoint3d* vertexList)
{
	std::vector<repo::lib::RepoVector3D64> vertices;

	for (OdInt32 i = 0; i < (numPoints - 1); i++)
	{
		vertices.clear();
		vertices.push_back(toRepoVector(vertexList[i]));
		vertices.push_back(toRepoVector(vertexList[i + 1]));
		collector->addFace(vertices);
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
	repo_material_t& material)
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
	repo_material_t material;
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

void DataProcessor::endViewVectorization()
{
	OdGsBaseMaterialView::endViewVectorization();
	activeContext = nullptr;
}

void DataProcessor::initialise(GeometryCollector* collector)
{
	this->collector = collector;
}

/*
 * This drawing context will commit its meshes when it goes out of scope. 
 */
DataProcessor::AutoContext::AutoContext(GeometryCollector* collector, const std::string& layerId) :
	collector(collector),
	layerId(layerId),
	GeometryCollector::Context(collector->getWorldOffset(), collector->getLastMaterial())
{
}

DataProcessor::AutoContext::~AutoContext()
{
	collector->popDrawContext(this);
	auto parent = collector->getSharedId(layerId);
	auto meshes = extractMeshes();
	for (auto& p : meshes) {
		p.first.setParents({parent});
		collector->addMaterialReference(p.second, p.first.getSharedID());
		collector->addNode(p.first);
	}
}

void DataProcessor::setLayer(std::string id)
{
	// When this is reset to a new layer, or null in endViewVectorization, the
	// meshes will be committed under the specified layer. Destroying the old
	// context will pop it from the stack.

	activeContext = std::make_unique<AutoContext>(collector, id);
	collector->pushDrawContext(activeContext.get());
}
