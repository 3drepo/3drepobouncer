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

#include "repo_node_streaming_mesh.h"

repo::core::model::StreamingMeshNode::SupermeshingData::SupermeshingData(const repo::core::model::RepoBSON& bson, const std::vector<uint8_t>& buffer, const bool ignoreUVs)
{
	this->uniqueId = bson.getUUIDField(REPO_NODE_LABEL_ID);
	deserialise(bson, buffer, ignoreUVs);
}

void repo::core::model::StreamingMeshNode::SupermeshingData::bakeMeshes(const repo::lib::RepoMatrix& transform)
{
	// Vertices
	for (int i = 0; i < vertices.size(); i++) {
		vertices[i] = transform * vertices[i];
	}

	// Normals
	auto matInverse = transform.invert();
	auto worldMat = matInverse.transpose();

	auto data = worldMat.getData();
	data[3] = data[7] = data[11] = 0;
	data[12] = data[13] = data[14] = 0;

	repo::lib::RepoMatrix multMat(data);

	for (int i = 0; i < normals.size(); i++) {
		normals[i] = multMat * normals[i];
		normals[i].normalize();
	}
}

void repo::core::model::StreamingMeshNode::SupermeshingData::deserialise(const repo::core::model::RepoBSON& bson, const std::vector<uint8_t>& buffer, const bool ignoreUVs)
{
	auto blobRefBson = bson.getObjectField(REPO_LABEL_BINARY_REFERENCE);
	auto elementsBson = blobRefBson.getObjectField(REPO_LABEL_BINARY_ELEMENTS);

	if (elementsBson.hasField(REPO_NODE_MESH_LABEL_VERTICES)) {
		auto vertBson = elementsBson.getObjectField(REPO_NODE_MESH_LABEL_VERTICES);
		deserialiseVector(vertBson, buffer, vertices);
	}

	if (elementsBson.hasField(REPO_NODE_MESH_LABEL_NORMALS)) {
		auto normBson = elementsBson.getObjectField(REPO_NODE_MESH_LABEL_NORMALS);
		deserialiseVector(normBson, buffer, normals);
	}

	if (elementsBson.hasField(REPO_NODE_MESH_LABEL_FACES)) {

		int32_t faceCount = bson.getIntField(REPO_NODE_MESH_LABEL_FACES_COUNT);
		faces.reserve(faceCount);

		std::vector<uint32_t> serialisedFaces = std::vector<uint32_t>();
		auto faceBson = elementsBson.getObjectField(REPO_NODE_MESH_LABEL_FACES);
		deserialiseVector(faceBson, buffer, serialisedFaces);

		// Retrieve numbers of vertices for each face and subsequent
		// indices into the vertex array.
		// In API level 1, mesh is represented as
		// [n1, v1, v2, ..., n2, v1, v2...]

		int mNumIndicesIndex = 0;
		while (serialisedFaces.size() > mNumIndicesIndex)
		{
			int mNumIndices = serialisedFaces[mNumIndicesIndex];
			if (serialisedFaces.size() > mNumIndicesIndex + mNumIndices)
			{
				repo::lib::repo_face_t face;
				face.resize(mNumIndices);
				for (int i = 0; i < mNumIndices; ++i)
					face[i] = serialisedFaces[mNumIndicesIndex + 1 + i];
				faces.push_back(face);
				mNumIndicesIndex += mNumIndices + 1;
			}
			else
			{
				repoError << "Cannot copy all faces. Buffer size is smaller than expected!";
			}
		}

	}

	if (!ignoreUVs && elementsBson.hasField(REPO_NODE_MESH_LABEL_UV_CHANNELS)) {
		std::vector<repo::lib::RepoVector2D> serialisedChannels;
		auto uvBson = elementsBson.getObjectField(REPO_NODE_MESH_LABEL_UV_CHANNELS);
		deserialiseVector(uvBson, buffer, serialisedChannels);

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

repo::core::model::StreamingMeshNode::StreamingMeshNode(const repo::core::model::RepoBSON& bson)
{
	if (bson.hasField(REPO_NODE_LABEL_SHARED_ID)) {
		sharedId = bson.getUUIDField(REPO_NODE_LABEL_SHARED_ID);
	}
	if (bson.hasField(REPO_NODE_MESH_LABEL_VERTICES_COUNT)) {
		numVertices = bson.getIntField(REPO_NODE_MESH_LABEL_VERTICES_COUNT);
	}
	if (bson.hasField(REPO_NODE_LABEL_PARENTS)) {
		auto parents = bson.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);
		parent = parents[0];
	}
	if (bson.hasField(REPO_NODE_MESH_LABEL_BOUNDING_BOX)) {
		bounds = bson.getBoundsField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);
	}
}

void repo::core::model::StreamingMeshNode::loadSupermeshingData(const repo::core::model::RepoBSON& bson, const std::vector<uint8_t>& buffer, const bool ignoreUVs)
{
	if (supermeshingDataLoaded())
	{
		repoWarning << "StreamingMeshNode instructed to load geometry data, but geometry data is already loaded.";
		unloadSupermeshingData();
	}

	supermeshingData = std::make_unique<SupermeshingData>(bson, buffer, ignoreUVs);
}

void repo::core::model::StreamingMeshNode::transformBounds(const repo::lib::RepoMatrix& transform)
{
	auto newMinBound = transform * bounds.min();
	auto newMaxBound = transform * bounds.max();
	bounds = repo::lib::RepoBounds(newMinBound, newMaxBound);
}

const repo::lib::RepoUUID repo::core::model::StreamingMeshNode::getUniqueId()
{
	if (supermeshingDataLoaded()) {
		return supermeshingData->getUniqueId();
	}
	else {
		repoError << "Tried to access supermesh geometry of StreamingMeshNode without loading geometry first. Empty returned.";
		return repo::lib::RepoUUID();
	}
}

const std::uint32_t repo::core::model::StreamingMeshNode::getNumLoadedFaces() {
	if (supermeshingDataLoaded()) {
		return supermeshingData->getNumFaces();
	}
	else {
		repoError << "Tried to access supermesh geometry of StreamingMeshNode without loading geometry first. Empty returned.";
		return 0;
	}
}

const std::vector<repo::lib::repo_face_t>& repo::core::model::StreamingMeshNode::getLoadedFaces()
{
	if (supermeshingDataLoaded()) {
		return supermeshingData->getFaces();
	}
	else {
		repoError << "Tried to access supermesh geometry of StreamingMeshNode without loading geometry first. Empty returned.";
		return emptyFace;
	}
}

const std::uint32_t repo::core::model::StreamingMeshNode::getNumLoadedVertices() {
	if (supermeshingDataLoaded()) {
		return supermeshingData->getNumVertices();
	}
	else {
		repoError << "Tried to access supermesh geometry of StreamingMeshNode without loading geometry first. Empty returned.";
		return 0;
	}
}

const std::vector<repo::lib::RepoVector3D>& repo::core::model::StreamingMeshNode::getLoadedVertices() {
	if (supermeshingDataLoaded()) {
		return supermeshingData->getVertices();
	}
	else {
		repoError << "Tried to access supermesh geometry of StreamingMeshNode without loading geometry first. Empty returned.";
		return empty3D;
	}
}

void repo::core::model::StreamingMeshNode::bakeLoadedMeshes(const repo::lib::RepoMatrix& transform) {
	if (supermeshingDataLoaded()) {
		supermeshingData->bakeMeshes(transform);
	}
	else {
		repoError << "Tried to access supermesh geometry of StreamingMeshNode without loading geometry first. No action performed.";
	}
}

const std::vector<repo::lib::RepoVector3D>& repo::core::model::StreamingMeshNode::getLoadedNormals()
{
	if (supermeshingDataLoaded()) {
		return supermeshingData->getNormals();
	}
	else {
		repoError << "Tried to access supermesh geometry of StreamingMeshNode without loading geometry first. Empty returned.";
		return empty3D;
	}
}

const std::vector<std::vector<repo::lib::RepoVector2D>>& repo::core::model::StreamingMeshNode::getLoadedUVChannelsSeparated()
{
	if (supermeshingDataLoaded()) {
		return supermeshingData->getUVChannelsSeparated();
	}
	else {
		repoError << "Tried to access supermesh geometry of StreamingMeshNode without loading geometry first. Empty returned.";
		return emptyUV;
	}
}
