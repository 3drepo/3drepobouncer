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
#include "oda_vectorise_device.h"

#include "oda_gi_dumper_impl.h"
#include "oda_gi_geo_dumper.h"
#include "../../../../core/model/bson/repo_node_mesh.h"

#include <vector>


#include <DgElement.h>
#include <RxObjectImpl.h>
#include <ColorMapping.h>
#include <toString.h>
#include <DgColorTable.h>


OdaVectoriseDevice::OdaVectoriseDevice()
{
	/**********************************************************************/
	/* Set a palette with a white background.                             */
	/**********************************************************************/
	setLogicalPalette(OdDgColorTable::defaultPalette(), 256);

	/**********************************************************************/
	/* Set the size of the vectorization window                           */
	/**********************************************************************/
	onSize(OdGsDCRect(0, 100, 0, 100));
}

OdGsDevicePtr OdaVectoriseDevice::createObject(DeviceType type, std::vector<repo::core::model::MeshNode> * meshVec)
{
	OdGsDevicePtr pRes = OdRxObjectImpl<OdaVectoriseDevice, OdGsDevice>::createObject();
	OdaVectoriseDevice* pMyDev = static_cast<OdaVectoriseDevice*>(pRes.get());

	pMyDev->m_type = type;

	/**********************************************************************/
	/* Create the output geometry dumper                                  */
	/**********************************************************************/
	pMyDev->m_pDumper = OdaGiDumperImpl::createObject();

	/**********************************************************************/
	/* Create the destination geometry receiver                           */
	/**********************************************************************/
	pMyDev->m_pDestGeometry = OdGiConveyorGeometryDumper::createObject(pMyDev->m_pDumper);
	((OdGiConveyorGeometryDumper*)pMyDev->m_pDestGeometry)->setMeshCollector(meshVec);
	return pRes;
}

/************************************************************************/
/* Creates a new OdGsView object, and associates it with this Device    */
/* object.                                                              */
/*                                                                      */
/* pInfo is a pointer to the Client View Information for this Device    */
/* object.                                                              */
/*                                                                      */
/* bEnableLayerVisibilityPerView specifies that layer visibility        */
/* per viewport is to be supported.                                     */
/************************************************************************/
OdGsViewPtr OdaVectoriseDevice::createView(const OdGsClientViewInfo* pInfo,
	bool bEnableLayerVisibilityPerView)
{
	/**********************************************************************/
	/* Create a View                                                      */
	/**********************************************************************/
	OdGsViewPtr pView = ExSimpleView::createObject();
	ExSimpleView* pMyView = static_cast<ExSimpleView*>(pView.get());

	/**********************************************************************/
	/* This call is required by DD 1.13+                                  */
	/**********************************************************************/
	pMyView->init(this, pInfo, bEnableLayerVisibilityPerView);

	/**********************************************************************/
	/* Directs the output geometry for the view to the                    */
	/* destination geometry object                                        */
	/**********************************************************************/
	pMyView->output().setDestGeometry(*m_pDestGeometry);

	return (OdGsView*)pMyView;
}

/************************************************************************/
/* Returns the geometry receiver associated with this object.           */
/************************************************************************/
OdGiConveyorGeometry* OdaVectoriseDevice::destGeometry()
{
	return m_pDestGeometry;
}

/************************************************************************/
/* Returns the geometry dumper associated with this object.             */
/************************************************************************/
OdaGiDumper* OdaVectoriseDevice::dumper()
{
	return m_pDumper;
}

void ExSimpleView::ownerDrawDc(const OdGePoint3d& origin,
	const OdGeVector3d& u,
	const OdGeVector3d& v,
	const OdGiSelfGdiDrawable* /*pDrawable*/,
	bool dcAligned,
	bool allowClipping)
{
	// ownerDrawDc is not conveyor primitive. It means we must take all rendering processings
	// (transforms) by ourselves
	OdGeMatrix3d eyeToOutput = eyeToOutputTransform();
	OdGePoint3d originXformed(eyeToOutput * origin);
	OdGeVector3d uXformed(eyeToOutput * u), vXformed(eyeToOutput * v);

	OdaGiDumper* pDumper = device()->dumper();

	pDumper->output(OD_T("ownerDrawDc"));

	// It's shown only in 2d mode.
	if (mode() == OdGsView::k2DOptimized)
	{
		pDumper->pushIndent();
		pDumper->output(OD_T("origin xformed"), toString(originXformed));
		pDumper->output(OD_T("u xformed"), toString(uXformed));
		pDumper->output(OD_T("v xformed"), toString(vXformed));
		pDumper->output(OD_T("dcAligned"), toString(dcAligned));//
		pDumper->output(OD_T("allowClipping"), toString(allowClipping));
		pDumper->popIndent();
	}
}

/************************************************************************/
/* Called by the Teigha for .dwg files vectorization framework to update*/
/* the GUI window for this Device object                                */
/*                                                                      */
/* pUpdatedRect specifies a rectangle to receive the region updated     */
/* by this function.                                                    */
/*                                                                      */
/* The override should call the parent version of this function,        */
/* OdGsBaseVectorizeDevice::update().                                   */
/************************************************************************/
void OdaVectoriseDevice::update(OdGsDCRect* pUpdatedRect)
{
	OdGsBaseVectorizeDevice::update(pUpdatedRect);
}

/************************************************************************/
/* Called by each associated view object to set the current RGB draw    */
/* color.                                                               */
/************************************************************************/
void OdaVectoriseDevice::draw_color(ODCOLORREF color)
{
	dumper()->output(OD_T("draw_color"), toRGBString(color));
}

/************************************************************************/
/* Called by each associated view object to set the current ACI draw    */
/* color.                                                               */
/************************************************************************/
void OdaVectoriseDevice::draw_color_index(OdUInt16 colorIndex)
{
	dumper()->output(OD_T("draw_color_index"), toString(colorIndex));
}

/************************************************************************/
/* Called by each associated view object to set the current draw        */
/* lineweight and pixel width                                           */
/************************************************************************/
void OdaVectoriseDevice::draw_lineWidth(OdDb::LineWeight weight, int pixelLineWidth)
{
	dumper()->output(OD_T("draw_lineWidth"));
	dumper()->pushIndent();
	dumper()->output(OD_T("weight"), toString(weight));
	dumper()->output(OD_T("pixelLineWidth"), toString(pixelLineWidth));
	dumper()->popIndent();
}


/************************************************************************/
/* Called by each associated view object to set the current Fill Mode   */
/************************************************************************/
void OdaVectoriseDevice::draw_fill_mode(OdGiFillType fillMode)
{
	OdString strMode;
	switch (fillMode)
	{
	case kOdGiFillAlways:
		strMode = OD_T("FillAlways");
		break;
	case kOdGiFillNever:
		strMode = OD_T("FillNever");
	}
	dumper()->output(OD_T("draw_fill_mode"), strMode);
}

/************************************************************************/
/* Called by the Teigha for .dwg files vectorization framework to give the */
/* client application a chance to terminate the current                 */
/* vectorization process.  Returning true from this function will       */
/* stop the current vectorization operation.                            */
/************************************************************************/

bool ExSimpleView::regenAbort() const
{
	// return true here to abort the vectorization process
	return false;
}

/************************************************************************/
/* Called by the Teigha for .dwg files vectorization framework to vectorize */
/* each entity in this view.  This override allows a client             */
/* application to examine each entity before it is vectorized.          */
/* The override should call the parent version of this function,        */
/* OdGsBaseVectorizeView::draw().                                       */
/************************************************************************/
/*
void ExSimpleView::draw(const OdGiDrawable* pDrawable)
{
OdDgElementPtr pElm = OdDgElement::cast(pDrawable);

device()->dumper()->output(OD_T(""));
if (pElm.get())
{
device()->dumper()->output(OD_T("Start Drawing "), toString(pElm->elementId().getHandle()));
}
else
{
device()->dumper()->output(OD_T("Start Drawing"), toString(OD_T("non-DbResident")));
}

device()->dumper()->pushIndent();
OdGsBaseVectorizer::draw(pDrawable);

device()->dumper()->popIndent();
if (pElm.get())
{
device()->dumper()->output(OD_T("End Drawing "), toString(pElm->elementId().getHandle()));
}
else
{
device()->dumper()->output(OD_T("End Drawing "), toString(OD_T("non-DbResident")));
}
}
*/
void ExSimpleView::draw(const OdGiDrawable* pDrawable)
{
	OdDgElementPtr pElm = OdDgElement::cast(pDrawable);

	device()->dumper()->output(OD_T(""));
	OdString sClassName = toString(pElm->isA());
	OdString sHandle = pElm->isDBRO() ? toString(pElm->elementId().getHandle()) : toString(OD_T("non-DbResident"));

	device()->dumper()->output(OD_T("Start Drawing ") + sClassName, sHandle);
	//device()->dumper()->pushIndent();

	/**********************************************************************/
	/* The parent class version of this function must be called.          */
	/**********************************************************************/
	OdGsBaseVectorizer::draw(pDrawable);

	//device()->dumper()->popIndent();
	device()->dumper()->output(OD_T("End Drawing ") + sClassName, sHandle);
}

/************************************************************************/
/* Flushes any queued graphics to the display device.                   */
/************************************************************************/
void ExSimpleView::updateViewport()
{
	/**********************************************************************/
	/* If geometry receiver is a simplifier, we must re-set the draw      */
	/* context for it                                                     */
	/**********************************************************************/
	(static_cast<OdGiGeometrySimplifier*>(device()->destGeometry()))->setDrawContext(drawContext());

	/**********************************************************************/
	/* Comment these functions to get primitives in eye coordinates.      */
	/**********************************************************************/
	OdGeMatrix3d eye2Screen(eyeToScreenMatrix());
	OdGeMatrix3d screen2Eye(eye2Screen.inverse());
	setEyeToOutputTransform(eye2Screen);

	// perform viewport clipping in eye coordinates:

	/**********************************************************************/
	/* Perform viewport clipping in eye coordinates.                      */
	/**********************************************************************/
	m_eyeClip.m_bClippingFront = isFrontClipped();
	m_eyeClip.m_bClippingBack = isBackClipped();
	m_eyeClip.m_dFrontClipZ = frontClip();
	m_eyeClip.m_dBackClipZ = backClip();
	m_eyeClip.m_vNormal = viewDir();
	m_eyeClip.m_ptPoint = target();
	m_eyeClip.m_Points.clear();

	OdGeVector2d halfDiagonal(fieldWidth() / 2.0, fieldHeight() / 2.0);

	OdIntArray nrcCounts;
	OdGePoint2dArray nrcPoints;
	viewportClipRegion(nrcCounts, nrcPoints);
	if (nrcCounts.size() == 1) // polygons aren't supported yet
	{
		// polygonal clipping
		int i;
		for (i = 0; i < nrcCounts[0]; i++)
		{
			OdGePoint3d screenPt(nrcPoints[i].x, nrcPoints[i].y, 0.0);
			screenPt.transformBy(screen2Eye);
			m_eyeClip.m_Points.append(OdGePoint2d(screenPt.x, screenPt.y));
		}
	}
	else
	{
		// rectangular clipping
		m_eyeClip.m_Points.append(OdGePoint2d::kOrigin - halfDiagonal);
		m_eyeClip.m_Points.append(OdGePoint2d::kOrigin + halfDiagonal);
	}

	m_eyeClip.m_xToClipSpace = getWorldToEyeTransform();

	pushClipBoundary(&m_eyeClip);

	OdGsBaseVectorizer::updateViewport();

	popClipBoundary();
} // end ExSimpleView::updateViewport()


  /************************************************************************/
  /* Notification function called by the vectorization framework          */
  /* whenever the rendering attributes have changed.                      */
  /*                                                                      */
  /* Retrieves the current vectorization traits from Teigha for .dwg files (color */
  /* lineweight, etc.) and sets them in this device.                      */
  /************************************************************************/
void ExSimpleView::onTraitsModified()
{
	OdGsBaseVectorizer::onTraitsModified();
	const OdGiSubEntityTraitsData& currTraits = effectiveTraits();
	if (currTraits.trueColor().isByColor())
	{
		device()->draw_color(ODTOCOLORREF(currTraits.trueColor()));
	}
	else
	{
		device()->draw_color_index(currTraits.color());
	}

	OdDb::LineWeight lw = currTraits.lineWeight();
	device()->draw_lineWidth(lw, lineweightToPixels(lw));
	device()->draw_fill_mode(currTraits.fillType());

} // end ExSimpleView::onTraitsModified()


void ExSimpleView::beginViewVectorization()
{
	OdGsBaseVectorizer::beginViewVectorization();
	device()->setupSimplifier(&m_pModelToEyeProc->eyeDeviation());
}

void OdaVectoriseDevice::setupSimplifier(const OdGiDeviation* pDeviation)
{
	m_pDestGeometry->setDeviation(pDeviation);
}


//==========================================================================================
