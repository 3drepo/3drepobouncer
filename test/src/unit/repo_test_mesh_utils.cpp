/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include "repo_test_mesh_utils.h"

repo::core::model::MeshNode* repo::test::utils::mesh::createRandomMesh(const int nVertices, const bool hasUV, const int primitiveSize, const std::vector<repo::lib::RepoUUID>& parent) 
{
	std::vector<repo::lib::RepoVector3D> vertices;
	std::vector<repo_face_t> faces;

	std::vector<std::vector<float>> bbox = {
		{FLT_MAX, FLT_MAX, FLT_MAX},
		{-FLT_MAX, -FLT_MAX, -FLT_MAX}
	};

	for (int i = 0; i < nVertices; ++i) {
		repo::lib::RepoVector3D vertex = {
			static_cast<float>(std::rand()),
			static_cast<float>(std::rand()),
			static_cast<float>(std::rand())
		};
		vertices.push_back(vertex);

		if (vertex.x < bbox[0][0])
		{
			bbox[0][0] = vertex.x;
		}
		if (vertex.y < bbox[0][1])
		{
			bbox[0][1] = vertex.y;
		}
		if (vertex.z < bbox[0][2])
		{
			bbox[0][2] = vertex.z;
		}

		if (vertex.x > bbox[1][0])
		{
			bbox[1][0] = vertex.x;
		}
		if (vertex.y > bbox[1][1])
		{
			bbox[1][1] = vertex.y;
		}
		if (vertex.z > bbox[1][2])
		{
			bbox[1][2] = vertex.z;
		}
	}

	int vertexIndex = 0;
	do
	{
		repo_face_t face;
		for (int j = 0; j < primitiveSize; j++)
		{
			face.push_back((vertexIndex++ % vertices.size()));
		}
		faces.push_back(face);
	} while (vertexIndex < vertices.size());

	std::vector<std::vector<repo::lib::RepoVector2D>> uvs;
	if (hasUV) {
		std::vector<repo::lib::RepoVector2D> channel;
		for (int i = 0; i < nVertices; ++i) {
			channel.push_back({ (float)std::rand() / RAND_MAX, (float)std::rand() / RAND_MAX });
		}
		uvs.push_back(channel);
	}

	auto mesh = new repo::core::model::MeshNode(repo::core::model::RepoBSONFactory::makeMeshNode(vertices, faces, {}, bbox, uvs, "mesh", parent));

	return mesh;
}

bool repo::test::utils::mesh::compareMeshes(repo::core::model::RepoNodeSet original, repo::core::model::RepoNodeSet stash)
{
	std::vector<GenericFace> defaultFaces;
	for (const auto node : original)
	{
		addFaces(dynamic_cast<repo::core::model::MeshNode*>(node), defaultFaces);
	}

	std::vector<GenericFace> stashFaces;
	for (const auto node : stash)
	{
		addFaces(dynamic_cast<repo::core::model::MeshNode*>(node), stashFaces);
	}

	if (defaultFaces.size() != stashFaces.size())
	{
		return false;
	}

	// Faces are compared exactly using a hash table for speed.

	std::map<long, std::vector<GenericFace>> actual;

	for (auto& face : stashFaces)
	{
		actual[face.hash()].push_back(face);
	}

	for (auto& face : defaultFaces)
	{
		auto& others = actual[face.hash()];
		for (auto& other : others) {
			if (other.hit)
			{
				continue; // Each actual face may only be matched once
			}
			if (other.equals(face))
			{
				face.hit++;
				other.hit++;
				break;
			}
		}
	}

	// Did we find a match for all faces?

	for (const auto& face : defaultFaces)
	{
		if (!face.hit)
		{
			return false;
		}
	}

	return true;
}

void repo::test::utils::mesh::addFaces(repo::core::model::MeshNode* mesh, std::vector<GenericFace>& faces)
{
	if (mesh == nullptr)
	{
		return;
	}

	auto vertices = mesh->getVertices();
	auto channels = mesh->getUVChannelsSeparated();
	auto normals = mesh->getNormals();

	for (const auto face : mesh->getFaces())
	{
		GenericFace portableFace;
		portableFace.hit = 0;

		for (const auto index : face)
		{
			portableFace.push(vertices[index]);

			for (const auto channel : channels)
			{
				portableFace.push(channel[index]);
			}

			if (normals.size())
			{
				portableFace.push(normals[index]);
			}
		}

		faces.push_back(portableFace);
	}
}