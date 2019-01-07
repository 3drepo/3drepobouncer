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
#include <Gs/GsBaseMaterialView.h>
#include "geometry_dumper_rvt.h"
#include <iostream>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class VectoriseDeviceRvt : public OdGsBaseVectorizeDevice
				{
				public:
					VectoriseDeviceRvt();

					void init(GeometryCollector *const geoCollector);

					OdGsViewPtr createView(
						const OdGsClientViewInfo* pInfo = 0,
						bool bEnableLayerVisibilityPerView = false);

					OdGiConveyorGeometry* destGeometry();

					void setupSimplifier(const OdGiDeviation* pDeviation);

				private:
					GeometryRvtDumperPtr destGeometryDumper;
					GeometryCollector* geoColl;
				};

				class VectorizeView : public OdGsBaseMaterialView
				{
				public:
					VectorizeView();

					static OdGsViewPtr createObject(GeometryCollector* geoColl);

					virtual void beginViewVectorization();

					VectoriseDeviceRvt* device();

					void draw(const OdGiDrawable*);

					void updateViewport();

					TD_USING(OdGsBaseMaterialView::updateViewport);

					OdGiMaterialItemPtr fillMaterialCache(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData);

				private:
					GeometryCollector* geoColl;
					uint64_t meshesCount;
				};
			}
		}
	}
}

