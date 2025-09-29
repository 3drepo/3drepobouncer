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

#include "clash_node_cache.h"

#include <repo/manipulator/modeloptimizer/bvh/bvh.hpp>
#include <repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp>

using namespace repo::manipulator::modelutility::clash;

NodeCache::NodeCache(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler):
	handler(handler)
{
}

std::shared_ptr<NodeCache::Node> NodeCache::getNode(repo::manipulator::modelutility::sparse::Node& sceneNode)
{
	return std::make_shared<NodeCache::Node>(*this, sceneNode);
}

NodeCache::Node::Node(NodeCache& cache, sparse::Node& sceneNode):
	cache(cache),
	sceneNode(sceneNode)
{
}

void NodeCache::Node::initialise()
{
	cache.handler->loadBinaryBuffers(
		sceneNode.container->teamspace,
		sceneNode.container->container + "." + REPO_COLLECTION_SCENE,
		sceneNode.mesh
	);

	// Put the meshes in project coordinates. We create new arrays below
	// as we want to keep the vertices in double precision from now on.

	auto mesh = repo::core::model::MeshNode(sceneNode.mesh);

	vertices.reserve(mesh.getNumVertices());
	for (const auto& v : mesh.getVertices())
	{
		auto vt = sceneNode.matrix * repo::lib::RepoVector3D64(v);
		vertices.push_back(vt);
	}

	faces = mesh.getFaces();

	sceneNode.mesh.unloadBinaryBuffers();

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

repo::lib::RepoTriangle NodeCache::Node::getTriangle(size_t primitive) const
{
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

const bvh::Bvh<double>& NodeCache::Node::getBvh() const
{
	return this->bvh;
}

const repo::lib::RepoUUID& NodeCache::Node::getUniqueId() const
{
	return sceneNode.uniqueId;
}
