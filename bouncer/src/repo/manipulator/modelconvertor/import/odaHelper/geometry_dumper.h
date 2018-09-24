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

#include "oda_geometry_collector.h"

#include <SharedPtr.h>
#include <Gi/GiGeometrySimplifier.h>
#include <Gs/GsBaseMaterialView.h>
#include <string>
#include <fstream>

#include "oda_vectorise_device.h"
#include "../../../../core/model/bson/repo_node_mesh.h"

#include <vector>
namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class GeometryDumper : public OdGiGeometrySimplifier, public OdGsBaseMaterialView
				{
					OdaGeometryCollector *collector;
					//FIXME: Should this live in collector?
					std::vector<repo::lib::RepoVector3D64> vertices;
					std::vector<repo_face_t> faces;
					std::vector<std::vector<double>> boundingBox;
					std::unordered_map<std::string, unsigned int> vToVIndex;

				public:
					GeometryDumper() {}
					
					virtual double deviation(
						const OdGiDeviationType deviationType, 
						const OdGePoint3d& pointOnCurve) const;

					OdaVectoriseDevice* device();

					bool doDraw(
						OdUInt32 i, 
						const OdGiDrawable* pDrawable);

					void init(
						OdaGeometryCollector *const geoCollector);

					void beginViewVectorization();

					void endViewVectorization();

					void setMode(OdGsView::RenderMode mode);
				
				protected:

					OdGiMaterialItemPtr fillMaterialCache(
						OdGiMaterialItemPtr prevCache, 
						OdDbStub* materialId, 
						const OdGiMaterialTraitsData & materialData);

					void triangleOut(
						const OdInt32* p3Vertices, 
						const OdGeVector3d* pNormal);

				private:
					OdCmEntityColor fixByACI(const ODCOLORREF *ids, const OdCmEntityColor &color);

				};
				typedef OdSharedPtr<GeometryDumper> OdGiConveyorGeometryDumperPtr;
			}
		}
	}
}