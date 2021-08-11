/**
*  Copyright (C) 2021 3D Repo Ltd
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
#include "globals.h"
#include "../bouncer/src/repo/lib/datastructure/repo_structs.h"

namespace IfcUtils {
	namespace Ifc2x3 {
		class IFC_UTILS_API_EXPORT GeometryHandler {
		public:
			static bool retrieveGeometry(
				const std::string &file,
				std::vector < std::vector<double>> &allVertices,
				std::vector<std::vector<repo_face_t>> &allFaces,
				std::vector < std::vector<double>> &allNormals,
				std::vector < std::vector<double>> &allUVs,
				std::vector<std::string> &allIds,
				std::vector<std::string> &allNames,
				std::vector<std::string> &allMaterials,
				std::unordered_map<std::string, repo_material_t> &matNameToMaterials,
				std::vector<double>		&offset,
				std::string              &errMsg);
		};
	};
}
