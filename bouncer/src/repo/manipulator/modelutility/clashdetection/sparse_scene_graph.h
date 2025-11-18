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

#pragma once

#include <unordered_map>

#include "repo/repo_bouncer_global.h"
#include "repo/lib/datastructure/repo_container.h"
#include "repo/lib/datastructure/repo_uuid.h"
#include "repo/lib/datastructure/repo_matrix.h"
#include "repo/core/model/bson/repo_node_streaming_mesh.h"

namespace repo {
	namespace core {
		namespace handler {
			class AbstractDatabaseHandler;
		}
	}

	namespace manipulator {
		namespace modelutility {
			namespace sparse {

				struct Node
				{
					repo::lib::RepoUUID uniqueId;
					repo::lib::RepoMatrix matrix;
					repo::core::model::RepoBSON mesh;
					const repo::lib::Container* container;
				};

				REPO_API_EXPORT struct SceneGraph
				{
				public:
					/*
					* Populates the scene graph starting at the leaf nodes. Can be called multiple times.
					*/
					void populate(
						std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
						const repo::lib::Container* container,
						const std::vector<repo::lib::RepoUUID>& uniqueIds
					);

					/*
					* Gets all the loaded nodes as a flat list. This generic method allows
					* callers to provide their own type, allowing them to append additional
					* data to the nodes afterwards if required. The type N should be a
					* subclass of Node, or have at least the same members.
					*/
					template<typename N>
					void getNodes(std::vector<N>& dest) const
					{
						dest.reserve(this->nodes.size());
						for (const auto& [id, node] : this->nodes)
						{
							N n;
							n.container = node.container;
							n.uniqueId = node.uniqueId;
							n.matrix = node.matrix;
							n.mesh = node.mesh;
							dest.push_back(n);
						}
					}

				protected:
					std::unordered_map<repo::lib::RepoUUID, Node, repo::lib::RepoUUIDHasher> nodes; // by Unique Id
				};


			}
		}
	}
}