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
#include <queue>

#include "clash_clearance.h"
#include "geometry_tests.h"
#include "geometry_tests_closed.h"
#include "geometry_utils.h"
#include "geometry_exceptions.h"
#include "bvh_operators.h"
#include "clash_scheduler.h"
#include "clash_node_cache.h"
#include "clash_pipelines_utils.h"

#include "repo/lib/datastructure/repo_matrix.h"
#include "repo/lib/datastructure/repo_triangle.h"
#include "repo/manipulator/modeloptimizer/bvh/bvh.hpp"

using namespace repo::manipulator::modelutility::clash;

namespace {
	struct Cached : public geometry::MeshView
	{
		Graph::Node* node;
		Bvh bvh;
		geometry::RepoIndexedMesh mesh;
		repo::lib::RepoBounds bounds;
		std::vector<size_t> indicesForContainsTests;
		bool isClosed = false;

		// Initialises everything the narrowphase (or this objects own methods) needs
		// to run. Initialise must be idempotent.

		void initialise(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler) {

			if (mesh.vertices.size()) { // (Already initialised)
				return;
			}

			geometry::RepoIndexedMeshBuilder builder(mesh);
			PipelineUtils::loadGeometry(handler, *node, builder);

			bounds = repo::lib::RepoBounds(mesh.vertices.data(), mesh.vertices.size());
			isClosed = geometry::isClosedAndManifold(mesh.faces);

			bvh::builders::build(bvh, mesh.vertices, mesh.faces);
		}

		const Bvh& getBvh() const override {
			return bvh;
		}

		repo::lib::RepoTriangle getTriangle(size_t primitive) const override {
			return mesh.getTriangle(primitive);
		}

		const repo::lib::RepoUUID& getCompositeObjectId() const {
			return node->compositeObject->id;
		}

		const std::vector<size_t>& getOrderedVertices() {
			if (!indicesForContainsTests.size()) {
				geometry::orderVertices(mesh.vertices, indicesForContainsTests);
			}
			return indicesForContainsTests;
		}
	};

	struct Cache : public ResourceCache<Graph::Node, Cached> 
	{
		void initialise(const Graph::Node& key, Cached& entry) const override {
			entry.node = const_cast<Graph::Node*>(&key); // We need to cast away const here in order to load the binary buffers into the contained bson
		}
	};

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
	struct ClearanceBroadphase : protected bvh::DistanceQuery
	{
		ClearanceBroadphase(double clearance) {
			this->d = clearance;
		}

		BroadphaseResults results;

		void operator()(const Bvh& a, const Bvh& b) {
			results.clear();
			bvh::DistanceQuery::operator()(a, b);
		}

		void operator()(const Bvh& a) {
			results.clear();
			bvh::DistanceQuery::operator()(a);
		}

		bool intersect(size_t primA, size_t primB) override {
			results.push_back({ primA, primB });
			return false; // Don't terminate traversal, we want to find all pairs within the tolerance
		}
	};
}

void Clearance::run(const Graph& graphA, const Graph& graphB, const Graph& graphC)
{
	Cache cache;
	ClearanceBroadphase broadphase(tolerance);
	std::vector<std::pair<Cache::Record*, Cache::Record*>> broadphaseResults;

	// The broadphase is run three times: between A and B, within A, and  within B.
	// A single node may appear in both intra and inter set tests, so we collect
	// and schedule all of these as one.

	auto interBroadphase = [&](const Graph& gA, const Graph& gB) {
		broadphase.operator()(gA.bvh, gB.bvh);
		for (auto [a, b] : broadphase.results) {
			broadphaseResults.push_back({
				cache.get(gA.getNode(a)),
				cache.get(gB.getNode(b))
			});
		}
	};

	interBroadphase(graphA, graphB);
	interBroadphase(graphA, graphC);
	interBroadphase(graphB, graphC);

	auto intraBroadphase = [&](const Graph& graph) {
		broadphase.operator()(graph.bvh);
		for(auto [a, b] : broadphase.results) {
			broadphaseResults.push_back({
				cache.get(graph.getNode(a)),
				cache.get(graph.getNode(b))
			});
		}
	};

	if(config.selfIntersectsA) {
		intraBroadphase(graphA);
	}

	if (config.selfIntersectsB) {
		intraBroadphase(graphB);
	}

	intraBroadphase(graphC);

	ClashScheduler::schedule(broadphaseResults);

	// Create reference counted equivalents of the records output by the broadphase.
	// Now, as the narrowphase tests are performed, the records will be fully
	// initialised and cleaned up automatically.

	using Narrowphase = std::pair<
		Cache::Entry,
		Cache::Entry
	>;

	std::queue<Narrowphase> narrowphaseTests;

	for (auto r : broadphaseResults) {
		narrowphaseTests.push({
			r.first->getReference(),
			r.second->getReference()
		});
	}

	while(!narrowphaseTests.empty()) {
		auto [a, b] = std::move(narrowphaseTests.front());
		narrowphaseTests.pop();

		a->initialise(handler);
		b->initialise(handler);

		if (a->bounds > b->bounds) {
			std::swap(a, b);
		}

		try
		{
			if (b->isClosed && geometry::contains(a->mesh.vertices, a->getOrderedVertices(), a->bounds, *b)) {
				// If a is completely inside b, the closest distance is zero so we can 
				// terminate immediately.

				createClash<ClearanceClash>(
					a->getCompositeObjectId(),
					b->getCompositeObjectId()
				)->append({ a->bounds.center(), a->bounds.center() });
				continue;
			}

			broadphase.operator()(a->getBvh(), b->getBvh());
			for (const auto& [aIndex, bIndex] : broadphase.results)
			{
				auto line = geometry::closestPoints(a->getTriangle(aIndex), b->getTriangle(bIndex));
				if (line.magnitude() < tolerance) {
					createClash<ClearanceClash>(
						a->getCompositeObjectId(),
						b->getCompositeObjectId()
					)->append(line);
				}
			}
		}
		catch (const geometry::GeometryTestException& e) {
			throw DegenerateTestException(a->getCompositeObjectId(), b->getCompositeObjectId(), e.what());
		}
	}
}

void Clearance::ClearanceClash::append(const repo::lib::RepoLine& otherLine)
{
	if (otherLine.magnitude() < line.magnitude()) {
		line = otherLine;
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
		hash ^= hasher(p.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= hasher(p.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= hasher(p.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}
	result.fingerprint = hash;
}