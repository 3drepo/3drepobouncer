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

#include "clash_hard.h"
#include "geometry_tests.h"
#include "bvh_operators.h"
#include "repo_polydepth.h"
#include "clash_scheduler.h"
#include "repo_polydepth.h"

#include <stack>
#include <set>

using namespace repo::manipulator::modelutility::clash;

/*
* The Hard broadphase returns all bounds that have an overlap, as this is a
* necessary condition for them to begin in an intersecting state. The overlap
* must also exceed the tolerance, otherwise the leaf geometry cannot intersect
* by more than that.
*/
struct HardBroadphase : public bvh::IntersectQuery
{
	HardBroadphase()
		:results(nullptr)
	{
	}

	BroadphaseResults* results;

	void intersect(size_t primA, size_t primB) override
	{
		results->push_back({ primA, primB });
	}

	void operator()(const Bvh& a, const Bvh& b, BroadphaseResults& results)
	{
		this->results = &results;
		bvh::IntersectQuery::operator()(a, b);
	}
};

void Hard::run(const Graph& graphA, const Graph& graphB)
{
	// Broadphase results are individual mesh nodes that potentially intersect.
	// From these we need to build the full composite objects for the narrow-
	// phase, which will work out penetration depth - if any - based on all the
	// meshes in the composite object.

	BroadphaseResults broadphaseResults;

	HardBroadphase b;
	b.operator()(graphA.bvh, graphB.bvh, broadphaseResults);

	std::set<std::pair<size_t, size_t>> compositePairs;
	for (auto [a, b] : broadphaseResults) {
		compositePairs.insert({
			graphA.getCompositeIndex(a),
			graphB.getCompositeIndex(b)
		});
	}

	std::vector<std::pair<size_t, size_t>> orderedCompositePairs(
		compositePairs.begin(),
		compositePairs.end()
	);

	ClashScheduler::schedule(orderedCompositePairs);
	
	std::vector<std::pair<
		std::shared_ptr<CompositeObjectCache>,
		std::shared_ptr<CompositeObjectCache>
		>> narrowphaseTests;

	for (auto [a, b] : orderedCompositePairs) {
		narrowphaseTests.push_back({
			getCompositeObjectCache(config.setA, a),
			getCompositeObjectCache(config.setB, b),
		});
	}

	for (auto& n : narrowphaseTests) {
		geometry::RepoPolyDepth pd(
			n.first->getTriangles(),
			n.second->getTriangles()
		);

		pd.iterate(20);

		auto v = pd.getPenetrationVector();

		if (v.norm() > FLT_EPSILON) {
			auto clash = createClash<HardClash>(
				n.first->id(),
				n.second->id()
			);
			clash->penetration = pd.getPenetrationVector();
		}
	}
}

void Hard::createClashReport(const OrderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const
{
	result.idA = objects.a;
	result.idB = objects.b;

	auto p = static_cast<const HardClash&>(clash).penetration;

	result.positions = {
	};

	size_t hash = 0;
	std::hash<double> hasher;
	hash ^= hasher(p.x) + 0x9e3779b9;
	hash ^= hasher(p.y) + 0x9e3779b9;
	hash ^= hasher(p.z) + 0x9e3779b9;
	result.fingerprint = hash;
}