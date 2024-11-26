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

#include <boost/functional/hash.hpp>
#include <repo/core/model/bson/repo_node_mesh.h>

namespace repo {
	namespace test {
		namespace utils {
			namespace mesh {

				/*
				* A set of mesh data that might be stored by a MeshNode. Geometry is
				* initialised based on the parameters provided to the constructor.
				* Faces are generated such that all vertices are referenced.
				*/
				struct mesh_data
				{
					mesh_data(
						bool name,
						bool sharedId,
						int numParents,
						int faceSize,
						bool normals,
						int numUvChannels,
						int numVertices
					);

					std::string name;
					repo::lib::RepoUUID uniqueId;
					repo::lib::RepoUUID sharedId;
					std::vector<repo::lib::RepoUUID> parents;
					std::vector<std::vector<float>> boundingBox;
					std::vector<repo::lib::RepoVector3D> vertices;
					std::vector<repo_face_t> faces;
					std::vector<repo::lib::RepoVector3D> normals;
					std::vector<std::vector<repo::lib::RepoVector2D>> uvChannels;
				};

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

				/*
				* Compare the geometric content of two meshes to determine if their
				* hulls are identical.
				*/
				bool compareMeshes(
					repo::core::model::RepoNodeSet original,
					repo::core::model::RepoNodeSet stash);

				void addFaces(
					repo::core::model::MeshNode* mesh,
					std::vector<GenericFace>& faces);

				/*
				* Creates a RepoBSON for a MeshNode based on the mesh_data
				*/
				repo::core::model::RepoBSON meshNodeTestBSONFactory(mesh_data);

				/*
				* Creates a MeshNode based on the mesh_data
				*/
				repo::core::model::MeshNode makeMeshNode(mesh_data);

				/*
				* Creates a random mesh with the given format using a repeated seed. The
				* RepoNode properties (uniqueId, sharedId, etc) are not initialised.
				*/
				repo::core::model::MeshNode makeDeterministicMeshNode(int primitive, bool normals, int uvs);

				/*
				* Creates an arbitrary transform matrix
				*/
				repo::lib::RepoMatrix makeTransform(bool translation, bool rotation);

				/*
				* Compares mesh nodes for absolute equality of all members. This is beyond
				* the hulls being the same - this method expects that the meshes are
				* effectively memory-equivalent.
				*/
				void compareMeshNode(mesh_data expected, repo::core::model::MeshNode actual);

				/*
				* Creates randomised vertices
				*/
				std::vector<repo::lib::RepoVector3D> makeVertices(int num);

				/*
				* Creates randomised normals
				*/
				std::vector<repo::lib::RepoVector3D> makeNormals(int num);

				/*
				* Creates randomised UVs
				*/
				std::vector<repo::lib::RepoVector2D> makeUVs(int num);

				/*
				* Creates randomised faces
				*/
				std::vector<repo_face_t> makeFaces(repo::core::model::MeshNode::Primitive);

				/*
				* Computes the bounding box for a set of vertices
				*/
				std::vector< repo::lib::RepoVector3D> getBoundingBox(std::vector< repo::lib::RepoVector3D> vertices);
			}
		}
	}
}
