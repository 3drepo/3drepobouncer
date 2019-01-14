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

#include "boost/filesystem.hpp"
#include "OdPlatformSettings.h"
#include <BimCommon.h>
#include <Database/BmElement.h>
#include <RxObjectImpl.h>
#include <ColorMapping.h>
#include <toString.h>
#include "Database/Entities/BmMaterialElem.h"
#include "Geometry/Entities/BmMaterial.h"
#include "Database/BmAssetHelpers.h"

#include "vectorise_view_rvt.h"
#include "vectorise_device_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

VectoriseDeviceRvt::VectoriseDeviceRvt()
{
	setLogicalPalette(odcmAcadLightPalette(), 256);
	onSize(OdGsDCRect(0, 100, 0, 100));
}

void VectoriseDeviceRvt::init(GeometryCollector *const geoCollector)
{
	geoColl = geoCollector;
}

OdGsViewPtr VectoriseDeviceRvt::createView(
	const OdGsClientViewInfo* pInfo,
	bool bEnableLayerVisibilityPerView)
{
	OdGsViewPtr pView = VectorizeView::createObject(geoColl);
	VectorizeView* pMyView = static_cast<VectorizeView*>(pView.get());
	pMyView->init(this, pInfo, bEnableLayerVisibilityPerView);
	pMyView->output().setDestGeometry(*pMyView);
	return (OdGsView*)pMyView;
}