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

/** 
* Contains a set of helper facilities for creating and comparing meshes
* for unit test implementations.
*/

#include <repo/core/model/bson/repo_bson_factory.h>
#include <boost/functional/hash.hpp>

namespace repo {
	namespace test {
		namespace utils {
			namespace mesh {

				struct GenericFace
				{
					std::vector<float> data;
					int hit;

					void push(repo::lib::RepoVector2D v)
					{
						data.push_back(v.x);
						data.push_back(v.y);
					}

					void push(repo::lib::RepoVector3D v)
					{
						data.push_back(v.x);
						data.push_back(v.y);
						data.push_back(v.z);
					}

					const long hash() const
					{
						size_t hash = 0;
						for (const auto value : data)
						{
							boost::hash_combine(hash, value);
						}
						return hash;
					}

					const bool equals(const GenericFace& other) const
					{
						return data == other.data;
					}
				};

				/**
				* Builds a triangle soup MeshNode with exactly nVertices, and faces
				* constructed such that each vertex is referenced at least once.
				*/
				repo::core::model::MeshNode* createRandomMesh(
					const int nVertices,
					const bool hasUV,
					const int primitiveSize,
					const std::vector<repo::lib::RepoUUID>& parent);

				bool compareMeshes(
					repo::core::model::RepoNodeSet original,
					repo::core::model::RepoNodeSet stash);

				void addFaces(
					repo::core::model::MeshNode* mesh, 
					std::vector<GenericFace>& faces);
			}
		}
	}
}
