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

#include <vector>
#include <repo/manipulator/modelutility/repo_clash_detection_engine.h>
#include <repo/manipulator/modelutility/repo_clash_detection_config.h>
#include <repo/manipulator/modeloptimizer/bvh/bvh.hpp>
#include <repo/lib/datastructure/repo_triangle.h>
#include "sparse_scene_graph.h"
#include "ordered_pair.h"

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				// All Clash modes are implemented as subclasses of Pipeline, using common
				// types to share data between stages and the output.

				using Bvh = bvh::Bvh<double>;
				using DatabasePtr = std::shared_ptr<repo::core::handler::AbstractDatabaseHandler>;

				using BroadphaseResults = std::vector<std::pair<size_t, size_t>>;

				// Tests may pass parameters between stages using subclasses of the following
				// struct.

				struct CompositeClash
				{
				};

				// This graph object is what the pipeline implementations operate on, and
				// provides various methods to help them query the graph of meshes and
				// composite objects.

				struct Graph
				{
					struct Node : public sparse::Node
					{
						// Reference to the Composite Object (in the config set) that this mesh
						// belongs to. This will always be set when the graph is populated.
						// However it must be a pointer in order for the node to be used in the
						// cache, which requires Node to be default-constructible.

						const CompositeObject* compositeObject;
					};

					// Once this is initialised, it should not be changed, as the bvh will use
					// indices into this vector to refer to the nodes.

					const std::vector<Node> meshes;

					// The bvh for the graph stores bounds in Project Coordinates.

					Bvh bvh;

					// From mesh node unique id to an index into the meshes array - same as the
					// one the bvh uses.

					std::unordered_map<repo::lib::RepoUUID, size_t, repo::lib::RepoUUIDHasher> uuidToIndexMap;

					Graph(std::vector<Node> nodes);

					Node& getNode(size_t primitive) const;

					Node& getNode(const repo::lib::RepoUUID& uniqueId) const;

					// Returns a reference to the Composite Object that the mesh, given by
					// the index into the meshes array, belongs to.

					const CompositeObject& getCompositeObject(size_t primitive) const;
				};

				class Pipeline
				{
				public:
					using RepoUUIDMap = std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>;

					Pipeline(DatabasePtr, const repo::manipulator::modelutility::ClashDetectionConfig&);

					ClashDetectionReport runPipeline();

				protected:
					/*
					* Perform the clash detection between the three graphs - all graphs will be
					* disjoint. graphC must always perform a self-intersection.
					*/
					virtual void run(const Graph& graphA, const Graph& graphB, const Graph& graphC) = 0;
					virtual void createClashReport(const OrderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const = 0;

					DatabasePtr handler;

					const repo::manipulator::modelutility::ClashDetectionConfig& config;

					std::unordered_map<OrderedPair, CompositeClash*, OrderedPairHasher> clashes;

					template<class T>
					T* createClash(const std::string& a, const std::string& b) {
						OrderedPair pair(a, b);
						auto it = clashes.find(pair);
						if (it == clashes.end())
						{
							auto clash = new T();
							it = clashes.emplace(pair, std::move(clash)).first;
						}
						return static_cast<T*>(it->second);
					}
				};
			}
		}
	}
}