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

#include "geometry_utils.h"

#include "hopscotch-map/bhopscotch_map.h"

using namespace geometry;

repo::lib::RepoTriangle RepoIndexedMesh::getTriangle(size_t index) const
{
	const auto& face = faces[index];
	return repo::lib::RepoTriangle(
		vertices[face[0]],
		vertices[face[1]],
		vertices[face[2]]
	);
}

class RepoIndexedMeshBuilder::Context
{
	// This method will re-index vertices as they are loaded. The current
	// implementation will only consider exact matches. If we want to weld
	// nearby vertices in the future a spatial hash should be used instead.

	struct Hasher
	{
		std::size_t operator()(const repo::lib::RepoVector3D64& v) const {
			std::size_t h1 = std::hash<double>{}(v.x);
			std::size_t h2 = std::hash<double>{}(v.y);
			std::size_t h3 = std::hash<double>{}(v.z);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	struct Less
	{
		bool operator()(const repo::lib::RepoVector3D64& a, const repo::lib::RepoVector3D64& b) const {
			if (a.x != b.x) return a.x < b.x;
			if (a.y != b.y) return a.y < b.y;
			return a.z < b.z;
		}
	};

	// This hopscotch map is much more efficient than std::unordered_map, since it
	// doesn't guarantee iterator stability, a property which is not important for
	// re-indexing like we do here.

	tsl::bhopscotch_map<
		repo::lib::RepoVector3D64,
		size_t,
		Hasher,
		std::equal_to<repo::lib::RepoVector3D64>,
		Less,
		std::allocator<std::pair<const repo::lib::RepoVector3D64, size_t>>,
		10,
		true>
		map;

	std::vector<repo::lib::RepoVector3D64>& vertices;

public:
	RepoIndexedMeshBuilder::Context(std::vector<repo::lib::RepoVector3D64>&	vertices)
		:vertices(vertices)
	{
	}

	size_t find(const repo::lib::RepoVector3D64& v) {
		auto it = map.find(v);
		if (it == map.end()) {
			size_t newIdx = vertices.size();
			vertices.push_back(v);
			map[v] = newIdx;
			return newIdx;
		}
		else {
			return it->second;
		}
	}
};

RepoIndexedMeshBuilder::RepoIndexedMeshBuilder(RepoIndexedMesh& mesh)
	: mesh(mesh)
{
	ctx = new Context(mesh.vertices);
}

RepoIndexedMeshBuilder::~RepoIndexedMeshBuilder()
{
	delete ctx;
}

void RepoIndexedMeshBuilder::append(
	const std::vector<repo::lib::RepoVector3D64>& vertices,
	const std::vector<repo::lib::repo_face_t>& faces)
{
	for (auto& f : faces) {
		repo::lib::repo_face_t newFace;
		for (size_t i = 0; i < f.size(); i++) {
			newFace.push_back(ctx->find(vertices[f[i]]));
		}
		mesh.faces.push_back(newFace);
	}
}

void RepoIndexedMeshBuilder::append(
	const repo::lib::RepoTriangle& triangle)
{
	repo::lib::repo_face_t newFace;
	newFace.push_back(ctx->find(triangle.a));
	newFace.push_back(ctx->find(triangle.b));
	newFace.push_back(ctx->find(triangle.c));
	mesh.faces.push_back(newFace);
}