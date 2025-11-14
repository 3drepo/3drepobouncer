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

#include "repo/manipulator/modeloptimizer/bvh/bvh.hpp"

#include <stack>
#include <utility>

namespace bvh {

	/*
	* Implementations of various common operators on BVHs.
	*/

	/*
	* Performs a top-down traversal of two BVH's looking for pairs for which the
	* minimum distance is below a threshold. The intersects implementation may
	* update d for a more efficient traversal, depending on the goals of the
	* query.
	*/
	struct DistanceQuery
	{
		/*
		* The upper bound on the distance between two primitiives. If two inner nodes
		* have a minimum distance larger than this (i.e. no two primitives under them
		* can update it), then that recursive branch will terminate.
		*/
		double d;

		/*
		* Get the true distance between two primitives. If it is lower than the current
		* upper bound d, then it will be used to update d.
		*/
		virtual void intersect(size_t primA, size_t primB) = 0;

		void operator()(const bvh::Bvh<double>& a, const bvh::Bvh<double>& b);
	};

	/*
	* Performs a top-down traversal of two BVH's looking for pairs of leaf nodes
	* that overlap. If the overlap is less than tolerance, ignores the intersection.
	*/
	struct IntersectQuery
	{
		double tolerance;

		virtual void intersect(size_t primA, size_t primB) = 0;

		void operator()(const bvh::Bvh<double>& a, const bvh::Bvh<double>& b);
	};
}