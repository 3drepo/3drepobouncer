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

#include "vectorize_device_rvt.h"
#include "geometry_dumper_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

VectorizeDeviceRvt::VectorizeDeviceRvt()
{
	setLogicalPalette(odcmAcadLightPalette(), 256);
	onSize(OdGsDCRect(0, 100, 0, 100));
}

void VectorizeDeviceRvt::init(GeometryCollector *const geoCollector) 
{
	destGeometryDumper = new GeometryDumperRvt();
	destGeometryDumper->init(geoCollector);
}

OdGsViewPtr VectorizeDeviceRvt::createView(
	const OdGsClientViewInfo* pInfo,
	bool bEnableLayerVisibilityPerView)
{
	OdGsViewPtr pView = VectorizeView::createObject();
	VectorizeView* pMyView = static_cast<VectorizeView*>(pView.get());
	pMyView->init(this, pInfo, bEnableLayerVisibilityPerView);
	pMyView->output().setDestGeometry(*destGeometryDumper);
	return (OdGsView*)pMyView;
}

OdGiConveyorGeometry* VectorizeDeviceRvt::destGeometry()
{
	return destGeometryDumper;
}

void VectorizeDeviceRvt::setupSimplifier(const OdGiDeviation* pDeviation)
{
	destGeometryDumper->setDeviation(pDeviation);
}

OdGsViewPtr VectorizeView::createObject()
{
	return OdRxObjectImpl<VectorizeView, OdGsView>::createObject();
}

VectorizeView::VectorizeView()
{
	eyeClip.m_bDrawBoundary = false;
}

void VectorizeView::ownerDrawDc(const OdGePoint3d& origin,
	const OdGeVector3d& u,
	const OdGeVector3d& v,
	const OdGiSelfGdiDrawable*,
	bool dcAligned,
	bool allowClipping)
{
	OdGeMatrix3d eyeToOutput = eyeToOutputTransform();
	OdGePoint3d originXformed(eyeToOutput * origin);
	OdGeVector3d uXformed(eyeToOutput * u), vXformed(eyeToOutput * v);
}

void VectorizeView::draw(const OdGiDrawable* pDrawable)
{
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

	OdGeMatrix3d eye2Screen(eyeToScreenMatrix());
	OdGeMatrix3d screen2Eye(eye2Screen.inverse());
	setEyeToOutputTransform(eye2Screen);

	eyeClip.m_bClippingFront = isFrontClipped();
	eyeClip.m_bClippingBack = isBackClipped();
	eyeClip.m_dFrontClipZ = frontClip();
	eyeClip.m_dBackClipZ = backClip();
	eyeClip.m_vNormal = viewDir();
	eyeClip.m_ptPoint = target();
	eyeClip.m_Points.clear();

	OdGeVector2d halfDiagonal(fieldWidth() / 2.0, fieldHeight() / 2.0);

	OdIntArray nrcCounts;
	OdGePoint2dArray nrcPoints;
	viewportClipRegion(nrcCounts, nrcPoints);
	if (nrcCounts.size() == 1)
	{
		for (int i = 0; i < nrcCounts[0]; i++)
		{
			OdGePoint3d screenPt(nrcPoints[i].x, nrcPoints[i].y, 0.0);
			screenPt.transformBy(screen2Eye);
			eyeClip.m_Points.append(OdGePoint2d(screenPt.x, screenPt.y));
		}
	}
	else
	{
		eyeClip.m_Points.append(OdGePoint2d::kOrigin - halfDiagonal);
		eyeClip.m_Points.append(OdGePoint2d::kOrigin + halfDiagonal);
	}

	eyeClip.m_xToClipSpace = getWorldToEyeTransform();
	pushClipBoundary(&eyeClip);
	OdGsBaseVectorizer::updateViewport();
	popClipBoundary();
}

void VectorizeView::onTraitsModified()
{
	OdGsBaseVectorizer::onTraitsModified();
	const OdGiSubEntityTraitsData& currTraits = effectiveTraits();
}

void VectorizeView::beginViewVectorization()
{
	OdGsBaseVectorizer::beginViewVectorization();
	device()->setupSimplifier(&m_pModelToEyeProc->eyeDeviation());
	setDrawContextFlags(drawContextFlags(), false);
}

VectorizeDeviceRvt* VectorizeView::device()
{
	return (VectorizeDeviceRvt*)OdGsBaseVectorizeView::device();
}
