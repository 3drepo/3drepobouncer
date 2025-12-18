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

#include "closed_mesh_view.h"

#include "repo/manipulator/modeloptimizer/bvh/single_ray_traverser.hpp"
#include "repo/manipulator/modelutility/clashdetection/geometry_tests.h"

using namespace geometry;
using namespace bvh;

using Ray_t = bvh::Ray<double>;
using Vector_t = bvh::Vector3<double>;
using Traverser_t = SingleRayTraverser<bvh::Bvh<double>, 64, bvh::RobustNodeIntersector<bvh::Bvh<double>>>;

struct PrimitiveIntersector
{
	const MeshView& mesh;

	/*
	* When this is true, it terminates the Single Ray Traverser. We set it to true
	* the first time a ray fails to intersect any node, as in that case at least
	* one point must be outside the mesh and so the operand cannot be contained.
	*/
	bool any_hit = false;

	size_t numIntersections = 0;

	PrimitiveIntersector(const MeshView& mesh):
		mesh(mesh)
	{
	}

	struct Result
	{
		// The library Single Ray Traverser will use this to update the ray's
		// tmax property. It should never be called because we should always
		// return a nullopt from the primitive intersection.

		double distance() {
			throw std::runtime_error("intersect() should always be nullopt for point tests");
		}
	};

	Result dummy;

	/*
	* Returns the result of the leaf intersection which the Single Ray Traverser
	* will use to update the best hit. We don't care about the actual hits, so
	* this always returns nullopt, ensuring the traversal continues until the end.
	*/
	std::optional<Result> intersect(size_t i, const Ray_t& ray) {
		auto t = geometry::intersects(
			reinterpret_cast<const repo::lib::RepoVector3D64&>(ray.origin),
			reinterpret_cast<const repo::lib::RepoVector3D64&>(ray.direction),
			mesh.getTriangle(i));
		if (t > 0 && t != std::numeric_limits<double>::infinity()) {
			numIntersections++;
		}
		return std::nullopt;
	}
};

static bool encapsulates(const bvh::Bvh<double>::Node& node, const repo::lib::RepoBounds& bounds)
{
	if (bounds.min().x < node.bounds[0]) return false;
	if (bounds.max().x > node.bounds[1]) return false;
	if (bounds.min().y < node.bounds[2]) return false;
	if (bounds.max().y > node.bounds[3]) return false;
	if (bounds.min().z < node.bounds[4]) return false;
	if (bounds.max().z > node.bounds[5]) return false;
	return true;
}

bool geometry::contains(const std::vector<repo::lib::RepoVector3D64>& vertices,
	const repo::lib::RepoBounds& bounds,
	const MeshView& mesh)
{
	// This method uses the SingleRayTraverser of the bvh library to peform the
	// ray-cast based point-in-mesh test:
	// 
	// If a point is inside a closed mesh, then ray-casting from that point in
	// any direction will result in an odd number of intersections. This works
	// for both convex and highly concave and complex objects.
	//
	// This method is compelling because it doesn't require holding the results
	// for all points in memory, and it can termintae as soon as one point is
	// identified to be outside the mesh. This can be made even more likely by
	// reordering the points so the most extreme values are tested first.

	// Before performing any tests, trivially check if this mesh can contain the
	// point set at all. If the bounds of the points do fit within the bounds of
	// the closed mesh, there is no way that all their vertices can be within
	// the surface as well.

	const auto& bvh = mesh.getBvh();

	if (!encapsulates(bvh.nodes[0], bounds)) {
		return false;
	}

	PrimitiveIntersector intersector(mesh);
	Traverser_t traverser(bvh);

	for (const auto& p : vertices) {
		Ray_t ray(reinterpret_cast<const Vector_t&>(p), Vector_t(1,0,0));

		intersector.numIntersections = 0;
		traverser.traverse(ray, intersector);

		if (intersector.numIntersections % 2) {

			return false; // This point is outside the mesh
		}
	}

	return true;
}

void geometry::reorderVertices(
	const std::vector<repo::lib::RepoVector3D64>& vertices)
{

}