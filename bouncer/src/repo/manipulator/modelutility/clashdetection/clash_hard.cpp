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
#include <thread>
#include <mutex>
#include <shared_mutex>

using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

namespace {
	struct CacheEntry
	{
		std::vector<Graph::Node*> nodes;
		repo::lib::RepoUUID id;
		std::shared_mutex mutex;

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
				mesh.addFaceRange(start, mesh.faces.size()); // Keep a record of where this node's faces are in the combined mesh
			}

			mesh.initialise();

			nodes.clear();
		}

		bool isInitialised()
		{
			return !nodes.size();
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

		void initialise(const CompositeObject& composite, CacheEntry& entry) const override {
			for(auto& meshRef : composite.meshes) {
				entry.nodes.push_back(&graph.getNode(meshRef.uniqueId));
			}
			entry.id = composite.id;
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

	using Narrowphase = std::pair<
		Cache::Entry,
		Cache::Entry
	>;

	// Prepare for splitting the samples accross queues for the threads
	const int numThreads = config.numThreads ? config.numThreads : std::thread::hardware_concurrency();
	std::vector<std::queue<Narrowphase>> narrowphaseQueues(numThreads);
	int parcelSize = std::ceil(orderedCompositePairs.size() / (double) numThreads);

	// When we take references to the records, we begin reference counting. From
	// this point on, when all the Cache::Entry objects for a given record are
	// destroyed, that record will be cleaned up.
	int sampleCount = 0;
	for (auto [a, b] : orderedCompositePairs) {
		int queueIndex = std::floor(sampleCount / parcelSize);
		narrowphaseQueues[queueIndex].push({
			a->getReference(),
			b->getReference()
		});
		sampleCount++;
	}

	// Mutexes
	std::mutex clashesMutex{};
	std::mutex cacheMutex{};

	// Define thread behaviour
	auto narrowPhaseThread = [&](std::queue<Narrowphase>& queue)
		{
			// Run intil we are out of samples, then terminate.
			while (!queue.empty()) {

				// Else, get sample from the queue exclusive to this thread
				auto [a, b] = std::move(queue.front());
				queue.pop();

				// Check initialisation status of entry a
				bool aInitialised = false;
				{
					// Shared lock on the object so we can check the status
					std::shared_lock lock{ a->mutex };
					aInitialised = a->isInitialised();
				}

				// If a is not initialised, do it now.
				if (!aInitialised)
				{
					// Lock exclusively
					std::scoped_lock entryLock{ a->mutex};

					// Now initialise the node
					a->initialise(handler);
				}

				// Check initialisation status of entry b
				bool bInitialised = false;
				{
					// Shared lock on the object so we can check the status
					std::shared_lock lock{ b->mutex };
					bInitialised = b->isInitialised();
				}

				// If b is not initialised, do it now.
				if (!bInitialised)
				{
					// Lock exclusively
					std::scoped_lock entryLock{b->mutex };

					// Now initialise the node
					b->initialise(handler);
				}

				// In DeformDepth, (b) is fixed, so consider the larger object the static one
				// to make it easier to fit (a) into the free space around it.
				if (a->getBounds() > b->getBounds()) {
					std::swap(a, b);
				}

				{
					// Acquire locks for both cache entries for the duration of the test.
					// This needs to be an exclusive lock since RepoDeformDepth alters the mesh
					std::scoped_lock lockEntries{ a->mutex, b->mutex };

					try
					{
						geometry::RepoDeformDepth pd(
							a->mesh,
							b->mesh,
							tolerance
						);

						double penDepth = pd.getPenetrationDepth();
						if (penDepth > tolerance) {
							// Lock the clashes map, then write the new clash
							std::scoped_lock lockClashes{ clashesMutex };
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

				// Explicitly delete the cache entries in a thread-safe environment
				{
					std::scoped_lock lockCache{ cacheMutex };
					a.reset();
					b.reset();
				}
			}
		};

	// Create and launch the threads
	std::vector<std::jthread> threads;
	for (int i = 0; i < numThreads; i++)
		threads.emplace_back(std::jthread{ narrowPhaseThread, std::ref(narrowphaseQueues[i]) });

	// Wait for all threads to complete
	for (auto& t : threads)
		t.join();
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