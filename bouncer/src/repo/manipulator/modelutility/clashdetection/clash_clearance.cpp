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
#include "bvh_operators.h"

using namespace repo::manipulator::modelutility::clash;

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
struct ClearanceBroadphase: public Broadphase, protected bvh::DistanceQuery
{
	ClearanceBroadphase(double clearance)
		:results(nullptr)
	{
		this->d = clearance;
	}

	BroadphaseResults* results;

	void operator()(const Bvh& a, const Bvh& b, BroadphaseResults& results) override
	{
		this->results = &results;
		bvh::DistanceQuery::operator()(a, b);
	}

	void intersect(size_t primA, size_t primB) override
	{
		results->push_back({ primA, primB});
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