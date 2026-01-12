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

#include <unordered_map>
#include <unordered_set>

using namespace repo::lib;
using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

using ContainerGroups = std::unordered_map<repo::lib::Container*, std::vector<repo::lib::RepoUUID>>;

Graph::Graph(std::vector<Node> nodes)
	: meshes(std::move(nodes))
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

	for(size_t i = 0; i < meshes.size(); i++)
	{
		uuidToIndexMap[meshes[i].uniqueId] = i;
	}
}

Graph::Node& Graph::getNode(size_t primitive) const
{
	// The const_cast removes the const qualifier from the node itself. It is
	// important the vector doesn't change once initialised, as we don't want
	// the memory layout to change while there are references to the nodes
	// held. It is acceptable for the non-const members of the nodes themselves
	// to change however.

	return const_cast<Node&>(meshes[primitive]);
}

Graph::Node& Graph::getNode(const repo::lib::RepoUUID& uniqueId) const
{
	return getNode(uuidToIndexMap.at(uniqueId));
}

const CompositeObject& Graph::getCompositeObject(size_t primitive) const
{
	return *getNode(primitive).compositeObject;
}

namespace {
	std::unique_ptr<Graph> createSceneGraph(
		DatabasePtr handler,
		const std::vector<const CompositeObject*>& set)
	{
		ContainerGroups containers;
		std::unordered_map<repo::lib::RepoUUID, size_t, repo::lib::RepoUUIDHasher> idToIndexMap;

		for (size_t i = 0; i < set.size(); i++) {

			auto& composite = set[i];
			for (auto& mesh : composite->meshes)
			{
				containers[mesh.container].push_back(mesh.uniqueId);
				auto [it, inserted] = idToIndexMap.insert({ mesh.uniqueId, i });
				if (!inserted) {
					throw DuplicateMeshIdsException(mesh.uniqueId);
				}
			}
		}

		sparse::SceneGraph scene;
		for (auto& [container, uniqueIds] : containers)
		{
			scene.populate(handler, container, uniqueIds);
		}

		std::vector<Graph::Node> nodes;
		scene.getNodes(nodes);

		for (auto& node : nodes) {
			node.compositeObject = set[idToIndexMap[node.uniqueId]];
		}

		return std::make_unique<Graph>(std::move(nodes));
	}

	bool isWithinLimits(const repo::lib::RepoVector3D64& v, double limit)
	{
		return std::abs(v.x) <= limit && std::abs(v.y) <= limit && std::abs(v.z) <= limit;
	}

	bool isWithinLimits(const repo::lib::RepoBounds& bounds, double limit)
	{
		if (!isWithinLimits(bounds.min(), limit)) return false;
		if (!isWithinLimits(bounds.max(), limit)) return false;
		return true;
	}

	bool isWithinLimits(const Bvh::Node& node, double limit)
	{
		for (size_t i = 0; i < 6; i++) {
			if (std::abs(node.bounds[i]) > limit) {
				return false;
			}
		}
		return true;
	}

	void validateSceneGraph(const Graph& graph)
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

	struct CompositeObjectSets {
		std::vector<const CompositeObject*> a;
		std::vector<const CompositeObject*> b;
		std::vector<const CompositeObject*> c;
	};

	/*
	* Takes the two sets from the config and splits them into three disjoint ones,
	* where the third set contains the objects that were in both of the originals.
	* The original sets are not modified.
	*/
	void createDisjointSets(CompositeObjectSets& sets, 
		const repo::manipulator::modelutility::ClashDetectionConfig& config)
	{
		auto& setA = config.setA;
		auto& setB = config.setB;

		std::unordered_map<repo::lib::RepoUUID, const CompositeObject*, repo::lib::RepoUUIDHasher> mapA;
		std::unordered_set<repo::lib::RepoUUID, repo::lib::RepoUUIDHasher> intersection;

		for (auto& obj : setA) {
			mapA[obj.id] = &obj;
		}

		for (auto& obj : setB) {
			auto it = mapA.find(obj.id);
			if (it != mapA.end()) {
				intersection.insert(obj.id);
				sets.c.push_back(&obj); // In both
			}
			else {
				sets.b.push_back(&obj); // In B only
			}
		}

		for (auto& obj : setA) {
			if (intersection.find(obj.id) == intersection.end()) {
				sets.a.push_back(&obj); // In A only
			}
		}
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
	CompositeObjectSets sets;
	createDisjointSets(sets, config);

	auto graphA = createSceneGraph(handler, sets.a);
	auto graphB = createSceneGraph(handler, sets.b);
	auto graphC = createSceneGraph(handler, sets.c);

	validateSceneGraph(*graphA);
	validateSceneGraph(*graphB);
	validateSceneGraph(*graphC);

	run(*graphA, *graphB, *graphC);

	ClashDetectionReport report;
	report.clashes.reserve(clashes.size());
	for (auto& [key, clash] : clashes)
	{
		ClashDetectionResult r;
		createClashReport(key, *clash, r);
		delete clash;
		report.clashes.push_back(r);
	}

	return report;
}
