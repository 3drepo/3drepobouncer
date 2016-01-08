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

RepoNode MeshNode::cloneAndApplyTransformation(
	const std::vector<float> &matrix) const
{
	std::vector<repo_vector_t> *vertices = getVertices();


	RepoBSONBuilder builder;

	if (vertices)
	{ 
		std::vector<repo_vector_t> resultVertice;
		resultVertice.reserve(vertices->size());
		for (const repo_vector_t &v : *vertices)
		{
			resultVertice.push_back(multiplyMatVec(matrix, v));
		}



		builder.appendBinary(REPO_NODE_MESH_LABEL_VERTICES, resultVertice.data(), resultVertice.size() * sizeof(repo_vector_t));

		delete vertices;

		std::vector < repo_vector_t >*normals = getNormals();
		if (normals && normals->size())
		{
			std::vector<repo_vector_t> resultNormals;

			const std::vector<float> normMat = transposeMat(invertMat(matrix));
			for (const repo_vector_t &n : *normals) {
				repo_vector_t transedNormal = multiplyMatVecFake3x3(matrix, n);
				normalize(transedNormal);
				resultNormals.push_back(transedNormal);
			}

			delete normals;
			builder.appendBinary(REPO_NODE_MESH_LABEL_NORMALS, resultNormals.data(), resultNormals.size() * sizeof(repo_vector_t));
		}
	}
	else
	{
		repoError << "Unable to apply transformation: Cannot find vertices within a mesh!";
	}

	return MeshNode(builder.appendElementsUnique(*this));
}

MeshNode MeshNode::cloneAndUpdateMeshMapping(
	const std::vector<repo_mesh_mapping_t> &vec)
{
	RepoBSONBuilder builder, mapbuilder;
	uint32_t index = 0; 
	std::vector<repo_mesh_mapping_t> mappings;
	RepoBSON mapArray = getObjectField(REPO_NODE_MESH_LABEL_MERGE_MAP);
	if (!mapArray.isEmpty())
	{
		//if map array isn't empty, find the next index it needs to slot in
		std::set<std::string> fields;
		mapArray.getFieldNames(fields);
		index = fields.size();
	}

	for (uint32_t i = 0; i < vec.size(); ++i)
	{
		mapbuilder << std::to_string(index+i) << meshMappingAsBSON(vec[i]);
	}
	repoLog("cloneAndUpdateMeshMapping : size of mesh map = " + (vec.size() + index));
	//append the rest of the array onto this new map bson
	mapbuilder.appendElementsUnique(mapArray);

	builder.appendArray(REPO_NODE_MESH_LABEL_MERGE_MAP, mapbuilder.obj());

	//append the rest of the mesh onto this new bson
	builder.appendElementsUnique(*this);

	return MeshNode(builder.obj(), bigFiles);
	//TODO run this tomorrow and see if i can get away with not overloading the equals operator!!!
}

std::vector<repo_color4d_t>* MeshNode::getColors() const
{

	std::vector<repo_color4d_t> *colors = new std::vector<repo_color4d_t>();
	if (hasBinField(REPO_NODE_MESH_LABEL_COLORS))
	{
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_COLORS, colors);
	}

	return colors;
}

std::vector<repo_vector_t>* MeshNode::getVertices() const
{
	std::vector<repo_vector_t> *vertices = new std::vector<repo_vector_t>();
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

std::vector<repo_mesh_mapping_t> MeshNode::getMeshMapping() const
{
	std::vector<repo_mesh_mapping_t> mappings;
	RepoBSON mapArray = getObjectField(REPO_NODE_MESH_LABEL_MERGE_MAP);
	if (!mapArray.isEmpty())
	{
		std::set<std::string> fields;
		mapArray.getFieldNames(fields);
		mappings.reserve(fields.size());
		for (const auto &name : fields)
		{
			repo_mesh_mapping_t mapping;
			RepoBSON mappingObj = mapArray.getObjectField(name);

			mapping.mesh_id     = mappingObj.getUUIDField(REPO_NODE_MESH_LABEL_MAP_ID);
			mapping.material_id = mappingObj.getUUIDField(REPO_NODE_MESH_LABEL_MATERIAL_ID);
			mapping.vertFrom    = mappingObj.getField(REPO_NODE_MESH_LABEL_VERTEX_FROM).Int();
			mapping.vertTo      = mappingObj.getField(REPO_NODE_MESH_LABEL_VERTEX_TO).Int();
			mapping.triFrom     = mappingObj.getField(REPO_NODE_MESH_LABEL_TRIANGLE_FROM).Int();
			mapping.triTo       = mappingObj.getField(REPO_NODE_MESH_LABEL_TRIANGLE_TO).Int();

			RepoBSON boundingBox = getObjectField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);
			
			std::set<std::string> bbfields;
			boundingBox.getFieldNames(bbfields);


			auto bbfieldIT = bbfields.begin();
			if (bbfields.size() >= 2)
			{
				std::vector<float> min = getFloatArray(*bbfieldIT++);
				if (min.size() >= 3)
				{
					mapping.min.x = min[0];
					mapping.min.y = min[1];
					mapping.min.z = min[2];
				}
				std::vector<float> max = getFloatArray(*bbfieldIT++);
				if (max.size() >= 3)
				{
					mapping.max.x = max[0];
					mapping.max.y = max[1];
					mapping.max.z = max[2];
				}
			}
			else
			{
				repoError << "bounding box has " << bbfields.size() << " vectors!";
			}

			mappings.push_back(mapping);

		}

	}

	return mappings;
}

std::vector<repo_vector_t>* MeshNode::getNormals() const
{
	std::vector<repo_vector_t> *vertices = new std::vector<repo_vector_t>();
	if (hasBinField(REPO_NODE_MESH_LABEL_NORMALS))
	{
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_NORMALS, vertices);
	}

	return vertices;
}


std::vector<repo_vector2d_t>* MeshNode::getUVChannels() const
{
	std::vector<repo_vector2d_t> *channels = nullptr;
	if (hasField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT))
	{
		channels = new std::vector<repo_vector2d_t>();

		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_UV_CHANNELS, channels);		
	}



	return channels;
}

std::vector<std::vector<repo_vector2d_t>>* MeshNode::getUVChannelsSeparated() const
{
	std::vector<std::vector<repo_vector2d_t>> *channels = nullptr;

	std::vector<repo_vector2d_t> *serialisedChannels = getUVChannels();

	if (serialisedChannels)
	{
		//get number of channels and split the serialised.
		channels = new std::vector<std::vector<repo_vector2d_t>>();

		uint32_t nChannels = getField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT).numberInt();
		uint32_t vecPerChannel = serialisedChannels->size() / nChannels;
		channels->reserve(nChannels);
		for (uint32_t i = 0; i < nChannels; i++)
		{
			channels->push_back(std::vector<repo_vector2d_t>());
			channels->at(i).reserve(vecPerChannel);

			uint32_t offset = i*vecPerChannel;
			channels->at(i).insert(channels->at(i).begin(), serialisedChannels->begin() + offset,
				serialisedChannels->begin() + offset + vecPerChannel);

		}

		delete serialisedChannels;
	}
	return channels;
}

std::vector<uint32_t> MeshNode::getFacesSerialized() const
{

	std::vector <uint32_t> serializedFaces;
	if (hasBinField(REPO_NODE_MESH_LABEL_FACES))
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_FACES, &serializedFaces);

	return serializedFaces;
}

std::vector<repo_face_t>* MeshNode::getFaces() const
{
	std::vector<repo_face_t> *faces = new std::vector<repo_face_t>();

	if (hasBinField(REPO_NODE_MESH_LABEL_FACES) && hasField(REPO_NODE_MESH_LABEL_FACES_COUNT))
	{
		std::vector <uint32_t> *serializedFaces = new std::vector<uint32_t>();
		int32_t facesCount = getField(REPO_NODE_MESH_LABEL_FACES_COUNT).numberInt();
		faces->reserve(facesCount);

		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_FACES, serializedFaces);
		
		// Retrieve numbers of vertices for each face and subsequent
		// indices into the vertex array.
		// In API level 1, mesh is represented as
		// [n1, v1, v2, ..., n2, v1, v2...]
		
		int mNumIndicesIndex = 0;
		while (serializedFaces->size() > mNumIndicesIndex)
		{

			int mNumIndices = serializedFaces->at(mNumIndicesIndex);
			if (serializedFaces->size() > mNumIndicesIndex + mNumIndices)
			{
				repo_face_t face;
				face.numIndices = mNumIndices;
				face.indices.resize(mNumIndices);
				for (int i = 0; i < mNumIndices; ++i)
					face.indices[i] = serializedFaces->at(mNumIndicesIndex + 1 + i);
				faces->push_back(face);
				mNumIndicesIndex += mNumIndices + 1;
			}
			else
			{
				repoError << "Cannot copy all faces. Buffer size is smaller than expected!";
			}
		}

		// Memory cleanup
		if (serializedFaces)
			delete serializedFaces;

	}

	return faces;
}

RepoBSON MeshNode::meshMappingAsBSON(const repo_mesh_mapping_t  &mapping)
{
	RepoBSONBuilder builder;
	builder.append(REPO_NODE_MESH_LABEL_MAP_ID, mapping.mesh_id);
	builder.append(REPO_NODE_MESH_LABEL_MATERIAL_ID, mapping.material_id);
	builder << REPO_NODE_MESH_LABEL_VERTEX_FROM << mapping.vertFrom;
	builder << REPO_NODE_MESH_LABEL_VERTEX_TO << mapping.vertTo;
	builder << REPO_NODE_MESH_LABEL_TRIANGLE_FROM << mapping.triFrom;
	builder << REPO_NODE_MESH_LABEL_TRIANGLE_TO << mapping.triTo;

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


	std::vector<repo_vector_t> *vertices, *vertices2, *normals, *normals2;
	std::vector<repo_vector2d_t>* uvChannels, *uvChannels2;
	std::vector<uint32_t> facesSerialized, facesSerialized2;
	std::vector<repo_color4d_t>* colors, *colors2;

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
	bool success = false;

	//FIXME: why is this so messy... actually why do we have pointers to vectors?!
	if (vertices && (success = (bool)vertices == (bool)vertices2))
	{
		success &= vertices->size() == vertices2->size();
	}

	if (normals && (success &= (bool) normals == (bool)normals2))
	{
		success &= normals->size() == normals2->size();
	}

	if (uvChannels && (success &= (bool) uvChannels == (bool)uvChannels2))
	{
		success &= uvChannels->size() == uvChannels2->size();
	}

	if (success)
		success &= facesSerialized.size() == facesSerialized2.size();

	if (colors && (success &= (bool)colors == (bool)colors2))
	{
		success &= colors->size() == colors2->size();
	}


	if (success)
	{

		if (vertices && vertices->size())
		{
			success &= !memcmp(vertices->data(), vertices2->data(), vertices->size() * sizeof(*vertices->data()));
		}

		if (normals && success && normals->size())
		{
			success &= !memcmp(normals->data(), normals2->data(), normals->size() * sizeof(*normals->data()));
		}

		if (uvChannels && success && uvChannels->size())
		{
			success &= !memcmp(uvChannels->data(), uvChannels2->data(), uvChannels->size() * sizeof(*uvChannels->data()));
		}

		if (colors && success && colors->size())
		{
			success &= !memcmp(colors->data(), colors2->data(), colors->size() * sizeof(*colors->data()));
		}

		if (success && facesSerialized.size())
		{
			success &= !memcmp(facesSerialized.data(), facesSerialized.data(), facesSerialized.size() * sizeof(*facesSerialized.data()));
		}
	}

	delete vertices;
	delete normals;
	delete uvChannels;
	delete vertices2;
	delete normals2;
	delete uvChannels2;

	return success;
}