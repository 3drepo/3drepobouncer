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
	* undefined behaviour and should not be called.
	*/
	
	struct RepoPolyDepth
	{
		RepoPolyDepth(
			const std::vector<repo::lib::RepoTriangle>& a, 
			const std::vector<repo::lib::RepoTriangle>& b
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

	private:
		const std::vector<repo::lib::RepoTriangle>& a;
		const std::vector<repo::lib::RepoTriangle>& b;

		using Bvh = bvh::Bvh<double>;

		Bvh bvhA;
		Bvh bvhB;

		repo::lib::RepoMatrix qt;
		repo::lib::RepoMatrix qs;

		void findInitialFreeConfiguration();

		/* 
		* Performs continous collision detection between a and b, where a begins at
		* q0 and ends at q1. Returns the transformation of a at the first point of
		* contact between a and b. The returned transformation will be a linear
		* interpolation between q0 and q1.
		*/
		repo::lib::RepoMatrix ccd(const repo::lib::RepoMatrix& q0, const repo::lib::RepoMatrix& q1);

		/*
		* Returns the minimum distance between a and b, where a is transformed by q.
		* If the meshes are intersecting, the returned distance will be 0.
		*/
		double distance(const repo::lib::RepoMatrix& q);

		/*
		* Tests for intersection between a transformed by m, and b. This will modify the BVHs.
		* The contact patches will be stored in contacts.
		*/
		Collision intersect(
			const repo::lib::RepoMatrix& m
		);

		struct Contact {
			repo::lib::RepoVector3D64 normal;
			double constant;
		};

		std::vector<Contact> contacts;
	};
}