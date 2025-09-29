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

#include <memory>

#include "sparse_scene_graph.h"

#include "repo/core/handler/repo_database_handler_abstract.h"
#include "repo/lib/datastructure/repo_structs.h"
#include "repo/lib/datastructure/repo_triangle.h"

#include <repo/manipulator/modeloptimizer/bvh/bvh.hpp>


namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				/*
				* The cache stores MeshNodes, along with their binaries and BVHs ready for
				* narrowphase tests.
				* Each MeshNode may be used in a number of Narrowphase tests, so the cache
				* uses reference counting to dispose of nodes when they are no longer
				* needed.
				*/

				class NodeCache
				{
				public:

					NodeCache(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler);

					class Node {
						NodeCache& cache;
						sparse::Node& sceneNode;
						std::vector<repo::lib::RepoVector3D64> vertices;
						std::vector<repo::lib::repo_face_t> faces;
						bvh::Bvh<double> bvh;

					public:
						Node(NodeCache& cache, sparse::Node& sceneNode);

						void initialise();
						const bvh::Bvh<double>& getBvh() const;
						repo::lib::RepoTriangle getTriangle(size_t primitive) const;
						const repo::lib::RepoUUID& getUniqueId() const;
					};

					std::shared_ptr<Node> getNode(sparse::Node& sceneNode);

				private:
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler;
				};
			}
		}
	}
}