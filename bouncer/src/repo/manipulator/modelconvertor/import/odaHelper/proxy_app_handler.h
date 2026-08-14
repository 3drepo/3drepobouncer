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

#include <OdaCommon.h>

#include <string>
#include <vector>
#include <unordered_map>

#include <DbDictionary.h>
#include <DbEntity.h>

#include "repo/lib/datastructure/repo_variant.h"
#include "repo/lib/datastructure/repo_vector.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class GeometryCollector;

				enum class ProxyAppType { Unknown, Civil3D, Plant3D, Custom };

				/* Optional capability a ProxyAppHandler may expose. Only implemented by
				handlers that need to intercept the ODA geometry-callback stream - today,
				only Civil3D TIN surfaces. A handler that doesn't need this (Plant3D, or
				any future app) simply never returns a non-null pointer from
				ProxyAppHandler::geometryCapture(), and the hot geometry callbacks never
				call through this interface for it. */
				class ProxyGeometryCapture
				{
				public:
					virtual ~ProxyGeometryCapture() = default;

					virtual void beginCapture() = 0;

					virtual bool addTriangle(
						GeometryCollector* collector,
						const repo::lib::RepoVector3D64& p0,
						const repo::lib::RepoVector3D64& p1,
						const repo::lib::RepoVector3D64& p2) = 0;
					virtual bool addEdge(
						GeometryCollector* collector,
						const repo::lib::RepoVector3D64& p0,
						const repo::lib::RepoVector3D64& p1) = 0;
					virtual bool addPolyline(
						GeometryCollector* collector,
						const std::vector<repo::lib::RepoVector3D64>& points) = 0;

					virtual bool hasTriangles() const = 0;
					virtual void clearTriangles() = 0;

					virtual void applyFaceLayers(
						GeometryCollector* collector,
						const std::string& parentLayerId,
						const std::string& sourceEntityId) const = 0;

					virtual void addComputedMetadata(
						OdDbEntityPtr pEntity,
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata) const = 0;
				};

				/* A pluggable, app-specific proxy feature (Civil3D, Plant3D, ...). The
				core DataProcessorDwg class only ever talks to proxies through this
				interface - it has no compiled-in knowledge of any concrete app. Adding a
				new app means implementing this interface and registering an instance;
				removing one means deleting its implementation file. */
				class ProxyAppHandler
				{
				public:
					virtual ~ProxyAppHandler() = default;

					// Same role as the old classifyApplication()'s per-app substring checks.
					virtual bool matches(const std::string& originalClass) const = 0;

					virtual ProxyAppType appType() const = 0;
					virtual std::string appName() const = 0; // "Civil3D" / "Plant3D", for display strings

					// Contribute app-specific extension-dictionary stored properties.
					// Default: no-op, for apps with nothing to add here.
					virtual void addDictionaryMetadata(
						OdDbDictionaryPtr dict,
						std::unordered_map<std::string, repo::lib::RepoVariant>& metadata) const
					{
					}

					// Friendly display name for a proxy class this app owns, e.g.
					// "AeccDbSurfaceTin" -> "TIN Surface". Default: no match.
					virtual bool getDisplayName(const std::string& originalClass, std::string& outName) const
					{
						return false;
					}

					// nullptr (default) => this app never needs geometry capture; the hot
					// geometry-callback path never sees a non-null-but-inert object for
					// apps that don't need this.
					virtual ProxyGeometryCapture* geometryCapture()
					{
						return nullptr;
					}

					// Does this proxy class need special-case geometry handling beyond
					// stored-graphics replay (e.g. Civil3D TIN surfaces)? Default: no
					// app has one, so this never needs to reach past the interface into
					// a concrete handler.
					virtual bool isSpecialSurfaceClass(const std::string& originalClass) const
					{
						return false;
					}
				};
			}
		}
	}
}
