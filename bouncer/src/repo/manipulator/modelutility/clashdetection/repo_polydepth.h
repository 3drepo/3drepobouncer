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
#include "repo/lib/datastructure/repo_triangle.h"
#include "repo/lib/datastructure/repo_matrix.h"
#include "repo/lib/datastructure/repo_vector3d.h"
#include "repo/manipulator/modeloptimizer/bvh/bvh.hpp"

namespace geometry {
    /*
	* RepoPolyDepth is our implementation of the algorithm described in:
	*
	* PolyDepth: Real-time Penetration Depth Computation using Iterative Contact-
	* Space Projection. Changsoo Je, Min Tang, Youngeun Lee, Minkyoung Lee, Young
	* J. Kim. ACM Transactions on Graphics (ToG 2012), Volume 31, Issue 1, Article
	* 5, pp. 1-14, January 1, 2012. https://arxiv.org/abs/1508.06181
	* 
	* The purpose of the class is to estimate the penetration depth between two
	* triangle meshes.
	*
	* This approach is chosen because it works with polygon soups, beyond which
	* we cannot make any assumptions about our meshes, while also computing an
	* exact upper bounds.
	*
	* The polydepth algorithm works by starting with a collision-free
	* configuration and using CCD to interatively project the meshes back towards
	* the starting configuration. This process is performed iteratively until a
	* contact collision is found.
	*
	* At each iteration there will be at least one collision-free configuration,
	* which defines the upper bound of the penetration depth. The algorithm can
	* run for as long or as little as needed.
	* 
	* RepoPolyDepth can be used to determine if two meshes are intersecting by
	* checking the penetration vector after initialisation. If it is zero, they
	* are already in a collision-free configuration. In this case, iterate has
	* will return without doing anything.
	*/
	
	struct RepoPolyDepth
	{
		/*
		* Optional functor that when set on a PolyDepth instance may be used to test
		* configurations of operand a (m). Should return true if under transform m, a
		* is entirely contained by b, otherwise false. If null, the test is ignored
		* PolyDepth proceeds without it.
		*/
		struct ContainsFunctor {
			virtual bool operator()(const repo::lib::RepoVector3D64& m) const = 0;
		};

		RepoPolyDepth(
			const std::vector<repo::lib::RepoTriangle>& a, 
			const std::vector<repo::lib::RepoTriangle>& b,
			const ContainsFunctor* contains = nullptr
		);

		/*
		* A vector by which to translate the triangles in set A to completely resolve
		* the collision with set B. If the meshes are not intersecting to begin with,
		* this will be zero. The method can be called at any time and return a valid
		* vector below or equal to the upper bounds, but will become more accurate the
		* the more iterations are performed.
		*/
		repo::lib::RepoVector3D64 getPenetrationVector() const;

		/*
		* Updates qi to a better approximation of the nearest in-contact configuration,
		* according to the PolyDepth algorithm. Terminates automatically, after at most
		* n interations.
		*/
		void iterate(size_t n);

		enum class Collision {
			Collision,
			Contact,
			Free
		};		

	protected:
		const std::vector<repo::lib::RepoTriangle>& a;
		const std::vector<repo::lib::RepoTriangle>& b;

		using Bvh = bvh::Bvh<double>;

		Bvh bvhA;
		Bvh bvhB;

		repo::lib::RepoVector3D64 qt;
		repo::lib::RepoVector3D64 qs;

		const ContainsFunctor* contains = nullptr;

		void findInitialFreeConfiguration();

		/* 
		* Performs continous collision detection between a and b, where a begins at
		* q0 and ends at q1. Returns the transformation of a at the first point of
		* contact between a and b. The returned transformation will be a linear
		* interpolation between q0 and q1.
		*/
		repo::lib::RepoVector3D64 ccd(
			const repo::lib::RepoVector3D64& q0, const repo::lib::RepoVector3D64& q1);

		/*
		* Returns the minimum distance between a and b, where a is transformed by q.
		* If the meshes are intersecting, the returned distance will be 0.
		*/
		double distance(const repo::lib::RepoVector3D64& q);

		/*
		* Tests for intersection between a transformed by m, and b. The intersection
		* test uses a distance measure compared to the coplanarity/contact threshold,
		* such that if a ccd iteration terminates at a contact, then this method will
		* return Collision::Contact for that configuration. This will modify the BVHs.
		*/
		Collision intersect(
			const repo::lib::RepoVector3D64& m
		);

		/*
		* Projects m onto the local contact space defined by the planes in the
		* contacts vector.
		*/
		repo::lib::RepoVector3D64 project();

		struct Contact {
			repo::lib::RepoVector3D64 normal;
			double constant;
			double tau;
		};

		std::vector<Contact> contacts;

		/*
		* Helper function to emplace contacts. If the contact has a time of contact
		* smaller than an existing contact, it will displace it. Duplicate contacts
		* are ignored. Depending on the order different time of contacts are added, 
		* there may still be duplicates with different times, so the filter should
		* still be run.
		*/
		void addContact(
			const repo::lib::RepoVector3D64& normal,
			const repo::lib::RepoVector3D64& point,
			double tau
		);

		/*
		* Removes all contacts with a contact time greater than tau.
		*/
		void filterContacts(double tau);

		/*
		* Maximum number of Gauss Seidel iterations to perform when performing in-
		* projection. The algorithm is tolerant to non-convergence, but the better
		* the estimate of q, the fewer overall iterations will be required.
		* In degenerate conditions (such as being trapped between opposing planes),
		* the optimisation could take enormous numbers of iterations to converge,
		* so it should be assumed that this limit will often be reached.
		*/
		size_t maxProjectionIterations = 25;

		/*
		* Tolerance for considering two contact times to be equivalent. The toc will
		* always be between 0 and 1 so this can be set regardless of the scale of the
		* meshes involved.
		*/
		double contactTimeEpsilon = 0.005;

		/*
		* How much to back off from the point of contact after a ccd step, in order
		* to get a collision-free configuration for out-projection.  As ccd returns
		* the first in-contact configuration, reverting a configuration by a non-zero
		* factor cannot result in a new collision.
		* The factor is a proportion of the step size. 1.0 would return to the previous
		* starting configuration, 0.0 does not move at all.
		* Depending on the shape, moving quite far back could be beneficial to 
		* finding the optimal solution, or it could just slow convergence. There is no
		* optimal value, but it must be non-trivially greater than zero to guarantee a
		* backstepped configuration will not remain in-contact.
		*/
		double backStepSize = 0.1;

		/*
		* Threshold for the change in the penetration vector under which we consider
		* the algorithm to have converged. This is an absolute value, in world distance
		* units. When the algorithm finds a local minima, even for large features the
		* distances will change only very slightly, so this can be set quite small.
		*/
		double convergenceEpsilon = 1e-3;

		/*
		* Threshold for the result of a dot product of two normal vectors to decide
		* whether those surfaces are coplanar.
		*/
		double coplanarEpsilon = 0.9999;

		/*
		* Threshold used to decide how small mesh (a) must be compared to mesh (b)
		* in order to justify a local search around (a). The volume of (a)'s bounds
		* divided by (b)'s bounds must be less than this value for the local search
		* to take place.
		*/
		double localSearchRatioThreshold = 0.3;

		/*
		* How far to move mesh (a) during each iteration when performing the local
		* collision free configuration search. This is a proportion of the bounds
		* size.
		*/
		double localSearchStepSize = 0.1;

		/*
		* How many steps to take along each axis when performing the local search.
		* The total number of steps completed will be 6 * numLocalSearchSteps, so
		* be careful not to make this too large.
		*/
		size_t numLocalSearchSteps = 10;
	};
}