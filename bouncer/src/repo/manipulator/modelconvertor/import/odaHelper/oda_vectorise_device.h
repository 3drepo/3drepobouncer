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

#pragma once
#include <Gs/GsBaseInclude.h>
#include <RxObjectImpl.h>
#include <iostream>
#include "oda_gi_dumper.h"
#include "oda_gi_geo_dumper.h"


class ExSimpleView;

/************************************************************************/
/* OdGsBaseVectorizeDevice objects own, update, and refresh one or more */
/* OdGsView objects.                                                    */
/************************************************************************/
class OdaVectoriseDevice :
	public OdGsBaseVectorizeDevice
{
	OdGiConveyorGeometryDumperPtr m_pDestGeometry;
	OdaGiDumperPtr                 m_pDumper;
	OdaGeometryCollector * geoCollector;
public:
	enum DeviceType
	{
		k2dDevice,
		k3dDevice
	};

	OdaVectoriseDevice();

	/**********************************************************************/
	/* Set the target data stream and the type.                           */
	/**********************************************************************/
	static OdGsDevicePtr createObject(DeviceType type, OdaGeometryCollector *const geoCollector);

	/**********************************************************************/
	/* Called by the Teigha for .dwg files vectorization framework to update */
	/* the GUI window for this Device object                              */
	/*                                                                    */
	/* pUpdatedRect specifies a rectangle to receive the region updated   */
	/* by this function.                                                  */
	/*                                                                    */
	/* The override should call the parent version of this function,      */
	/* OdGsBaseVectorizeDevice::update().                                 */
	/**********************************************************************/
	void update(OdGsDCRect* pUpdatedRect);

	/**********************************************************************/
	/* Creates a new OdGsView object, and associates it with this Device  */
	/* object.                                                            */
	/*                                                                    */
	/* pInfo is a pointer to the Client View Information for this Device  */
	/* object.                                                            */
	/*                                                                    */
	/* bEnableLayerVisibilityPerView specifies that layer visibility      */
	/* per viewport is to be supported.                                   */
	/**********************************************************************/
	OdGsViewPtr createView(const OdGsClientViewInfo* pInfo = 0,
		bool bEnableLayerVisibilityPerView = false);


	/**********************************************************************/
	/* Returns the geometry receiver associated with this object.         */
	/**********************************************************************/
	OdGiConveyorGeometry* destGeometry();

	/**********************************************************************/
	/* Returns the geometry dumper associated with this object.           */
	/**********************************************************************/
	OdaGiDumper* dumper();

	/**********************************************************************/
	/* Called by each associated view object to set the current RGB draw  */
	/* color.                                                             */
	/**********************************************************************/
	void draw_color(ODCOLORREF color);

	/**********************************************************************/
	/* Called by each associated view object to set the current ACI draw  */
	/* color.                                                             */
	/**********************************************************************/
	void draw_color_index(OdUInt16 colorIndex);

	/**********************************************************************/
	/* Called by each associated view object to set the current draw      */
	/* lineweight and pixel width                                         */
	/**********************************************************************/
	void draw_lineWidth(OdDb::LineWeight weight, int pixelLineWidth);

	/************************************************************************/
	/* Called by each associated view object to set the current Fill Mode   */
	/************************************************************************/
	void draw_fill_mode(OdGiFillType fillMode);


	void setupSimplifier(const OdGiDeviation* pDeviation);

private:
	DeviceType              m_type;
}; // end OdaVectoriseDevice

   /************************************************************************/
   /* This template class is a specialization of the OdSmartPtr class for  */
   /* OdaVectoriseDevice object pointers                                     */
   /************************************************************************/
typedef OdSmartPtr<OdaVectoriseDevice> OdaVectoriseDevicePtr;

/************************************************************************/
/* Example client view class that demonstrates how to receive           */
/* geometry from the vectorization process.                             */
/************************************************************************/
class ExSimpleView : public OdGsBaseVectorizeViewDef
{
	OdGiClipBoundary        m_eyeClip;
protected:

	/**********************************************************************/
	/* Returns the OdaVectoriseDevice instance that owns this view.         */
	/**********************************************************************/
	virtual void beginViewVectorization();

	OdaVectoriseDevice* device()
	{
		return (OdaVectoriseDevice*)OdGsBaseVectorizeView::device();
	}

	ExSimpleView()
	{
		m_eyeClip.m_bDrawBoundary = false;
	}

	friend class OdaVectoriseDevice;

	/**********************************************************************/
	/* PseudoConstructor                                                  */
	/**********************************************************************/
	static OdGsViewPtr createObject()
	{
		return OdRxObjectImpl<ExSimpleView, OdGsView>::createObject();
	}

	/**********************************************************************/
	/* To know when to set traits                                         */
	/**********************************************************************/
	void setEntityTraits();

public:
	/**********************************************************************/
	/* Called by the Teigha for .dwg files vectorization framework to vectorize */
	/* each entity in this view.  This override allows a client           */
	/* application to examine each entity before it is vectorized.        */
	/* The override should call the parent version of this function,      */
	/* OdGsBaseVectorizeView::draw().                                     */
	/**********************************************************************/
	void draw(const OdGiDrawable*);

	/**********************************************************************/
	/* Called by the Teigha for .dwg files vectorization framework to give the */
	/* client application a chance to terminate the current               */
	/* vectorization process.  Returning true from this function will     */
	/* stop the current vectorization operation.                          */
	/**********************************************************************/
	bool regenAbort() const;

	/**********************************************************************/
	/* Returns the recommended maximum deviation of the current           */
	/* vectorization.                                                     */
	/**********************************************************************/
	double deviation(const OdGiDeviationType, const OdGePoint3d&) const
	{
		// NOTE: use some reasonable value
		return 0.8; //0.000001; 
	}

	/**********************************************************************/
	/* Flushes any queued graphics to the display device.                 */
	/**********************************************************************/
	void updateViewport();

	TD_USING(OdGsBaseVectorizeViewDef::updateViewport);

	/**********************************************************************/
	/* Notification function called by the vectorization framework        */
	/* whenever the rendering attributes have changed.                    */
	/*                                                                    */
	/* Retrieves the current vectorization traits from Teigha for .dwg files (color */
	/* lineweight, etc.) and sets them in this device.                    */
	/**********************************************************************/
	void onTraitsModified();

	void ownerDrawDc(
		const OdGePoint3d& origin,
		const OdGeVector3d& u,
		const OdGeVector3d& v,
		const OdGiSelfGdiDrawable* pDrawable,
		bool bDcAligned,
		bool bAllowClipping);
}; // end ExSimpleView 

