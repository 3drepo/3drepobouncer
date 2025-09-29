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

ClashScheduler::ClashScheduler(std::vector<std::pair<int, int>> broadphaseResults)
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

			bool nodeIsSetA = node == node->edges[0].first;

			std::vector<Node*> others;

			for (auto& edge : node->edges) {
				others.push_back(nodeIsSetA ? edge.second : edge.first);
			}

			sortByEdges(others);

			for (const auto other : others) {
				Edge edge(node, other);
				if (!nodeIsSetA) {
					std::swap(edge.first, edge.second);
				}
				results.push_back({ edge.first->index, edge.second->index });
				remove(edge);
				next.push(other);
			}
		}
	}
}

void ClashScheduler::sortByEdges(std::vector<Node*>& set)
{
	std::ranges::sort(set, [](const Node* a, const Node* b) {
		return a->edges.size() < b->edges.size();
	});
}

void ClashScheduler::addNode(size_t index, std::unordered_map<int, int>& map) 
{
	auto it = map.find(index);
	if (it == map.end()) {
		map[index] = nodes.size();
		nodes.push_back(Node(index));
	}
}

void ClashScheduler::createGraph(std::vector<std::pair<int, int>> edges)
{
	std::unordered_map<int, int> nodesA;
	std::unordered_map<int, int> nodesB;

	for (const auto& e : edges) {
		addNode(e.first, nodesA);
		addNode(e.second, nodesB);
	}

	// From now, the nodes vector shall not change so we can take pointers to its
	// contents.

	for (const auto& e : edges) {
		std::pair<Node*, Node*> edge(&nodes[nodesA[e.first]], &nodes[nodesB[e.second]]);
		edge.first->edges.push_back(edge);
		edge.second->edges.push_back(edge);
	}

	for (auto& n : nodes) {
		allNodes.push_back(&n);
	}
}

void ClashScheduler::remove(Edge edge)
{
	if (!edge.first->remove(edge)) {
		remove(edge.first);
	}
	if (!edge.second->remove(edge)) {
		remove(edge.second);
	}
}

void ClashScheduler::remove(Node* node)
{
	allNodes.erase(std::remove(allNodes.begin(), allNodes.end(), node), allNodes.end());
}

bool ClashScheduler::Node::remove(Edge e)
{
	edges.erase(std::remove(edges.begin(), edges.end(), e), edges.end());
	return edges.size();
}

void ClashScheduler::schedule(std::vector<std::pair<int, int>>& broadphaseResults)
{
	ClashScheduler scheduler(broadphaseResults);
	broadphaseResults.swap(scheduler.results);
}
