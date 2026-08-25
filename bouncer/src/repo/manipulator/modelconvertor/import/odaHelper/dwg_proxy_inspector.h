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
#include <unordered_map>

#include <SharedPtr.h>
#include <DbProxyEntity.h>
#include <DbEntityWithGrData.h>
#include <DbDictionary.h>
#include <DbXrecord.h>
#include <Gi/GiWorldDraw.h>

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

					bool isProxy() const { return !entity.isNull(); }
					bool hasFullGraphics() const { return hasFullGraphicsFlag; }
				};

				/* Generic (app-agnostic) inspection, stored-graphics replay, and
				metadata assembly for DWG proxy entities (custom object-enabler
				classes ODA cannot natively load) for one DWG file. Has no compiled-in
				knowledge of any specific authoring app - app-specific classification,
				display names, and dictionary metadata (e.g. Civil3D's) are the
				concern of whatever consumes ProxyInfo, such as DataProcessorDwg.

				One DwgProxyInspector is constructed per file - see
				FileProcessorDwg::importModel(), which owns it alongside the
				GeometryCollector - and shared by pointer with every DataProcessorDwg
				view instance ODA creates while vectorising that file (a file may be
				vectorised more than once; see the comment on DataProcessor). */
				class DwgProxyInspector
				{
				public:
					bool getProxyInfo(OdDbEntityPtr entity, ProxyInfo& info);

					bool drawStoredProxyGraphics(OdDbEntityPtr pEntity, const ProxyInfo& info, OdGiWorldDraw* worldDraw);

					std::unordered_map<std::string, repo::lib::RepoVariant> getProxyEntityMetadata(OdDbEntityPtr pEntity, ProxyInfo& info);

					/* Sets metadata[key] = value only if key is not already present.
					Shared by any proxy metadata source that needs to contribute a
					computed property without overwriting one already set by an
					earlier, higher-priority source. */
					static void setMetadataIfMissing(
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata,
						const std::string& key,
						const repo::lib::RepoVariant& value);

				private:
					void ensureProxyXData(ProxyInfo& info);
					void ensureProxyExtensionDictionary(ProxyInfo& info);

					void addProxyBasicMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyGeneralMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyGeometryMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void addProxyXDataMetadata(const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

					void extractXDataProperties(OdResBufPtr pRb, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
					void extractTextPropertiesFromProxy(OdDbProxyEntityPtr proxyEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
				};
			}
		}
	}
}
