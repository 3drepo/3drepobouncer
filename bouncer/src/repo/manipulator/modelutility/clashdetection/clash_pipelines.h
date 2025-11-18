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
						// Index into the set (A or B) of the composite object to which this mesh belongs.
						size_t compositeObjectIndex;
					};

					// Once this is initialised, it should not be changed, as the bvh will use
					// indices into this vector to refer to the nodes.
					const std::vector<Node> meshes;

					// The bvh for the graph stores bounds in Project Coordinates.
					Bvh bvh;

					// From mesh node unique id to an index into the meshes array, same as the
					// one the bvh uses.
					std::unordered_map<repo::lib::RepoUUID, size_t, repo::lib::RepoUUIDHasher> uuidToIndexMap;

					Graph(std::vector<Node> nodes);

					Node& getNode(size_t primitive) const;

					Node& getNode(const repo::lib::RepoUUID& uniqueId) const;

					// Returns the index into the set of Composite Objects used to build this
					// graph, for the given primitive (mesh) index.
					size_t getCompositeIndex(size_t primitive) const;
				};

				class Pipeline
				{
				public:
					using RepoUUIDMap = std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>;

					Pipeline(DatabasePtr, const repo::manipulator::modelutility::ClashDetectionConfig&);

					ClashDetectionReport runPipeline();

				protected:
					virtual void run(const Graph& graphA, const Graph& graphB) = 0;
					virtual void createClashReport(const OrderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const = 0;

					DatabasePtr handler;

					const repo::manipulator::modelutility::ClashDetectionConfig& config;

					// These are initialised at the beginning of runPipeline.
					std::unique_ptr<Graph> graphA;
					std::unique_ptr<Graph> graphB;

					std::unordered_map<OrderedPair, CompositeClash*, OrderedPairHasher> clashes;

					template<class T>
					T* createClash(const repo::lib::RepoUUID& a, const repo::lib::RepoUUID& b) {
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