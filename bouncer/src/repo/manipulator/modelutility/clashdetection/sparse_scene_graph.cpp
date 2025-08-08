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

#include "sparse_scene_graph.h"

#include <repo/lib/datastructure/repo_matrix.h>
#include <repo/lib/datastructure/repo_bounds.h>
#include <repo/core/handler/repo_database_handler_abstract.h>
#include <repo/core/handler/database/repo_query.h>
#include <repo/core/model/repo_model_global.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_node_transformation.h>
#include <repo/core/model/bson/repo_node_streaming_mesh.h>

using namespace repo::manipulator::modelutility::sparse;

void SceneGraph::populate(
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
	const repo::lib::Container* container, 
	const std::vector<repo::lib::RepoUUID>& uniqueIds
)
{
	// This method populates the sparse scene graph.

	// It attempts to balance the number of database requests by accessing each
	// level of the tree in one go, creating an upper bound on the number of
	// requests in the worst case, but not reading the entire tree just for one
	// or two nodes.

	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.includeField(REPO_NODE_LABEL_PARENTS);
	projection.includeField(REPO_NODE_LABEL_MATRIX);
	projection.includeField(REPO_NODE_LABEL_TYPE);
	projection.includeField(REPO_LABEL_BINARY_REFERENCE);
	projection.includeField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);

	std::unordered_map<repo::lib::RepoUUID, std::vector<repo::lib::RepoUUID>, repo::lib::RepoUUIDHasher> parentToChild; // by Shared Id

	using NodesToProcess = std::vector<repo::lib::RepoUUID>;

	// Initialise the job with the mesh nodes within this container.

	NodesToProcess ids = uniqueIds;

	// Nodes are delivered by their unique Ids, so when starting off use those.
	// Subsequent lookups use the shared Ids, as those are used to define
	// relationships in the database.

	bool useSharedId = false;

	while (ids.size()) {
		repo::core::handler::database::query::Eq query(useSharedId ? REPO_NODE_LABEL_SHARED_ID : REPO_NODE_LABEL_ID, ids);
		ids.clear();
		auto cursor = handler->findCursorByCriteria(
			container->teamspace,
			container->container,
			query,
			projection);

		for (auto& bson : (*cursor))
		{
			repo::core::model::RepoNode node(bson);

			auto type = bson.getStringField(REPO_NODE_LABEL_TYPE);
			if (type == REPO_NODE_TYPE_TRANSFORMATION)
			{
				auto transform = repo::core::model::TransformationNode(bson);

				// The sparse graph assumes a unidirectional tree.

				std::vector<repo::lib::RepoUUID>* children = nullptr;
				if (node.getParentIDs().size()) {
					children = &parentToChild[node.getParentIDs()[0]];
				}

				// Transformations are always processed bottom-up, so we can
				// pre-multiply

				for (auto& uniqueId : parentToChild[node.getSharedID()]) {
					auto& instance = nodes[node.getUniqueID()];
					instance.matrix = repo::lib::RepoMatrix64(transform.getTransMatrix()) * instance.matrix;

					// Transforms are not stored in the sparse graph - instead, associate the
					// premultiplied node with the current transforms parent, for when we get
					// round to processing it.

					if (children) {
						children->push_back(uniqueId);
					}
				}

				parentToChild.erase(node.getSharedID());

			} 
			else if (type == REPO_NODE_TYPE_MESH)
			{
				auto& instance = nodes[node.getUniqueID()];
				instance.container = container;
				instance.uniqueId = node.getUniqueID();
				instance.matrix = repo::lib::RepoMatrix64();
				instance.mesh = bson;
			} 

			for (auto& p : node.getParentIDs()) {
				ids.push_back(p);
			}
		}

		useSharedId = true;
	}

	// The final step is to premultiply the offset.
}

void SceneGraph::getNodes(std::vector<Node>& nodes) const
{
	for(auto& [id, node] : this->nodes)
	{
		sparse::Node n;
		n.container = node.container;
		n.uniqueId = node.uniqueId;
		n.matrix = node.matrix;
		n.mesh = node.mesh;

		nodes.push_back(node);
	}
}
