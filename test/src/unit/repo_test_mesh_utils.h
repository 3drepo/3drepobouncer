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
#include <repo/manipulator/modelconvertor/export/repo_model_export_abstract.h>
#include <repo/core/handler/database/repo_query.h>
#include <repo/core/handler/fileservice/repo_data_ref.h>
#include <repo/core/handler/fileservice/repo_blob_files_handler.h>

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
						int numVertices,
						std::string grouping
					);

					std::string name;
					repo::lib::RepoUUID uniqueId;
					repo::lib::RepoUUID sharedId;
					std::vector<repo::lib::RepoUUID> parents;
					std::vector<std::vector<float>> boundingBox;
					std::vector<repo::lib::RepoVector3D> vertices;
					std::vector<repo::lib::repo_face_t> faces;
					std::vector<repo::lib::RepoVector3D> normals;
					std::vector<std::vector<repo::lib::RepoVector2D>> uvChannels;
					std::string grouping;
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

				// Dummy exporter to catch geometry produced by the multipart optimizer
				class TestModelExport : public repo::manipulator::modelconvertor::AbstractModelExport
				{
				public:
					TestModelExport(
						repo::core::handler::AbstractDatabaseHandler *dbHandler,
						const std::string databaseName,
						const std::string projectName,
						const repo::lib::RepoUUID revId,
						const std::vector<double> worldOffset
					) : AbstractModelExport(dbHandler, databaseName, projectName, revId, worldOffset)
					{						
					}

					void addSupermesh(repo::core::model::SupermeshNode* supermesh) {
						supermeshNodes.push_back(*supermesh);
					}

					void finalise() {
						finalised = true;
						// Do nothing else
					};

					bool isFinalised() {
						return finalised;
					};

					std::vector<repo::core::model::SupermeshNode> getSupermeshes() {
						return supermeshNodes;
					};

					int getSupermeshCount() {
						return supermeshNodes.size();
					};

				private:
					bool finalised = false;
					std::vector<repo::core::model::SupermeshNode> supermeshNodes;
				};

				/**
				* Builds a triangle soup MeshNode with exactly nVertices, and faces
				* constructed such that each vertex is referenced at least once.
				*/
				std::unique_ptr<repo::core::model::MeshNode> createRandomMesh(
					const int nVertices,
					const bool hasUV,
					const int primitiveSize,
					const std::string grouping,
					const std::vector<repo::lib::RepoUUID>& parent);

				/*
				* Compare the geometric content of two meshes to determine if their
				* hulls are identical.
				*/
				bool compareMeshes(
					std::string database,
					std::string projectName,
					repo::lib::RepoUUID revId,
					TestModelExport* mockExporter
				);

				std::vector<GenericFace> getFacesFromDatabase(
					std::string database,
					std::string projectName,
					repo::lib::RepoUUID revId
				);

				std::vector<GenericFace> getFacesFromMockExporter(
					TestModelExport* mockExporter
				);

				void addFaces(
					repo::core::model::MeshNode &mesh,
					std::vector<GenericFace>& faces);

				void addFaces(
					const std::vector<repo::lib::RepoVector3D> &vertices,
					const std::vector<repo::lib::RepoVector3D> &normals,
					const std::vector<repo::lib::repo_face_t> &faces,
					const std::vector < std::vector<repo::lib::RepoVector2D>>& uvChannels,
					std::vector<GenericFace>& genericFaces);

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
				std::vector<repo::lib::repo_face_t> makeFaces(repo::core::model::MeshNode::Primitive);

				/*
				* Computes the bounding box for a set of vertices
				*/
				repo::lib::RepoBounds getBoundingBox(std::vector< repo::lib::RepoVector3D> vertices);
				repo::lib::RepoBounds getBoundingBox(std::vector< repo::lib::RepoVector3D64> vertices);

				float hausdorffDistance(const repo::core::model::MeshNode& ma, const repo::core::model::MeshNode& mb);
				float hausdorffDistance(const std::vector<repo::core::model::MeshNode>& meshes);

				float shortestDistance(const repo::core::model::MeshNode& m, repo::lib::RepoVector3D p);
				float shortestDistance(const std::vector<repo::core::model::MeshNode>& m, repo::lib::RepoVector3D p);
				float shortestDistance(const std::vector<repo::lib::RepoVector3D>& v, repo::lib::RepoVector3D p);

				/*
				* Creates a cube centered on 0,0,0 with a side length of 1 in all dimensions.
				*/
				repo::core::model::MeshNode makeUnitCube();

				/*
				* Creates a cone centered on 0,0,0 with a height of 1 and a base radius of 1.
				*/
				repo::core::model::MeshNode makeUnitCone();

				/*
				* Creates a sphere centered on 0,0,0 with a diameter of 1. This uses uv/lat
				* -long grid approach, as opposed to the geodesic/icosahedron approach. This
				* is chosen as it is arguably the more challenging mesh for testing purposes.
				*/
				repo::core::model::MeshNode makeUnitSphere();

				repo::core::model::MeshNode fromVertices(
					const std::vector<repo::lib::RepoVector3D>& vertices,
					repo::core::model::MeshNode::Primitive primitive = repo::core::model::MeshNode::Primitive::TRIANGLES
				);
			}
		}
	}
}
