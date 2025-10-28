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
#include <unordered_map>

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {
				/*
				* Goal of the scheduler is to order the narrowphase tests so that we only
				* load & pre-process each MeshNode once, but also that they are offloaded as
				* soon as possible to avoid running out of memory.
				* 
				* One way to express this is MIN->RMS([SPAN]), where SPAN is the differnce
				* between the first and last index of any MeshNode in the broadphase results.
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

				class ClashScheduler
				{
				public:
					static void schedule(std::vector<std::pair<size_t, size_t>>& broadphaseResults);

				private:
					ClashScheduler(std::vector<std::pair<size_t, size_t>> broadphaseResults);

					struct Node;

					using Edge = std::pair<Node*, Node*>;

					struct Node
					{
						size_t index;
						std::vector<Edge> edges;

						Node(size_t index):
							index(index)
						{
						}

						bool remove(Edge e);
					};

					// This manages the memory for the nodes, and its contents does not change
					// after initialisation, though their edge count does. The pointer arrays
					// below are used to track which nodes are live; they can be re-ordered,
					// and have elements removed when the edge counts go to zero.

					std::vector<Node> nodes;

					std::vector<Node*> allNodes;

					std::vector<std::pair<size_t, size_t>> results;

					void createGraph(std::vector<std::pair<size_t, size_t>> edges);

					void addNode(size_t index, std::unordered_map<size_t, size_t>& map);

					void remove(Edge e);

					// Removes node from allNodes
					void remove(Node* node);

					void sortByEdges(std::vector<Node*>& set);
				};
			}
		}
	}
}