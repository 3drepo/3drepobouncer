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
#include "bvh_operators.h"
#include "repo_polydepth.h"

using namespace repo::manipulator::modelutility::clash;

/*
* The Hard broadphase returns all bounds that have an overlap, as this is a
* necessary condition for them to begin in an intersecting state. The overlap
* must also exceed the tolerance, otherwise the leaf geometry cannot intersect
* by more than that.
*/
struct HardBroadphase: public Broadphase, public bvh::IntersectQuery
{
	HardBroadphase(double tolerance)
		:results(nullptr)
	{
		this->tolerance = tolerance;
	}

	BroadphaseResults* results;

	void intersect(size_t primA, size_t primB) override
	{
		results->push_back({ primA, primB });
	}

	void operator()(const Bvh& a, const Bvh& b, BroadphaseResults& results) override
	{
		this->results = &results;
		bvh::IntersectQuery::operator()(a, b);
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

		return geometry::intersects(a, b) > 0;
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
	/*
	const auto& a = compositeObjects.at(pair.a);
	for (const auto& ref : a.meshes) {
		ref.

	}
	*/
	

	//todo: get all meshes from ordered pairs for polydepth

	//geometry::RepoPolyDepth pd()
	return 0;
}