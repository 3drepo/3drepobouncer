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

#pragma once
#include <repo/lib/datastructure/repo_uuid.h>
#include <repo/lib/datastructure/repo_vector.h>
#include <repo/lib/datastructure/repo_structs.h>
#include <repo/lib/datastructure/repo_matrix.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_node_mesh.h>

namespace repo {
	namespace core {
		namespace model {
			class StreamingMeshNode {

				class SupermeshingData {

					repo::lib::RepoUUID uniqueId;

					// Geometry
					std::vector<repo::lib::RepoVector3D> vertices;
					std::vector<repo::lib::repo_face_t> faces;
					std::vector<repo::lib::RepoVector3D> normals;
					std::vector<std::vector<repo::lib::RepoVector2D>> channels;

				public:
					SupermeshingData(
						const repo::core::model::RepoBSON& bson,
						const std::vector<uint8_t>& buffer,
						const bool ignoreUVs);

					repo::lib::RepoUUID getUniqueId() const {
						return uniqueId;
					}

					std::uint32_t getNumFaces() const {
						return faces.size();
					}
					std::vector<repo::lib::repo_face_t>& getFaces()
					{
						return faces;
					}

					std::uint32_t getNumVertices() const {
						return vertices.size();
					}
					const std::vector<repo::lib::RepoVector3D>& getVertices() const {
						return vertices;
					}

					void bakeMeshes(const repo::lib::RepoMatrix& transform);

					const std::vector<repo::lib::RepoVector3D>& getNormals() const
					{
						return normals;
					}

					const std::vector<std::vector<repo::lib::RepoVector2D>>& getUVChannelsSeparated() const
					{
						return channels;
					}

				private:
					void deserialise(
						const repo::core::model::RepoBSON& bson,
						const std::vector<uint8_t>& buffer,
						const bool ignoreUVs);

					template <class T>
					void deserialiseVector(
						const repo::core::model::RepoBSON& bson,
						const std::vector<uint8_t>& buffer,
						std::vector<T>& vec)
					{
						auto start = bson.getIntField(REPO_LABEL_BINARY_START);
						auto size = bson.getIntField(REPO_LABEL_BINARY_SIZE);

						vec.resize(size / sizeof(T));
						memcpy(vec.data(), buffer.data() + (sizeof(uint8_t) * start), size);
					}
				};

				repo::lib::RepoUUID sharedId;
				std::uint32_t numVertices = 0;
				repo::lib::RepoUUID parent;
				repo::lib::RepoBounds bounds;

				std::unique_ptr<SupermeshingData> supermeshingData;

			public:
				StreamingMeshNode()
				{
					// Default constructor so instances can be initialised for vectors
				}

				StreamingMeshNode(const repo::core::model::RepoBSON& bson);

				bool supermeshingDataLoaded() {
					return supermeshingData != nullptr;
				}

				void loadSupermeshingData(
					const repo::core::model::RepoBSON& bson,
					const std::vector<uint8_t>& buffer,
					const bool ignoreUVs);

				void unloadSupermeshingData() {
					supermeshingData.reset();
				}

				const repo::lib::RepoUUID getSharedId() const {
					return sharedId;
				}

				const std::uint32_t getNumVertices() const
				{
					return numVertices;
				}

				const repo::lib::RepoBounds getBoundingBox() const
				{
					return bounds;
				}

				const repo::lib::RepoUUID getParent() const
				{
					return parent;
				}

				void transformBounds(const repo::lib::RepoMatrix& transform);

				// Requiring the supermeshing data to be loaded

				const repo::lib::RepoUUID getUniqueId();

				const std::uint32_t getNumLoadedFaces();

				const std::vector<repo::lib::repo_face_t>& getLoadedFaces();

				const std::uint32_t getNumLoadedVertices();

				const std::vector<repo::lib::RepoVector3D>& getLoadedVertices();

				void bakeLoadedMeshes(const repo::lib::RepoMatrix& transform);

				const std::vector<repo::lib::RepoVector3D>& getLoadedNormals();

				const std::vector<std::vector<repo::lib::RepoVector2D>>& getLoadedUVChannelsSeparated();
			};
		}
	}
}