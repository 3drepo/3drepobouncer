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
#include "repo/lib/datastructure/repo_vector.h"
#include "repo/lib/repo_hash_combine.h"
#include "repo/lib/hopscotch_map/hopscotch_set.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				struct PointHash
				{
					size_t operator()(const repo::lib::RepoVector3D64& p) const
					{
						size_t seed = 0;
						repo::lib::hash_combine(seed, p.x);
						repo::lib::hash_combine(seed, p.y);
						repo::lib::hash_combine(seed, p.z);
						return seed;
					}
				};

				// A direction-independent key for an edge between two points: the
				// endpoints are stored ordered by their individual hash values, so the
				// same key is produced whichever order they are given in.
				struct EdgeKey
				{
					repo::lib::RepoVector3D64 p0;
					repo::lib::RepoVector3D64 p1;
					size_t h0;
					size_t h1;

					EdgeKey(const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b)
					{
						auto ha = PointHash()(a);
						auto hb = PointHash()(b);
						if (ha <= hb)
						{
							p0 = a; p1 = b;
							h0 = ha; h1 = hb;
						}
						else
						{
							p0 = b; p1 = a;
							h0 = hb; h1 = ha;
						}
					}

					bool operator==(const EdgeKey& other) const
					{
						return p0 == other.p0 && p1 == other.p1;
					}
				};

				struct EdgeKeyHash
				{
					size_t operator()(const EdgeKey& key) const
					{
						size_t seed = 0;
						repo::lib::hash_combine(seed, key.h0);
						repo::lib::hash_combine(seed, key.h1);
						return seed;
					}
				};

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

					/* Per-entity dedup of triangle edges seen while streaming a Civil3D
					TIN surface's stored graphics, so a shared edge between adjacent
					triangles is only added to the wireframe overlay once. Returns false
					(and records the edge) the first time it is seen since the last
					resetEdges() call, true on every subsequent call for the same edge,
					in either direction. */
					bool hasEdge(const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b);

					// Clears the edge dedup state. Called at the start of each entity's draw.
					void resetEdges();

				private:
					tsl::hopscotch_set<EdgeKey, EdgeKeyHash> edges;
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

					static bool drawProxyGraphics(OdDbEntityPtr pEntity, const ProxyInfo& info, OdGiWorldDraw* worldDraw);

					static void addProxyMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

				private:
					static void addProxyGeneralMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

					static void addProxyGeometryMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);
				};
			}
		}
	}
}
