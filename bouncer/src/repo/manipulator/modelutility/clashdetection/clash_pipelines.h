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
#include "clash_node_cache.h"
#include "sparse_scene_graph.h"

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				// All Clash modes are implemented as subclasses of Pipeline, using common
				// types to share data between stages and the output.

				using Bvh = bvh::Bvh<double>;

				using BroadphaseResults = std::vector<std::pair<size_t, size_t>>;

				// Tests may pass parameters between stages using subclasses of the following
				// struct.

				struct CompositeClash
				{
				};

				// The clash engine will return composite object a on the left, and b on the
				// right, always. Though it is good practice when matching the result
				// downstream to ignore the order, in case the user swaps the sets between
				// runs.

				struct OrderedPair
				{
					repo::lib::RepoUUID a;
					repo::lib::RepoUUID b;

					OrderedPair(const repo::lib::RepoUUID& a, const repo::lib::RepoUUID& b)
						:a(a), b(b)
					{
					}

					size_t getHash() const
					{
						return a.getHash() ^ b.getHash();
					}

					bool operator == (const OrderedPair& other) const
					{
						return (a == other.a && b == other.b);
					}
				};

				struct OrderedPairHasher
				{
					std::size_t operator()(const OrderedPair& p) const
					{
						return p.getHash();
					}
				};

				struct Graph
				{
					using CompositeMap = std::unordered_map<repo::lib::RepoUUID, size_t, repo::lib::RepoUUIDHasher>;

					// Once this is initialised, it should not be changed, as the bvh will use
					// indices into this vector to refer to the nodes.
					const std::vector<sparse::Node> meshes;

					// The bvh for the graph stores bounds in Project Coordinates.
					Bvh bvh;

					// Maps from unique mesh ids to indices into setA/B of the config
					CompositeMap compositeMap;

					Graph(std::vector<sparse::Node> nodes, CompositeMap& compositeMap);

					sparse::Node& getNode(size_t primitive) const;

					// Returns the index into the set of Composite Objects used to build this
					// graph, for the given primitive (mesh) index.
					size_t getCompositeIndex(size_t primitive) const;
				};

				struct CompositeObjectCache
				{
					std::vector<std::shared_ptr<NodeCache::Node>> meshNodes;

					void initialise();

					/*
					* Loads the triangle soup of the meshNodes into memory and returns a
					* reference to it. Initialises the triangles array on-demand if
					* required.
					*/
					const std::vector<repo::lib::RepoTriangle>& getTriangles() const;

					const repo::lib::RepoUUID& id() const;
				};

				class Pipeline
				{
				public:
					using DatabasePtr = std::shared_ptr<repo::core::handler::AbstractDatabaseHandler>;
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

					RepoUUIDMap uniqueToCompositeId;
					std::unordered_map<OrderedPair, CompositeClash*, OrderedPairHasher> clashes;
					NodeCache cache;

					std::shared_ptr<CompositeObjectCache> getCompositeObjectCache(
						const std::vector<CompositeObject>& set,
						size_t index
					);

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