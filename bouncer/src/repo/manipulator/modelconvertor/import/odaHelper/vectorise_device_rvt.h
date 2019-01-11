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

#include "SharedPtr.h"
#include "Gi/GiGeometrySimplifier.h"
#include "Gs/GsBaseInclude.h"
#include <Gs/GsBaseMaterialView.h>

#include "geometry_collector.h"

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

				private:
					GeometryCollector* geoColl;
				};

				class VectorizeView : public OdGsBaseMaterialView, public OdGiGeometrySimplifier
				{
				public:
					VectorizeView();

					static OdGsViewPtr createObject(GeometryCollector* geoColl);

					virtual void beginViewVectorization();

					VectoriseDeviceRvt* device();

					void draw(const OdGiDrawable*);

				protected:

					OdGiMaterialItemPtr fillMaterialCache(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData);

					void triangleOut(const OdInt32* vertices,
						const OdGeVector3d* pNormal);

				private:
					void fillTexture(OdDbStub* materialId, repo_material_t& material);
					void fillMaterial(const OdGiMaterialTraitsData & materialData, repo_material_t& material);

					GeometryCollector* geoColl;
					uint64_t meshesCount;
				};
			}
		}
	}
}

