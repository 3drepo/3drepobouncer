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
#include "Gs/GsBaseInclude.h"
#include "conveyor_geometry_dumper.h"
#include <iostream>

class SimpleView;

class SimpleDevice : public OdGsBaseVectorizeDevice
{
	ConveyorGeometryDumperPtr m_pDestGeometry;
public:
	enum DeviceType
	{
		k2dDevice,
		k3dDevice
	};

	SimpleDevice();

	static OdGsDevicePtr createObject(DeviceType type, GeometryCollector* gcollector);

	void update(OdGsDCRect* pUpdatedRect);

	OdGsViewPtr createView(const OdGsClientViewInfo* pInfo = 0,
		bool bEnableLayerVisibilityPerView = false);

	OdGiConveyorGeometry* destGeometry();

	void setupSimplifier(const OdGiDeviation* pDeviation);

private:
	GeometryCollector* gcollector;
	DeviceType m_type;
};

typedef OdSmartPtr<SimpleDevice> SimpleDevicePtr;

class SimpleView : public OdGsBaseVectorizeViewDef
{
	OdGiClipBoundary m_eyeClip;
protected:

	virtual void beginViewVectorization();

	SimpleDevice* device()
	{
		return (SimpleDevice*)OdGsBaseVectorizeView::device();
	}

	SimpleView()
	{
		m_eyeClip.m_bDrawBoundary = false;
	}

	friend class SimpleDevice;

	static OdGsViewPtr createObject()
	{
		return OdRxObjectImpl<SimpleView, OdGsView>::createObject();
	}

	void setEntityTraits();

public:
	void draw(const OdGiDrawable*);

	bool regenAbort() const;

	double deviation(const OdGiDeviationType, const OdGePoint3d&) const
	{
		return 0.000001;
	}

	void updateViewport();

	TD_USING(OdGsBaseVectorizeViewDef::updateViewport);

	void onTraitsModified();

	void ownerDrawDc(
		const OdGePoint3d& origin,
		const OdGeVector3d& u,
		const OdGeVector3d& v,
		const OdGiSelfGdiDrawable* pDrawable,
		bool bDcAligned,
		bool bAllowClipping);
};

