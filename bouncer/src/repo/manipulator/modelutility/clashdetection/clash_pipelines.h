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

				// Clash tests follow a very similar pattern, being distinguished primarily
				// by which geometric tests are used.
				// 
				// All clash modes are implemented as subclasses of a pipeline, overriding
				// a small subset of methods to implement these tests.

				using Bvh = bvh::Bvh<double>;

				// Pipeline stages, such as a BVH traversal, often have to maintain a state.
				// Therefore stages are implemented as functors, which can be created and
				// re-used (per thread, where necessary).

				using BroadphaseResults = std::vector<std::pair<size_t, size_t>>;

				struct Broadphase
				{
					virtual void operator()(const Bvh& a, const Bvh& b, BroadphaseResults& results) = 0;
				};

				struct Narrowphase
				{
					// If the narrowphase functor returns true, it is passed to the append
					// function.

					virtual bool operator()(const repo::lib::RepoTriangle& a, const repo::lib::RepoTriangle& b) = 0;
				};

				// Tests may pass parameters between stages using subclasses of the following
				// structs.

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

				class NodeCache;

				class Pipeline
				{
				public:
					using DatabasePtr = std::shared_ptr<repo::core::handler::AbstractDatabaseHandler>;
					using RepoUUIDMap = std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>;

					Pipeline(DatabasePtr, const repo::manipulator::modelutility::ClashDetectionConfig&);
					ClashDetectionReport runPipeline();

					/*
					* The broadphase test operates on BVHs. These may created at multiple
					* levels of granularity, depending on the stage.
					*/
					virtual std::unique_ptr<Broadphase> createBroadphase() const = 0;

					virtual std::unique_ptr<Narrowphase> createNarrowphase() const = 0;

					virtual CompositeClash* createCompositeClash() = 0; // This needs to be typical pointer so it can be stored in the map below.

					virtual void append(CompositeClash& clash, const Narrowphase& result) const = 0;

					virtual void createClashReport(const OrderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const = 0;

				protected:
					DatabasePtr handler;
					const repo::manipulator::modelutility::ClashDetectionConfig& config;
					RepoUUIDMap uniqueToCompositeId;
					std::unordered_map<OrderedPair, CompositeClash*, OrderedPairHasher> clashes;
					NodeCache cache;
				};
			}
		}
	}
}