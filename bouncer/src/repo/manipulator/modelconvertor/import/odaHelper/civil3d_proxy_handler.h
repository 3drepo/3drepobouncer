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
#include <unordered_set>
#include <vector>

#include "proxy_app_handler.h"
#include "geometry_collector.h"
#include "repo/lib/datastructure/repo_structs.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				/* Civil3D-specific proxy handling: class-name/XData classification,
				extension-dictionary stored properties, the Civil3D proxy-class
				display-name map, and TIN surface geometry capture. This is the only
				file in the DWG importer that knows anything about Civil3D; removing
				Civil3D support means deleting this file and its registration in
				DataProcessorDwg. */
				class Civil3DProxyHandler : public ProxyAppHandler
				{
				public:
					/* Callable before an instance exists, so DwgProxyInspector can decide
					which concrete handler (if any) to construct without having to
					construct both up front just to ask. */
					static bool matchesClassName(const std::string& originalClass);
					bool matches(const std::string& originalClass) const override { return matchesClassName(originalClass); }
					ProxyAppType appType() const override { return ProxyAppType::Civil3D; }
					std::string appName() const override { return "Civil3D"; }

					void addDictionaryMetadata(
						OdDbDictionaryPtr dict,
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata) const override;

					bool getDisplayName(const std::string& originalClass, std::string& outName) const override;

					ProxyGeometryCapture* geometryCapture() override { return &tinCapture; }

					bool isTinSurfaceClass(const std::string& originalClass) const;

					// The only concrete ProxyAppHandler with a "special surface class"
					// today - see ProxyAppHandler::isSpecialSurfaceClass().
					bool isSpecialSurfaceClass(const std::string& originalClass) const override
					{
						return isTinSurfaceClass(originalClass);
					}

				private:
					/* Holds the transient state needed to capture TIN surface triangles and
					edges replayed from a Civil3D TIN surface proxy's stored graphics, and to
					turn that captured geometry into per-face layers in the GeometryCollector. */
					class TinCapture : public ProxyGeometryCapture
					{
					public:
						void beginCapture() override;

						bool addTriangle(
							GeometryCollector* collector,
							const repo::lib::RepoVector3D64& p0,
							const repo::lib::RepoVector3D64& p1,
							const repo::lib::RepoVector3D64& p2) override;
						bool addEdge(
							GeometryCollector* collector,
							const repo::lib::RepoVector3D64& p0,
							const repo::lib::RepoVector3D64& p1) override;
						bool addPolyline(
							GeometryCollector* collector,
							const std::vector<repo::lib::RepoVector3D64>& points) override;

						bool hasTriangles() const override { return !capturedTriangles.empty(); }
						void clearTriangles() override { capturedTriangles.clear(); }

						void applyFaceLayers(
							GeometryCollector* collector,
							const std::string& parentLayerId,
							const std::string& sourceEntityId) const override;

						void addComputedMetadata(
							OdDbEntityPtr pEntity,
							std::unordered_map<std::string, repo::lib::RepoVariant>& metadata) const override;

					private:
						void captureMaterialIfNeeded(GeometryCollector* collector);

						std::unordered_set<std::string> capturedEdgeKeys;
						std::vector<GeometryCollector::Face> capturedTriangles;
						bool hasFaceMaterial = false;
						repo::lib::repo_material_t faceMaterial;
					};

					TinCapture tinCapture;
				};
			}
		}
	}
}
