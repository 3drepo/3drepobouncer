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
#include "geometry_dumper_rvt.h"

#include <iostream>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class VectorizeDeviceRvt : public OdGsBaseVectorizeDevice
				{
				public:
					VectorizeDeviceRvt();

					void init(GeometryCollector *const geoCollector);

					void setupSimplifier(const OdGiDeviation* pDeviation);

					OdGsViewPtr createView(
						const OdGsClientViewInfo* pInfo = 0,
						bool bEnableLayerVisibilityPerView = false);

					OdGiConveyorGeometry* destGeometry();

				private:
					OdGiConveyorGeometryRvtDumperPtr destGeometryDumper;
				};

				class VectorizeView : public OdGsBaseVectorizeViewDef
				{
				public:
					VectorizeView();

					static OdGsViewPtr createObject();

					virtual void beginViewVectorization();

					VectorizeDeviceRvt* device();

					void setEntityTraits();

					void draw(const OdGiDrawable*);

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

				private:
					OdGiClipBoundary eyeClip;

				};
			}
		}
	}
}

