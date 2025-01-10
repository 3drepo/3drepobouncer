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
					RepoMeshBuilder(std::vector<repo::lib::RepoUUID> parents);
					~RepoMeshBuilder();

					void addFace(const std::vector<repo::lib::RepoVector3D64>& vertices);
					void addFace(
						const std::vector<repo::lib::RepoVector3D64>& vertices,
						const repo::lib::RepoVector3D64& normal,
						const std::vector<repo::lib::RepoVector2D>& uvCoords);

					void extractMeshes(std::vector<repo::core::model::MeshNode>& nodes);

				private:


					void addFace(
						const std::vector<repo::lib::RepoVector3D64>& vertices,
						std::optional<repo::lib::RepoVector3D64> normal,
						const std::vector<repo::lib::RepoVector2D>& uvCoords
					);

					struct mesh_data_t;

					mesh_data_t* startOrContinueMeshByFormat(uint32_t format);
					uint32_t getMeshFormat(bool hasUvs, bool hasNormals, int faceSize);

					// This exists to cache the result of the last startOrContinueMeshByFormat
					// call for performance reasons.
					mesh_data_t* currentMesh = nullptr;

					std::unordered_map<uint32_t, mesh_data_t*> meshes;

					std::vector<repo::lib::RepoUUID> parents;
				};
			}
		}
	}
}