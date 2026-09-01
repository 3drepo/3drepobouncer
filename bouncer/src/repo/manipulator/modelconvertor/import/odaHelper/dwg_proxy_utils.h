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
#include <unordered_map>
#include <unordered_set>

#include "repo/lib/datastructure/repo_variant.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {

				/* Stores metadata and graphics information for a DWG proxy entity.
				   Used to identify proxy entities, determine whether stored graphics are
				   available, and identify Civil 3D TIN surface proxies for specialized
				   rendering.
				 */
				struct ProxyInfo
				{
					OdDbProxyEntityPtr entity; // null => not a proxy
					std::string originalClass;
					OdDbProxyEntity::GraphicsMetafileType graphicsType = OdDbProxyEntity::kNoMetafile;
					OdDbEntityWithGrDataPEPtr graphicsPE;
					std::unordered_set<std::string> currentSurfaceEdgeKeys;

					// Returns true when this represents a proxy entity.
					bool isProxy() const { return !entity.isNull(); }

					// Returns true when complete stored proxy graphics are available.
					bool hasFullGraphics() const { return graphicsType == OdDbProxyEntity::kFullGraphics; }

					// Returns true when the proxy represents a Civil 3D TIN surface.
					bool isCivil3DSurfaceClass() const
					{
						return originalClass.find("SurfaceTin") != std::string::npos ||
							originalClass.find("TinSurface") != std::string::npos;
					}
				};

				/* Generic (app-agnostic) inspection and stored-graphics replay for DWG
				proxy entities (custom object-enabler classes ODA cannot natively load).
				Has no compiled-in knowledge of any specific authoring app - app-specific
				classification, display names, and dictionary metadata (e.g. Civil3D's)
				are the concern of whatever consumes ProxyInfo, such as DataProcessorDwg.

				DwgProxyUtils holds no state of its own - both methods are pure
				functions of their parameters - so there is nothing to construct or
				own; call the static methods directly. */
				class DwgProxyUtils
				{
				public:
					static ProxyInfo getProxyInfo(OdDbEntityPtr entity);

					static bool drawStoredProxyGraphics(OdDbEntityPtr pEntity, const ProxyInfo& info, OdGiWorldDraw* worldDraw);

					static void addProxyMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

				private:
					static void addProxyGeneralMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

					static void addProxyGeometryMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
				};
			}
		}
	}
}
