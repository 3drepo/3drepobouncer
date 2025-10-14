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
#include <algorithm>

namespace geometry {

    repo::lib::RepoVector3D64 closestPointTriangle(const repo::lib::RepoVector3D64& p, const repo::lib::RepoTriangle& T);
    repo::lib::RepoLine closestPointPointTriangle(const repo::lib::RepoVector3D64& p, const repo::lib::RepoTriangle& T);

    repo::lib::RepoLine closestPointLineLine(const repo::lib::RepoLine& A, const repo::lib::RepoLine& B);

    /*
    * Given two Triangles, A & B, return a Line that connects the A and B at
    * their closest points to eachother. The Line's start and end points are
    * *not* guaranteed to always be on the same triangle (i.e. they could be
	* from A to B in one case, and from B to A in another).
    */
    repo::lib::RepoLine closestPointTriangleTriangle(const repo::lib::RepoTriangle& A, const repo::lib::RepoTriangle& B);
}