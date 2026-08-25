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

					/* Civil3D proxy classification. This is the only place in the DWG
					importer that knows anything about Civil3D; removing Civil3D support
					means deleting these three methods (and their call sites in doDraw/
					getClassDisplayName/setEntityMetadata below). Static (no instance
					state), so they're directly testable without constructing a
					DataProcessorDwg. */
					static bool isCivil3DProxyClass(const std::string& originalClass);

					// TIN = Triangulated Irregular Network - Civil3D's surface-mesh
					// representation; see AeccDbSurfaceTin/AeccDbTinSurface.
					static bool isCivil3DSurfaceClass(const std::string& originalClass);

					static bool getCivil3DDisplayName(const std::string& originalClass, std::string& outName);

					// The single DwgProxyInspector instance for the file being imported,
					// shared by every DataProcessorDwg view instance ODA creates while
					// vectorising it. Owned by FileProcessorDwg::importModel(), alongside
					// the GeometryCollector; see dwg_proxy_inspector.h for why it needs
					// to be file-scoped rather than per-instance.
					void setProxy(DwgProxyInspector* proxy) { this->proxy = proxy; }

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
						ProxyInfo& info);

					void convertTo3DRepoColor(OdCmEntityColor& color, repo::lib::repo_color3d_t& out);

					/* Derives and draws the 3 (deduped) edges of one triangle for the
					current surface-type entity, as a wireframe overlay. Returns false,
					doing nothing, for a degenerate triangle (two coincident points).
					Only called when drawingSurfaceEdges is true - see doDraw. */
					bool addSurfaceTriangle(
						const repo::lib::RepoVector3D64& p0,
						const repo::lib::RepoVector3D64& p1,
						const repo::lib::RepoVector3D64& p2);

					void addSurfaceEdgeIfNeeded(
						const repo::lib::RepoVector3D64& p0,
						const repo::lib::RepoVector3D64& p1);

					// Civil3D extension-dictionary metadata contribution - see
					// setEntityMetadata, called right after proxy->getProxyEntityMetadata()
					// returns (info.extensionDictionary is already populated by then).
					static void addCivil3DDictionaryMetadata(
						const ProxyInfo& info,
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

					// "Civil3D (<class>)" / "CustomApp (<class>)" / XData-based fallback,
					// for the "Proxy::Application" metadata field.
					static std::string formatProxyApplicationString(const ProxyInfo& info);

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

					// Runs the actual draw call for one entity/drawable within doDraw(),
					// with Civil3D proxy/TIN-surface awareness: dispatches to stored
					// proxy graphics when isProxy is set, and scopes the surface-edge
					// overlay state (see addSurfaceTriangle) to this one draw. Extracted
					// from doDraw() because this whole chunk - proxy dispatch and
					// surface-edge state - has no equivalent for non-proxy entities.
					bool drawProxyAwareGeometry(
						OdUInt32 i,
						const OdGiDrawable* pDrawable,
						OdDbEntityPtr pEntity,
						bool isProxy,
						const ProxyInfo& info,
						GeometryCollector::Context* ctx);

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

					// Not owned; see setProxy().
					DwgProxyInspector* proxy = nullptr;

					/* State for the current entity's surface-edge rendering (see
					addSurfaceTriangle/addSurfaceEdgeIfNeeded). Both are fully saved
					and restored around each doDraw() call - not just reset on
					transition - so that sibling or nested surface-type entities never
					leak state into one another; a file with several TIN surfaces must
					produce one distinct mesh (plus one distinct edge mesh) per surface
					entity. */
					bool drawingSurfaceEdges = false;
					std::unordered_set<std::string> currentSurfaceEdgeKeys;

					std::string getClassDisplayName(OdDbEntityPtr entity, const ProxyInfo& info);
				};

				typedef OdSharedPtr<DataProcessorDwg> DataProcessorDwgPtr;
			}
		}
	}
}
