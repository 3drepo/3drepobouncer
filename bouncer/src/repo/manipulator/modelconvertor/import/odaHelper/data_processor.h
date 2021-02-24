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


#include <vector>
#include <Gi/GiGeometrySimplifier.h>
#include <Gs/GsBaseMaterialView.h>
#include <string>
#include <fstream>

#include "geometry_collector.h"
#include "vectorise_device_dgn.h"
#include "../../../../core/model/bson/repo_node_mesh.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {

				class DataProcessor : public OdGiGeometrySimplifier, public OdGsBaseMaterialView
				{
				protected:
					GeometryCollector *collector;
					struct MaterialColours
					{
						ODCOLORREF colorDiffuse = 0;
						ODCOLORREF colorAmbient = 0;
						ODCOLORREF colorSpecular = 0;
						ODCOLORREF colorEmissive = 0;

						bool colorDiffuseOverride = false;
						bool colorAmbientOverride = false;
						bool colorSpecularOverride = false;
						bool colorEmissiveOverride = false;
					};

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
						MaterialColours& matColors,
						repo_material_t& material,
						bool& missingTexture);

					/**
					* Should be overriden in derived classes to process triangles
					* @param p3Vertices - input vertices of the triangle
					* @param verticesOut - output vertices in 3D Repo format
					* @param uvOut - output texture coordinates
					*/
					virtual void convertTo3DRepoTriangle(
						const OdInt32* p3Vertices,
						std::vector<repo::lib::RepoVector3D64>& verticesOut,
						repo::lib::RepoVector3D64& normalOut,
						std::vector<repo::lib::RepoVector2D>& uvOut);

					repo_material_t GetDefaultMaterial() const;

					/**
					* Given vertice location, obtain vertices in teigha type and 3drepo type.
					* @param p3Vertices  - input vertice locations
					* @param odaPoint - [OUTPUT] vector to store results in teigha type
					* @param repoPoint - [OUTPUT] vector to store repoPoint
					*/
					void getVertices(
						int numVertices,
						const OdInt32* p3Vertices,
						std::vector<OdGePoint3d> &odaPoint,
						std::vector<repo::lib::RepoVector3D64> &repoPoint
					);

					/**
					* Function to convert a teigha point to 3D Repo point
					* By default, this just returns the same point converted into 3drepo type.
					* Should the format require, this needs to be overwritten to translate the point back to project point
					* @param pnt point in teigha format
					* @return returns converted, transformed point in RepoVector3D64
					*/
					std::function<repo::lib::RepoVector3D64(OdGePoint3d)> convertTo3DRepoWorldCoorindates = [](OdGePoint3d pnt) { return repo::lib::RepoVector3D64(pnt.x, pnt.y, pnt.z); };


					double deviationValue = 0;
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
					* This callback is invoked when the next line should be processed.
					* Defined in OdGiGeometrySimplifier
					* @param numPoints Number of points.
					* @param vertexIndexList Pointer to an array of vertex indices.
					*/
					void polylineOut(
						OdInt32 numPoints,
						const OdInt32* vertexIndexList) final;

					void polylineOut(OdInt32 numPoints, 
						const OdGePoint3d* vertexList) final;

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
