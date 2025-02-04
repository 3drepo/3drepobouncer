/**
*  Copyright (C) 2015 3D Repo Ltd
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

/**
*  Mesh node
*/

#include "repo_node_mesh.h"

#include "repo/lib/repo_log.h"
#include "repo_bson_builder.h"

using namespace repo::core::model;

MeshNode::MeshNode() :
	RepoNode()
{
	grouping = "";
	primitive = MeshNode::Primitive::TRIANGLES;
}

MeshNode::MeshNode(RepoBSON bson) :
	RepoNode(bson)
{
	deserialise(bson);
}

MeshNode::~MeshNode()
{
}

void MeshNode::deserialise(RepoBSON& bson)
{
	grouping = "";
	if (bson.hasField(REPO_NODE_MESH_LABEL_GROUPING))
		grouping = bson.getStringField(REPO_NODE_MESH_LABEL_GROUPING);

	primitive = MeshNode::Primitive::TRIANGLES;
	if (bson.hasField(REPO_NODE_MESH_LABEL_PRIMITIVE))
		primitive = static_cast<MeshNode::Primitive>(bson.getIntField(REPO_NODE_MESH_LABEL_PRIMITIVE));

	boundingBox = bson.getBoundsField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);

	if (bson.hasBinField(REPO_NODE_MESH_LABEL_FACES) && bson.hasField(REPO_NODE_MESH_LABEL_FACES_COUNT))
	{
		std::vector <uint32_t> serializedFaces = std::vector<uint32_t>();
		int32_t facesCount = bson.getIntField(REPO_NODE_MESH_LABEL_FACES_COUNT);
		faces.reserve(facesCount);

		bson.getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_FACES, serializedFaces);

		// Retrieve numbers of vertices for each face and subsequent
		// indices into the vertex array.
		// In API level 1, mesh is represented as
		// [n1, v1, v2, ..., n2, v1, v2...]

		int mNumIndicesIndex = 0;
		while (serializedFaces.size() > mNumIndicesIndex)
		{
			int mNumIndices = serializedFaces[mNumIndicesIndex];
			if (serializedFaces.size() > mNumIndicesIndex + mNumIndices)
			{
				repo_face_t face;
				face.resize(mNumIndices);
				for (int i = 0; i < mNumIndices; ++i)
					face[i] = serializedFaces[mNumIndicesIndex + 1 + i];
				faces.push_back(face);
				mNumIndicesIndex += mNumIndices + 1;
			}
			else
			{
				repoError << "Cannot copy all faces. Buffer size is smaller than expected!";
			}
		}
	}

	if (bson.hasBinField(REPO_NODE_MESH_LABEL_VERTICES))
	{
		bson.getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_VERTICES, vertices);
	}
	else
	{
		repoWarning << "Could not find any vertices within mesh node (" << getUniqueID() << ")";
	}

	if (bson.hasBinField(REPO_NODE_MESH_LABEL_NORMALS))
	{
		bson.getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_NORMALS, normals);
	}


	if (bson.hasField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT))
	{
		std::vector<repo::lib::RepoVector2D> serialisedChannels;
		bson.getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_UV_CHANNELS, serialisedChannels);

		if (serialisedChannels.size())
		{
			//get number of channels and split the serialised.
			uint32_t nChannels = bson.getIntField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT);
			uint32_t vecPerChannel = serialisedChannels.size() / nChannels;
			channels.reserve(nChannels);
			for (uint32_t i = 0; i < nChannels; i++)
			{
				channels.push_back(std::vector<repo::lib::RepoVector2D>());
				channels[i].reserve(vecPerChannel);

				uint32_t offset = i * vecPerChannel;
				channels[i].insert(channels[i].begin(), serialisedChannels.begin() + offset,
					serialisedChannels.begin() + offset + vecPerChannel);
			}
		}
	}
}

void appendBounds(RepoBSONBuilder& builder, const repo::lib::RepoBounds& boundingBox)
{
	builder.append(REPO_NODE_MESH_LABEL_BOUNDING_BOX, boundingBox);
}

void appendVertices(RepoBSONBuilder& builder, const std::vector<repo::lib::RepoVector3D>& vertices)
{
	if (vertices.size() > 0)
	{
		builder.append(REPO_NODE_MESH_LABEL_VERTICES_COUNT, (int32_t)(vertices.size()));
		builder.appendLargeArray(REPO_NODE_MESH_LABEL_VERTICES, vertices);
	}
}

void appendFaces(RepoBSONBuilder& builder, const std::vector<repo::repo_face_t>& faces)
{
	if (faces.size() > 0)
	{
		builder.append(REPO_NODE_MESH_LABEL_FACES_COUNT, (int32_t)(faces.size()));

		// In API LEVEL 1, faces are stored as
		// [n1, v1, v2, ..., n2, v1, v2...]

		MeshNode::Primitive primitive = MeshNode::Primitive::UNKNOWN;

		std::vector<uint32_t> facesLevel1;
		for (auto& face : faces) {
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
}

void appendNormals(RepoBSONBuilder& builder, const std::vector<repo::lib::RepoVector3D>& normals)
{
	if (normals.size() > 0)
	{
		builder.appendLargeArray(REPO_NODE_MESH_LABEL_NORMALS, normals);
	}
}

void appendUVChannels(RepoBSONBuilder& builder, size_t numChannels, const std::vector<repo::lib::RepoVector2D> concatenated)
{
	if (concatenated.size() > 0)
	{
		if (concatenated.size() > 0)
		{
			// Could be unsigned __int64 if BSON had such construct (the closest is only __int64)
			builder.append(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT, (int32_t)numChannels);
			builder.appendLargeArray(REPO_NODE_MESH_LABEL_UV_CHANNELS, concatenated);
		}
	}
}

void MeshNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	RepoNode::serialise(builder);
	appendBounds(builder, boundingBox);
	appendVertices(builder, vertices);
	appendFaces(builder, faces);
	appendNormals(builder, normals);
	appendUVChannels(builder, channels.size(), getUVChannelsSerialised());
}

std::vector<repo::lib::RepoVector2D> MeshNode::getUVChannelsSerialised() const
{
	std::vector<repo::lib::RepoVector2D> concatenated;
	for (auto it = channels.begin(); it != channels.end(); ++it)
	{
		std::vector<repo::lib::RepoVector2D> channel = *it;
		std::vector<repo::lib::RepoVector2D>::iterator cit;
		for (cit = channel.begin(); cit != channel.end(); ++cit)
		{
			concatenated.push_back(*cit);
		}
	}
	return concatenated;
}

void MeshNode::setUVChannel(size_t channel, std::vector<repo::lib::RepoVector2D> uvs)
{
	if (uvs.size())
	{
		channels.resize(std::max(channels.size(), channel + 1));
		channels[channel] = std::vector<repo::lib::RepoVector2D>(uvs.begin(), uvs.end());
	}
}

void MeshNode::updateBoundingBox()
{
	boundingBox = repo::lib::RepoBounds(); // reset the bounding box
	for (auto& v : vertices) {
		boundingBox.encapsulate(v);
	}
}

MeshNode MeshNode::cloneAndApplyTransformation(
	const repo::lib::RepoMatrix &matrix) const
{
	MeshNode copy = *this;	// Copy the node
	copy.applyTransformation(matrix);
	return copy;
}

void MeshNode::applyTransformation(
	const repo::lib::RepoMatrix& matrix)
{
	if (!matrix.isIdentity())
	{
		boundingBox = repo::lib::RepoBounds();

		for (auto& v : vertices) {
			v = matrix * v;
			boundingBox.encapsulate(v);
		}

		if (normals.size())
		{
			auto matInverse = matrix.invert();
			auto worldMat = matInverse.transpose();

			auto data = worldMat.getData();
			data[3] = data[7] = data[11] = 0;
			data[12] = data[13] = data[14] = 0;

			repo::lib::RepoMatrix multMat(data);

			for (auto& n : normals)
			{
				n = multMat * n;
				n.normalize();
			}
		}
	}
}

void MeshNode::transformBoundingBox(
	repo::lib::RepoBounds& bounds,
	repo::lib::RepoMatrix matrix)
{
	// Compute the updated AABB by the method of the extrema of transformed
	// corners.
	// For completeness, there is a faster way described by Jim Avro in Graphics
	// Gems (1990), but it requires the Translation and Rotation to be to be
	// separated (and the performance improvement would only be noticable if we
	// were doing, e.g. realtime physics etc).

	std::vector<lib::RepoVector3D64> corners = {
		matrix * lib::RepoVector3D64(bounds.min().x, bounds.min().y, bounds.min().z),
		matrix * lib::RepoVector3D64(bounds.min().x, bounds.min().y, bounds.max().z),
		matrix * lib::RepoVector3D64(bounds.min().x, bounds.max().y, bounds.min().z),
		matrix * lib::RepoVector3D64(bounds.min().x, bounds.max().y, bounds.max().z),
		matrix * lib::RepoVector3D64(bounds.max().x, bounds.min().y, bounds.min().z),
		matrix * lib::RepoVector3D64(bounds.max().x, bounds.min().y, bounds.max().z),
		matrix * lib::RepoVector3D64(bounds.max().x, bounds.max().y, bounds.min().z),
		matrix * lib::RepoVector3D64(bounds.max().x, bounds.max().y, bounds.max().z),
	};

	bounds = repo::lib::RepoBounds();
	for (auto& c : corners)
	{
		bounds.encapsulate(c);
	}
}

uint32_t MeshNode::getMFormat(const bool isTransparent, const bool isInvisibleDefault) const
{
	/*
	 * maximum of 32 bit, each bit represent the presents of the following
	 * vertices faces normals colors #uvs #type
	 */

	uint32_t vBit = (uint32_t)(bool)vertices.size();
	uint32_t fBit = (uint32_t)(bool)faces.size() << 1;
	uint32_t nBit = (uint32_t)(bool)normals.size() << 2;
	uint32_t rBit = (uint32_t)0 << 3; // Reserved (was the Colour bit)

	/*
	 * The current packing supports 255 uv channels and 255 face index counts, both
	 * much higher than currently used in any realtime graphics pipeline
	 * The offsets for the multi-bit fields are powers of 2 for simplicity.
	 */

	uint32_t uvBits = (channels.size() & 0xFF) << 8;
	uint32_t typeBits = (static_cast<int>(getPrimitive()) & 0xFF) << 16;

	/*
	* Mesh-level flags that are not representative of the geometry but
	* are useful for filtering.
	*/

	uint32_t transBit = isTransparent ? 1 : 0 << 4;
	uint32_t visiBit = isInvisibleDefault ? 1 : 0 << 5;

	return vBit | fBit | nBit | rBit | uvBits | typeBits;
}

bool MeshNode::sEqual(const RepoNode &other) const
{
	auto otherMesh = dynamic_cast<const MeshNode*>(&other);

	bool success = false;

	if (otherMesh != nullptr)
	{
		success = true;
		success &= grouping == otherMesh->grouping;
		success &= primitive == otherMesh->primitive;
		success &= boundingBox == otherMesh->boundingBox;
		success &= faces == otherMesh->faces;
		success &= vertices == otherMesh->vertices;
		success &= normals == otherMesh->normals;
		success &= channels == otherMesh->channels;
	}

	return success;
}

size_t MeshNode::getSize() const
{
	// Implementation aims to be as quick as possible as we can expect this to
	// be called alot now streaming imports are used.

	size_t size = 0;
	size += faces.size() * (int)primitive * 4;
	size += vertices.size() * sizeof(repo::lib::RepoVector3D);
	size += normals.size() * sizeof(repo::lib::RepoVector3D);
	size += channels.size() * vertices.size() * sizeof(repo::lib::RepoVector2D);
	size += sizeof(*this);
	return size;
}

void hash_combine(size_t& seed, float v)
{
	std::hash<float> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct Vertex
{
	repo::lib::RepoVector3D position;
	std::optional<repo::lib::RepoVector3D> normal;
	std::optional<repo::lib::RepoVector2D> uv;

	size_t hash()
	{
		size_t hash = 0;
		hash_combine(hash, position.x);
		hash_combine(hash, position.y);
		hash_combine(hash, position.z);
		if (normal)
		{
			hash_combine(hash, normal->x);
			hash_combine(hash, normal->y);
			hash_combine(hash, normal->z);
		}
		if (uv)
		{
			hash_combine(hash, uv->x);
			hash_combine(hash, uv->y);
		}
		return hash;
	}

	bool operator== (const Vertex& other) const = default;
};

void MeshNode::removeDuplicateVertices()
{
	bool useNormals = normals.size();
	bool useUvs = channels.size();

	if (channels.size() > 1) {
		throw repo::lib::RepoGeometryProcessingException("removeDuplicateVertices currently only supports one uv channel.");
	}

	std::vector<repo::lib::RepoVector2D> uvs;
	if (useUvs) {
		uvs = channels[0];
	}

	std::vector<Vertex> newVertices;
	std::unordered_multimap<size_t, size_t> map;

	for (auto& f : faces)
	{
		for (auto i = 0; i < f.size(); i++)
		{
			auto& index = f[i];

			Vertex v;
			v.position = vertices[index];

			if (useNormals) {
				v.normal = normals[index];
			}

			if (useUvs) {
				v.uv = uvs[index];
			}

			auto hash = v.hash();
			auto matching = map.equal_range(hash);

			index = -1;
			for (auto it = matching.first; it != matching.second; it++)
			{
				if (newVertices[it->second] == v)
				{
					index = it->second;
					break;
				}
			}

			if (index == -1) // (face indices are unsigned, so compare directly to a rolled over positive integer, instead of ltz)
			{
				index = newVertices.size();
				newVertices.push_back(v);
				map.insert(std::pair<size_t, size_t>(hash, index));
			}
		}
	}

	vertices.clear();
	normals.clear();
	uvs.clear();

	for (auto& v : newVertices)
	{
		vertices.push_back(v.position);
		if (v.normal)
		{
			normals.push_back(*v.normal);
		}
		if (v.uv) {
			uvs.push_back(*v.uv);
		}
	}

	if (useUvs) {
		channels[0] = uvs;
	}
}