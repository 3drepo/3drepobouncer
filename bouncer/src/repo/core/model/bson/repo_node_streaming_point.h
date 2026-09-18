/**
*  Copyright (C) 2026 3D Repo Ltd
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
#include <repo/core/model/bson/repo_node_point.h>

// TODO: FT rename file for consistency

namespace repo {
	namespace core {
		namespace model {
			class StreamingPointNode {

				class PointOptimisationData {

					repo::lib::RepoUUID uniqueId;

					// Geometry
					std::vector<repo::core::model::PointData> points;

				public:
					PointOptimisationData(
						const repo::core::model::RepoBSON& bson,
						const std::vector<uint8_t>& buffer);

					repo::lib::RepoUUID getUniqueId() const {
						return uniqueId;
					}


					std::uint32_t getNumPoints() const {
						return points.size();
					}
					const std::vector<repo::core::model::PointData>& getPoints() const {
						return points;
					}

				private:
					void deserialise(
						const repo::core::model::RepoBSON& bson,
						const std::vector<uint8_t>& buffer);

					template <class T>
					void deserialiseVector(
						const repo::core::model::RepoBSON& bson,
						const std::vector<uint8_t>& buffer,
						std::vector<T>& vec)
					{
						auto start = bson.getLongField(REPO_LABEL_BINARY_START);
						auto size = bson.getLongField(REPO_LABEL_BINARY_SIZE);

						vec.resize(size / sizeof(T));
						memcpy(vec.data(), buffer.data() + (sizeof(uint8_t) * start), size);
					}
				};

				repo::lib::RepoUUID sharedId;
				std::uint32_t numPoints = 0;
				repo::lib::RepoUUID parent;
				repo::lib::RepoBounds bounds;
				std::vector<uint8_t> treePosition;
				std::unique_ptr<PointOptimisationData> pOpData;

			public:
				StreamingPointNode()
				{
					// Default constructor so instances can be initialised for vectors
				}

				StreamingPointNode(const repo::core::model::RepoBSON& bson);

				// StreamingMeshNode must have explicit move operators, even if default
				// implementations, as we define explicit constructors above.

				StreamingPointNode(StreamingPointNode&&) noexcept = default;
				StreamingPointNode& operator=(StreamingPointNode&&) noexcept = default;

				bool pointOptimisationDataLoaded() {
					return pOpData != nullptr;
				}

				void loadPointOptimisationData(
					const repo::core::model::RepoBSON& bson,
					const std::vector<uint8_t>& buffer);

				void unloadPointOptimisationData() {
					pOpData.reset();
				}

				const repo::lib::RepoUUID getSharedId() const {
					return sharedId;
				}

				const std::uint32_t getNumPoints() const
				{
					return numPoints;
				}

				const repo::lib::RepoBounds getBoundingBox() const
				{
					return bounds;
				}

				const repo::lib::RepoUUID getParent() const
				{
					return parent;
				}

				const std::vector<uint8_t> getTreePosition() const
				{
					return treePosition;
				}

				void transformBounds(const repo::lib::RepoMatrix& transform);

				// Requiring the point optimisation data to be loaded

				const repo::lib::RepoUUID getUniqueId();

				const std::uint32_t getNumLoadedPoints();

				const std::vector<repo::core::model::PointData>& getLoadedPoints();

				private:
					void assertPointOptimisationDataLoaded(); // Throws if the point optimisation data is not loaded.
			};
		}
	}
}