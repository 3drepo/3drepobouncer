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

#include "geometry_tests_closed.h"
#include "geometry_exceptions.h"

#include <random>
#include <cmath> 
#include <algorithm>

#include "repo/manipulator/modeloptimizer/bvh/single_ray_traverser.hpp"
#include "repo/manipulator/modelutility/clashdetection/geometry_tests.h"

using namespace geometry;
using namespace bvh;

using Ray_t = bvh::Ray<double>;
using Vector_t = bvh::Vector3<double>;
using Traverser_t = SingleRayTraverser<bvh::Bvh<double>, 64, bvh::RobustNodeIntersector<bvh::Bvh<double>>>;

#define DEGEN_RETRY_LIMIT 10

namespace {
	struct PrimitiveIntersector
	{
		const MeshView& mesh;
		const bvh::Bvh<double>& bvh = mesh.getBvh();

		/*
		* This is used with the return value of intersect() to control the early
		* termination of the traversal. We leave it is true so intersects decides
		* when the traversal should end.
		*/
		bool any_hit = true;

		bool degenerate = false;

		size_t numFaces = 0;
		size_t numEdges = 0;
		size_t numVertices = 0;

		PrimitiveIntersector(const MeshView& mesh) :
			mesh(mesh)
		{
		}

		void reset() {
			numFaces = 0;
			numEdges = 0;
			numVertices = 0;
			degenerate = false;
		}

		size_t numFeatures()
		{
			return numFaces + numEdges / 2 + numVertices / 3;
		}

		struct Result
		{
			// The library Single Ray Traverser will use this to update the ray's tmax
			// property. This value *will* be used to cull bounding boxes, so ensure it
			// is always larger than a possible intersection with any bounds that might
			// be important to count.

			double distance() {
				return DBL_MAX;
			}
		};

		Result dummy;

		/*
		* Returns the result of the leaf intersection which the Single Ray Traverser
		* will use to update the best hit. We don't care about the actual hits, so
		* this always returns nullopt, ensuring the traversal continues until the end.
		*/
		std::optional<Result> intersect(size_t i, const Ray_t& ray) {
			auto edges = 0;
			auto t = geometry::intersects(
				reinterpret_cast<const repo::lib::RepoVector3D64&>(ray.origin),
				reinterpret_cast<const repo::lib::RepoVector3D64&>(ray.direction),
				mesh.getTriangle(bvh.primitive_indices[i]),
				&edges,
				&degenerate);

			if (degenerate) {
				// If the test is degenerate, then we should exit right away and instruct
				// the contains method to try again for this vertex with another ray.
				return dummy;
			}

			if (t >= 0 && t < std::numeric_limits<double>::infinity()) {
				if(edges == 0) {
					numFaces++;
				}
				else if (edges == 1) {
					numEdges++;
				}
				else if (edges == 2) {
					numVertices++;
				}
				else {
					throw GeometryTestException("Ray-triangle intersection reported invalid number of edges hit.");
				}
			}

			// We have recorded the intersection; signal to the traverser to continue
			// until we have explored the whole tree.

			return std::nullopt;
		}
	};

	bool encapsulates(const bvh::Bvh<double>::Node& node, const repo::lib::RepoBounds& bounds)
	{
		if (bounds.min().x < node.bounds[0]) return false;
		if (bounds.max().x > node.bounds[1]) return false;
		if (bounds.min().y < node.bounds[2]) return false;
		if (bounds.max().y > node.bounds[3]) return false;
		if (bounds.min().z < node.bounds[4]) return false;
		if (bounds.max().z > node.bounds[5]) return false;
		return true;
	}

	struct RandomGenerator
	{
		std::random_device rd;
		std::mt19937 gen;
		std::uniform_real_distribution<double> dist;
		
		RandomGenerator() : gen(rd()), dist(-1.0, 1.0) 
		{
		}
		
		repo::lib::RepoVector3D64 direction() {
			double x = dist(gen);
			double y = dist(gen);
			double z = dist(gen);
			return repo::lib::RepoVector3D64(x, y, z).normalized();
		}
	};
}

bool geometry::contains(const std::vector<repo::lib::RepoVector3D64>& vertices,
	const repo::lib::RepoBounds& bounds,
	const MeshView& mesh,
	const repo::lib::RepoVector3D64 offset)
{
	static RandomGenerator random;

	// This method uses the SingleRayTraverser of the bvh library to peform the
	// ray-cast based point-in-mesh test:
	// 
	// If a point is inside a closed mesh, then ray-casting from that point in
	// any direction will result in an odd number of intersections. This works
	// for both convex and highly concave and complex objects.
	//
	// This method is compelling because it doesn't require holding the results
	// for all points in memory, and it can terminate as soon as one point is
	// identified to be outside the mesh. This can be made even more likely by
	// reordering the points so the most extreme values are tested first.

	// Before performing any tests, trivially check if this mesh can contain the
	// point set at all. If the bounds of the points do fit within the bounds of
	// the closed mesh, there is no way that all their vertices can be within
	// the surface as well.

	const auto& bvh = mesh.getBvh();

	if (!encapsulates(bvh.nodes[0], bounds + offset)) {
		return false;
	}

	PrimitiveIntersector intersector(mesh);
	Traverser_t traverser(bvh);

	auto d = random.direction();

	for (const auto& p : vertices) {	
		size_t retryCounter = 0;
		while(true) {
			Ray_t ray(reinterpret_cast<const Vector_t&>(p), reinterpret_cast<const Vector_t&>(d));
			ray.origin += reinterpret_cast<const Vector_t&>(offset);
			intersector.reset();
			traverser.traverse(ray, intersector);

			if (!intersector.degenerate) {
				break;
			}

			// If the traversal terminated because of a degenerate test, we cannot make
			// a reliable assertion about this point using this ray, so make another
			// one to try again.

			d = random.direction();

			if (retryCounter++ > DEGEN_RETRY_LIMIT) {
				// In normal use, finding a degenerate test for a randomly sampled ray is
				// extremely unlikely. To do so 10 times is effectively impossible and
				// suggests something is wrong with the mesh.

				// This exception should be caught upstream and the mesh info appended for
				// reporting back to the user.

				throw GeometryTestException("Degenerate ray-triangle retry limit exceeded");
			}
		}

		if (intersector.numFeatures() % 2 == 0) {
			return false; // This point is outside the mesh. No need to test further.
		}
	}

	return true;
}

namespace {
	double score(const repo::lib::RepoVector3D64& v) {
		return std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z)});
	}

	size_t group(const repo::lib::RepoVector3D64& v) {
		auto ax = std::abs(v.x);
		auto ay = std::abs(v.y);
		auto az = std::abs(v.z);

		if (ax >= ay && ax >= az) {
			return (v.x >= 0.0) ? 0 : 1;
		}
		if (ay >= ax && ay >= az) {
			return (v.y >= 0.0) ? 2 : 3;
		}
		return (v.z >= 0.0) ? 4 : 5;
	}		
}

void geometry::reorderVertices(
	std::vector<repo::lib::RepoVector3D64>& vertices)
{
	// This method implements a sorting algorithm that orders the vertices by
	// greatest extent in one of six directions. The directions are interleaved,
	// meaning the most extreme vertices in all directions are tested first.

	// This component-wise algorithm is very quick, though requires O(n) temporary
	// allocations.

	// Six groups / directions: // +X, -X, +Y, -Y, +Z, -Z

	size_t count[6] = { 0, 0, 0, 0, 0, 0 };
	for (const auto& v : vertices) {
		count[group(v)]++;
	}

	// (The grouping can actually be performed in-place with swap, however a
	// temporary buffer is required to interleave the results, so we may as well
	// make the implementation simpler by using different allocations per-group
	// from the start.)

	std::vector<repo::lib::RepoVector3D64> groups[6];

	for (size_t g = 0; g < 6; g++) {
		groups[g].reserve(count[g]);
	}

	for (auto& v : vertices) {
		groups[group(v)].push_back(v);
	}

	for (size_t g = 0; g < 6; g++) {
		std::sort(groups[g].begin(), groups[g].end(),
			[&](const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b) {
				return score(a) > score(b);
			}
		);
	}

	size_t idx[6] = { 0, 0, 0, 0, 0, 0 };
	size_t i = 0;
	while (true) {
		bool hasElementsRemaining = false;
		for (size_t g = 0; g < 6; g++) {
			if (idx[g] < groups[g].size()) {
				vertices[i++] = groups[g][idx[g]++];
				hasElementsRemaining = true;
			}
		}
		if (!hasElementsRemaining) {
			break;
		}
	}
}
