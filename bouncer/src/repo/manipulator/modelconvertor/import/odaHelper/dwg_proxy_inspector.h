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

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <SharedPtr.h>
#include <DbProxyEntity.h>
#include <DbEntityWithGrData.h>
#include <DbDictionary.h>
#include <DbXrecord.h>
#include <Gi/GiWorldDraw.h>

#include "proxy_app_handler.h"
#include "civil3d_proxy_handler.h"
#include "plant3d_proxy_handler.h"
#include "repo/lib/datastructure/repo_variant.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
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

					/* Lazily populated; see DwgProxyInspector's internal ensureProxyXData()
					and ensureProxyExtensionDictionary(). */
					OdResBufPtr xData;
					bool xDataLoaded = false;
					OdDbObjectId extensionDictionaryId;
					OdDbDictionaryPtr extensionDictionary;
					bool extensionDictionaryLoaded = false;

					// Set together with appType by DwgProxyInspector::classifyApplication(); never diverges from it.
					ProxyAppHandler* matchedHandler = nullptr;

					bool isProxy() const { return !entity.isNull(); }
					bool isCivil3D() const { return appType == ProxyAppType::Civil3D; }
					bool isPlant3D() const { return appType == ProxyAppType::Plant3D; }
					bool hasFullGraphics() const { return hasFullGraphicsFlag; }
				};

				/* Everything needed to inspect, classify, replay and report on proxy
				entities (Civil3D/Plant3D/custom object-enabler classes ODA cannot
				natively load) for one DWG file.

				One DwgProxyInspector is constructed per file - see
				FileProcessorDwg::importModel(), which owns it alongside the
				GeometryCollector - and shared by pointer with every DataProcessorDwg
				view instance ODA creates while vectorising that file (a file may be
				vectorised more than once; see the comment on DataProcessor). Sharing
				one DwgProxyInspector across every pass is what lets it commit to a
				single detected authoring app for the life of the file: a DWG is
				produced by one app, never both, so once any proxy entity is classified
				as Civil3D or Plant3D, every subsequent entity - across every
				vectorisation pass - is only ever tested against that same app; see
				classifyApplication(). */
				class DwgProxyInspector
				{
				public:
					bool getProxyInfo(OdDbEntityPtr entity, ProxyInfo& info);

					/* Is this proxy's class a "special" geometry case that needs more
					than plain stored-graphics replay (e.g. Civil3D TIN surfaces)?
					Delegates to the matched handler - DwgProxyInspector has no
					compiled-in knowledge of which app, if any, has such a case. */
					bool isSpecialGeometryClass(const ProxyInfo& info) const;

					bool drawStoredProxyGraphics(OdDbEntityPtr pEntity, const ProxyInfo& info, OdGiWorldDraw* worldDraw);

					std::unordered_map<std::string, repo::lib::RepoVariant> getProxyEntityMetadata(OdDbEntityPtr pEntity, ProxyInfo& info);

					/* Sets metadata[key] = value only if key is not already present.
					Shared by any proxy metadata source (this class, or app handlers
					such as Civil3DProxyHandler::TinCapture) that need to contribute
					a computed property without overwriting one already set by an
					earlier, higher-priority source. */
					static void setMetadataIfMissing(
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata,
						const std::string& key,
						const repo::lib::RepoVariant& value);

					/* Routes the hot OdGi geometry callbacks (processTriangleOut,
					polygonOut, tristripProc, processPolylineOut) on DataProcessorDwg.
					Non-null only for the duration of one entity's stored-graphics
					replay when that entity needs geometry capture (see
					isSpecialGeometryClass()); DataProcessorDwg saves/restores the
					previous value around each replay so nested draws can't leak into
					the wrong capture. */
					ProxyGeometryCapture* activeCapture() const { return activeGeometryCapture; }
					void setActiveCapture(ProxyGeometryCapture* capture) { activeGeometryCapture = capture; }

				private:
					ProxyAppType classifyApplication(const std::string& originalClass, ProxyAppHandler*& outHandler);
					static std::string formatApplicationDisplayString(const ProxyInfo& info);

					void ensureProxyXData(ProxyInfo& info);
					void ensureProxyExtensionDictionary(ProxyInfo& info);

					void addProxyBasicMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyGeneralMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyGeometryMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyXDataMetadata(const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyDictionaryMetadata(const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

					void extractXDataProperties(OdResBufPtr pRb, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void extractTextPropertiesFromProxy(OdDbProxyEntityPtr proxyEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

					/* The detected app's handler for this file, constructed lazily by
					classifyApplication() the first time any proxy entity matches
					Civil3DProxyHandler::matchesClassName() or
					Plant3DProxyHandler::matchesClassName() - both are static, so they
					can be probed without constructing either instance. A DWG is
					authored by one app, never both, so at most one of
					Civil3DProxyHandler/Plant3DProxyHandler is ever constructed for the
					life of the file, and once set, entities that would otherwise match
					the *other* app are classified Custom rather than misattributed. */
					std::unique_ptr<ProxyAppHandler> activeHandler;

					ProxyGeometryCapture* activeGeometryCapture = nullptr;
				};
			}
		}
	}
}
