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

#include "repo/lib/datastructure/repo_vector.h"
#include <vector>
#include <map>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {

				/*
				 * Helper class for collecting vertex attributes in step. This type does not
				 * do any error checking - the caller must make sure only one of its methods
				 * is called for its entire lifetime or the attributes will become out of 
				 * sync.
				 */
				class VertexMap {
				public:
					std::vector<repo::lib::RepoVector3D64> vertices;
					std::vector<repo::lib::RepoVector3D64> normals;
					std::vector<repo::lib::RepoVector2D> uvs;

					VertexMap()
					{
						vertices.reserve(1000);
						normals.reserve(1000);
						uvs.reserve(1000);
					}

					size_t insert(const repo::lib::RepoVector3D64& position);
					size_t insert(const repo::lib::RepoVector3D64& position, const repo::lib::RepoVector3D64& normal);
					size_t insert(const repo::lib::RepoVector3D64& position, const repo::lib::RepoVector3D64& normal, const repo::lib::RepoVector2D& uv);
				};

			}
		}
	}
}