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
#include <repo/core/model/bson/repo_node_model_revision.h>
#include <repo/core/model/bson/repo_node_transformation.h>
#include <repo/core/model/bson/repo_node_streaming_mesh.h>
#include <repo/core/model/bson/repo_bson_project_settings.h>

using namespace repo::manipulator::modelutility::sparse;
using namespace repo::lib;

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
			container->container + "." + REPO_COLLECTION_SCENE,
			query,
			projection);

		for (auto& bson : (*cursor))
		{
			repo::core::model::RepoNode node(bson);

			// The sparse graph assumes a unidirectional tree.

			std::vector<repo::lib::RepoUUID>* children = nullptr;
			if (node.getParentIDs().size()) {
				children = &parentToChild[node.getParentIDs()[0]];
			}

			auto type = bson.getStringField(REPO_NODE_LABEL_TYPE);
			if (type == REPO_NODE_TYPE_TRANSFORMATION)
			{
				auto transform = repo::core::model::TransformationNode(bson);

				// Transformations are always processed bottom-up, so we can
				// pre-multiply

				for (auto& uniqueId : parentToChild[node.getSharedID()]) {
					auto& instance = nodes[uniqueId];
					instance.matrix = repo::lib::RepoMatrix(transform.getTransMatrix()) * instance.matrix;

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
				instance.matrix = repo::lib::RepoMatrix();
				instance.mesh = bson;

				if (children) {
					children->push_back(instance.uniqueId);
				}
			} 

			for (auto& p : node.getParentIDs()) {
				ids.push_back(p);
			}
		}

		useSharedId = true;
	}

	// The final step is to premultiply the world offset and units

	repo::core::model::ModelRevisionNode history(handler->findOneByCriteria(
		container->teamspace,
		container->container + "." + REPO_COLLECTION_HISTORY,
		repo::core::handler::database::query::Eq(REPO_NODE_LABEL_ID, container->revision)
	));
	auto offset = history.getCoordOffset();

	// In the future we may want to store the units with the revision node, as then
	// it is unambiguous what scale was applied on import for BIM formats

	repo::core::model::RepoProjectSettings settings(handler->findOneByCriteria(
		container->teamspace,
		REPO_COLLECTION_SETTINGS,
		repo::core::handler::database::query::Eq(REPO_NODE_LABEL_ID, container->container)
	));
	auto scale = units::determineScaleFactor(settings.getUnits(), ModelUnits::MILLIMETRES);

	auto rootTransform = repo::lib::RepoMatrix::scale(scale) * repo::lib::RepoMatrix::translate(offset);
	for(auto& id : uniqueIds)
	{
		auto& node = nodes[id];
		node.matrix = rootTransform * node.matrix;
	}
}