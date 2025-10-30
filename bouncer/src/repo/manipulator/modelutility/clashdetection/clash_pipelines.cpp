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

#include "clash_pipelines.h"
#include "clash_scheduler.h"
#include "clash_node_cache.h"
#include "clash_exceptions.h"
#include "clash_constants.h"
#include "sparse_scene_graph.h"

#include <repo/lib/datastructure/repo_matrix.h>
#include <repo/lib/datastructure/repo_bounds.h>
#include <repo/lib/datastructure/repo_triangle.h>
#include <repo/core/handler/repo_database_handler_abstract.h>
#include <repo/core/handler/database/repo_query.h>
#include <repo/core/model/repo_model_global.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_node_transformation.h>

#include <repo/manipulator/modeloptimizer/bvh/bvh.hpp>
#include <repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp>

using namespace repo::lib;
using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

using ContainerGroups = std::unordered_map<repo::lib::Container*, std::vector<repo::lib::RepoUUID>>;
using Bvh = bvh::Bvh<double>;

struct Graph
{
	// Once this is initialised, it should not be changed, as the bvh will use
	// indices into this vector to refer to the nodes.
	const std::vector<sparse::Node> meshes;

	// The bvh for the graph stores bounds in Project Coordinates.
	Bvh bvh;

	Graph(std::vector<sparse::Node> nodes)
		:meshes(std::move(nodes))
	{
		// create the bvh for the scene
		auto boundingBoxes = std::vector<bvh::BoundingBox<double>>();
		auto centers = std::vector<bvh::Vector3<double>>();

		for (const auto& node : meshes)
		{
			auto bounds = node.matrix * node.mesh.getBoundsField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);
			boundingBoxes.push_back(*(reinterpret_cast<bvh::BoundingBox<double>*>(&bounds)));
			centers.push_back(boundingBoxes.back().center());
		}

		auto globalBounds = bvh::compute_bounding_boxes_union(boundingBoxes.data(), boundingBoxes.size());

		bvh::SweepSahBuilder<Bvh> builder(bvh);
		builder.max_leaf_size = 1;
		builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());
	}

	sparse::Node& getNode(size_t primitive) const
	{
		// The const_cast removes the const qualifier from the node itself. It is
		// important the vector doesn't change once initialised, as we don't want
		// the memory layout to change while there are references to the nodes
		// held. It is acceptable for the non-const members of the nodes themselves
		// to change however.

		return const_cast<sparse::Node&>(meshes[primitive]);
	}
};

static Graph createSceneGraph(
	Pipeline::DatabasePtr handler, 
	const std::vector<CompositeObject>& set, 
	Pipeline::RepoUUIDMap& uniqueToCompositeId)
{
	ContainerGroups containers;
	for (auto& composite : set)
	{
		for (auto& mesh : composite.meshes)
		{
			containers[mesh.container].push_back(mesh.uniqueId);
			uniqueToCompositeId[mesh.uniqueId] = composite.id;
		}
	}

	sparse::SceneGraph scene;
	for (auto& [container, uniqueIds] : containers)
	{
		scene.populate(handler, container, uniqueIds);
	}

	std::vector<sparse::Node> nodes;
	scene.getNodes(nodes);

	Graph graph(std::move(nodes));

	return graph;
}

static bool isWithinLimits(const repo::lib::RepoVector3D64& v, double limit)
{
	return std::abs(v.x) <= limit && std::abs(v.y) <= limit && std::abs(v.z) <= limit;
}

static bool isWithinLimits(const repo::lib::RepoBounds& bounds, double limit)
{
	if (!isWithinLimits(bounds.min(), limit)) return false;
	if (!isWithinLimits(bounds.max(), limit)) return false;
	return true;
}

static bool isWithinLimits(const Bvh::Node& node, double limit)
{
	for (size_t i = 0; i < 6; i++) {
		if (std::abs(node.bounds[i]) > limit) {
			return false;
		}
	}
	return true;
}

static bool validateSceneGraph(const Graph& graph)
{
	// This method traverses the scene graph looking for any nodes where (a) the
	// matrix has a translation of more than TRANSLATION_LIMIT (1e11), or the
	// scaled mesh bounds exceed MESH_LIMIT (8e6): these situations are not
	// supported, because they can lead to rounding errors beyond our guaranteed
	// accuracy range.

	std::stack<size_t> nodesToProcess;

	if (graph.meshes.size()) {
		nodesToProcess.push(0);
	}
	while (!nodesToProcess.empty()) 
	{
		const auto& node = graph.bvh.nodes[nodesToProcess.top()];
		nodesToProcess.pop();
		bvh::BoundingBox<double> bb = node.bounding_box_proxy();

		// The bvh is constructed in world space, so if a box is within the mesh limits,
		// we know the mesh and all transforms involved will be within limits without
		// further checks and can terminate early.

		if (!isWithinLimits(node, MESH_LIMIT)) {
			if (node.is_leaf()) {

				// For leaf nodes, check how the mesh bounds grow beyond the mesh limits and
				// ensure it is via an acceptable transform.

				for (auto i = 0; i < node.primitive_count; i++) {
					const auto& meshNode = graph.meshes[graph.bvh.primitive_indices[node.first_child_or_primitive + i]];
					
					auto meshBounds = meshNode.mesh.getBoundsField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);
					if (!isWithinLimits(meshBounds, MESH_LIMIT)) {
						throw MeshBoundsException(*meshNode.container, meshNode.uniqueId);
					}

					// For evaluating the effect on scale on the bounds, we must be cautious
					// that scale decomposition is potentially lossy.
					// As the purpose of this test is to ensure effective scale does not
					// introduce significant error into the single-precision vertices, we only
					// care about changes greater than the ULP of the supported range.

					auto scaledBounds = meshNode.matrix.scale() * meshBounds;
					if (!isWithinLimits(scaledBounds, MESH_LIMIT + 1)) {
						throw TransformBoundsException(*meshNode.container, meshNode.uniqueId);
					}

					if (!isWithinLimits(meshNode.matrix.translation(), TRANSLATION_LIMIT)) {
						throw TransformBoundsException(*meshNode.container, meshNode.uniqueId);
					}
				}
			}
			else {
				nodesToProcess.push(node.first_child_or_primitive);
				nodesToProcess.push(node.first_child_or_primitive + 1);
			}
		}
	}
}

static void validateSets(const repo::manipulator::modelutility::ClashDetectionConfig& config)
{
	// Checks for any overlaps. Currently this is not supported in any mode.

	std::set<repo::lib::RepoUUID> a;
	std::set<repo::lib::RepoUUID> b;

	auto getId = [](const repo::manipulator::modelutility::CompositeObject& obj) {
		return obj.id;
	};

	std::transform(config.setA.begin(), config.setA.end(), std::inserter(a, a.begin()), getId);
	std::transform(config.setB.begin(), config.setB.end(), std::inserter(b, b.begin()), getId);

	std::set<repo::lib::RepoUUID> intersection;

	std::set_intersection(
		a.begin(), a.end(),
		b.begin(), b.end(),
		std::inserter(intersection, intersection.begin())
	);

	if(!intersection.empty()) {
		throw OverlappingSetsException(intersection);
	}
}

Pipeline::Pipeline(
	DatabasePtr handler, const repo::manipulator::modelutility::ClashDetectionConfig& config)
	: handler(handler),
	config(config)
{
}

ClashDetectionReport Pipeline::runPipeline()
{
	validateSets(config);

	auto graphA = createSceneGraph(handler, config.setA, uniqueToCompositeId);
	auto graphB = createSceneGraph(handler, config.setB, uniqueToCompositeId);

	validateSceneGraph(graphA);
	validateSceneGraph(graphB);

	BroadphaseResults broadphaseResults;

	auto broadphase = createBroadphase();

	broadphase->operator()(graphA.bvh, graphB.bvh, broadphaseResults);

	ClashScheduler::schedule(broadphaseResults);

	NodeCache cache(handler);

	std::vector<std::pair<
		std::shared_ptr<NodeCache::Node>,
		std::shared_ptr<NodeCache::Node>
		>> narrowphaseTests;

	for (auto r : broadphaseResults) {
		narrowphaseTests.push_back({
			cache.getNode(graphA.getNode(r.first)),
			cache.getNode(graphB.getNode(r.second))
		});
	}

	for (const auto& [a, b] : narrowphaseTests)
	{
		a->initialise();
		b->initialise();

		// The current implementation of the pipeline uses double precision floating
		// point throughout. This is because the way the scene data is stored limits
		// the accuracy of the engine, and double precision exceeds the requirments
		// for our guaranteed range (as verified by unit tests). It is therefore the
		// most straightforward solution. If any of the above should change however, a
		// dedicated library such as MPIR should be used insted.

		BroadphaseResults results;
		broadphase->operator()(a->getBvh(), b->getBvh(), results);

		auto narrowphase = createNarrowphase();

		for (const auto& [aIndex, bIndex] : results)
		{
			if (narrowphase->operator()(
					a->getTriangle(aIndex),
					b->getTriangle(bIndex))
				)
			{
				OrderedPair pair(
					uniqueToCompositeId[a->getUniqueId()],
					uniqueToCompositeId[b->getUniqueId()]
				);

				auto it = clashes.find(pair);
				if(it == clashes.end())
				{
					auto clash = createCompositeClash();
					it = clashes.emplace(pair, std::move(clash)).first;
				}

				append(*it->second, *narrowphase);
			}
		}
	}

	ClashDetectionReport report;
	report.clashes.reserve(clashes.size());
	for (auto& [key, clash] : clashes)
	{
		ClashDetectionResult r;
		createClashReport(key, *clash, r);
		delete clash;
		report.clashes.push_back(std::move(r));
	}

	return report;
}