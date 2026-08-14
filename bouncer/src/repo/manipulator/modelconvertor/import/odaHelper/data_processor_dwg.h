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

#include "vectorise_device_dgn.h"
#include "repo/core/model/bson/repo_node_mesh.h"
#include "repo/lib/datastructure/repo_variant.h"
#include "dwg_proxy_inspector.h"

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

					// The single DwgProxyInspector instance for the file being imported,
					// shared by every DataProcessorDwg view instance ODA creates while
					// vectorising it. Owned by FileProcessorDwg::importModel(), alongside
					// the GeometryCollector; see dwg_proxy_inspector.h for why it needs
					// to be file-scoped rather than per-instance.
					void setProxy(DwgProxyInspector* proxy) { this->proxy = proxy; }

					struct DiagnosticStats {
						size_t totalEntities = 0;
						size_t entitiesWithGeometry = 0;
						size_t entitiesWithoutGeometry = 0;
						std::unordered_map<std::string, size_t> entityTypeCount;
					};

					DiagnosticStats getStats() const { return stats; }
					void printDiagnostics() const;

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
					void setEntityMetadata(
						const std::string& layerId,
						const std::string& handleMetaValue,
						OdDbEntityPtr pEntity,
						ProxyInfo& info,
						ProxyGeometryCapture* capturedGeometry);

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

					// Not owned; see setProxy().
					DwgProxyInspector* proxy = nullptr;

					std::string getClassDisplayName(OdDbEntityPtr entity, const ProxyInfo& info);
				};

				typedef OdSharedPtr<DataProcessorDwg> DataProcessorDwgPtr;
			}
		}
	}
}
