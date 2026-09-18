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

#include "repo\lib\datastructure\repo_bounds.h"

namespace repo {
	namespace lib {
		namespace pointcloud {
			class PointCloudUtils 
			{
			public: 
				static std::vector<uint8_t> getTreePositionFromCellCoordinates(
					int x,
					int y,
					int z,
					int splits);

				static int getIndexFromCellCoordinates(
					int x,
					int y,
					int z,
					int splits);

				static int getIndexFromTreePosition(
					std::vector<uint8_t>& treePosition,
					int splits);

				static repo::lib::RepoBounds getBoundsFromTreePosition(
					std::vector<uint8_t>& treePosition,
					repo::lib::RepoBounds bounds);

				/// <summary>
				/// Projects a 3D position into the cube to get the coordinate
				/// of the cell it belongs to.
				/// Positions that fall on the mathematical boundary of one of
				/// the cells on one of the axis are nudged into the lower cell
				/// as a tie breaker behaviour.
				/// </summary>
				/// <param name="position">The position to be projected.</param>
				/// <param name="bounds">The overall bounds of the cube.</param>
				/// <param name="splits">The number of splits applied to the cube.</param>
				/// <param name="x">The x coordinate after projection.</param>
				/// <param name="y">The y coordinate after projection.</param>
				/// <param name="z">The z coordinate after projection.</param>
				static void project3DPositionToCellCoordinates(
					repo::lib::RepoVector3D64 position,
					repo::lib::RepoBounds bounds,
					int splits,
					int& x,
					int& y,
					int& z);

				/// <summary>
				/// Projects a 3D position into the cube to get the index
				/// of the cell it belongs to.
				/// Positions that fall on the mathematical boundary of one of
				/// the cells on one of the axis are nudged into the lower cell
				/// as a tie breaker behaviour.
				/// </summary>
				/// <param name="position">The position to be projected.</param>
				/// <param name="bounds">The overall bounds of the cube.</param>
				/// <param name="splits">The number of splits applied to the cube.</param>
				/// <returns>Cell index the point was projected into.</returns>
				static int project3DPositionToCellIndex(
					repo::lib::RepoVector3D64 position,
					repo::lib::RepoBounds bounds,
					int splits);

				/// <summary>
				/// Projects a 3D position into the cube to get the tree position
				/// it belongs to.
				/// Positions that fall on the mathematical boundary of one of
				/// the cells on one of the axis are nudged into the lower cell
				/// as a tie breaker behaviour.
				/// </summary>
				/// <param name="position">The position to be projected.</param>
				/// <param name="bounds">The overall bounds of the cube.</param>
				/// <param name="splits">The number of splits applied to the cube.</param>
				/// <returns>Tree position the point was projected into.</returns>
				static std::vector<uint8_t> project3DPositionToTreePosition(
					repo::lib::RepoVector3D64 position,
					repo::lib::RepoBounds bounds,
					int splits);

				/// <summary>
				/// Retrieves the mathematical bounds of this one of the
				/// eight children of this set of bounds as indicated by
				/// the child index.
				/// Note that if used in conjunction with project3DPostitionTo...(...)
				/// the lower edge of these bounds has to be treated as exclusive
				/// unless bordering the overall bounds on that axis i.e. min() of
				/// these bounds might not be projected into this child. 
				/// This comes from the tie-breaker behaviour implemented in the
				/// projection function.
				/// </summary>
				/// <param name="bounds">The overall bounds to be split.</param>
				/// <param name="childIndex">The index of the child to get the bounds of</param>
				/// <returns>The segment of the overall bounds represented by this child.</returns>
				static repo::lib::RepoBounds getChildBoundsByIndex(
					repo::lib::RepoBounds bounds,
					int childIndex);

				/// <summary>
				/// Combines two tree positions so that a and b turn to ab.
				/// </summary>
				/// <param name="a">The first part to combine.</param>
				/// <param name="b">The second part to combine.</param>
				/// <returns>The combined result.</returns>
				static std::vector<uint8_t> combineTreePositions(
					std::vector<uint8_t> a,
					std::vector<uint8_t> b);

			private:
				static int project3DPositionToCellIndexSingleAxis(
					double value,
					double min,
					double max,
					int cellsPerSide);
			};
		} // namespace pointcloud
	} // namespace lib
} // namespace repo