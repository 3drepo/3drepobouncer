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
#include "geometry_tests_closed.h"
#include "geometry_exceptions.h"
#include "geometry_utils.h"
#include "bvh_operators.h"
#include "repo_deformdepth.h"
#include "clash_scheduler.h"
#include "clash_node_cache.h"
#include "clash_pipelines_utils.h"

#include <set>
#include <queue>

using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

namespace {
	struct CacheEntry
	{
		std::vector<Graph::Node*> nodes;
		repo::lib::RepoUUID id;

		// Holds all the geometry for all nodes in one mesh. MeshViews can be
		// used to address the individual MeshNodes' geometry by range.

		geometry::RepoDeformDepth::Mesh mesh;

		void initialise(DatabasePtr handler) {

			if (!nodes.size()) { // (Already initialised)
				return;
			}

			geometry::RepoIndexedMeshBuilder builder(mesh);

			for (auto& node : nodes) {
				auto start = mesh.faces.size();
				PipelineUtils::loadGeometry(handler, *node, builder);

				// Keep a record of where this node's faces are in the combined mesh

				mesh.addFaceRange(start, mesh.faces.size());
			}

			// If we actually have multiple face-groups, create a primary group that
			// will be used for whole-mesh tests. By convention, this is the last
			// group added. (If we only have one, then don't do anything because then
			// we will just end up with two copies!)

			if (nodes.size() > 1) {
				mesh.addFaceRange(0, mesh.faces.size());
			}

			mesh.initialise();

			nodes.clear();
		}

		const repo::lib::RepoUUID& getId() const {
			return id;
		}

		repo::lib::RepoBounds getBounds() const {
			return mesh.bounds();
		}
	};

	struct Cache : public ResourceCache<CompositeObject, CacheEntry>
	{
		Cache(const Graph& graph)
			: graph(graph) {
		}

		const Graph& graph;

		void initialise(const CompositeObject& key, CacheEntry& entry) const override {
			for(auto& meshRef : key.meshes) {
				entry.nodes.push_back(&graph.getNode(meshRef.uniqueId));
			}
			entry.id = key.id;
		}
	};

	/*
	* The Hard broadphase returns all bounds that have an overlap, as this is a
	* necessary condition for them to begin in an intersecting state. The overlap
	* must also exceed the tolerance, otherwise the leaf geometry cannot intersect
	* by more than that.
	*/
	struct HardBroadphase : public bvh::IntersectQuery
	{
		BroadphaseResults results;

		bool intersect(size_t primA, size_t primB) override {
			results.push_back({ primA, primB });
			return false; // Don't terminate traversal, we want to find all overlaps
		}

		void operator()(const Bvh& a, const Bvh& b) {
			results.clear();
			bvh::IntersectQuery::operator()(a, b);
		}

		void operator()(const Bvh& a) {
			results.clear();
			bvh::IntersectQuery::operator()(a);
		}
	};
}

void Hard::run(const Graph& graphA, const Graph& graphB, const Graph& graphC)
{
	// Broadphase results are individual mesh nodes that potentially intersect.
	// From these we need to build the full composite objects for the narrow-
	// phase, which will work out penetration depth - if any - based on all the
	// meshes in the composite object.

	Cache cacheA(graphA);
	Cache cacheB(graphB);
	Cache cacheC(graphC);

	std::set<std::pair<Cache::Record*, Cache::Record*>> compositePairs;

	HardBroadphase broadphase;

	auto interBroadphase = [&](const Graph& gA, const Graph& gB,
		Cache& cA, Cache& cB)
	{
		broadphase.operator()(gA.bvh, gB.bvh);
		for (auto& [a, b] : broadphase.results) {
			compositePairs.insert({
				cA.get(gA.getCompositeObject(a)),
				cB.get(gB.getCompositeObject(b))
			});
		}
	};

	auto intraBroadphase = [&](const Graph& graph, Cache& cache) {
		broadphase.operator()(graph.bvh);
		for (auto& [a, b] : broadphase.results) {
			auto& compA = graph.getCompositeObject(a);
			auto& compB = graph.getCompositeObject(b);

			if(compA.id == compB.id) {
				continue;
			}

			compositePairs.insert({
				cache.get(compA),
				cache.get(compB)
			});
		}
	};

	interBroadphase(graphA, graphB, cacheA, cacheB);
	interBroadphase(graphA, graphC, cacheA, cacheC);
	interBroadphase(graphB, graphC, cacheB, cacheC);

	if (config.selfIntersectsA) {
		intraBroadphase(graphA, cacheA);
	}

	if (config.selfIntersectsB) {
		intraBroadphase(graphB, cacheB);
	}

	intraBroadphase(graphC, cacheC);

	std::vector<std::pair<Cache::Record*, Cache::Record*>> orderedCompositePairs(
		compositePairs.begin(),
		compositePairs.end()
	);

	ClashScheduler::schedule(orderedCompositePairs);

	// The following snippet turns the records into actual narrowphase tests by
	// resolving them to counted references to the actual nodes.

	std::queue<std::pair<
		Cache::Entry,
		Cache::Entry
		>> narrowphaseTests;

	for (auto [a, b] : orderedCompositePairs) {
		narrowphaseTests.push({
			a->getReference(),
			b->getReference()
		});
	}

	while(!narrowphaseTests.empty()) {
		auto [a, b] = std::move(narrowphaseTests.front());
		narrowphaseTests.pop();

		a->initialise(handler);
		b->initialise(handler);

		// In PolyDepth, b is fixed, so consider the larger object the static one
		// to make it easier to fit a into the free space around it.

		if (a->getBounds() > b->getBounds()) {
			std::swap(a, b);
		}

		try
		{
			geometry::RepoDeformDepth pd(
				a->mesh,
				b->mesh,
				tolerance
			);

			pd.iterate();

			auto v = pd.getPenetrationDepth();

			if (pd.getPenetrationDepth() > tolerance) {
				auto clash = createClash<HardClash>(
					a->getId(),
					b->getId()
				);
				clash->contacts = pd.getContactManifold();
			}
		}
		catch (const geometry::GeometryTestException& e) {
			throw DegenerateTestException(a->getId(), b->getId(), e.what());
		}
	}
}

void Hard::createClashReport(const OrderedPair& objects,
	const CompositeClash& clash, ClashDetectionResult& result) const
{
	result.idA = objects.a;
	result.idB = objects.b;

	auto h = static_cast<const HardClash&>(clash);
	result.positions = h.contacts;

	size_t hash = 0;
	std::hash<double> hasher;
	for (auto& p : result.positions) {
		hash ^= hasher(p.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= hasher(p.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= hasher(p.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}
	result.fingerprint = hash;
}