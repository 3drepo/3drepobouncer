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

/**
* Contains a set of helper facilities for creating and comparing point clouds
* for unit test implementations.
*/

#include <repo/core/model/bson/repo_node_point.h>

namespace repo {
	namespace test {
		namespace utils {
			namespace point {

				// A set of point data that might be store by a PointNode.
				struct point_data
				{
					point_data(
						bool name,
						bool sharedId,
						int numParents,
						int numPoints,
						int treeLevels
					);

					std::string name;
					repo::lib::RepoUUID uniqueId;
					repo::lib::RepoUUID sharedId;
					std::vector<repo::lib::RepoUUID> parents;
					std::vector<std::vector<float>> boundingBox;
					std::vector<uint8_t> treePosition;
					std::vector<repo::lib::RepoVector3D> points;
					std::vector<repo::lib::repo_color4d_t> colourAttributes;
				};

				std::unique_ptr<repo::core::model::PointNode> createRandomPoints(
					const int nPoints,
					const std::vector<repo::lib::RepoUUID>& parent
				);

				// Creates a RepoBSON for a PointNode based on the point_data
				repo::core::model::RepoBSON pointNodeTestBSONFactory(point_data data);

				/*
				* Compares mesh nodes for absolute equality of all members. This is beyond
				* the hulls being the same - this method expects that the points are
				* effectively memory-equivalent.
				*/
				void comparePointNode(point_data expected, repo::core::model::PointNode actual);

				// Creates randomised points
				std::vector<repo::lib::RepoVector3D> makePoints(int num);


				// Creates randomised colours
				std::vector<repo::lib::repo_color4d_t> makeColourAttributes(int num);

				// Create randomised tree position
				std::vector<uint8_t> makeTreePosition(int levels);

				// Creates a random point node with the given format using a repeated seed.
				// The RepoNode properties (uniqueId, sharedId, etc) are not initialised.
				repo::core::model::PointNode makeDeterministicPointNode(int treeLevels);
			}
		} // namespace utils
	} // namespace test
} // namespace repo