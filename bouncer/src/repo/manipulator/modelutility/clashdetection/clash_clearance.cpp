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

#include "clash_clearance.h"
#include "geometry_tests.h"

using namespace repo::manipulator::modelutility::clash;

bvh::BoundingBox<double> intersection(const bvh::BoundingBox<double>& a, const bvh::BoundingBox<double>& b)
{
	bvh::BoundingBox<double> result;
	result.min = {
		std::max(a.min[0], b.min[0]),
		std::max(a.min[1], b.min[1]),
		std::max(a.min[2], b.min[2])
	};
	result.max = {
		std::min(a.max[0], b.max[0]),
		std::min(a.max[1], b.max[1]),
		std::min(a.max[2], b.max[2])
	};
	return result;
}

double minDistanceSq(const bvh::BoundingBox<double>& a, const bvh::BoundingBox<double>& b)
{
	auto ib = intersection(a, b);
	double sq = 0.0;
	for (auto i = 0; i < 3; ++i) {
		if (ib.min[i] > ib.max[i]) {
			sq += std::pow(ib.min[i] - ib.max[i], 2);
		}
	}
	return sq;
}

/*
* The Clearance broadphase returns all bounds that have a minimum distance
* smaller than the tolerance.
* Even if two bounds are intersecting, it does not necessarily mean that the
* primitives within them have a distance of zero. The best we can say is that
* the closest two primitives will definitely have a distance smaller than the
* running smallest distance between the maximum distance between any two bounds.
* Therefore this test is conservative and returns all potential pairs. If a user
* is only ever interested in the closest pair, then early termination based on
* tracking the smallest maxmimum distance could be applied.
*/
struct ClearanceBroadphase: public Broadphase
{
	ClearanceBroadphase(double clearance)
		:clearanceSq(clearance* clearance)
	{
	}

	double clearanceSq;

	std::stack<std::pair<size_t, size_t>> pairs;

	void operator()(const Bvh& a, const Bvh& b, std::vector<std::pair<int, int>>& results) override
	{
		pairs.push({ 0, 0 });
		while (!pairs.empty())
		{
			auto [idxLeft, idxRight] = pairs.top();
			pairs.pop();

			auto& left = a.nodes[idxLeft];
			auto& right = b.nodes[idxRight];

			auto mds = minDistanceSq(left.bounding_box_proxy(), right.bounding_box_proxy());

			if (mds > clearanceSq)
			{
				continue; // No possible intersection within the tolerance
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

struct ClearanceNarrowphase: public Narrowphase
{
	ClearanceNarrowphase(double tolerance)
		:tolerance(tolerance),
		line(repo::lib::RepoLine::Max())
	{
	}

	double tolerance;
	repo::lib::RepoLine line;

	bool operator()(const repo::lib::RepoTriangle& a, const repo::lib::RepoTriangle& b) override
	{
		line = geometry::closestPointTriangleTriangle(a, b);
		auto m = line.magnitude();
		return m <= tolerance;
	}
};

std::unique_ptr<Broadphase> Clearance::createBroadphase() const
{
	return std::make_unique<ClearanceBroadphase>(tolerance);
}

std::unique_ptr<Narrowphase> Clearance::createNarrowphase() const
{
	return std::make_unique<ClearanceNarrowphase>(tolerance);
}

void Clearance::append(CompositeClash& c, const Narrowphase& r) const
{
	auto& clash = static_cast<ClearanceClash&>(c);
	auto& result = static_cast<const ClearanceNarrowphase&>(r);
	if (result.line.magnitude() < clash.line.magnitude()) {
		clash.line = result.line;
	}
}

void Clearance::createClashReport(const OrderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const
{
	result.idA = objects.a;
	result.idB = objects.b;

	result.positions = {
		static_cast<const ClearanceClash&>(clash).line.start,
		static_cast<const ClearanceClash&>(clash).line.end
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