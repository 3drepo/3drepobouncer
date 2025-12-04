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

	namespace predicates {
		/*
		* If the bounds of two nodes overlap or not.
		*/
		bool intersects(
			const bvh::Bvh<double>::Node& a,
			const bvh::Bvh<double>::Node& b);

	}

	/*
	* Base class for queries between two BVH's.
	*/
	struct Traversal {
		/*
		* Should return true if two branch nodes intersect, otherwise false (to
		* terminate the traversal for that branch). This will only ever be called
		* for branch/inner nodes.
		*/
		virtual bool intersect(
			const bvh::Bvh<double>::Node& a,
			const bvh::Bvh<double>::Node& b) = 0;

		/*
		* Called for all combinations of primitives for a pair of overlapping leaf
		* nodes.
		*/
		virtual void intersect(size_t primA, size_t primB) = 0;

		void operator()(
			const bvh::Bvh<double>& a,
			const bvh::Bvh<double>& b);

		/*
		* Performs an inter-bvh traversal returning all primitives that intersect
		* eachother.
		*/
		void operator()(
			const bvh::Bvh<double>& a);
	};

	/*
	* Convenience template to perform a traversal of two BHV's with any callable
	* types.
	*/
	template<typename PrimitiveIntersectionCallable, typename NodesIntersectionCallable>
	void traverse(
		const bvh::Bvh<double>& a,
		const bvh::Bvh<double>& b,
		NodesIntersectionCallable intersectNodes,
		PrimitiveIntersectionCallable intersectPrimitives)
	{
		// It would be preferable to use std::function here, but currently there is no
		// Concept that would ensure a given lambda fits within the limit for SOO, so
		// we do the type erasure manually. (A reference version of std function is
		// coming in C++26).

		struct Query : public Traversal {
			NodesIntersectionCallable& nodes;
			PrimitiveIntersectionCallable& primitives;
			Query(NodesIntersectionCallable& nodes, PrimitiveIntersectionCallable& primitives)
				: nodes(nodes), primitives(primitives)
			{
			}

			void intersect(size_t primA, size_t primB) override
			{
				primitives(primA, primB);
			}

			bool intersect(
				const bvh::Bvh<double>::Node& a,
				const bvh::Bvh<double>::Node& b) override
			{
				return nodes(a, b);
			}
		};

		Query q(intersectNodes, intersectPrimitives);
		q(a, b);
	}

	/*
	* Performs a top-down traversal of two BVH's looking for pairs for which the
	* minimum distance is below a threshold. The intersects implementation may
	* update d for a more efficient traversal, depending on the goals of the
	* query.
	*/
	struct DistanceQuery : public Traversal
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

	private:
		bool intersect(
			const bvh::Bvh<double>::Node& a,
			const bvh::Bvh<double>::Node& b) override;
	};

	/*
	* Performs a top-down traversal of two BVH's looking for pairs of leaf nodes
	* that overlap. If the overlap is less than tolerance, ignores the intersection.
	*/
	struct IntersectQuery : public Traversal
	{
		virtual void intersect(size_t primA, size_t primB) = 0;

	private:
		bool intersect(
			const bvh::Bvh<double>::Node& a,
			const bvh::Bvh<double>::Node& b) override;
	};

	/*
	* Performs a top-down traversal of two BVH's, calling intersects with the
	* primitive indices of each overlapping leaf node.
	*/
	template<typename PrimitiveIntersectsFunction>
	void intersect(
		const bvh::Bvh<double>& a,
		const bvh::Bvh<double>& b,
		PrimitiveIntersectsFunction intersects)
	{
		traverse(
			a,
			b,
			predicates::intersects,
			intersects
		);
	}
}