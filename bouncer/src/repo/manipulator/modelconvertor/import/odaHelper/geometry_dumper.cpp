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
		std::vector<repo::lib::RepoVector3D64> vertices;
		for (int i = 0; i < 3; ++i)
		{			
			vertices.push_back({ pVertexDataList[p3Vertices[i]].x , pVertexDataList[p3Vertices[i]].y, pVertexDataList[p3Vertices[i]].z });						
		}
		collector->addFace(vertices);
	}
}

double GeometryDumper::deviation(
	const OdGiDeviationType deviationType, 
	const OdGePoint3d& pointOnCurve) const {
	return 0;
}

VectoriseDevice* GeometryDumper::device()
{
	return static_cast<VectoriseDevice*>(OdGsBaseVectorizeView::device());
}

bool GeometryDumper::doDraw(OdUInt32 i, const OdGiDrawable* pDrawable)
{
	OdDgElementPtr pElm = OdDgElement::cast(pDrawable);

	OdString sClassName = toString(pElm->isA());
	OdString sHandle = pElm->isDBRO() ? toString(pElm->elementId().getHandle()) : toString(OD_T("non-DbResident"));

	std::stringstream ss;
	ss << sHandle;
	collector->setNextMeshName(ss.str());
	return OdGsBaseMaterialView::doDraw(i, pDrawable);
}

void GeometryDumper::onTraitsModified() {
	OdGsBaseVectorizer::onTraitsModified();

	OdGiSubEntityTraitsData traits = effectiveTraits();
	OdDgElementId idLevel = traits.layer();
	if (!idLevel.isNull())
	{
		OdDgLevelTableRecordPtr pLevel = idLevel.openObject(OdDg::kForRead);
		OdUInt32 iLevelEntry = pLevel->getEntryId();

		std::stringstream ss;
		ss << pLevel->getName();
		collector->setLayer(ss.str());
	}
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

	collector->setCurrentMaterial(material);
	collector->stopMeshEntry();
	collector->startMeshEntry();

	return OdGiMaterialItemPtr();
}

void GeometryDumper::init(GeometryCollector *const geoCollector) {
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
	collector->stopMeshEntry();
	OdGsBaseMaterialView::endViewVectorization();
}

void GeometryDumper::setMode(OdGsView::RenderMode mode)
{
	OdGsBaseVectorizeView::m_renderMode = kGouraudShaded;
	m_regenerationType = kOdGiRenderCommand;
	OdGiGeometrySimplifier::m_renderMode = OdGsBaseVectorizeView::m_renderMode;
}