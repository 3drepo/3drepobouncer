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
#include <array>
#include <DbProxyEntity.h>
#include <DbEntityWithGrData.h>
#include <DbRegAppTable.h>
#include <DbRegAppTableRecord.h>
#include <DbDictionary.h>
#include <DbXrecord.h>

#include "vectorise_device_dgn.h"
#include "repo/core/model/bson/repo_node_mesh.h"
#include "repo/lib/datastructure/repo_variant.h"
#include "proxy_app_handler.h"
#include "civil3d_proxy_handler.h"
#include "plant3d_proxy_handler.h"

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
					/* ===== Proxy entity inspection =====

					A proxy entity is inspected once via getProxyInfo(), which performs the
					single OdDbProxyEntity::cast() for that entity. The resulting ProxyInfo is
					reused for classification, TIN detection, stored graphics replay, metadata
					and diagnostics, instead of every consumer re-casting ODA.

					The XData chain and the extension dictionary are deliberately not read up
					front: opening the extension dictionary is a database object open, and most
					proxies never need it - they produce no geometry, or their layer already
					carries metadata. Those reads are loaded on demand by ensureProxyXData() and
					ensureProxyExtensionDictionary(), which are no-ops once cached. */

					struct ProxyReadOptions
					{
						/* Defaults are the cheap draw-time profile; metadata and
						diagnostic paths opt in to what they actually need. */
						bool readXData = false;
						bool readExtensionDictionary = false;
						/* Falls back to scanning the whole registered application
						table when xData() yields nothing. Expensive, so off
						outside diagnostics. */
						bool scanRegisteredAppsFallback = false;
					};

					struct ProxyInfo
					{
						OdDbProxyEntityPtr entity; // null => not a proxy
						std::string originalClass;
						std::string originalDxfName;
						std::string applicationDescription;
						std::vector<std::string> xDataApps;
						ProxyAppType appType = ProxyAppType::Unknown;
						OdDbProxyEntity::GraphicsMetafileType graphicsType = OdDbProxyEntity::kNoMetafile;
						bool hasFullGraphicsFlag = false;
						OdDbEntityWithGrDataPEPtr graphicsPE;

						/* Lazily populated; see ensureProxyXData() and
						ensureProxyExtensionDictionary(). */
						OdResBufPtr xData;
						bool xDataLoaded = false;
						OdDbObjectId extensionDictionaryId;
						OdDbDictionaryPtr extensionDictionary;
						bool extensionDictionaryLoaded = false;

						// Set together with appType by classifyApplication(); never diverges from it.
						ProxyAppHandler* matchedHandler = nullptr;

						bool isProxy() const { return !entity.isNull(); }
						bool isCivil3D() const { return appType == ProxyAppType::Civil3D; }
						bool isPlant3D() const { return appType == ProxyAppType::Plant3D; }
						bool hasFullGraphics() const { return hasFullGraphicsFlag; }
					};

					bool getProxyInfo(OdDbEntityPtr entity, ProxyInfo& info, const ProxyReadOptions& options = ProxyReadOptions());

					/* On demand loaders for the two expensive proxy reads. Both return
					immediately once the corresponding data has been cached in info. */
					void ensureProxyXData(ProxyInfo& info, const ProxyReadOptions& options = ProxyReadOptions());
					void ensureProxyExtensionDictionary(ProxyInfo& info);

					ProxyAppType classifyApplication(const std::string& originalClass, ProxyAppHandler*& outHandler);
					static std::string formatApplicationDisplayString(const ProxyInfo& info);

					/* The class-name check formerly on ProxyInfo::isCivil3DTinSurface(),
					moved here since a plain data struct can't reach civil3DHandler. */
					bool isCivil3DTinSurface(const ProxyInfo& info) const
					{
						return info.isCivil3D() && civil3DHandler.isTinSurfaceClass(info.originalClass);
					}

					bool drawStoredProxyGraphics(OdDbEntityPtr pEntity, const ProxyInfo& info);
					void logProxyWithoutRenderableGeometry(
						OdDbEntityPtr pEntity,
						ProxyInfo& info,
						bool replayedStoredProxyGraphics,
						bool replayReturnedGeometry);

					std::unordered_map<std::string, repo::lib::RepoVariant> getProxyEntityMetadata(OdDbEntityPtr pEntity, ProxyInfo& info);
					void addProxyBasicMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyGeneralMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyGeometryMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyXDataMetadata(const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyDictionaryMetadata(const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

					void extractXDataProperties(OdResBufPtr pRb, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void extractTextPropertiesFromProxy(OdDbProxyEntityPtr proxyEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void extractEntityProperties(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void removeDuplicateGeneralMetadata(
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
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

					/* Registered proxy-app handlers, tried in order for each proxy entity.
					Both are always constructed (never conditionally) so a drawing containing
					both Civil3D and Plant3D proxies classifies each entity independently.
					Declared before proxyHandlers so taking their addresses below is
					self-evidently safe regardless of member-initializer-list order. */
					Civil3DProxyHandler civil3DHandler;
					Plant3DProxyHandler plant3DHandler;
					std::array<ProxyAppHandler*, 2> proxyHandlers = { &civil3DHandler, &plant3DHandler };

					// Set for the duration of one entity's stored-graphics replay; the
					// geometry callbacks route into it when non-null. Replaces the old
					// TinSurfaceCapture tinCapture member/isActive() toggle.
					ProxyGeometryCapture* activeGeometryCapture = nullptr;

					/* Proxies that produced no renderable geometry are reported once each,
					but only for the first kMaxLoggedProxyGeometryFailures of them; the rest
					are counted and summarised, so a proxy heavy drawing cannot flood the log. */
					static const size_t kMaxLoggedProxyGeometryFailures = 20;
					std::unordered_set<std::string> loggedProxyGeometryFailures;
					size_t suppressedProxyGeometryFailures = 0;

					std::string getClassDisplayName(OdDbEntityPtr entity, const ProxyInfo& info);
				};

				typedef OdSharedPtr<DataProcessorDwg> DataProcessorDwgPtr;
			}
		}
	}
}
