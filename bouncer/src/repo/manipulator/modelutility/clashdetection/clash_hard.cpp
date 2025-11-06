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

#include <stack>

#include "clash_hard.h"
#include "geometry_tests.h"
#include "repo_polydepth.h"

using namespace repo::manipulator::modelutility::clash;

/*
* The Hard broadphase returns all bounds that have an overlap, as this is a
* necessary condition for them to begin in an intersecting state. The overlap
* must also exceed the tolerance, otherwise the leaf geometry cannot intersect
* by more than that.
*/
struct HardBroadphase: public Broadphase
{
	HardBroadphase(double tolerance)
		:toleranceSq(tolerance * tolerance)
	{
	}

	double toleranceSq;

	std::stack<std::pair<size_t, size_t>> pairs;

	void operator()(const Bvh& a, const Bvh& b, BroadphaseResults& results) override
	{
		pairs.push({ 0, 0 });
		while (!pairs.empty())
		{
			auto [idxLeft, idxRight] = pairs.top();
			pairs.pop();

			auto& left = a.nodes[idxLeft];
			auto& right = b.nodes[idxRight];

			// This snippet computes the overlap

			bvh::BoundingBox<double> result;
			result.min = {
				std::max(left.bounds[0], right.bounds[0]),
				std::max(left.bounds[1], right.bounds[1]),
				std::max(left.bounds[2], right.bounds[2])
			};
			result.max = {
				std::min(left.bounds[3], right.bounds[3]),
				std::min(left.bounds[4], right.bounds[4]),
				std::min(left.bounds[5], right.bounds[5])
			};

			if (result.max[0] < result.min[0] ||
				result.max[1] < result.min[1] ||
				result.max[2] < result.min[2])
			{
				continue; // No overlap
			}

			auto overlapSq = bvh::dot(result.diagonal(), result.diagonal());

			if (overlapSq < toleranceSq)
			{
				continue; // No possible intersection above the tolerance
			}

			if (left.is_leaf() && right.is_leaf())
			{
				for (size_t l = 0; l < left.primitive_count; l++) {
					for (size_t r = 0; r < right.primitive_count; r++) {
						results.push_back({
							a.primitive_indices[left.first_child_or_primitive + l],
							b.primitive_indices[right.first_child_or_primitive + r]
						});
					}
				}
			}
			else if (left.is_leaf() && !right.is_leaf())
			{
				pairs.push({ idxLeft, right.first_child_or_primitive + 0 });
				pairs.push({ idxLeft, right.first_child_or_primitive + 1 });
			}
			else if (!left.is_leaf() && right.is_leaf())
			{
				pairs.push({ left.first_child_or_primitive + 0, idxRight });
				pairs.push({ left.first_child_or_primitive + 1, idxRight });
			}
			else
			{
				pairs.push({ left.first_child_or_primitive + 0, right.first_child_or_primitive + 0 });
				pairs.push({ left.first_child_or_primitive + 0, right.first_child_or_primitive + 1 });
				pairs.push({ left.first_child_or_primitive + 1, right.first_child_or_primitive + 0 });
				pairs.push({ left.first_child_or_primitive + 1, right.first_child_or_primitive + 1 });
			}
		}
	}
};

struct HardNarrowphase : public Narrowphase
{
	bool operator()(const repo::lib::RepoTriangle& a, const repo::lib::RepoTriangle& b) override
	{
		// The true penetration depth cannot be estimated pair-wise in isolation,
		// because geometry of one mesh that is deepest within the other may not
		// intersect with that ones surface.
		// For the narrowphase, we simply check if the triangles are touching, and
		// then the depth estimation takes place in the composition step. This also
		// means we can tolerate some false-positives here (though not false
		// negatives).

		auto line = geometry::closestPointTriangleTriangle(a, b);
		auto m = line.magnitude();
		return m < FLT_EPSILON;
	}
};

std::unique_ptr<Broadphase> Hard::createBroadphase() const
{
	return std::make_unique<HardBroadphase>(tolerance);
}

std::unique_ptr<Narrowphase> Hard::createNarrowphase() const
{
	return std::make_unique<HardNarrowphase>();
}

void Hard::append(CompositeClash& c, const Narrowphase& r) const
{
	// We don't need to do anything here because the keys of the composite clash
	// hold the identities we need to recover.
}

void Hard::createClashReport(const OrderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const
{
	result.idA = objects.a;
	result.idB = objects.b;

	auto pd = estimatePenetrationDepth(objects);

	result.positions = {
	};

	size_t hash = 0;
	std::hash<double> hasher;
	for (auto& p : result.positions) {
		hash ^= hasher(p.x) + 0x9e3779b9;
		hash ^= hasher(p.y) + 0x9e3779b9;
		hash ^= hasher(p.z) + 0x9e3779b9;
	}
	result.fingerprint = hash;
}

double Hard::estimatePenetrationDepth(const OrderedPair& pair) const
{
	//todo: get all meshes from ordered pairs for polydepth

	//geometry::RepoPolyDepth pd()
	return 0;
}