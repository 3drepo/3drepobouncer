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
	struct MeshView : public geometry::MeshView {
		const std::vector<repo::lib::RepoTriangle>& triangles;
		size_t start;
		size_t length;
		Bvh bvh;
		bool closed;

		MeshView(const std::vector<repo::lib::RepoTriangle>& triangles, 
			size_t start, size_t length, bool closed)
			: triangles(triangles), start(start), length(length), 
			closed(closed)
		{
		}

		const Bvh& getBvh() const override {
			return bvh;
		}

		repo::lib::RepoTriangle getTriangle(size_t primitive) const override {
			return triangles[start + primitive];
		}

		void buildBvh() {
			if (bvh.node_count == 0) {
				bvh::builders::build(bvh, &triangles[start], length);
			}
		}
	};

	struct CacheEntry
	{
		std::vector<Graph::Node*> nodes;
		std::vector<repo::lib::RepoTriangle> triangles;
		repo::lib::RepoUUID id;
		repo::lib::RepoBounds bounds;

		// Tracks the ranges of each MeshNode within the triangles array in case
		// we need to perform testing against a subset of triangles. Each view
		// holds a distinct BVH, but they are only initialised on-demand.

		std::vector<MeshView> meshViews;

		// The primary view that encompasses the whole Composite Object

		MeshView* view;

		// Views into the Composite Object of MeshNodes that are closed. If there is
		// only one node (and it is closed), this will be the same as [view].

		std::vector<MeshView*> closed;

		// All the vertices from all the nodes, ordered with the most extreme ones
		// first for more efficient point-wise testing.

		std::vector<repo::lib::RepoVector3D64> orderedVertices;

		void initialise(DatabasePtr handler) {

			if (!nodes.size()) { // (Already initialised)
				return;
			}

			std::vector<repo::lib::RepoVector3D64> vertices;
			std::vector<repo::lib::repo_face_t> faces;
			for (auto& node : nodes) {
				vertices.clear();
				faces.clear();
				
				PipelineUtils::loadGeometry(handler, *node, vertices, faces);
				
				std::for_each(
					vertices.begin(),
					vertices.end(),
					[&](auto& v) {
						bounds.encapsulate(v);
					}
				);

				// We take a copy of the vertices so they can be re-ordered to make pointwise
				// tests more efficient.

				orderedVertices.insert(orderedVertices.end(), vertices.begin(), vertices.end());

				meshViews.push_back(MeshView(
					triangles, 
					triangles.size(), 
					faces.size(), 
					geometry::isClosedAndManifold(faces))
				);

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

			// A view that encompasses the whole composite object - this will be the one
			// that holds the primary BVH. This composite view will never be used for
			// containment tests - only individual components may be used for that.

			if (nodes.size() > 1) {
				meshViews.push_back(MeshView(triangles, 0, triangles.size(), false));
			}

			// Create a quick reference of all the closed meshes - do this after the
			// meshViews vector has been fully populated so we can take references to its
			// elements.

			for (auto& mv : meshViews) {
				if (mv.closed) {
					closed.push_back(&mv);
				}
			}

			view = &meshViews.back();

			geometry::reorderVertices(orderedVertices);

			nodes.clear();
		}

		const std::vector<repo::lib::RepoTriangle>& getTriangles() const {
			return triangles;
		}

		const repo::lib::RepoUUID& getId() const {
			return id;
		}

		const std::vector<repo::lib::RepoVector3D64>& getOrderedVertices() const {
			return orderedVertices;
		}

		const std::vector<MeshView*>& getClosedMeshes() {
			for(auto c : closed) {
				c->buildBvh();
			}
			return closed;
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

	/*
	* The contains functor is a predicate which tells PolyDepth whether mesh (a)
	* is inside mesh (b) under transform/offset (m). If (b) does not have any
	* closed meshes, it returns false.
	*/
	struct ContainsFunctor : public geometry::RepoPolyDepth::ContainsFunctor
	{
		const std::vector<repo::lib::RepoVector3D64>& vertices;
		const repo::lib::RepoBounds& bounds;
		const std::vector<MeshView*>& closed;

		ContainsFunctor(CacheEntry& a, CacheEntry& b):
			vertices(a.getOrderedVertices()),
			bounds(a.bounds),
			closed(b.getClosedMeshes())
		{
		}

		bool operator()(const repo::lib::RepoVector3D64& m) const override {
			for (auto c : closed) {
				if (geometry::contains(vertices, bounds, *c, m)) {
					return true;
				}
			}
			return false;
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

		if (a->bounds > b->bounds) {
			std::swap(a, b);
		}

		try
		{
			// Always create the contains functor - if b has no closed meshes, the
			// operator will simply be a no-op.

			ContainsFunctor contains(*a, *b);

			geometry::RepoPolyDepth pd(
				a->getTriangles(),
				b->getTriangles(),
				&contains
			);

			pd.iterate(maxPolyDepthIterations);

			auto v = pd.getPenetrationVector();

			if (v.norm() > tolerance) {
				auto clash = createClash<HardClash>(
					a->getId(),
					b->getId()
				);
				clash->a = a->bounds.center();
				clash->b = b->bounds.center() + pd.getPenetrationVector();
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
	result.positions = {
		h.a,
		h.b
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