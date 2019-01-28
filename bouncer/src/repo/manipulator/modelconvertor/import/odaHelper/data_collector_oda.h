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

#include "geometry_collector.h"

#include <SharedPtr.h>
#include <Gi/GiGeometrySimplifier.h>
#include <Gs/GsBaseMaterialView.h>
#include <string>
#include <fstream>

#include "vectorise_device_dgn.h"
#include "../../../../core/model/bson/repo_node_mesh.h"

#include <vector>
#include "material_colors.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {

				class DataCollectorOda : public OdGiGeometrySimplifier, public OdGsBaseMaterialView
				{
				protected:
					GeometryCollector *collector;

				public:
					DataCollectorOda() {}

					virtual double deviation(
						const OdGiDeviationType deviationType,
						const OdGePoint3d& pointOnCurve) const;
					
					void beginViewVectorization();

				protected:	
					virtual void OnTriangleOut(const std::vector<repo::lib::RepoVector3D64>& vertices) = 0;

					virtual void OnFillMaterialCache(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData,
						const MaterialColors& matColors,
						repo_material_t& material) = 0;

				private:
					void triangleOut(
						const OdInt32* p3Vertices,
						const OdGeVector3d* pNormal) final;

					OdGiMaterialItemPtr fillMaterialCache(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData) final;
				};
			}
		}
	}
}