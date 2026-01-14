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

#include "repo/lib/datastructure/repo_structs.h"
#include "repo/lib/datastructure/repo_vector.h"
#include "repo/lib/datastructure/repo_triangle.h"

// This module contains a set of convenience types and functions used by the
// members of the geometry namespace to manipulate meshes and perform tests.

namespace geometry {
	
	/*
	* Very simple convenience type to bind arrays of vertices and faces without
	* needing to use a std::pair.
	* This type is intended to be used as the *output* of a set of operations
	* and represent the geometry in its final form, after being processed from,
	* e.g. MeshNodes, so supports query operations but not transformations.
	*/
	struct RepoIndexedMesh
	{
		std::vector<repo::lib::RepoVector3D64> vertices;
		std::vector<repo::lib::repo_face_t> faces;

		repo::lib::RepoTriangle getTriangle(size_t index) const;
	};

	/*
	* Constructs a single RepoIndexedMesh from a set of discrete meshes.
	*/
	struct RepoIndexedMeshBuilder 
	{
		RepoIndexedMeshBuilder(RepoIndexedMesh& mesh);
		~RepoIndexedMeshBuilder();


		void append(
			const std::vector<repo::lib::RepoVector3D64>& vertices,
			const std::vector<repo::lib::repo_face_t>& faces);

	protected:
		RepoIndexedMesh& mesh;

		class Context;
		Context* ctx;
	};
}