/**
*  Copyright (C) 2026 3D Repo Ltd
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
#include "geometry_utils.h"
#include "repo/lib/datastructure/repo_triangle.h"
#include "repo/manipulator/modeloptimizer/bvh/bvh.hpp"

namespace geometry {
	
	/*
	* RepoDeformDepth is the successor to RepoPolyDepth that estimates the
	* penetration depth between polygon soups under a specific tolerance.
	* 
	* This algorithm uses local deformations, up to a limit, to attempt to find a
	* configuration where the meshes are not intersecting. If the algorithm is
	* unable to find such a configuration, the clash is considered irreconcilable.
	* 
	* Configurations are only valid where the point-wise distance between the start
	* and ending configurations is below the tolerance. The algorithm only attempts
	* to deform mesh (a), which should be the smaller of the two.
	* 
	* This approach is designed to handle cases were meshes have many contact
	* patches, but are not actually overlapping in a meaningful way, and to be
	* robust to the case where a junction is coplanar with a collider.
	*/
	struct RepoDeformDepth {

		/*
		* Optional functor that when set on a PolyDepth instance may be used to test
		* configurations of operand a (m). Should return true if under transform m, a
		* is entirely contained by b, otherwise false. If null, the test is ignored
		* PolyDepth proceeds without it.
		*/
		struct ContainsFunctor {
			virtual bool operator()(
				const std::vector<repo::lib::RepoVector3D64>& points,
				const repo::lib::RepoVector3D64& m) const = 0;
		};

		RepoDeformDepth(
			const geometry::RepoIndexedMesh& a,
			const geometry::RepoIndexedMesh& b,
			ContainsFunctor* containsFunctor = nullptr,
			double tolerance = 0.0);

		void iterate(size_t maxIterations);

		double getPenetrationDepth() const;

	protected:
		double tolerance;

		/*
		* An upper bound on the penetration depth found so far. If this drops below
		* tolerance, we can terminate.
		*/
		double distance;

		/*
		* The current configuration of a's mesh.
		*/
		std::vector<repo::lib::RepoVector3D64> vertices;

		const geometry::RepoIndexedMesh& a;
		const geometry::RepoIndexedMesh& b;

		using Bvh = bvh::Bvh<double>;

		Bvh bvhA;
		Bvh bvhB;

		/*
		* If not null, will be used by the intersect method to determine if mesh (a)
		* is entirely enclosed by mesh (b) under the current configuration. Note that
		* if mesh (b) becomes enclosed during the deformation stage, the algorithm 
		* will terminate immediately, under the assumption that tolerances are always
		* trivial compared to the size of the operands.
		*/
		ContainsFunctor* contains;

		/*
		* Intersects (a) with (b) in the current configuration. Returns true if the
		* meshes are touching (in-contact or in-collision), otherwise false.
		*/
		bool intersect(const repo::lib::RepoVector3D64& m);
		bool intersect();

		/*
		* Attempts to find a better configuration of (a) based on the current contact
		* information in displacements. Returns true if the configuration was 
		* sucessfully updated, or false if for some reason the update could not be made
		* or the changes were otherwise trivial; this may occur if the algorithm has
		* reached a local minima, in which case its likely that the iterate method
		* should terminate as no further improvements will be found.
		*/
		bool updateConfiguration();

		/*
		* Gets the distance of the current configuration from the starting
		* configuration. If this is greater than the tolerance, the algorithm should
		* terminate.
		*/
		double getConfigurationDistance();

		/*
		* Re-fit the BVH of mesh a to match the current configuration.
		*/
		void refitBvh(const repo::lib::RepoVector3D64& m);

		struct Displacement {
			size_t count;
			repo::lib::RepoVector3D64 displacement;
		};

		std::vector<Displacement> displacements;

		void resetDisplacements();

		/*
		* Threshold used to decide how small mesh (a) must be compared to mesh (b)
		* in order to justify a local search around (a). The volume of (a)'s bounds
		* divided by (b)'s bounds must be less than this value for the local search
		* to take place.
		*/
		double localSearchRatioThreshold = 0.3;

		/*
		* How many steps to take along each axis when performing the local search.
		* The total number of steps completed will be 6 * numLocalSearchSteps, so
		* be careful not to make this too large.
		*/
		size_t numLocalSearchSteps = 5;
	};


}