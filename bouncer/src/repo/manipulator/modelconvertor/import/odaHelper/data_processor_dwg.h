/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include "data_processor.h"
#include "geometry_collector.h"

#include <SharedPtr.h>
#include <Gi/GiGeometrySimplifier.h>
#include <Gs/GsBaseMaterialView.h>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <DbProxyEntity.h>
#include <DbRegAppTable.h>
#include <DbRegAppTableRecord.h>
#include <DbDictionary.h>
#include <DbXrecord.h>

#include "vectorise_device_dgn.h"
#include "repo/core/model/bson/repo_node_mesh.h"
#include "repo/lib/datastructure/repo_variant.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class DataProcessorDwg : public DataProcessor
				{
				public:
					bool doDraw(OdUInt32 i,	const OdGiDrawable* pDrawable) override;
					void setMode(OdGsView::RenderMode mode);
					~DataProcessorDwg();
					// ===== ADD THESE LINES (START) =====
					struct DiagnosticStats {
						size_t totalEntities = 0;
						size_t civil3dEntities = 0;
						size_t plant3dEntities = 0;
						size_t proxyEntities = 0;
						size_t entitiesWithGeometry = 0;
						size_t entitiesWithoutGeometry = 0;
						std::unordered_map<std::string, size_t> entityTypeCount;
					};

					DiagnosticStats getStats() const { return stats; }
					void printDiagnostics() const;
					// ===== ADD THESE LINES (END) =====

				protected:

					void convertTo3DRepoMaterial(
						OdGiMaterialItemPtr prevCache,
						OdDbStub* materialId,
						const OdGiMaterialTraitsData & materialData,
						MaterialColours& matColors,
						repo::lib::repo_material_t& material) override;

					void processTriangleOut(
						const OdInt32* p3Vertices,
						const OdGeVector3d* pNormal) override;

					void processPolylineOut(
						OdInt32 numPoints,
						const OdInt32* vertexIndexList) override;

					void processPolylineOut(
						OdInt32 numPoints,
						const OdGePoint3d* vertexList) override;

					void polygonOut(
						OdInt32 numPoints,
						const OdGePoint3d* vertexList,
						const OdGeVector3d* pNormal = 0) override;

					void shellProc(
						OdInt32 numVertices,
						const OdGePoint3d* vertexList,
						OdInt32 faceListSize,
						const OdInt32* faceList,
						const OdGiEdgeData* pEdgeData = 0,
						const OdGiFaceData* pFaceData = 0,
						const OdGiVertexData* pVertexData = 0) override;

					void meshProc(
						OdInt32 numRows,
						OdInt32 numColumns,
						const OdGePoint3d* vertexList,
						const OdGiEdgeData* pEdgeData = 0,
						const OdGiFaceData* pFaceData = 0,
						const OdGiVertexData* pVertexData = 0) override;

					void tristripProc(
						OdInt32 numVertices,
						const OdGePoint3d* vertexList,
						OdInt32 stripListSize = 0,
						const OdInt32* stripList = 0,
						const OdGiEdgeData* pEdgeData = 0,
						const OdGiVertexData* pVertexData = 0) override;

				private:
					// ===== ADDED: Helper methods for Civil3D/Plant3D detection =====
					bool isProxyEntity(OdDbEntityPtr pEntity);
					bool isTinSurfaceProxy(OdDbEntityPtr pEntity);
					bool shouldReplayStoredProxyGraphics(OdDbEntityPtr pEntity);
					std::string getProxyOriginalClassName(OdDbEntityPtr pEntity);
					std::vector<std::string> getProxyXDataApps(OdDbEntityPtr pEntity);
					std::string detectApplicationType(OdDbEntityPtr pEntity);
					bool drawStoredProxyGraphics(OdDbEntityPtr pEntity);
					std::unordered_map<std::string, repo::lib::RepoVariant> getProxyEntityMetadata(OdDbEntityPtr pEntity);
					void logProxyWithoutRenderableGeometry(
						OdDbEntityPtr pEntity,
						bool replayedStoredProxyGraphics,
						bool replayReturnedGeometry);
					bool addTinSurfaceTriangle(
						const repo::lib::RepoVector3D64& p0,
						const repo::lib::RepoVector3D64& p1,
						const repo::lib::RepoVector3D64& p2);
					bool addTinSurfaceEdge(
						const repo::lib::RepoVector3D64& p0,
						const repo::lib::RepoVector3D64& p1);
					bool addTinSurfaceTrianglePolyline(const std::vector<repo::lib::RepoVector3D64>& points);
					std::unordered_map<std::string, std::string> getProxyMetadata(OdDbEntityPtr pEntity);
					// ===== END: Helper methods for Civil3D/Plant3D detection =====

					void extractCivil3DStoredProperties(OdDbDictionaryPtr pDict, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void extractPlant3DStoredProperties(OdDbDictionaryPtr pDict, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void extractXDataProperties(OdResBufPtr pRb, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void extractTextPropertiesFromProxy(OdDbProxyEntityPtr proxyEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void extractEntityProperties(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addTinSurfaceComputedMetadata(
						OdDbEntityPtr pEntity,
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void removeDuplicateGeneralMetadata(
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void setEntityMetadata(
						const std::string& layerId,
						const std::string& handleMetaValue,
						OdDbEntityPtr pEntity);

					void convertTo3DRepoColor(OdCmEntityColor& color, repo::lib::repo_color3d_t& out);

					class Layer
					{
					public:
						std::string id;
						std::string name;

						Layer(std::string id, std::string name):
							id(id),
							name(name)
						{
						}

						Layer() 
						{
						}

						operator bool() const {
							return !id.empty() && !name.empty();
						}
					};

					void addTinSurfaceFaceLayers(
						const std::string& parentLayerId,
						const std::string& sourceEntityId);

					// Some properties to be held between invocations of doDraw()
					class Context
					{
					public:
						bool inBlock = false;
						Layer currentBlockReferenceLayer;
						Layer currentBlockReference;
						OdDbObjectId layoutId;
					};

					Context context;
					mutable DiagnosticStats stats;
					bool capturingTinSurfaceProxy = false;
					std::unordered_set<std::string> tinSurfaceEdgeKeys;
					std::vector<GeometryCollector::Face> tinSurfaceTriangles;
					std::unordered_set<std::string> loggedProxyGeometryFailures;
					bool hasTinSurfaceFaceMaterial = false;
					repo::lib::repo_material_t tinSurfaceFaceMaterial;

					std::string getClassDisplayName(OdDbEntityPtr entity);
				};

				typedef OdSharedPtr<DataProcessorDwg> DataProcessorDwgPtr;
			}
		}
	}
}
