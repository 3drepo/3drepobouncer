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
#include "repo/lib/datastructure/repo_bounds.h"
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
				const repo::lib::RepoBounds& bounds,
				const repo::lib::RepoVector3D64& m) const = 0;
		};

		RepoDeformDepth(
			const geometry::RepoIndexedMesh& a,
			const geometry::RepoIndexedMesh& b,
			ContainsFunctor* containsFunctor = nullptr,
			double tolerance = 0.0);

		void iterate(int maxIterations = -1);

		double getPenetrationDepth() const;

		/*
		* Returns the points on A that are in contact with B when the algorithm
		* terminates. This can be returned to the user as a characterisation of the
		* clash. This set may be empty if no contacts were found. Points are returned
		* relative to A's bounding box.
		*/
		std::vector<repo::lib::RepoVector3D64> getContactManifold() const;

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

		/*
		* Normals per vertex of A that are the best estimate of its outer surface.
		*/
		std::vector<repo::lib::RepoVector3D64> pseudoNormals;

		/*
		* The current bounds of the vertices in their deformed configuration. These
		* are used to detect if the mesh is growing instead of shrinking, which can
		* indicate broken normals. They are also necessary for the contains test.
		*/
		repo::lib::RepoBounds verticesBounds;

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
		* Reduces mesh A along its outer surface by the amounts specified in the per-
		* vertex displacements.
		*/
		void deflateMesh();

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

		std::vector<double> distances;

		void resetDisplacements();

		void computePseudoNormals();

		/*
		* How many steps to take along each axis when performing the local search.
		* The total number of steps completed will be 6 * numLocalSearchSteps, so
		* be careful not to make this too large.
		*/
		size_t numLocalSearchSteps = 5;

		/*
		* How much to deflate the mesh in each search step, as a fraction of the
		* tolerance.
		*/
		double deflateStepSize = 0.05;
	};


}