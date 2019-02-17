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

				class DataProcessor : public OdGiGeometrySimplifier, public OdGsBaseMaterialView
				{
				protected:
					GeometryCollector *collector;

				public:
					DataProcessor() {}

					virtual double deviation(
						const OdGiDeviationType deviationType,
						const OdGePoint3d& pointOnCurve) const;
					
					void beginViewVectorization();

				protected:
					/**
					* Should be overriden in derived classes to process materials
					* @param prevCache - previous material cache
					* @param materialId - database id of the material
					* @param materialData - structure with material data
					* @param matColors - reference that receives material color
					* @param material - 3D Repo material structure received from materialData
					*/
					virtual void convertTo3DRepoMaterial(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData,
						MaterialColors& matColors,
						repo_material_t& material,
						bool& missingTexture);

					/**
					* Should be overriden in derived classes to process triangles
					* @param p3Vertices - input vertices of the triangle
					* @param verticesOut - output vertices in 3D Repo format
					* @param uvOut - output texture coordinates
					*/
					virtual void convertTo3DRepoVertices(
						const OdInt32* p3Vertices, 
						std::vector<repo::lib::RepoVector3D64>& verticesOut,
						std::vector<repo::lib::RepoVector2D>& uvOut);

				private:
					/**
					* This callback is invoked when next triangle should be processed
					* defined in OdGiGeometrySimplifier class
					* @param p3Vertices - input vertices of the triangle
					* @param pNormal - input veritces normal
					*/
					void triangleOut(
						const OdInt32* p3Vertices,
						const OdGeVector3d* pNormal) final;

					/**
					* This callback is invoked when next material should be processed
					* @param prevCache - previous material cache
					* @param materialId - database id of the material
					* @param materialData - structure with material data
					*/
					OdGiMaterialItemPtr fillMaterialCache(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData) final;
				};
			}
		}
	}
}