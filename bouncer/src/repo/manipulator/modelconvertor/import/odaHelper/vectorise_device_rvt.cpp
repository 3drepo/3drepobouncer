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

void VectoriseDeviceRvt::init(GeometryCollector *const geoCollector, MaterialCollectorRvt& matCollector)
{
	destGeometryDumper = new GeometryDumperRvt();
	destGeometryDumper->init(geoCollector);
	geoColl = geoCollector;
	matColl = &matCollector;
}

OdGsViewPtr VectoriseDeviceRvt::createView(
	const OdGsClientViewInfo* pInfo,
	bool bEnableLayerVisibilityPerView)
{
	OdGsViewPtr pView = VectorizeView::createObject(geoColl, matColl);
	VectorizeView* pMyView = static_cast<VectorizeView*>(pView.get());
	pMyView->init(this, pInfo, bEnableLayerVisibilityPerView);
	pMyView->output().setDestGeometry(*destGeometryDumper);
	return (OdGsView*)pMyView;
}

OdGiConveyorGeometry* VectoriseDeviceRvt::destGeometry()
{
	return destGeometryDumper;
}

OdGsViewPtr VectorizeView::createObject(GeometryCollector* geoColl, MaterialCollectorRvt* matColl)
{
	auto ptr = OdRxObjectImpl<VectorizeView, OdGsView>::createObject();
	static_cast<VectorizeView*>(ptr.get())->geoColl = geoColl;
	static_cast<VectorizeView*>(ptr.get())->matColl = matColl;
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
	
	repo_material_t mat;
	if (matColl->getMaterial(pDrawable, mat))
	{
		geoColl->setCurrentMaterial(mat);
	}
	
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

void VectorizeView::onTraitsModified()
{
	OdGsBaseVectorizer::onTraitsModified();
	const OdGiSubEntityTraitsData& currTraits = effectiveTraits();
}

void VectorizeView::beginViewVectorization()
{
	OdGsBaseVectorizer::beginViewVectorization();
	setEyeToOutputTransform(getEyeToWorldTransform());
}

VectoriseDeviceRvt* VectorizeView::device()
{
	return (VectoriseDeviceRvt*)OdGsBaseVectorizeView::device();
}
