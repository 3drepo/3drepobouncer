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

#include "BimCommon.h"
#include "Database/BmElement.h"
#include "RxObjectImpl.h"
#include "simple_device.h"
#include "conveyor_geometry_dumper.h"
#include "ColorMapping.h"
#include "toString.h"

SimpleDevice::SimpleDevice()
{
	setLogicalPalette(odcmAcadLightPalette(), 256);
	onSize(OdGsDCRect(0, 100, 0, 100));
}

OdGsDevicePtr SimpleDevice::createObject(DeviceType type, GeometryCollector* gcollector)
{
	OdGsDevicePtr pRes = OdRxObjectImpl<SimpleDevice, OdGsDevice>::createObject();
	SimpleDevice* pMyDev = static_cast<SimpleDevice*>(pRes.get());
	pMyDev->m_type = type;
	pMyDev->m_pDestGeometry = ConveyorGeometryDumper::createObject(gcollector);
	return pRes;
}

OdGsViewPtr SimpleDevice::createView(const OdGsClientViewInfo* pInfo,
	bool bEnableLayerVisibilityPerView)
{
	OdGsViewPtr pView = SimpleView::createObject();
	SimpleView* pMyView = static_cast<SimpleView*>(pView.get());
	pMyView->init(this, pInfo, bEnableLayerVisibilityPerView);
	pMyView->output().setDestGeometry(*m_pDestGeometry);
	return (OdGsView*)pMyView;
}

OdGiConveyorGeometry* SimpleDevice::destGeometry()
{
	return m_pDestGeometry;
}

void SimpleView::ownerDrawDc(const OdGePoint3d& origin,
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

void SimpleDevice::update(OdGsDCRect* pUpdatedRect)
{
	OdGsBaseVectorizeDevice::update(pUpdatedRect);
}

bool SimpleView::regenAbort() const
{
	return false;
}

void SimpleView::draw(const OdGiDrawable* pDrawable)
{
	OdString sClassName = toString(pDrawable->isA());
	bool bDBRO = false;
	OdBmElementPtr pElem = OdBmElement::cast(pDrawable);
	if (!pElem.isNull())
		bDBRO = pElem->isDBRO();
	OdString sHandle = bDBRO ? toString(pElem->objectId().getHandle()) : toString(L"non-DbResident");
	OdGsBaseVectorizer::draw(pDrawable);
}

void SimpleView::updateViewport()
{
	(static_cast<OdGiGeometrySimplifier*>(device()->destGeometry()))->setDrawContext(drawContext());

	OdGeMatrix3d eye2Screen(eyeToScreenMatrix());
	OdGeMatrix3d screen2Eye(eye2Screen.inverse());
	setEyeToOutputTransform(eye2Screen);

	m_eyeClip.m_bClippingFront = isFrontClipped();
	m_eyeClip.m_bClippingBack = isBackClipped();
	m_eyeClip.m_dFrontClipZ = frontClip();
	m_eyeClip.m_dBackClipZ = backClip();
	m_eyeClip.m_vNormal = viewDir();
	m_eyeClip.m_ptPoint = target();
	m_eyeClip.m_Points.clear();

	OdGeVector2d halfDiagonal(fieldWidth() / 2.0, fieldHeight() / 2.0);

	OdIntArray m_nrcCounts;
	OdGePoint2dArray m_nrcPoints;
	viewportClipRegion(m_nrcCounts, m_nrcPoints);
	if (m_nrcCounts.size() == 1)
	{
		int i;
		for (i = 0; i < m_nrcCounts[0]; i++)
		{
			OdGePoint3d screenPt(m_nrcPoints[i].x, m_nrcPoints[i].y, 0.0);
			screenPt.transformBy(screen2Eye);
			m_eyeClip.m_Points.append(OdGePoint2d(screenPt.x, screenPt.y));
		}
	}
	else
	{
		m_eyeClip.m_Points.append(OdGePoint2d::kOrigin - halfDiagonal);
		m_eyeClip.m_Points.append(OdGePoint2d::kOrigin + halfDiagonal);
	}

	m_eyeClip.m_xToClipSpace = getWorldToEyeTransform();
	pushClipBoundary(&m_eyeClip);
	OdGsBaseVectorizer::updateViewport();
	popClipBoundary();
}

void SimpleView::onTraitsModified()
{
	OdGsBaseVectorizer::onTraitsModified();
	const OdGiSubEntityTraitsData& currTraits = effectiveTraits();

}

void SimpleView::beginViewVectorization()
{
	OdGsBaseVectorizer::beginViewVectorization();
	device()->setupSimplifier(&m_pModelToEyeProc->eyeDeviation());
	setDrawContextFlags(drawContextFlags(), false);
}

void SimpleDevice::setupSimplifier(const OdGiDeviation* pDeviation)
{
	m_pDestGeometry->setDeviation(pDeviation);
}

