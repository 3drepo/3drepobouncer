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

#include "../../../../error_codes.h"
#include "../../../../core/model/bson/repo_bson_factory.h"
#include "../../../../lib/datastructure/repo_structs.h"
#include "helper_functions.h"
#include <fstream>
#include <vector>
#include <string>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {

				/*
				 * Helper class for referencing existing vertices into indices. This class relies on the format checking in 
				 * GeometryCollector, and is only for use within mesh_data_t.
				 */
				class VertexMap {
					std::multimap<size_t, size_t> map;

				public:
					std::vector<repo::lib::RepoVector3D64> vertices;
					std::vector<repo::lib::RepoVector3D64> normals;
					std::vector<repo::lib::RepoVector2D> uvs;

					struct result_t
					{
						bool added;
						uint32_t index;
					};

					result_t find(const repo::lib::RepoVector3D64& position);
					result_t find(const repo::lib::RepoVector3D64& position, const repo::lib::RepoVector3D64& normal);
					result_t find(const repo::lib::RepoVector3D64& position, const repo::lib::RepoVector3D64& normal, const repo::lib::RepoVector2D& uv);
				};

			}
		}
	}
}