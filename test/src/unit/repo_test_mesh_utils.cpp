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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include "repo_test_mesh_utils.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/model/bson/repo_bson_builder.h"

using namespace repo::core::model;
using namespace testing;

MeshNode* repo::test::utils::mesh::createRandomMesh(const int nVertices, const bool hasUV, const int primitiveSize, const std::vector<repo::lib::RepoUUID>& parent) 
{
	auto mesh = makeMeshNode(mesh_data(true, true, 0, primitiveSize, false, hasUV ? 1 : 0, nVertices));
	mesh.addParents(parent);
	return new MeshNode(mesh);
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

/**
* This implementation should be the reference for the node database schema and
* should be effectively independent, but equivalent, to the serialise method
* in MeshNode.
*/
RepoBSON repo::test::utils::mesh::meshNodeTestBSONFactory(mesh_data data)
{
	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_ID, data.uniqueId);

	if (!data.sharedId.isDefaultValue())
	{
		builder.append(REPO_NODE_LABEL_SHARED_ID, data.sharedId);
	}

	builder.append(REPO_NODE_LABEL_TYPE, "mesh");

	if (data.parents.size() > 0)
	{
		builder.appendArray(REPO_NODE_LABEL_PARENTS, data.parents);
	}

	if (!data.name.empty())
	{
		builder.append(REPO_NODE_LABEL_NAME, data.name);
	}

	if (data.boundingBox.size() > 0)
	{
		RepoBSONBuilder arrayBuilder;
		for (int i = 0; i < data.boundingBox.size(); i++)
		{
			arrayBuilder.appendArray(std::to_string(i), data.boundingBox[i]);
		}

		builder.appendArray(REPO_NODE_MESH_LABEL_BOUNDING_BOX, arrayBuilder.obj());
	}

	if (data.vertices.size() > 0)
	{
		builder.append(REPO_NODE_MESH_LABEL_VERTICES_COUNT, (uint32_t)(data.vertices.size()));
		builder.appendLargeArray(REPO_NODE_MESH_LABEL_VERTICES, data.vertices);
	}

	if (data.faces.size() > 0)
	{
		builder.append(REPO_NODE_MESH_LABEL_FACES_COUNT, (uint32_t)(data.faces.size()));

		// In API LEVEL 1, faces are stored as
		// [n1, v1, v2, ..., n2, v1, v2...]
		std::vector<repo_face_t>::iterator faceIt;

		MeshNode::Primitive primitive = MeshNode::Primitive::UNKNOWN;

		std::vector<uint32_t> facesLevel1;
		for (auto& face : data.faces) {
			auto nIndices = face.size();
			if (!nIndices)
			{
				repoWarning << "number of indices in this face is 0!";
			}
			if (primitive == MeshNode::Primitive::UNKNOWN) // The primitive type is unknown, so attempt to infer it
			{
				if (nIndices == 2) {
					primitive = MeshNode::Primitive::LINES;
				}
				else if (nIndices == 3) {
					primitive = MeshNode::Primitive::TRIANGLES;
				}
				else // The primitive type is not one we support
				{
					repoWarning << "unsupported primitive type - only lines and triangles are supported but this face has " << nIndices << " indices!";
				}
			}
			else  // (otherwise check for consistency with the existing type)
			{
				if (nIndices != static_cast<int>(primitive))
				{
					repoWarning << "mixing different primitives within a mesh is not supported!";
				}
			}
			facesLevel1.push_back(nIndices);
			for (uint32_t ind = 0; ind < nIndices; ind++)
			{
				facesLevel1.push_back(face[ind]);
			}
		}

		builder.append(REPO_NODE_MESH_LABEL_PRIMITIVE, static_cast<int>(primitive));

		builder.appendLargeArray(REPO_NODE_MESH_LABEL_FACES, facesLevel1);
	}

	if (data.normals.size() > 0)
	{
		builder.appendLargeArray(REPO_NODE_MESH_LABEL_NORMALS, data.normals);
	}

	if (data.uvChannels.size() > 0)
	{
		std::vector<repo::lib::RepoVector2D> concatenated;

		for (auto it = data.uvChannels.begin(); it != data.uvChannels.end(); ++it)
		{
			std::vector<repo::lib::RepoVector2D> channel = *it;

			std::vector<repo::lib::RepoVector2D>::iterator cit;
			for (cit = channel.begin(); cit != channel.end(); ++cit)
			{
				concatenated.push_back(*cit);
			}
		}

		if (concatenated.size() > 0)
		{
			// Could be unsigned __int64 if BSON had such construct (the closest is only __int64)
			builder.append(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT, (uint32_t)(data.uvChannels.size()));
			builder.appendLargeArray(REPO_NODE_MESH_LABEL_UV_CHANNELS, concatenated);
		}
	}

	return builder.obj();
}

MeshNode repo::test::utils::mesh::makeMeshNode(mesh_data data)
{
	return MeshNode(meshNodeTestBSONFactory(data));
}

MeshNode repo::test::utils::mesh::makeMeshNode(int primitive, bool normals, int uvs)
{
	return makeMeshNode(mesh_data(false, false, false, primitive, normals, uvs, 100));
}

std::vector<repo::lib::RepoVector3D> repo::test::utils::mesh::makeVertices(int num)
{
	std::vector<repo::lib::RepoVector3D> vertices;
	for (int i = 0; i < num; i++) {
		vertices.push_back(repo::lib::RepoVector3D(rand() - rand(), rand() - rand(), rand() - rand()));
	}
	return vertices;
}

std::vector<repo::lib::RepoVector3D> repo::test::utils::mesh::makeNormals(int num)
{
	std::vector<repo::lib::RepoVector3D> normals;
	for (int i = 0; i < num; i++) {
		auto n = repo::lib::RepoVector3D(rand(), rand(), rand());
		n.normalize();
		normals.push_back(n);
	}
	return normals;
}

std::vector<repo::lib::RepoVector2D> repo::test::utils::mesh::makeUVs(int num)
{
	std::vector<repo::lib::RepoVector2D> uvs;
	for (int i = 0; i < num; i++) {
		uvs.push_back(repo::lib::RepoVector2D(rand(), rand()));
	}
	return uvs;
}

std::vector<repo_face_t> repo::test::utils::mesh::makeFaces(MeshNode::Primitive primitive)
{
	std::vector<repo_face_t> faces;
	for (int i = 0; i < 100; i++)
	{
		repo_face_t face;
		for (int i = 0; i < (int)primitive; i++)
		{
			face.push_back(rand());
		}
		faces.push_back(face);
	}
	return faces;
}

std::vector<repo::lib::RepoVector3D> repo::test::utils::mesh::getBoundingBox(std::vector< repo::lib::RepoVector3D> vertices)
{
	repo::lib::RepoVector3D min = repo::lib::RepoVector3D(FLT_MAX, FLT_MAX, FLT_MAX);
	repo::lib::RepoVector3D max = repo::lib::RepoVector3D(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (auto& v : vertices)
	{
		min = repo::lib::RepoVector3D::min(min, v);
		max = repo::lib::RepoVector3D::max(max, v);
	}
	return std::vector<repo::lib::RepoVector3D>({ min, max });
}

repo::test::utils::mesh::mesh_data::mesh_data(
	bool name,
	bool sharedId,
	int numParents,
	int faceSize,
	bool normals,
	int numUvChannels,
	int numVertices
)
{
	std::srand(0);

	if (name) {
		this->name = "Named Mesh";
	}

	if (sharedId) {
		this->sharedId = repo::lib::RepoUUID::createUUID();
	}

	for (int i = 0; i < numParents; i++) {
		parents.push_back(repo::lib::RepoUUID::createUUID());
	}

	uniqueId = repo::lib::RepoUUID::createUUID();

	repo::lib::RepoVector3D min = repo::lib::RepoVector3D(FLT_MAX, FLT_MAX, FLT_MAX);
	repo::lib::RepoVector3D max = repo::lib::RepoVector3D(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int i = 0; i < numVertices; i++)
	{
		vertices.push_back(repo::lib::RepoVector3D(rand() - rand(), rand() - rand(), rand() - rand()));
		min = repo::lib::RepoVector3D::min(min, vertices[vertices.size() - 1]);
		max = repo::lib::RepoVector3D::max(max, vertices[vertices.size() - 1]);
	}

	int vertexIndex = 0;
	do
	{
		repo_face_t face;
		for (int j = 0; j < faceSize; j++)
		{
			face.push_back((vertexIndex++ % vertices.size()));
		}
		faces.push_back(face);
	} while (vertexIndex < vertices.size());

	if (normals)
	{
		this->normals = makeNormals(numVertices);
	}

	for (int i = 0; i < numUvChannels; i++)
	{
		this->uvChannels.push_back(makeUVs(numVertices));
	}

	this->boundingBox.push_back({
		min.x, min.y, min.z
		});

	this->boundingBox.push_back({
		max.x, max.y, max.z
		});
}

repo::lib::RepoMatrix repo::test::utils::mesh::makeTransform(bool translation, bool rotation)
{
	repo::lib::RepoMatrix m;
	
	if (rotation)
	{
		m = m * repo::lib::RepoMatrix::rotationX((float)rand() / RAND_MAX) * repo::lib::RepoMatrix::rotationY((float)rand() / RAND_MAX) * repo::lib::RepoMatrix::rotationZ((float)rand() / RAND_MAX);
	}

	if (translation)
	{
		m = m * repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D(rand(), rand(), rand()));
	}
	
	return m;
}

void repo::test::utils::mesh::compareMeshNode(mesh_data expected, MeshNode actual)
{
	EXPECT_THAT(actual.getUniqueID(), Eq(expected.uniqueId));
	EXPECT_THAT(actual.getSharedID(), Eq(expected.sharedId));
	EXPECT_THAT(actual.getParentIDs(), UnorderedElementsAreArray(expected.parents));
	EXPECT_THAT(actual.getName(), Eq(expected.name));
	EXPECT_THAT(actual.getNumFaces(), Eq(expected.faces.size()));
	EXPECT_THAT(actual.getFaces(), ElementsAreArray(expected.faces));
	EXPECT_THAT(actual.getNumVertices(), Eq(expected.vertices.size()));
	EXPECT_THAT(actual.getVertices(), ElementsAreArray(expected.vertices));
	EXPECT_THAT(actual.getNormals(), ElementsAreArray(expected.normals));
	EXPECT_THAT(actual.getNumUVChannels(), Eq(expected.uvChannels.size()));
	for (int i = 0; i < expected.uvChannels.size(); i++)
	{
		EXPECT_THAT(actual.getUVChannelsSeparated()[i], ElementsAreArray(expected.uvChannels[i]));
	}
}
