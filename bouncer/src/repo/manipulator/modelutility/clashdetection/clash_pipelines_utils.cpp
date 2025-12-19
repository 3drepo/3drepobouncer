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

#include "clash_pipelines_utils.h"

#include "hopscotch-map/bhopscotch_map.h"

using namespace repo::manipulator::modelutility::clash;

void PipelineUtils::loadGeometry(
	DatabasePtr handler,
	Graph::Node& node,
	std::vector<repo::lib::RepoVector3D64>& vertices,
	std::vector<repo::lib::repo_face_t>& faces
)
{
	handler->loadBinaryBuffers(
		node.container->teamspace,
		node.container->container + "." + REPO_COLLECTION_SCENE,
		node.mesh
	);

	auto mesh = repo::core::model::MeshNode(node.mesh);

	vertices.reserve(mesh.getNumVertices());

	struct Hasher
	{
		std::size_t operator()(const repo::lib::RepoVector3D& v) const {
			std::size_t h1 = std::hash<float>{}(v.x);
			std::size_t h2 = std::hash<float>{}(v.y);
			std::size_t h3 = std::hash<float>{}(v.z);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};

	struct Less
	{
		bool operator()(const repo::lib::RepoVector3D& a, const repo::lib::RepoVector3D& b) const {
			if (a.x != b.x) return a.x < b.x;
			if (a.y != b.y) return a.y < b.y;
			return a.z < b.z;
		}
	};

	tsl::bhopscotch_map<
		repo::lib::RepoVector3D,
		uint32_t,
		Hasher,
		std::equal_to<repo::lib::RepoVector3D>,
		Less,
		std::allocator<std::pair<const repo::lib::RepoVector3D, uint32_t>>,
		10,
		true>
	map;

	std::vector<uint32_t> newIndex;
	newIndex.reserve(mesh.getNumVertices());

	auto& originalVertices = mesh.getVertices();
	for(size_t i = 0; i < mesh.getNumVertices(); i++)
	{
		auto v = originalVertices[i];
		auto it = map.find(v);
		if (it == map.end()) {
			size_t newIdx = vertices.size();
			map[v] = newIdx;
			
			// Put the meshes in project coordinates. We create new arrays below
			// as we want to keep the vertices in double precision from now on.

			vertices.push_back(node.matrix * repo::lib::RepoVector3D64(v));

			newIndex.push_back(newIdx);
		}
		else {
			newIndex.push_back(it->second);
		}
	}

	faces = mesh.getFaces();
	for (auto& face : faces) {
		for(size_t i = 0; i < face.size(); i++) {
			face[i] = newIndex[face[i]];
		}
	}

	node.mesh.unloadBinaryBuffers();
}