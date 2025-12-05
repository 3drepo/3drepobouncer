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
#include "clash_node_cache.h"
#include "clash_pipelines_utils.h"

#include <set>
#include <queue>

using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

namespace {
	static struct CacheEntry
	{
		std::vector<Graph::Node*> nodes;
		std::vector<repo::lib::RepoTriangle> triangles;
		repo::lib::RepoUUID id;

		void initialise(DatabasePtr handler) {
			std::vector<repo::lib::RepoVector3D64> vertices;
			std::vector<repo::lib::repo_face_t> faces;
			for (auto& node : nodes) {
				vertices.clear();
				faces.clear();
				PipelineUtils::loadGeometry(handler, *node, vertices, faces);
				std::transform(faces.begin(), faces.end(), std::back_inserter(triangles),
					[&](auto& face) {
						return repo::lib::RepoTriangle(
							vertices[face[0]],
							vertices[face[1]],
							vertices[face[2]]
						);
					}
				);
			}
			nodes.clear();
		}

		const std::vector<repo::lib::RepoTriangle>& getTriangles() const {
			return triangles;
		}

		const repo::lib::RepoUUID& getId() const {
			return id;
		}
	};

	static struct Cache : public ResourceCache<CompositeObject, CacheEntry>
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

		void intersect(size_t primA, size_t primB) override {
			results.push_back({ primA, primB });
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

void Hard::run(const Graph& graphA, const Graph& graphB)
{
	// Broadphase results are individual mesh nodes that potentially intersect.
	// From these we need to build the full composite objects for the narrow-
	// phase, which will work out penetration depth - if any - based on all the
	// meshes in the composite object.

	Cache cacheA(graphA);
	Cache cacheB(graphB);

	std::set<std::pair<Cache::Record*, Cache::Record*>> compositePairs;

	HardBroadphase broadphase;

	broadphase.operator()(graphA.bvh, graphB.bvh);
	for (auto [a, b] : broadphase.results) {
		compositePairs.insert({
			cacheA.get(graphA.getCompositeObject(a)),
			cacheB.get(graphB.getCompositeObject(b))
		});
	}

	auto selfIntersectionBroadphase = [&](const Graph& graph, Cache& cache) {
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

	if (config.selfIntersectsA) {
		selfIntersectionBroadphase(graphA, cacheA);
	}

	if (config.selfIntersectsB) {
		selfIntersectionBroadphase(graphB, cacheB);
	}

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

		geometry::RepoPolyDepth pd(
			a->getTriangles(),
			b->getTriangles()
		);

		pd.iterate(20);

		auto v = pd.getPenetrationVector();

		if (v.norm() > tolerance) {
			auto clash = createClash<HardClash>(
				a->getId(),
				b->getId()
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