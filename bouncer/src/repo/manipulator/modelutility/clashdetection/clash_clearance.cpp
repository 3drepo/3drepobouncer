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
#include "bvh_operators.h"
#include "clash_scheduler.h"
#include "clash_node_cache.h"
#include "clash_pipelines_utils.h"

#include "repo/lib/datastructure/repo_matrix.h"
#include "repo/lib/datastructure/repo_triangle.h"
#include "repo/manipulator/modeloptimizer/bvh/bvh.hpp"
#include "repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp"

using namespace repo::manipulator::modelutility::clash;

namespace {
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
		ClearanceBroadphase(double clearance)
			:results(nullptr)
		{
			this->d = clearance;
		}

		BroadphaseResults* results;

		void operator()(const Bvh& a, const Bvh& b, BroadphaseResults& results)
		{
			this->results = &results;
			bvh::DistanceQuery::operator()(a, b);
		}

		void intersect(size_t primA, size_t primB) override
		{
			results->push_back({ primA, primB });
		}
	};

	struct Cached
	{
		Graph::Node* node;
		Bvh bvh;
		std::vector<repo::lib::RepoVector3D64> vertices;
		std::vector<repo::lib::repo_face_t> faces;

		void initialise(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler) {

			PipelineUtils::loadGeometry(handler, *node, vertices, faces);

			auto boundingBoxes = std::vector<bvh::BoundingBox<double>>();
			auto centers = std::vector<bvh::Vector3<double>>();

			for (const auto& face : faces)
			{
				auto bbox = bvh::BoundingBox<double>::empty();
				for (size_t i = 0; i < face.sides; i++) {
					auto v = vertices[face[i]];
					bbox.extend(bvh::Vector3<double>(v.x, v.y, v.z));
				}
				boundingBoxes.push_back(bbox);
				centers.push_back(boundingBoxes.back().center());
			}

			auto globalBounds = bvh::compute_bounding_boxes_union(boundingBoxes.data(), boundingBoxes.size());

			bvh::SweepSahBuilder<bvh::Bvh<double>> builder(bvh);
			builder.max_leaf_size = 1;
			builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());
		}

		const Bvh& getBvh() const {
			return bvh;
		}

		repo::lib::RepoTriangle getTriangle(size_t primitive) const {
			auto& face = faces[primitive];
			if (face.sides < 3) {
				throw std::runtime_error("Clash detection engine only supports Triangles.");
			}
			return repo::lib::RepoTriangle(
				vertices[face[0]],
				vertices[face[1]],
				vertices[face[2]]
			);
		}

		const size_t getCompositeObjectIndex() const {
			return node->compositeObjectIndex;
		}
	};

	struct Cache : public ResourceCache<const Graph::Node, Cached>
	{
		void initialise(const Graph::Node& key, Cached& entry) const override {
			entry.node = const_cast<Graph::Node*>(&key); // We need to cast away const here in order to load the binary buffers into the contained bson
		}
	};
}

void Clearance::run(const Graph& graphA, const Graph& graphB)
{
	BroadphaseResults broadphaseResults;

	ClearanceBroadphase broadphase(tolerance);
	broadphase.operator()(graphA.bvh, graphB.bvh, broadphaseResults);

	ClashScheduler::schedule(broadphaseResults);

	using Narrowphase = std::pair<
		Cache::Entry,
		Cache::Entry
	>;

	std::queue<Narrowphase> narrowphaseTests;

	Cache cache;

	for (auto r : broadphaseResults) {
		narrowphaseTests.push({
			cache.get(graphA.getNode(r.first)),
			cache.get(graphB.getNode(r.second))
		});
	}

	while(!narrowphaseTests.empty()) {
		auto [a, b] = std::move(narrowphaseTests.front());
		narrowphaseTests.pop();

		a->initialise(handler);
		b->initialise(handler);

		BroadphaseResults results;
		broadphase.operator()(a->getBvh(), b->getBvh(), results);

		for (const auto& [aIndex, bIndex] : results)
		{
			auto line = geometry::closestPointTriangleTriangle(a->getTriangle(aIndex), b->getTriangle(bIndex));
			if (line.magnitude() < tolerance) {
				auto clash = createClash<ClearanceClash>(
					config.setA[a->getCompositeObjectIndex()].id,
					config.setB[b->getCompositeObjectIndex()].id
				);
				clash->append(line);
			}
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
		hash ^= hasher(p.x) + 0x9e3779b9;
		hash ^= hasher(p.y) + 0x9e3779b9;
		hash ^= hasher(p.z) + 0x9e3779b9;
	}
	result.fingerprint = hash;
}