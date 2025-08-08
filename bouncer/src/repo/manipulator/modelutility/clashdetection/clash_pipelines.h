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
#include <repo/manipulator/modeloptimizer/bvh/bvh.hpp>
#include <repo/lib/datastructure/repo_triangle.h>
#include "sparse_scene_graph.h"

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				// Clash tests follow a very similar pattern, being distinguished primarily
				// by which geometric tests are used.
				// 
				// All clash modes are implemented as subclasses of a pipeline, overriding
				// a small subset of methods to implement these tests. For most modes, the
				// overrides should be simple enough to allow header only definitions.

				using Bvh = bvh::Bvh<double>;

				// Tests may pass parameters between stages using subclasses of the following
				// structs.

				struct NarrowphaseResult
				{
				};

				struct CompositeClash
				{
				};

				struct UnorderedPair
				{
					repo::lib::RepoUUID a;
					repo::lib::RepoUUID b;

					UnorderedPair(const repo::lib::RepoUUID& a, const repo::lib::RepoUUID& b)
						:a(a), b(b)
					{
						if(b < a)
						{
							std::swap(this->a, this->b);
						}
					}

					size_t getHash() const
					{
						return a.getHash() ^ b.getHash();
					}

					bool operator == (const UnorderedPair& other) const
					{
						return (a == other.a && b == other.b);
					}
				};

				struct UnorderedPairHasher
				{
					std::size_t operator()(const UnorderedPair& p) const
					{
						return p.getHash();
					}
				};

				class Pipeline
				{
				public:
					using DatabasePtr = std::shared_ptr<repo::core::handler::AbstractDatabaseHandler>;
					using RepoUUIDMap = std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>;
					using BroadphaseResults = std::vector<std::pair<int, int>>;

					Pipeline(DatabasePtr, const repo::manipulator::modelutility::ClashDetectionConfig&);
					ClashDetectionReport runPipeline();

					virtual std::unique_ptr<NarrowphaseResult> createNarrowphaseResult() = 0;
					virtual CompositeClash* createCompositeClash() = 0; // This needs to be typical pointer so it can be stored in the map below.

					/*
					* The broadphase test operates on BVHs. These may created at multiple
					* levels of granularity, depending on the stage.
					*/
					virtual void broadphase(const Bvh& a, const Bvh& b, BroadphaseResults& results) const = 0;
					virtual bool narrowphase(const repo::lib::RepoTriangle& a, const repo::lib::RepoTriangle& b, NarrowphaseResult&) const = 0;
					virtual void append(CompositeClash& clash, const NarrowphaseResult& result) const = 0;
					virtual void createClashReport(const UnorderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const = 0;

				protected:
					DatabasePtr handler;
					const repo::manipulator::modelutility::ClashDetectionConfig& config;
					RepoUUIDMap uniqueToCompositeId;
					std::unordered_map<UnorderedPair, CompositeClash*, UnorderedPairHasher> clashes;
				};
			}
		}
	}
}