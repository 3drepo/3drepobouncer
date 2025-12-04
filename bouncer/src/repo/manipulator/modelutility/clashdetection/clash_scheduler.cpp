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

#include "clash_scheduler.h"

#include <algorithm>
#include <unordered_map>
#include <queue>

using namespace::repo::manipulator::modelutility::clash;

class ClashSchedulerImpl
{
	struct Node;

	using Edge = std::pair<Node*, Node*>;

	struct Node
	{
		void* id;
		std::vector<Edge> edges;

		Node(void* id) :
			id(id) {
		}

		bool remove(Edge e) {
			edges.erase(std::remove(edges.begin(), edges.end(), e), edges.end());
			return edges.size();
		}
	};

	// This manages the memory for the nodes, and its contents does not change
	// after initialisation, though their edge count does. The pointer arrays
	// below are used to track which nodes are live; they can be re-ordered,
	// and have elements removed when the edge counts go to zero.

	std::vector<Node> nodes;

	std::vector<Node*> allNodes;

public:
	std::vector<std::pair<void*, void*>> results;

	ClashSchedulerImpl(std::vector<std::pair<void*, void*>> broadphaseResults)
	{
		createGraph(broadphaseResults);

		sortByEdges(allNodes);

		std::queue<Node*> next;
		while (allNodes.size()) {
			next.push(allNodes.front());

			while (!next.empty()) {
				auto node = next.front();
				next.pop();

				if (!node->edges.size()) {
					continue;
				}

				struct Other {
					Node* node;
					bool direction;
				};

				std::vector<Other> others;

				for (auto& edge : node->edges) {
					Other other;
					other.direction = (node == edge.first);
					other.node = other.direction ? edge.second : edge.first;
					others.push_back(other);
				}

				// Sort by edges as above, but with an indirection to node

				std::ranges::sort(others, [](const Other& a, const Other& b) {
					return a.node->edges.size() < b.node->edges.size();
				});

				for (const auto& other : others) {
					Edge edge = other.direction ? Edge(node, other.node) : Edge(other.node, node);
					results.push_back({ edge.first->id, edge.second->id });
					remove(edge);
					next.push(other.node);
				}
			}
		}
	}

	void sortByEdges(std::vector<Node*>&set)
	{
		std::ranges::sort(set, [](const Node* a, const Node* b) {
			return a->edges.size() < b->edges.size();
		});
	}

	void addNode(void* id, std::unordered_map<void*, size_t>&map)
	{
		auto it = map.find(id);
		if (it == map.end()) {
			map[id] = nodes.size();
			nodes.push_back(Node(id));
		}
	}

	void createGraph(std::vector<std::pair<void*, void*>> edges)
	{
		std::unordered_map<void*, size_t> map; // Pointer to node index

		for (const auto& e : edges) {
			addNode(e.first, map);
			addNode(e.second, map);
		}

		// From now, the nodes vector shall not change so we can take pointers to its
		// contents.

		for (const auto& e : edges) {
			std::pair<Node*, Node*> edge(&nodes[map[e.first]], &nodes[map[e.second]]);
			edge.first->edges.push_back(edge);
			if(edge.first == edge.second) {
				continue;
			}
			edge.second->edges.push_back(edge);
		}

		for (auto& n : nodes) {
			allNodes.push_back(&n);
		}
	}

	void remove(Edge edge)
	{
		if (!edge.first->remove(edge)) {
			remove(edge.first);
		}
		if (!edge.second->remove(edge)) {
			remove(edge.second);
		}
	}

	void remove(Node * node)
	{
		allNodes.erase(std::remove(allNodes.begin(), allNodes.end(), node), allNodes.end());
	}
};

void ClashScheduler::_schedule(std::vector<std::pair<void*, void*>>& broadphaseResults)
{
	ClashSchedulerImpl scheduler(broadphaseResults);
	broadphaseResults.swap(scheduler.results);
}
