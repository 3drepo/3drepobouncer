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

#include <vector>
#include "repo/lib/datastructure/repo_triangle.h"
#include "repo/lib/datastructure/repo_vector3d.h"
#include "repo/lib/datastructure/repo_bounds.h"
#include "repo/manipulator/modeloptimizer/bvh/bvh.hpp"

namespace geometry {
	/*
	* A convenience type that offers a view onto an existing set of RepoTriangles
	* to perform oriention checks.
	*/
	struct MeshView
	{
		virtual const bvh::Bvh<double>& getBvh() const = 0;
		virtual repo::lib::RepoTriangle getTriangle(size_t primitive) const = 0;
	};

	/*
	* Checks if a set of vertices is entirely contained in the mesh exposed by the
	* MeshView. A bounds should be provided for the vertices to allow for early
	* rejections. The vertices may represent any topology. Beware that a positive
	* result does not mean that there is no intersection between the meshes, as an
	* edge for the source mesh may intersect the MeshView, even if all the source
	* vertices are contained. It is a necessary precondition for the whole mesh
	* being containde however.
	*/
	bool contains(
		const std::vector<repo::lib::RepoVector3D64>& vertices,
		const repo::lib::RepoBounds& bounds,
		const MeshView& mesh);

	/*
	* For a set of vertices of an open or closed mesh, reorder them so that the
	* most extreme vertices are close to the beginning of the list. This increases
	* the likelihood of early terminations when working through the list
	* sequentially.
	*/
	void reorderVertices(
		const std::vector<repo::lib::RepoVector3D64>& vertices);
}