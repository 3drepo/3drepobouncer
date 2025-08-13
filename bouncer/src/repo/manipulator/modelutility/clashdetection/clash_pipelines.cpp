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

#include <repo/manipulator/modeloptimizer/bvh/bvh.hpp>
#include <repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp>

using namespace repo::lib;
using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

#pragma optimize("", off)

using ContainerGroups = std::unordered_map<repo::lib::Container*, std::vector<repo::lib::RepoUUID>>;
using Bvh = bvh::Bvh<double>;

static inline bvh::BoundingBox<double> operator*(const RepoMatrix64& matrix, const repo::lib::RepoBounds& bounds)
{
	repo::lib::RepoBounds b(
		matrix * bounds.min(),
		matrix * bounds.max()
	);
	return *reinterpret_cast<bvh::BoundingBox<double>*>(&b);
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

/*
* Non - intrusive convenience type that stores some information about the mesh
* with the Bvh
*/
class MeshNodeBvh : public Bvh
{
public:
	// This assumes the node has already been transformed into the correct
	// coordinate system.
	MeshNodeBvh(const repo::core::model::MeshNode& node)
		: Bvh(),
		mesh(node)
	{
		auto boundingBoxes = std::vector<bvh::BoundingBox<double>>();
		auto centers = std::vector<bvh::Vector3<double>>();

		auto vertices = mesh.getVertices();
		for (const auto& face : mesh.getFaces())
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

	const repo::core::model::MeshNode& mesh;

	repo::lib::RepoTriangle getTriangle(int primitive) const
	{
		auto& face = mesh.getFaces()[primitive];
		if (face.sides < 3) {
			throw std::runtime_error("Clash detection engine only supports Triangles.");
		}
		auto& vertices = mesh.getVertices();
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

	auto broadphase = createBroadphase();

	broadphase->operator()(graphA.bvh, graphB.bvh, broadphaseResults);

	for (const auto& [aIndex, bIndex] : broadphaseResults)
	{
		auto& a = graphA.getNode(aIndex);
		auto& b = graphB.getNode(bIndex);

		// For now, we do the actual tests in Project Coordinates, even if the
		// offset may be quite large.
		// If experiments show we are vulnerable to numerical errors, we could
		// move them into a common space by computing the difference between
		// the matrices of the two nodes, then transforming the bounds of b by
		// this. The reference for the final results would be a.

		// Load the binary data

		handler->loadBinaryBuffers(
			a.container->teamspace,
			a.container->container + "." + REPO_COLLECTION_SCENE,
			a.mesh
		);

		handler->loadBinaryBuffers(
			b.container->teamspace,
			b.container->container + "." + REPO_COLLECTION_SCENE,
			b.mesh
		);

		// Put the meshes in project coordinates

		auto nodeA = repo::core::model::MeshNode(a.mesh);
		nodeA.applyTransformation(a.matrix);
		auto nodeB = repo::core::model::MeshNode(b.mesh);
		nodeB.applyTransformation(b.matrix);

		MeshNodeBvh bvhA(nodeA);
		MeshNodeBvh bvhB(nodeB);

		BroadphaseResults results;
		broadphase->operator()(bvhA, bvhB, results);

		auto narrowphase = createNarrowphase();

		for (const auto& [aIndex, bIndex] : results)
		{
			if (narrowphase->operator()(
					bvhA.getTriangle(aIndex),
					bvhB.getTriangle(bIndex))
				)
			{
				OrderedPair pair(
					uniqueToCompositeId[a.uniqueId],
					uniqueToCompositeId[b.uniqueId]
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

		// Unload the binaries

		a.mesh.unloadBinaryBuffers();
		b.mesh.unloadBinaryBuffers();
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