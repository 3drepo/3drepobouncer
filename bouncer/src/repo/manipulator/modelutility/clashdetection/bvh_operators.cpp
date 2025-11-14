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

using namespace bvh;

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

static double intersection2(const bvh::BoundingBox<double>& a, const bvh::BoundingBox<double>& b)
{
	auto ib = overlap(a, b);
	double sq = 0.0;
	for (auto i = 0; i < 3; ++i) {
		if (ib.min[i] < ib.max[i]) {
			sq += std::pow(ib.min[i] - ib.max[i], 2);
		}
		else {
			return 0.0; // If the boxes are separated by any axis, there is no intersection.
		}
	}
	return sq;
}

void DistanceQuery::operator()(const bvh::Bvh<double>& a, const bvh::Bvh<double>& b)
{
	std::stack<std::pair<size_t, size_t>> pairs;
	pairs.push({ 0, 0 });
	while (!pairs.empty())
	{
		auto [idxLeft, idxRight] = pairs.top();
		pairs.pop();

		auto& left = a.nodes[idxLeft];
		auto& right = b.nodes[idxRight];

		auto mds = distance2(left.bounding_box_proxy(), right.bounding_box_proxy());
		if (mds > (d * d))
		{
			continue; // No two primitives in left and right can modify the current distance.
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

#pragma optimize("", off)	

void IntersectQuery::operator()(const bvh::Bvh<double>& a, const bvh::Bvh<double>& b)
{
	auto toleranceSq = tolerance * tolerance;

	std::stack<std::pair<size_t, size_t>> pairs;
	pairs.push({ 0, 0 });
	while (!pairs.empty())
	{
		auto [idxLeft, idxRight] = pairs.top();
		pairs.pop();

		auto& left = a.nodes[idxLeft];
		auto& right = b.nodes[idxRight];

		auto overlapSq = intersection2(
			left.bounding_box_proxy(),
			right.bounding_box_proxy()
		);

		if (overlapSq <= toleranceSq)
		{
			continue; // No possible intersection above the tolerance
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