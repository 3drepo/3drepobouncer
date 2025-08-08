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
#include <repo/core/model/bson/repo_node_streaming_mesh.h>

#include <repo/manipulator/modeloptimizer/bvh/bvh.hpp>
#include <repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp>

using namespace repo::lib;
using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

using ContainerGroups = std::unordered_map<const repo::lib::Container*, std::vector<repo::lib::RepoUUID>>;
using Bvh = bvh::Bvh<double>;


static inline bvh::BoundingBox<double> operator*(const RepoMatrix64& matrix, const repo::lib::RepoBounds& bounds)
{
	throw std::exception("not implemented");
}

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
			boundingBoxes.push_back(node.matrix * node.mesh.getBoundsField(REPO_NODE_MESH_LABEL_BOUNDING_BOX));
			centers.push_back(boundingBoxes.back().center());
		}

		auto globalBounds = bvh::compute_bounding_boxes_union(boundingBoxes.data(), boundingBoxes.size());

		bvh::SweepSahBuilder<Bvh> builder(bvh);
		builder.max_leaf_size = 1;
		builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());
	}

	const sparse::Node& getNode(size_t bvhNodeIdx) const
	{
		return meshes[bvh.primitive_indices[bvh.nodes[bvhNodeIdx].first_child_or_primitive]];
	}
};

/*
* Non - intrusive convenience type that stores some information about the mesh
* with the Bvh
*/
class MeshNodeBvh : public Bvh
{
public:
	// This assumes the node has already been transformed into the correct
	// coordinate system.
	MeshNodeBvh(const repo::core::model::StreamingMeshNode& node)
		: Bvh(),
		mesh(node)
	{
		auto boundingBoxes = std::vector<bvh::BoundingBox<double>>();
		auto centers = std::vector<bvh::Vector3<double>>();

		auto vertices = mesh.getLoadedVertices();
		for (const auto& face : mesh.getLoadedFaces())
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

		bvh::SweepSahBuilder<Bvh> builder(*this);
		builder.max_leaf_size = 1;
		builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());
	}

	const repo::core::model::StreamingMeshNode& mesh;

	repo::lib::RepoTriangle getTriangle(int nodeIdx) const
	{
		auto node = nodes[nodeIdx];
		auto primitive = primitive_indices[node.first_child_or_primitive];
		auto& face = mesh.getLoadedFaces()[primitive];
		if (face.sides < 3) {
			throw std::runtime_error("Clash detection engine only supports Triangles.");
		}
		auto& vertices = mesh.getLoadedVertices();
		return repo::lib::RepoTriangle(
			vertices[face[0]],
			vertices[face[1]],
			vertices[face[2]]
		);
	}
};

Graph createSceneGraph(
	Pipeline::DatabasePtr handler, 
	const std::vector<CompositeObject>& set, 
	Pipeline::RepoUUIDMap uniqueToCompositeId)
{
	ContainerGroups containers;
	for (auto& composite : set)
	{
		for (auto& mesh : composite.meshes)
		{
			containers[&mesh.container].push_back(mesh.uniqueId);
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

Pipeline::Pipeline(
	DatabasePtr handler, const repo::manipulator::modelutility::ClashDetectionConfig& config)
	: handler(handler),
	config(config)
{
}

ClashDetectionReport Pipeline::runPipeline()
{
	auto graphA = createSceneGraph(handler, config.setA, uniqueToCompositeId);
	auto graphB = createSceneGraph(handler, config.setB, uniqueToCompositeId);

	BroadphaseResults broadphaseResults;
	broadphase(graphA.bvh, graphB.bvh, broadphaseResults);

	for (const auto& [aIndex, bIndex] : broadphaseResults)
	{
		const auto& a = graphA.getNode(aIndex);
		const auto& b = graphB.getNode(bIndex);

		// For now, we do the actual tests in Project Coordinates, even if the
		// offset may be quite large.
		// If experiments show we are vulnerable to numerical errors, we could
		// move them into a common space by computing the difference between
		// the matrices of the two nodes, then transforming the bounds of b by
		// this. The reference for the final results would be a.

		// Load the supermeshing data

		// todo...

		// Put the meshes in project coordinates

		MeshNodeBvh bvhA(a.mesh);
		MeshNodeBvh bvhB(b.mesh);

		BroadphaseResults results;
		broadphase(bvhA, bvhB, results);

		for (const auto& [aIndex, bIndex] : results)
		{
			auto c = createNarrowphaseResult();

			if (narrowphase(
				bvhA.getTriangle(aIndex),
				bvhB.getTriangle(bIndex),
				*c
			))
			{
				UnorderedPair pair(
					uniqueToCompositeId[a.uniqueId],
					uniqueToCompositeId[b.uniqueId]
				);

				auto it = clashes.find(pair);
				if(it == clashes.end())
				{
					auto clash = createCompositeClash();
					it = clashes.emplace(pair, std::move(clash)).first;
				}

				append(*it->second, *c);
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
	}
}