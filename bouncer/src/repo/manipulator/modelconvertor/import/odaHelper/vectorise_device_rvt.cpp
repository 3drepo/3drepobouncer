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

#include <BimCommon.h>
#include <Database/BmElement.h>
#include <RxObjectImpl.h>
#include <ColorMapping.h>
#include <toString.h>

#include "vectorise_device_rvt.h"
#include "geometry_dumper_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

VectoriseDeviceRvt::VectoriseDeviceRvt()
{
	setLogicalPalette(odcmAcadLightPalette(), 256);
	onSize(OdGsDCRect(0, 100, 0, 100));
}

void VectoriseDeviceRvt::init(GeometryCollector *const geoCollector)
{
	destGeometryDumper = new GeometryDumperRvt();
	destGeometryDumper->init(geoCollector);
	geoColl = geoCollector;
}

OdGsViewPtr VectoriseDeviceRvt::createView(
	const OdGsClientViewInfo* pInfo,
	bool bEnableLayerVisibilityPerView)
{
	OdGsViewPtr pView = VectorizeView::createObject(geoColl);
	VectorizeView* pMyView = static_cast<VectorizeView*>(pView.get());
	pMyView->init(this, pInfo, bEnableLayerVisibilityPerView);
	pMyView->output().setDestGeometry(*destGeometryDumper);
	return (OdGsView*)pMyView;
}

void VectoriseDeviceRvt::setupSimplifier(const OdGiDeviation* pDeviation)
{
	destGeometryDumper->setDeviation(pDeviation);
}

OdGiConveyorGeometry* VectoriseDeviceRvt::destGeometry()
{
	return destGeometryDumper;
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

void VectorizeView::updateViewport()
{
	(static_cast<OdGiGeometrySimplifier*>(device()->destGeometry()))->setDrawContext(drawContext());
	OdGsBaseVectorizer::updateViewport();
}

void VectorizeView::beginViewVectorization()
{
	OdGsBaseVectorizer::beginViewVectorization();
	device()->setupSimplifier(&m_pModelToEyeProc->eyeDeviation());
	setDrawContextFlags(drawContextFlags(), false);
	setEyeToOutputTransform(getEyeToWorldTransform());
}

VectoriseDeviceRvt* VectorizeView::device()
{
	return (VectoriseDeviceRvt*)OdGsBaseVectorizeView::device();
}

OdGiMaterialItemPtr VectorizeView::fillMaterialCache(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData)
{
	auto id = (OdUInt64)(OdIntPtr)materialId;

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

	repo_material_t material;
	const float norm = 255.f;

	material.diffuse = { ODGETRED(colorDiffuse) / norm, ODGETGREEN(colorDiffuse) / norm, ODGETBLUE(colorDiffuse) / norm, 1.0f };
	material.specular = { ODGETRED(colorSpecular) / norm, ODGETGREEN(colorSpecular) / norm, ODGETBLUE(colorSpecular) / norm, 1.0f };
	material.ambient = { ODGETRED(colorAmbient) / norm, ODGETGREEN(colorAmbient) / norm, ODGETBLUE(colorAmbient) / norm, 1.0f };
	material.emissive = { ODGETRED(colorEmissive) / norm, ODGETGREEN(colorEmissive) / norm, ODGETBLUE(colorEmissive) / norm, 1.0f };

	material.shininessStrength = 1 - glossFactor;
	material.shininess = materialData.reflectivity();

	material.opacity = opacityPercentage;

	geoColl->setCurrentMaterial(material);
	geoColl->stopMeshEntry();
	geoColl->startMeshEntry();

	return OdGiMaterialItemPtr();
}