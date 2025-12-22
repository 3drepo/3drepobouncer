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

#include "bvh_operators.h"

#include "repo/lib/datastructure/repo_structs.h"
#include "repo/lib/datastructure/repo_triangle.h"

#include "repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp"

using namespace bvh;

void Traversal::operator()(const bvh::Bvh<double>& a, const bvh::Bvh<double>& b)
{
	std::stack<std::pair<size_t, size_t>> pairs;
	pairs.push({ 0, 0 });
	while (!pairs.empty())
	{
		auto [idxLeft, idxRight] = pairs.top();
		pairs.pop();

		auto& left = a.nodes[idxLeft];
		auto& right = b.nodes[idxRight];

		if (!intersect(left, right)) {
			continue;
		}

		if (left.is_leaf() && right.is_leaf())
		{
			for (size_t l = 0; l < left.primitive_count; l++) {
				for (size_t r = 0; r < right.primitive_count; r++) {
					intersect(
						a.primitive_indices[left.first_child_or_primitive + l],
						b.primitive_indices[right.first_child_or_primitive + r]
					);
				}
			}
		}
		else if (left.is_leaf() && !right.is_leaf())
		{
			pairs.push({ idxLeft, right.first_child_or_primitive + 0 });
			pairs.push({ idxLeft, right.first_child_or_primitive + 1 });
		}
		else if (!left.is_leaf() && right.is_leaf())
		{
			pairs.push({ left.first_child_or_primitive + 0, idxRight });
			pairs.push({ left.first_child_or_primitive + 1, idxRight });
		}
		else
		{
			pairs.push({ left.first_child_or_primitive + 0, right.first_child_or_primitive + 0 });
			pairs.push({ left.first_child_or_primitive + 0, right.first_child_or_primitive + 1 });
			pairs.push({ left.first_child_or_primitive + 1, right.first_child_or_primitive + 0 });
			pairs.push({ left.first_child_or_primitive + 1, right.first_child_or_primitive + 1 });
		}
	}
}

void Traversal::operator()(const bvh::Bvh<double>& a)
{
	// The goal of an internal traversal is to find all pairs in a single tree
	// that overlap eachother. The traversal operates on pairs of siblings.

	std::vector<bool> frontier(a.node_count, false);

	std::stack<std::pair<size_t, size_t>> pairs;
	auto& root = a.nodes[0];
	pairs.push({ root.first_child_or_primitive + 0, root.first_child_or_primitive + 1 });
	while (!pairs.empty()) 
	{
		auto [idxLeft, idxRight] = pairs.top();
		pairs.pop();

		auto& left = a.nodes[idxLeft];
		auto& right = a.nodes[idxRight];

		// When we first reach a branch node, we need to tell the traversal to check
		// its children against eachother. However, we don't want this to happen
		// multiple times, if the node is encountered again, say because it intersects
		// with a set of leaf nodes.

		// To achieve this we track the frontier of the traversal and this decides
		// when intra-node checks are required. If the frontier has reached a node,
		// only the inter-node checks are performed.

		if (!frontier[idxLeft]) {
			if (left.is_leaf()) {
				for (size_t l = 0; l < left.primitive_count; l++) {
					for (size_t r = l + 1; r < left.primitive_count; r++) {
						intersect(
							a.primitive_indices[left.first_child_or_primitive + l],
							a.primitive_indices[left.first_child_or_primitive + r]
						);
					}
				}
			}
			else {
				pairs.push({ 
					left.first_child_or_primitive + 0, 
					left.first_child_or_primitive + 1 
				});
			}
			
			frontier[idxLeft] = true;
		}

		if(!frontier[idxRight]) {
			if (right.is_leaf()) {
				for (size_t l = 0; l < right.primitive_count; l++) {
					for (size_t r = l + 1; r < right.primitive_count; r++) {
						intersect(
							a.primitive_indices[right.first_child_or_primitive + l],
							a.primitive_indices[right.first_child_or_primitive + r]
						);
					}
				}
			}
			else {
				pairs.push({ 
					right.first_child_or_primitive + 0, 
					right.first_child_or_primitive + 1 
				});
			}
			frontier[idxRight] = true;
		}

		// Then if the siblings intersect, we also check the cross combinations of
		// their children.

		if (!intersect(left, right)) {
			continue;
		}

		if (left.is_leaf() && right.is_leaf())
		{
			for (size_t l = 0; l < left.primitive_count; l++) {
				for (size_t r = 0; r < right.primitive_count; r++) {
					intersect(
						a.primitive_indices[left.first_child_or_primitive + l],
						a.primitive_indices[right.first_child_or_primitive + r]
					);
				}
			}
		}
		else if (left.is_leaf() && !right.is_leaf())
		{
			pairs.push({ idxLeft, right.first_child_or_primitive + 0 });
			pairs.push({ idxLeft, right.first_child_or_primitive + 1 });
		}
		else if (!left.is_leaf() && right.is_leaf())
		{
			pairs.push({ left.first_child_or_primitive + 0, idxRight });
			pairs.push({ left.first_child_or_primitive + 1, idxRight });
		}
		else
		{
			pairs.push({ left.first_child_or_primitive + 0, right.first_child_or_primitive + 0 });
			pairs.push({ left.first_child_or_primitive + 0, right.first_child_or_primitive + 1 });
			pairs.push({ left.first_child_or_primitive + 1, right.first_child_or_primitive + 0 });
			pairs.push({ left.first_child_or_primitive + 1, right.first_child_or_primitive + 1 });
		}
	}
}

static bvh::BoundingBox<double> overlap(const bvh::BoundingBox<double>& a, const bvh::BoundingBox<double>& b)
{
	bvh::BoundingBox<double> result;
	result.min = {
		std::max(a.min[0], b.min[0]),
		std::max(a.min[1], b.min[1]),
		std::max(a.min[2], b.min[2])
	};
	result.max = {
		std::min(a.max[0], b.max[0]),
		std::min(a.max[1], b.max[1]),
		std::min(a.max[2], b.max[2])
	};
	return result;
}

static double distance2(const bvh::BoundingBox<double>& a, const bvh::BoundingBox<double>& b)
{
	auto ib = overlap(a, b);
	double sq = 0.0;
	for (auto i = 0; i < 3; ++i) {
		if (ib.min[i] > ib.max[i]) {
			sq += std::pow(ib.min[i] - ib.max[i], 2);
		}
	}
	return sq;
}

bool bvh::predicates::intersects(
	const bvh::Bvh<double>::Node& a,
	const bvh::Bvh<double>::Node& b)
{
	return
		a.bounds[0] <= b.bounds[1] && a.bounds[1] >= b.bounds[0] &&
		a.bounds[2] <= b.bounds[3] && a.bounds[3] >= b.bounds[2] &&
		a.bounds[4] <= b.bounds[5] && a.bounds[5] >= b.bounds[4];
}

bool DistanceQuery::intersect(
	const bvh::Bvh<double>::Node& a,
	const bvh::Bvh<double>::Node& b)
{
	auto mds = distance2(a.bounding_box_proxy(), b.bounding_box_proxy());
	return mds <= (d * d); // True if two primitives in a and b have the potential to modify the current distance.
}

bool IntersectQuery::intersect(
	const bvh::Bvh<double>::Node& a,
	const bvh::Bvh<double>::Node& b)
{
	return predicates::intersects(a, b);
}

void bvh::builders::build(bvh::Bvh<double>& bvh,
	const std::vector<repo::lib::RepoVector3D64>& vertices,
	const std::vector<repo::lib::repo_face_t>& faces)
{
	auto boundingBoxes = std::vector<bvh::BoundingBox<double>>();
	auto centers = std::vector<bvh::Vector3<double>>();

	for (const auto& face : faces)
	{
		auto bbox = bvh::BoundingBox<double>::empty();
		for (size_t i = 0; i < face.sides; i++) {
			auto v = vertices[face[i]];
			bbox.extend(bvh::Vector3<double>(v.x, v.y, v.z));
		}
		boundingBoxes.push_back(bbox);
		centers.push_back(boundingBoxes.back().center());
	}

	auto globalBounds = bvh::compute_bounding_boxes_union(boundingBoxes.data(), boundingBoxes.size());

	bvh::SweepSahBuilder<bvh::Bvh<double>> builder(bvh);
	builder.max_leaf_size = 1;
	builder.build(globalBounds, boundingBoxes.data(), centers.data(), boundingBoxes.size());
}

void bvh::builders::build(bvh::Bvh<double>& bvh,
	const std::vector<repo::lib::RepoTriangle>& triangles)
{
	build(bvh, triangles.data(), triangles.size());
}

void bvh::builders::build(bvh::Bvh<double>& bvh,
	const repo::lib::_RepoTriangle<double>* triangles,
	size_t numTriangles
)
{
	auto bounds = std::vector<bvh::BoundingBox<double>>();
	auto centers = std::vector<bvh::Vector3<double>>();

	bounds.reserve(numTriangles);
	centers.reserve(numTriangles);

	for (size_t i = 0; i < numTriangles; ++i)
	{
		const auto& triangle = triangles[i];
		auto aabb = bvh::BoundingBox<double>::empty();
		aabb.extend(reinterpret_cast<const bvh::Vector3<double>&>(triangle.a));
		aabb.extend(reinterpret_cast<const bvh::Vector3<double>&>(triangle.b));
		aabb.extend(reinterpret_cast<const bvh::Vector3<double>&>(triangle.c));
		centers.push_back(aabb.center());
		bounds.push_back(aabb);
	}

	auto globalBounds = bvh::compute_bounding_boxes_union(bounds.data(), bounds.size());

	bvh::SweepSahBuilder<bvh::Bvh<double>> builder(bvh);
	builder.max_leaf_size = 1;
	builder.build(globalBounds, bounds.data(), centers.data(), bounds.size());
}