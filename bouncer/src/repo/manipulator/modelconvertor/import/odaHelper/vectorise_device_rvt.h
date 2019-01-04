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
#include "material_collector_rvt.h"
#include <iostream>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class VectoriseDeviceRvt : public OdGsBaseVectorizeDevice
				{
				public:
					VectoriseDeviceRvt();

					void init(GeometryCollector *const geoCollector, MaterialCollectorRvt& matCollector);

					OdGsViewPtr createView(
						const OdGsClientViewInfo* pInfo = 0,
						bool bEnableLayerVisibilityPerView = false);

					OdGiConveyorGeometry* destGeometry();

				private:
					GeometryRvtDumperPtr destGeometryDumper;

					GeometryCollector* geoColl;
					MaterialCollectorRvt* matColl;
				};

				class VectorizeView : public OdGsBaseVectorizeViewDef
				{
				public:
					VectorizeView();

					static OdGsViewPtr createObject(GeometryCollector* geoColl,	MaterialCollectorRvt* matColl);

					virtual void beginViewVectorization();

					VectoriseDeviceRvt* device();

					void draw(const OdGiDrawable*);

					void updateViewport();

					TD_USING(OdGsBaseVectorizeViewDef::updateViewport);

					void onTraitsModified();

				private:
					GeometryCollector* geoColl;
					MaterialCollectorRvt* matColl;
					uint64_t meshesCount;
				};
			}
		}
	}
}

