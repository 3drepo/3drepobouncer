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

#include "../../error_codes.h"
#include "../../core/model/bson/repo_bson_factory.h"
#include "repo_structs.h"
#include <fstream>
#include <vector>
#include <boost/functional/hash.hpp>
#include <string>

namespace repo {
	namespace lib {
		/*
		 * Helper class for referencing existing vertices into indices. This class relies on the format checking in
		 * GeometryCollector, and is only for use within mesh_data_t.
		 */
		template <typename T>
		class _VertexMap {
			std::multimap<size_t, size_t> map;

		public:
			std::vector<T> vertices;
			std::vector<T> normals;
			std::vector<repo::lib::RepoVector2D> uvs;

			struct result_t
			{
				bool added;
				uint32_t index;
			};

			result_t find(const T& position) {
				{
					size_t hash = 0;
					boost::hash_combine(hash, position.x);
					boost::hash_combine(hash, position.y);
					boost::hash_combine(hash, position.z);

					auto matching = map.equal_range(hash);

					result_t result;

					for (auto it = matching.first; it != matching.second; it++)
					{
						if (vertices[it->second] == position)
						{
							result.index = it->second;
							result.added = false;
							return result;
						}
					}

					auto idx = vertices.size();
					vertices.push_back(position);

					map.insert(std::pair<size_t, size_t>(hash, idx));

					result.index = idx;
					result.added = true;

					return result;
				}
			}
			result_t find(const T& position, const T& normal) {
				{
					size_t hash = 0;
					boost::hash_combine(hash, position.x);
					boost::hash_combine(hash, position.y);
					boost::hash_combine(hash, position.z);
					boost::hash_combine(hash, normal.x);
					boost::hash_combine(hash, normal.y);
					boost::hash_combine(hash, normal.z);

					auto matching = map.equal_range(hash);

					result_t result;

					for (auto it = matching.first; it != matching.second; it++)
					{
						if (vertices[it->second] == position)
						{
							if (normals[it->second] == normal)
							{
								result.index = it->second;
								result.added = false;
								return result;
							}
						}
					}

					auto idx = vertices.size();

					vertices.push_back(position);
					normals.push_back(normal);

					map.insert(std::pair<size_t, size_t>(hash, idx));

					result.index = idx;
					result.added = true;

					return result;
				}
			}
			result_t find(const T& position, const T& normal, const repo::lib::RepoVector2D& uv) {
				size_t hash = 0;
				boost::hash_combine(hash, position.x);
				boost::hash_combine(hash, position.y);
				boost::hash_combine(hash, position.z);
				boost::hash_combine(hash, normal.x);
				boost::hash_combine(hash, normal.y);
				boost::hash_combine(hash, normal.z);
				boost::hash_combine(hash, uv.x);
				boost::hash_combine(hash, uv.y);

				auto matching = map.equal_range(hash);

				result_t result;

				for (auto it = matching.first; it != matching.second; it++)
				{
					if (vertices[it->second] == position)
					{
						if (normals[it->second] == normal)
						{
							if (uvs[it->second] == uv)
							{
								result.index = it->second;
								result.added = false;
								return result;
							}
						}
					}
				}

				auto idx = vertices.size();

				vertices.push_back(position);
				normals.push_back(normal);
				uvs.push_back(uv);

				map.insert(std::pair<size_t, size_t>(hash, idx));

				result.index = idx;
				result.added = true;

				return result;
			}
		};

		using VertexMap32 = _VertexMap<RepoVector3D>;
		using VertexMap = _VertexMap<RepoVector3D64>;
	}
}