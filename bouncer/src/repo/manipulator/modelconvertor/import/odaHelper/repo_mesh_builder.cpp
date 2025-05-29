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

#include "repo_mesh_builder.h"

#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/lib/repo_exception.h"
#include "vertex_map.h"
#include "helper_functions.h"
#include <map>

using namespace repo::lib;
using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace repo::core::model;

struct RepoMeshBuilder::mesh_data_t {
	std::vector<repo_face_t> faces;
	repo::lib::RepoBounds boundingBox;
	VertexMap vertexMap;
	uint32_t format;
};

RepoMeshBuilder::RepoMeshBuilder(std::vector<repo::lib::RepoUUID> parents, const repo::lib::RepoVector3D64& offset, repo_material_t material)
	: parents(parents),
	offset(offset),
	material(material)
{
}

const std::vector<repo::lib::RepoUUID>& RepoMeshBuilder::getParents()
{
	return parents;
}

repo_material_t RepoMeshBuilder::getMaterial()
{
	return material;
}

RepoMeshBuilder::~RepoMeshBuilder()
{
	// extractMeshes *must* be called, because there is where the mesh data instances are deleted/cleaned up.
	if (meshes.size()) {
		throw repo::lib::RepoGeometryProcessingException("RepoMeshBuilder destroyed with meshes that have not been extracted.");
	}
}

uint32_t RepoMeshBuilder::getMeshFormat(bool hasUvs, bool hasNormals, int faceSize)
{
	uint32_t vBit = 1;
	uint32_t fBit = faceSize << 8;
	uint32_t nBit = hasNormals ? 1 : 0 << 16;
	uint32_t uBit = hasUvs ? 1 : 0 << 17;
	return vBit | fBit | nBit | uBit;
}

uint32_t RepoMeshBuilder::face::getFormat() const
{
	return RepoMeshBuilder::getMeshFormat(hasUvs(), hasNormals(), getSize());
}

bool RepoMeshBuilder::face::hasNormals() const
{
	return numVertices == 3;
}

bool RepoMeshBuilder::face::hasUvs() const
{
	return numUvs > 0;
}

uint8_t RepoMeshBuilder::face::getSize() const
{
	return numVertices;
}

repo::lib::RepoVector3D64 RepoMeshBuilder::face::vertex(size_t i) const 
{
	return vertices[i];
}

repo::lib::RepoVector2D RepoMeshBuilder::face::uv(size_t i) const
{
	return uvs[i];
}

repo::lib::RepoVector3D64 RepoMeshBuilder::face::normal() const
{
	return norm;
}

RepoMeshBuilder::face::face():
	numVertices(0),
	numUvs(0)
{
}

RepoMeshBuilder::face::face(std::initializer_list<repo::lib::RepoVector3D64> vertices):
	face()
{
	setVertices(vertices);
}

RepoMeshBuilder::face::face(std::initializer_list<repo::lib::RepoVector3D64> vertices, repo::lib::RepoVector3D64 normal)
{
	setVertices(vertices);
	this->norm = normal;
	this->norm.normalize();
}

RepoMeshBuilder::face::face(const repo::lib::RepoVector3D64* vertices, size_t numVertices):
	face()
{
	setVertices(vertices, numVertices);
}

RepoMeshBuilder::face::face(const repo::lib::RepoVector3D64* vertices, size_t numVertices, const repo::lib::RepoVector2D* uvs, size_t numUvs) :
	face()
{
	setVertices(vertices, numVertices);
	setUvs(uvs, numUvs);
}

void RepoMeshBuilder::face::setVertices(std::initializer_list<repo::lib::RepoVector3D64> vertices)
{
	setVertices(vertices.begin(), vertices.size());
}

void RepoMeshBuilder::face::setVertices(const repo::lib::RepoVector3D64* newVertices, size_t numNewVertices)
{
	for (auto i = 0; i < numNewVertices; i++) {
		vertices[i] = newVertices[i];
	}
	numVertices = numNewVertices;
	if (numVertices == 3) {
		updateNormal();
	}
}

void RepoMeshBuilder::face::setUvs(std::initializer_list<repo::lib::RepoVector2D> uvs) 
{
	setUvs(uvs.begin(), uvs.size());
}

void RepoMeshBuilder::face::setUvs(const repo::lib::RepoVector2D* newUvs, size_t numNewUvs)
{
	for (auto i = 0; i < numNewUvs; i++) {
		uvs[i] = newUvs[i];
	}
	numUvs = numNewUvs;
}

void RepoMeshBuilder::face::updateNormal()
{
	norm = calcNormal(vertices[0], vertices[1], vertices[2]);
}

RepoMeshBuilder::mesh_data_t* RepoMeshBuilder::startOrContinueMeshByFormat(uint32_t format)
{
	if (!currentMesh || currentMesh->format != format)
	{
		auto itr = meshes.find(format);
		if (itr == meshes.end()) {
			currentMesh = new mesh_data_t();
			currentMesh->format = format;
			meshes[format] = currentMesh;
		}
		else
		{
			currentMesh = itr->second;
		}
	}

	return currentMesh;
}

void RepoMeshBuilder::addFace(const face& bf)
{
	if (!bf.getSize())
	{
		throw repo::lib::RepoGeometryProcessingException("addFace is being called without any vertices. A face must have more than 0 vertices.");
	}

	auto meshData = startOrContinueMeshByFormat(bf.getFormat());

	repo_face_t face;
	for (auto i = 0; i < bf.getSize(); ++i) {
		auto v = bf.vertex(i) + offset;

		size_t vertexReference;
		if (bf.hasNormals())
		{
			if (bf.hasUvs())
			{
				vertexReference = meshData->vertexMap.insert(v, bf.normal(), bf.uv(i));
			}
			else
			{
				vertexReference = meshData->vertexMap.insert(v, bf.normal());
			}
		}
		else if (bf.hasUvs())
		{
			throw repo::lib::RepoGeometryProcessingException("Face has uvs but no normals. This is not supported. Faces that have uvs must also have a normal.");
		}
		else
		{
			vertexReference = meshData->vertexMap.insert(v);
		}

		meshData->boundingBox.encapsulate(v);

		face.push_back(vertexReference);
	}

	meshData->faces.push_back(face);
}

void RepoMeshBuilder::extractMeshes(std::vector<MeshNode>& nodes)
{
	for (auto pair : meshes)
	{
		auto meshData = pair.second;

		if (!meshData->vertexMap.vertices.size()) {
			continue;
		}

		if (meshData->vertexMap.uvs.size() && (meshData->vertexMap.uvs.size() != meshData->vertexMap.vertices.size()))
		{
			throw new repo::lib::RepoGeometryProcessingException("RepoMeshBuilder mesh_data_t vertices size does not match the uvs size");
		}

		auto uvChannels = meshData->vertexMap.uvs.size() ?
			std::vector<std::vector<repo::lib::RepoVector2D>>{meshData->vertexMap.uvs} :
			std::vector<std::vector<repo::lib::RepoVector2D>>();

		std::vector<repo::lib::RepoVector3D> normals32;

		if (meshData->vertexMap.normals.size()) {
			if ((meshData->vertexMap.normals.size() != meshData->vertexMap.vertices.size()))
			{
				throw new repo::lib::RepoGeometryProcessingException("RepoMeshBuilder mesh_data_t vertices size does not match the normals size, where normals are required.");
			}

			normals32.reserve(meshData->vertexMap.normals.size());

			for (int i = 0; i < meshData->vertexMap.vertices.size(); ++i) {
				auto& n = meshData->vertexMap.normals[i];
				normals32.push_back({ (float)(n.x), (float)(n.y), (float)(n.z) });
			}
		}

		std::vector<repo::lib::RepoVector3D> vertices32;
		vertices32.reserve(meshData->vertexMap.vertices.size());

		for (int i = 0; i < meshData->vertexMap.vertices.size(); ++i) {
			auto& v = meshData->vertexMap.vertices[i];
			vertices32.push_back({ (float)(v.x), (float)(v.y), (float)(v.z) });
		}

		auto meshNode = repo::core::model::RepoBSONFactory::makeMeshNode(
			vertices32,
			meshData->faces,
			normals32,
			meshData->boundingBox,
			uvChannels,
			{},
			parents
		);

		delete meshData;

		nodes.push_back(meshNode);
	}

	meshes.clear();
}