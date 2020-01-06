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

#include "../../../lib/repo_log.h"
#include "repo_bson_builder.h"
using namespace repo::core::model;

MeshNode::MeshNode() :
RepoNode()
{
}

MeshNode::MeshNode(RepoBSON bson,
	const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>&binMapping) :
	RepoNode(bson, binMapping)
{
}

MeshNode::~MeshNode()
{
}

MeshNode MeshNode::cloneAndFlagIndependent() const {
	RepoBSONBuilder builder;
	builder.append(REPO_NODE_MESH_LABEL_INDEPENDENT, true);
	builder.appendElementsUnique(*this);
	return MeshNode(builder.obj(), bigFiles);
}

bool MeshNode::isIndependent() const {
	return hasField(REPO_NODE_MESH_LABEL_INDEPENDENT) && getBoolField(REPO_NODE_MESH_LABEL_INDEPENDENT);
}

RepoNode MeshNode::cloneAndApplyTransformation(
	const repo::lib::RepoMatrix &matrix) const
{
	if(matrix.isIdentity()) {
		RepoBSONBuilder builder;
		builder.appendElementsUnique(*this);
		return MeshNode(builder.obj(), bigFiles);
	}

	std::vector<repo::lib::RepoVector3D> vertices = getVertices();
	std::vector<repo::lib::RepoVector3D> normals = getNormals();

	auto newBigFiles = bigFiles;

	RepoBSONBuilder builder;
	std::vector<repo::lib::RepoVector3D> resultVertice;
	std::vector<repo::lib::RepoVector3D> newBbox;
	if (vertices.size())
	{
		resultVertice.reserve(vertices.size());
		for (const repo::lib::RepoVector3D &v : vertices)
		{
			resultVertice.push_back(matrix * v);
			if (newBbox.size())
			{
				if (resultVertice.back().x < newBbox[0].x)
					newBbox[0].x = resultVertice.back().x;

				if (resultVertice.back().y < newBbox[0].y)
					newBbox[0].y = resultVertice.back().y;

				if (resultVertice.back().z < newBbox[0].z)
					newBbox[0].z = resultVertice.back().z;

				if (resultVertice.back().x > newBbox[1].x)
					newBbox[1].x = resultVertice.back().x;

				if (resultVertice.back().y > newBbox[1].y)
					newBbox[1].y = resultVertice.back().y;

				if (resultVertice.back().z > newBbox[1].z)
					newBbox[1].z = resultVertice.back().z;
			}
			else
			{
				newBbox.push_back(resultVertice.back());
				newBbox.push_back(resultVertice.back());
			}
		}
		if (newBigFiles.find(REPO_NODE_MESH_LABEL_VERTICES) != newBigFiles.end())
		{
			const uint64_t verticesByteCount = resultVertice.size() * sizeof(repo::lib::RepoVector3D);
			newBigFiles[REPO_NODE_MESH_LABEL_VERTICES].second.resize(verticesByteCount);
			memcpy(newBigFiles[REPO_NODE_MESH_LABEL_VERTICES].second.data(), resultVertice.data(), verticesByteCount);
		}
		else
			builder.appendBinary(REPO_NODE_MESH_LABEL_VERTICES, resultVertice.data(), resultVertice.size() * sizeof(repo::lib::RepoVector3D));

		if (normals.size())
		{
			auto matInverse = matrix.invert();
			auto worldMat = matInverse.transpose();

			std::vector<repo::lib::RepoVector3D> resultNormals;
			resultNormals.reserve(normals.size());

			auto data = worldMat.getData();
			data[3] = data[7] = data[11] = 0;
			data[12] = data[13] = data[14] = 0;

			repo::lib::RepoMatrix multMat(data);

			for (const repo::lib::RepoVector3D &v : normals)
			{
				auto transformedNormal = multMat * v;
				transformedNormal.normalize();
				resultNormals.push_back(transformedNormal);
			}

			if (newBigFiles.find(REPO_NODE_MESH_LABEL_NORMALS) != newBigFiles.end())
			{
				const uint64_t byteCount = resultNormals.size() * sizeof(repo::lib::RepoVector3D);
				newBigFiles[REPO_NODE_MESH_LABEL_NORMALS].second.resize(byteCount);
				memcpy(newBigFiles[REPO_NODE_MESH_LABEL_NORMALS].second.data(), resultNormals.data(), byteCount);
			}
			else
				builder.appendBinary(REPO_NODE_MESH_LABEL_NORMALS, resultNormals.data(), resultNormals.size() * sizeof(repo::lib::RepoVector3D));
		}

		RepoBSONBuilder arrayBuilder, outlineBuilder;
		for (size_t i = 0; i < newBbox.size(); ++i)
		{
			std::vector<float> boundVec = { newBbox[i].x, newBbox[i].y, newBbox[i].z };
			arrayBuilder.appendArray(std::to_string(i), boundVec);
		}

		if (newBbox[0].x > newBbox[1].x || newBbox[0].z > newBbox[1].z || newBbox[0].y > newBbox[1].y)
		{
			repoError << "New bounding box is incorrect!!!";
		}
		builder.appendArray(REPO_NODE_MESH_LABEL_BOUNDING_BOX, arrayBuilder.obj());

		std::vector<float> outline0 = { newBbox[0].x, newBbox[0].y };
		std::vector<float> outline1 = { newBbox[1].x, newBbox[0].y };
		std::vector<float> outline2 = { newBbox[1].x, newBbox[1].y };
		std::vector<float> outline3 = { newBbox[0].x, newBbox[1].y };
		outlineBuilder.appendArray("0", outline0);
		outlineBuilder.appendArray("1", outline1);
		outlineBuilder.appendArray("2", outline2);
		outlineBuilder.appendArray("3", outline3);
		builder.appendArray(REPO_NODE_MESH_LABEL_OUTLINE, outlineBuilder.obj());

		builder.appendElementsUnique(*this);

		return MeshNode(builder.obj(), newBigFiles);
	}
	else
	{
		repoError << "Unable to apply transformation: Cannot find vertices within a mesh!";
		return  RepoNode(*this, bigFiles);
	}
}

MeshNode MeshNode::cloneAndUpdateMeshMapping(
	const std::vector<repo_mesh_mapping_t> &vec,
	const bool                             &overwrite)
{
	RepoBSONBuilder builder, mapbuilder;
	uint32_t index = 0;
	std::vector<repo_mesh_mapping_t> mappings;
	RepoBSON mapArray = getObjectField(REPO_NODE_MESH_LABEL_MERGE_MAP);
	if (!overwrite && !mapArray.isEmpty())
	{
		//if map array isn't empty, find the next index it needs to slot in
		std::set<std::string> fields = mapArray.getFieldNames();
		index = fields.size();
	}

	for (uint32_t i = 0; i < vec.size(); ++i)
	{
		mapbuilder.append(std::to_string(index + i), meshMappingAsBSON(vec[i]));
	}
	//append the rest of the array onto this new map bson
	if (!overwrite) mapbuilder.appendElementsUnique(mapArray);

	builder.appendArray(REPO_NODE_MESH_LABEL_MERGE_MAP, mapbuilder.obj());

	//append the rest of the mesh onto this new bson
	builder.appendElementsUnique(*this);

	return MeshNode(builder.obj(), bigFiles);
}

std::vector<repo::lib::RepoVector3D> MeshNode::getBoundingBox() const
{
	RepoBSON bbArr = getObjectField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);

	std::vector<repo::lib::RepoVector3D> bbox = getBoundingBox(bbArr);

	return bbox;
}

std::vector<repo::lib::RepoVector3D> MeshNode::getBoundingBox(RepoBSON &bbArr)
{
	std::vector<repo::lib::RepoVector3D> bbox;
	if (!bbArr.isEmpty() && bbArr.couldBeArray())
	{
		size_t nVec = bbArr.nFields();
		bbox.reserve(nVec);
		for (uint32_t i = 0; i < nVec; ++i)
		{
			auto bbVectorBson = bbArr.getObjectField(std::to_string(i));
			if (!bbVectorBson.isEmpty() && bbVectorBson.couldBeArray())
			{
				int32_t nFields = bbVectorBson.nFields();

				if (nFields >= 3)
				{
					repo::lib::RepoVector3D vector;
					vector.x = bbVectorBson.getDoubleField("0");
					vector.y = bbVectorBson.getDoubleField("1");
					vector.z = bbVectorBson.getDoubleField("2");

					bbox.push_back(vector);
				}
				else
				{
					repoError << "Insufficient amount of elements within bounding box! #fields: " << nFields;
				}
			}
			else
			{
				repoError << "Failed to get a vector for bounding box!";
			}
		}
	}
	else
	{
		repoError << "Failed to fetch bounding box from Mesh Node!";
	}

	return bbox;
}

std::vector<repo_color4d_t> MeshNode::getColors() const
{
	std::vector<repo_color4d_t> colors = std::vector<repo_color4d_t>();
	if (hasBinField(REPO_NODE_MESH_LABEL_COLORS))
	{
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_COLORS, colors);
	}

	return colors;
}

std::vector<repo::lib::RepoVector3D> MeshNode::getVertices() const
{
	std::vector<repo::lib::RepoVector3D> vertices;
	if (hasBinField(REPO_NODE_MESH_LABEL_VERTICES))
	{
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_VERTICES, vertices);
	}
	else
	{
		repoWarning << "Could not find any vertices within mesh node (" << getUniqueID() << ")";
	}

	return vertices;
}

uint32_t MeshNode::getMFormat() const
{
	/*
	 * maximum of 32 bit, each bit represent the presents of the following
	 * vertices faces normals colors #uvs
	 */

	uint32_t vBit = (uint32_t)hasBinField(REPO_NODE_MESH_LABEL_VERTICES);
	uint32_t fBit = (uint32_t)hasBinField(REPO_NODE_MESH_LABEL_FACES) << 1;
	uint32_t nBit = (uint32_t)hasBinField(REPO_NODE_MESH_LABEL_NORMALS) << 2;
	uint32_t cBit = (uint32_t)hasBinField(REPO_NODE_MESH_LABEL_COLORS) << 3;
	uint32_t uvBits = (hasField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT) ? getIntField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT) : 0) << 4;

	return vBit | fBit | nBit | cBit | uvBits;
}

std::vector<repo_mesh_mapping_t> MeshNode::getMeshMapping() const
{
	std::vector<repo_mesh_mapping_t> mappings;
	RepoBSON mapArray = getObjectField(REPO_NODE_MESH_LABEL_MERGE_MAP);
	if (!mapArray.isEmpty())
	{
		std::set<std::string> fields = mapArray.getFieldNames();
		mappings.resize(fields.size());
		for (const auto &name : fields)
		{
			repo_mesh_mapping_t mapping;
			RepoBSON mappingObj = mapArray.getObjectField(name);

			mapping.mesh_id = mappingObj.getUUIDField(REPO_NODE_MESH_LABEL_MAP_ID);
			mapping.material_id = mappingObj.getUUIDField(REPO_NODE_MESH_LABEL_MATERIAL_ID);
			mapping.vertFrom = mappingObj.getIntField(REPO_NODE_MESH_LABEL_VERTEX_FROM);
			mapping.vertTo = mappingObj.getIntField(REPO_NODE_MESH_LABEL_VERTEX_TO);
			mapping.triFrom = mappingObj.getIntField(REPO_NODE_MESH_LABEL_TRIANGLE_FROM);
			mapping.triTo = mappingObj.getIntField(REPO_NODE_MESH_LABEL_TRIANGLE_TO);

			RepoBSON boundingBox = mappingObj.getObjectField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);

			std::vector<repo::lib::RepoVector3D> bboxVec = getBoundingBox(boundingBox);
			mapping.min.x = bboxVec[0].x;
			mapping.min.y = bboxVec[0].y;
			mapping.min.z = bboxVec[0].z;

			mapping.max.x = bboxVec[1].x;
			mapping.max.y = bboxVec[1].y;
			mapping.max.z = bboxVec[1].z;

			mappings[std::stoi(name)] = mapping;
		}
	}
	return mappings;
}

std::vector<repo::lib::RepoVector3D> MeshNode::getNormals() const
{
	std::vector<repo::lib::RepoVector3D> normals;
	if (hasBinField(REPO_NODE_MESH_LABEL_NORMALS))
	{
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_NORMALS, normals);
	}

	return normals;
}

std::vector<repo::lib::RepoVector2D> MeshNode::getUVChannels() const
{
	std::vector<repo::lib::RepoVector2D> channels;
	if (hasField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT))
	{
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_UV_CHANNELS, channels);
	}

	return channels;
}

std::vector<std::vector<repo::lib::RepoVector2D>> MeshNode::getUVChannelsSeparated() const
{
	std::vector<std::vector<repo::lib::RepoVector2D>> channels;

	std::vector<repo::lib::RepoVector2D> serialisedChannels = getUVChannels();

	if (serialisedChannels.size())
	{
		//get number of channels and split the serialised.
		uint32_t nChannels = getIntField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT);
		uint32_t vecPerChannel = serialisedChannels.size() / nChannels;
		channels.reserve(nChannels);
		for (uint32_t i = 0; i < nChannels; i++)
		{
			channels.push_back(std::vector<repo::lib::RepoVector2D>());
			channels[i].reserve(vecPerChannel);

			uint32_t offset = i*vecPerChannel;
			channels[i].insert(channels[i].begin(), serialisedChannels.begin() + offset,
				serialisedChannels.begin() + offset + vecPerChannel);
		}
	}
	return channels;
}

std::vector<uint32_t> MeshNode::getFacesSerialized() const
{
	std::vector <uint32_t> serializedFaces;
	if (hasBinField(REPO_NODE_MESH_LABEL_FACES))
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_FACES, serializedFaces);

	return serializedFaces;
}

std::vector<repo_face_t> MeshNode::getFaces() const
{
	std::vector<repo_face_t> faces;

	if (hasBinField(REPO_NODE_MESH_LABEL_FACES) && hasField(REPO_NODE_MESH_LABEL_FACES_COUNT))
	{
		std::vector <uint32_t> serializedFaces = std::vector<uint32_t>();
		int32_t facesCount = getIntField(REPO_NODE_MESH_LABEL_FACES_COUNT);
		faces.reserve(facesCount);

		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_FACES, serializedFaces);

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

	return faces;
}

RepoBSON MeshNode::meshMappingAsBSON(const repo_mesh_mapping_t  &mapping)
{
	RepoBSONBuilder builder;
	builder.append(REPO_NODE_MESH_LABEL_MAP_ID, mapping.mesh_id);
	builder.append(REPO_NODE_MESH_LABEL_MATERIAL_ID, mapping.material_id);
	builder.append(REPO_NODE_MESH_LABEL_VERTEX_FROM, mapping.vertFrom);
	builder.append(REPO_NODE_MESH_LABEL_VERTEX_TO, mapping.vertTo);
	builder.append(REPO_NODE_MESH_LABEL_TRIANGLE_FROM, mapping.triFrom);
	builder.append(REPO_NODE_MESH_LABEL_TRIANGLE_TO, mapping.triTo);

	RepoBSONBuilder bbBuilder;
	bbBuilder.append("0", mapping.min);
	bbBuilder.append("1", mapping.max);

	builder.appendArray(REPO_NODE_MESH_LABEL_BOUNDING_BOX, bbBuilder.obj());

	return builder.obj();
}

bool MeshNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::MESH || other.getParentIDs().size() != getParentIDs().size())
	{
		return false;
	}

	MeshNode otherMesh = MeshNode(other);

	std::vector<repo::lib::RepoVector3D> vertices, vertices2, normals, normals2;
	std::vector<repo::lib::RepoVector2D> uvChannels, uvChannels2;
	std::vector<uint32_t> facesSerialized, facesSerialized2;
	std::vector<repo_color4d_t> colors, colors2;

	vertices = getVertices();
	vertices2 = otherMesh.getVertices();

	normals = getNormals();
	normals2 = otherMesh.getNormals();

	uvChannels = getUVChannels();
	uvChannels2 = otherMesh.getUVChannels();

	facesSerialized = getFacesSerialized();
	facesSerialized2 = otherMesh.getFacesSerialized();

	colors = getColors();
	colors2 = otherMesh.getColors();

	//check all the sizes match first, as comparing the content will be costly
	bool success = vertices.size() == vertices2.size()
		&& normals.size() == normals2.size()
		&& uvChannels.size() == uvChannels2.size()
		&& facesSerialized.size() == facesSerialized2.size()
		&& colors.size() == colors2.size();

	if (success)
	{
		if (vertices.size())
		{
			success &= !memcmp(vertices.data(), vertices2.data(), vertices.size() * sizeof(*vertices.data()));
		}

		if (success && normals.size())
		{
			success &= !memcmp(normals.data(), normals2.data(), normals.size() * sizeof(*normals.data()));
		}

		if (success && uvChannels.size())
		{
			success &= !memcmp(uvChannels.data(), uvChannels2.data(), uvChannels.size() * sizeof(*uvChannels.data()));
		}

		if (success && colors.size())
		{
			success &= !memcmp(colors.data(), colors2.data(), colors.size() * sizeof(*colors.data()));
		}

		if (success && facesSerialized.size())
		{
			success &= !memcmp(facesSerialized.data(), facesSerialized.data(), facesSerialized.size() * sizeof(*facesSerialized.data()));
		}
	}

	return success;
}