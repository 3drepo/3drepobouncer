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
					// Per-entity dedup of triangle edges seen while streaming a Civil3D TIN
					// surface's stored graphics (keyed by edgeKey(p0, p1)), so a shared edge
					// between adjacent triangles is only added to the wireframe overlay once.
					// Cleared at the start of each entity's draw.
					std::unordered_set<std::string> currentSurfaceEdgeKeys;

					// Per-entity dedup of triangle vertices seen for the same reason (keyed by
					// pointKey(p)). Its size is used as the surface's "Number Of Points"
					// metadata - an approximation, since Civil3D's true TIN point count isn't
					// otherwise available from the tessellated proxy graphics. Cleared at the
					// start of each entity's draw.
					std::unordered_set<std::string> currentSurfacePointKeys;

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

					static void addProxyGeometryMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
				};
			}
		}
	}
}
