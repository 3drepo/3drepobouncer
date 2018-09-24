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


#include "geometry_dumper.h"
#include "../../../../core/model/bson/repo_bson_factory.h"
using namespace repo::manipulator::modelconvertor::odaHelper;

void GeometryDumper::triangleOut(const OdInt32* p3Vertices, const OdGeVector3d* pNormal)
{
	const OdGePoint3d*  pVertexDataList = vertexDataList();
	const OdGeVector3d* pNormals = NULL;
	
	if ((pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[1]) &&
		(pVertexDataList + p3Vertices[0]) != (pVertexDataList + p3Vertices[2]) &&
		(pVertexDataList + p3Vertices[1]) != (pVertexDataList + p3Vertices[2]))
	{
		repo_face_t face;
		for (int i = 0; i < 3; ++i)
		{
			std::stringstream ss;
			ss.precision(17);
			ss << std::fixed << pVertexDataList[p3Vertices[i]].x << "," << std::fixed << pVertexDataList[p3Vertices[i]].y << "," << std::fixed << pVertexDataList[p3Vertices[i]].z;
			auto vStr = ss.str();
			if (vToVIndex.find(vStr) == vToVIndex.end())
			{
				vToVIndex[vStr] = vertices.size();
				vertices.push_back({ pVertexDataList[p3Vertices[i]].x , pVertexDataList[p3Vertices[i]].y, pVertexDataList[p3Vertices[i]].z });
				if (boundingBox.size()) {
					boundingBox[0][0] = boundingBox[0][0] > pVertexDataList[p3Vertices[i]].x ? pVertexDataList[p3Vertices[i]].x : boundingBox[0][0];
					boundingBox[0][1] = boundingBox[0][1] > pVertexDataList[p3Vertices[i]].y ? pVertexDataList[p3Vertices[i]].y : boundingBox[0][1];
					boundingBox[0][2] = boundingBox[0][2] > pVertexDataList[p3Vertices[i]].z ? pVertexDataList[p3Vertices[i]].z : boundingBox[0][2];

					boundingBox[1][0] = boundingBox[1][0] < pVertexDataList[p3Vertices[i]].x ? pVertexDataList[p3Vertices[i]].x : boundingBox[1][0];
					boundingBox[1][1] = boundingBox[1][1] < pVertexDataList[p3Vertices[i]].y ? pVertexDataList[p3Vertices[i]].y : boundingBox[1][1];
					boundingBox[1][2] = boundingBox[1][2] < pVertexDataList[p3Vertices[i]].z ? pVertexDataList[p3Vertices[i]].z : boundingBox[1][2];
				}
				else {
					boundingBox.push_back({ pVertexDataList[p3Vertices[i]].x, pVertexDataList[p3Vertices[i]].y, pVertexDataList[p3Vertices[i]].z });
					boundingBox.push_back({ pVertexDataList[p3Vertices[i]].x, pVertexDataList[p3Vertices[i]].y, pVertexDataList[p3Vertices[i]].z });
				}
			}
			face.push_back(vToVIndex[vStr]);

			
		}
		faces.push_back(face);
	}

}

double GeometryDumper::deviation(
	const OdGiDeviationType deviationType, 
	const OdGePoint3d& pointOnCurve) const {
	return 0;
}

OdaVectoriseDevice* GeometryDumper::device()
{
	return static_cast<OdaVectoriseDevice*>(OdGsBaseVectorizeView::device());
}

bool GeometryDumper::doDraw(OdUInt32 i, const OdGiDrawable* pDrawable)
{
	return OdGsBaseMaterialView::doDraw(i, pDrawable);
}

OdCmEntityColor GeometryDumper::fixByACI(const ODCOLORREF *ids, const OdCmEntityColor &color)
{
	if (color.isByACI() || color.isByDgnIndex())
	{
		return OdCmEntityColor(ODGETRED(ids[color.colorIndex()]), ODGETGREEN(ids[color.colorIndex()]), ODGETBLUE(ids[color.colorIndex()]));
	}
	else if (!color.isByColor())
	{
		return OdCmEntityColor(0, 0, 0);
	}
	return color;
}


OdGiMaterialItemPtr GeometryDumper::fillMaterialCache(
	OdGiMaterialItemPtr prevCache, 
	OdDbStub* materialId, 
	const OdGiMaterialTraitsData & materialData
) {
	if (vertices.size() && faces.size())
		collector->addMeshEntry(vertices, faces, boundingBox);

	vertices.clear();
	faces.clear();
	boundingBox.clear();
	vToVIndex.clear();

	auto id = (OdUInt64)(OdIntPtr)materialId;

	OdGiMaterialColor diffuseColor; OdGiMaterialMap diffuseMap;
	OdGiMaterialColor ambientColor;
	OdGiMaterialColor specularColor; OdGiMaterialMap specularMap; double glossFactor;
	double opacityPercentage; OdGiMaterialMap opacityMap;
	double refrIndex; OdGiMaterialMap refrMap;

	materialData.diffuse(diffuseColor, diffuseMap);
	materialData.ambient(ambientColor);
	materialData.specular(specularColor, specularMap, glossFactor);
	materialData.opacity(opacityPercentage, opacityMap);
	materialData.refraction(refrIndex, refrMap);

	OdGiMaterialMap bumpMap;
	materialData.bump(bumpMap);


	ODCOLORREF colorDiffuse(0), colorAmbient(0), colorSpecular(0);
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


	OdCmEntityColor color = fixByACI(this->device()->getPalette(), effectiveTraits().trueColor());
	repo_material_t material;
	// diffuse
	if (diffuseColor.method() == OdGiMaterialColor::kOverride)
		material.diffuse = { ODGETRED(colorDiffuse) / 255.0f, ODGETGREEN(colorDiffuse) / 255.0f, ODGETBLUE(colorDiffuse) / 255.0f, 1.0f };
	else
		material.diffuse = { color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f, 1.0f };
	/*
	TODO: texture support
	mData.bDiffuseChannelEnabled = (GETBIT(materialData.channelFlags(), OdGiMaterialTraits::kUseDiffuse)) ? true : false;
	if (mData.bDiffuseChannelEnabled && diffuseMap.source() == OdGiMaterialMap::kFile && !diffuseMap.sourceFileName().isEmpty())
	{
	mData.bDiffuseHasTexture = true;
	mData.sDiffuseFileSource = diffuseMap.sourceFileName();
	}*/

	// specular
	if (specularColor.method() == OdGiMaterialColor::kOverride)
		material.specular = { ODGETRED(colorSpecular) / 255.0f, ODGETGREEN(colorSpecular) / 255.0f, ODGETBLUE(colorSpecular) / 255.0f, 1.0f };
	else
		material.specular = { color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f, 1.0f };

	material.shininessStrength = 1 - glossFactor;
	material.shininess = materialData.reflectivity();

	// opacity
	material.opacity = opacityPercentage;


	// refraction
	/*mData.bRefractionChannelEnabled = (GETBIT(materialData.channelFlags(), OdGiMaterialTraits::kUseRefraction)) ? 1 : 0;
	mData.dRefractionIndex = materialData.reflectivity();*/

	// transclucence
	//mData.dTranslucence = materialData.translucence();						

	collector->addMaterial(id, material);

	return OdGiMaterialItemPtr();
}

void GeometryDumper::init(OdaGeometryCollector *const geoCollector) {
	collector = geoCollector;
}

void GeometryDumper::beginViewVectorization()
{
	OdGsBaseMaterialView::beginViewVectorization();
	setEyeToOutputTransform(getEyeToWorldTransform());

	OdGiGeometrySimplifier::setDrawContext(OdGsBaseMaterialView::drawContext());
	output().setDestGeometry((OdGiGeometrySimplifier&)*this);
}

void GeometryDumper::endViewVectorization()
{
	if (vertices.size() && faces.size())
		collector->addMeshEntry(vertices, faces, boundingBox);
	OdGsBaseMaterialView::endViewVectorization();
}

void GeometryDumper::setMode(OdGsView::RenderMode mode)
{
	OdGsBaseVectorizeView::m_renderMode = kGouraudShaded;
	m_regenerationType = kOdGiRenderCommand;
	OdGiGeometrySimplifier::m_renderMode = OdGsBaseVectorizeView::m_renderMode;
}