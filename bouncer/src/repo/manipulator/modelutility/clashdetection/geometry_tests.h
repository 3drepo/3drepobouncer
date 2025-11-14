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

#include "repo/lib/datastructure/repo_vector.h"
#include "repo/lib/datastructure/repo_line.h"
#include "repo/lib/datastructure/repo_triangle.h"
#include "repo/lib/datastructure/repo_bounds.h"
#include <algorithm>

namespace geometry {

    repo::lib::RepoVector3D64 closestPointTriangle(const repo::lib::RepoVector3D64& p, const repo::lib::RepoTriangle& T);
    
    repo::lib::RepoLine closestPointPointTriangle(const repo::lib::RepoVector3D64& p, const repo::lib::RepoTriangle& T);

    repo::lib::RepoLine closestPointLineLine(const repo::lib::RepoLine& A, const repo::lib::RepoLine& B);

    /*
    * Given two Triangles, A & B, return a Line that connects the A and B at
    * their closest points to eachother. The line starts on A and ends on B;
    * moving A by that line will bring the two triangles into contact. If the
    * triangles are co-planar, will return a zero-length line.
    */
    repo::lib::RepoLine closestPointTriangleTriangle(const repo::lib::RepoTriangle& A, const repo::lib::RepoTriangle& B);

    /*
    * Orient predicate in 3D - this returns whether point d is above, below or
    * on the plane defined by a, b and c. The sign is negative if above, and 
    * positive if below (the opposite to the cross-product).
    */
    double orient(const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b, const repo::lib::RepoVector3D64& c, const repo::lib::RepoVector3D64& d);

    /*
    * Determines if two triangles intersect, and if so returns an upper bound on
    * the distance one triangle must move to completely resolve the intersection.
	* Unambiguously disjoint triangles will return exactly zero. Triangles that
    * are in-contact or coplanar will return a small value, or zero.
    * If ip is provided, it will be set to a point on the line of intersection,
    * if any.
    */
	double intersects(const repo::lib::RepoTriangle& a, const repo::lib::RepoTriangle& b, repo::lib::RepoVector3D64* ip = nullptr);

    /*
    * Given two bounds, return the minimum separating axis between them, in the
    * form of a vector, such that if a is moved by that vector, the two bounds
    * will touch on that axis. If the bounds do not intersect, the vector will
    * be empty.
    */
    repo::lib::RepoVector3D64 minimumSeparatingAxis(const repo::lib::RepoBounds& a, repo::lib::RepoBounds& b);

    /*
    * Returns a value approximating the unit of least precision.
    */
    double ulp(double x);

    /*
    * Given two triangles, return a value representing the uncertainty of queries
    * run on them due to rounding error. This value is primarily for use with the
    * intersects method to disambiguate the in-contact and in-collision states.
    */
    double coplanarityThreshold(const repo::lib::RepoTriangle& a, const repo::lib::RepoTriangle& b);

    /*
    * The distance threshold to use for coplanarity tests. This is primarily for
	* the intersects method, but may be useful elsewhere.
    */
	const static double COPLANAR = 1e-15;
}