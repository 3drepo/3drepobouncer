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
#include <concepts>

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {
				/*
				* The ClashScheduler orders a vector of edges between nodes (represented as
				* void*), in such a way that nodes are clustered together as closely as
				* possible.
				*
				* The goal of the scheduler is to order the narrowphase tests so that we only
				* load & pre-process each MeshNode once, but also that they are offloaded as
				* soon as possible to avoid running out of memory.
				* 
				* One way to express this is MIN->RMS([SPAN]), where SPAN is the differnce
				* between the first and last index of any MeshNode on either side of the
				* broadphase results.
				*
				* This is an NP-hard problem, so this object creates a best guess.
				*
				* It works by starting with the Mesh with the fewest references from set A.
				* Those potential colliders in Set B are also ordererd by their number of
				* references. This defines the first set of narrowphase tasks. We repeat the
				* process, starting from the first Mesh in the ordered set of B colliders
				* that still has outstanding narrowphase tasks.
				*
				* By moving through the Meshes with the fewest associated jobs first, they
				* can be unloaded as early as possible. By adding all jobs associated with
				* a particular mesh to the list contiguously, we ensure that at least one
				* Mesh is unloaded after each 'block'.
				*
				* This hopefully makes the algorithm robust to the worst case, which is that
				* of meshes alternating between Set A and Set B, laid out linearly and so
				* making the graph of all Meshes fully connected.
				*/

				template<typename T>
				concept ValidNodeIdentifer = (sizeof(T) == sizeof(void*));

				class ClashScheduler
				{
				public:
					// The schedular is intended to work with pointers to nodes (where nodes
					// are not fungible). The pointers will never be dereferenced however, so
					// any type of equivalent size will work. For example, indices could be
					// provided instead.

					template<ValidNodeIdentifer T>
					static void schedule(std::vector<std::pair<T, T>>& broadphaseResults) {
						// T must be the same size as void* but we must still cast the individual
						// elements as the pairs/vectors are not type aliases. These loops should
						// be optimised away by the compiler.
						std::vector<std::pair<void*, void*>> r;
						r.resize(broadphaseResults.size());
						for (size_t i = 0; i < broadphaseResults.size(); i++) {
							r[i].first = (void*)broadphaseResults[i].first;
							r[i].second = (void*)broadphaseResults[i].second;
						}
						_schedule(r);
						for (size_t i = 0; i < broadphaseResults.size(); i++) {
							broadphaseResults[i].first = (T)r[i].first;
							broadphaseResults[i].second = (T)r[i].second;
						}
					}

				private:
					static void _schedule(std::vector<std::pair<void*, void*>>& broadphaseResults);
				};
			}
		}
	}
}