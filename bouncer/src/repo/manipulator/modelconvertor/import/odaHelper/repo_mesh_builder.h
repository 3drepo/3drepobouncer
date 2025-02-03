/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo/core/model/repo_model_global.h"
#include "repo/lib/datastructure/repo_structs.h"
#include "repo/core/model/bson/repo_node_mesh.h"
#include <vector>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				/*
				* Used to construct one or more MeshNodes over time. After creating the
				* builder, call addFace repeatedly to add geometry. MeshBuilder will create as
				* many meshes as there are different mesh formats (defined by the primitives).
				* Meshes must be extracted before MeshBuilder goes out of scope, or an
				* exception will be thrown.
				*/
				class RepoMeshBuilder
				{
				public:
					RepoMeshBuilder(std::vector<repo::lib::RepoUUID> parents, const repo::lib::RepoVector3D64& offset, repo_material_t material);

					~RepoMeshBuilder();

					struct face {

						face();

						bool hasNormals() const;
						bool hasUvs() const;
						uint8_t getSize() const;
						uint32_t getFormat() const;

						repo::lib::RepoVector3D64 vertex(size_t i) const;
						repo::lib::RepoVector2D uv(size_t i) const;
						repo::lib::RepoVector3D64 normal() const;

					private:
						repo::lib::RepoVector3D64 vertices[3];
						repo::lib::RepoVector2D uvs[3];
						repo::lib::RepoVector3D64 n;
						bool _hasNormal;
						bool _hasUvs;
						uint8_t size;
					};

					void addFace(const std::vector<repo::lib::RepoVector3D64>& vertices);
					void addFace(
						const std::vector<repo::lib::RepoVector3D64>& vertices,
						const repo::lib::RepoVector3D64& normal,
						const std::vector<repo::lib::RepoVector2D>& uvCoords);

					void addFace(const face& face);

					void extractMeshes(std::vector<repo::core::model::MeshNode>& nodes);

					const std::vector<repo::lib::RepoUUID>& getParents();
					repo_material_t getMaterial();

				private:
					void addFace(
						const std::vector<repo::lib::RepoVector3D64>& vertices,
						std::optional<repo::lib::RepoVector3D64> normal,
						const std::vector<repo::lib::RepoVector2D>& uvCoords
					);

					struct mesh_data_t;

					mesh_data_t* startOrContinueMeshByFormat(uint32_t format);
					static uint32_t getMeshFormat(bool hasUvs, bool hasNormals, int faceSize);

					// This exists to cache the result of the last startOrContinueMeshByFormat
					// call for performance reasons.
					mesh_data_t* currentMesh = nullptr;

					std::unordered_map<uint32_t, mesh_data_t*> meshes;

					std::vector<repo::lib::RepoUUID> parents;

					/*
					* A convenience property that tracks the material assocated with this
					* builder. This doesn't change the actual mesh construction.
					*/
					repo_material_t material;

					repo::lib::RepoVector3D64 offset;
				};
			}
		}
	}
}