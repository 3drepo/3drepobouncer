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

using namespace repo::core::model::bson;

MeshNode::MeshNode() :
RepoNode()
{
}

MeshNode::MeshNode(RepoBSON bson) :
RepoNode(bson)
{

}

MeshNode::~MeshNode()
{
}

MeshNode* MeshNode::createMeshNode(
	std::vector<repo_vector_t>                  &vertices,
	std::vector<repo_face_t>                    &faces,
	std::vector<repo_vector_t>                  &normals,
	std::vector<std::vector<float>>             &boundingBox,
	std::vector<std::vector<repo_vector2d_t>>   &uvChannels,
	std::vector<repo_color4d_t>                 &colors,
	std::vector<std::vector<float>>             &outline,
	const int                                   &apiLevel,
	const std::string                           &name)
{
	RepoBSONBuilder builder;

	appendDefaults(builder, REPO_NODE_TYPE_MESH, apiLevel, generateUUID(), name);

	if (vertices.size() > 0)
	{
		builder.appendBinary(
			REPO_NODE_LABEL_VERTICES,
			&vertices[0],
			vertices.size() * sizeof(vertices[0]),
			REPO_NODE_LABEL_VERTICES_BYTE_COUNT,
			REPO_NODE_LABEL_VERTICES_COUNT
			);

	}

	if (faces.size() > 0)
	{
		builder << REPO_NODE_LABEL_FACES_COUNT << (uint32_t)(faces.size());

		// In API LEVEL 1, faces are stored as
		// [n1, v1, v2, ..., n2, v1, v2...]
		std::vector<repo_face_t>::iterator faceIt;

		std::vector<uint32_t> facesLevel1;
		for (faceIt = faces.begin(); faceIt != faces.end(); ++faceIt){
			repo_face_t face = *faceIt;
			facesLevel1.push_back(face.numIndices);
			for (uint32_t ind = 0; ind < face.numIndices; ind++)
			{
				facesLevel1.push_back(face.indices[ind]);
			}
		}

		builder.appendBinary(
			REPO_NODE_LABEL_FACES,
			&facesLevel1[0],
			facesLevel1.size() * sizeof(facesLevel1[0]),
			REPO_NODE_LABEL_FACES_BYTE_COUNT
			);
	}

	if (normals.size() > 0)
	{
		builder.appendBinary(
			REPO_NODE_LABEL_NORMALS,
			&normals[0],
			normals.size() * sizeof(normals[0]));
	}

	if (boundingBox.size() > 0)
	{
		RepoBSONBuilder arrayBuilder;

		for (int i = 0; i < boundingBox.size(); i++)
		{
			arrayBuilder.appendArray(boost::lexical_cast<std::string>(i), builder.createArrayBSON(boundingBox[i]));
		}

		builder.appendArray(REPO_NODE_LABEL_BOUNDING_BOX, arrayBuilder.obj());
	}
	

	if (outline.size() > 0)
	{
		RepoBSONBuilder arrayBuilder;

		for (int i = 0; i < outline.size(); i++)
		{
			arrayBuilder.appendArray(boost::lexical_cast<std::string>(i), builder.createArrayBSON(outline[i]));
		}

		builder.appendArray(REPO_NODE_LABEL_OUTLINE, arrayBuilder.obj());
	}
	
	//if (!vertexHash.empty())
	//{
	//	// TODO: Fix this call - needs to be fixed as int conversion is overloaded
	//	//builder << REPO_NODE_LABEL_SHA256 << (long unsigned int)(vertexHash);
	//}


	//--------------------------------------------------------------------------
	// Vertex colors
	if (colors.size())
		builder.appendBinary(
		REPO_NODE_LABEL_COLORS,
		&colors[0],
		colors.size() * sizeof(colors[0]));

	//--------------------------------------------------------------------------
	// UV channels
	if (uvChannels.size() > 0)
	{
		// Could be unsigned __int64 if BSON had such construct (the closest is only __int64)
		builder << REPO_NODE_LABEL_UV_CHANNELS_COUNT << (uint32_t)(uvChannels.size());

		std::vector<repo_vector2d_t> concatenated;

		std::vector<std::vector<repo_vector2d_t>>::iterator it;
		for (it = uvChannels.begin(); it != uvChannels.end(); ++it)
		{
			std::vector<repo_vector2d_t> channel = *it;

			std::vector<repo_vector2d_t>::iterator cit;
			for (cit = channel.begin(); cit != channel.end(); ++cit)
			{
				concatenated.push_back(*cit);
			}
		}

		builder.appendBinary(
			REPO_NODE_LABEL_UV_CHANNELS,
			&concatenated[0],
			concatenated.size() * sizeof(concatenated[0]),
			REPO_NODE_LABEL_UV_CHANNELS_BYTE_COUNT);
	}

	return new MeshNode(builder.obj());
}

std::vector<repo_color4d_t>* MeshNode::getColors() const
{
	std::vector<repo_color4d_t> *colors = new std::vector<repo_color4d_t>();
	if (hasField(REPO_NODE_LABEL_COLORS))
	{
		getBinaryFieldAsVector(getField(REPO_NODE_LABEL_COLORS), colors);
	}

	return colors;
}

std::vector<repo_vector_t>* MeshNode::getVertices() const
{
	std::vector<repo_vector_t> *vertices = new std::vector<repo_vector_t>();
	if (hasField(REPO_NODE_LABEL_VERTICES_COUNT)
		&& hasField(REPO_NODE_LABEL_VERTICES))
	{
		const uint32_t verticesSize = getField(REPO_NODE_LABEL_VERTICES_COUNT).numberInt();
		getBinaryFieldAsVector(getField(REPO_NODE_LABEL_VERTICES), verticesSize, vertices);
	}

	return vertices;
}


std::vector<repo_vector_t>* MeshNode::getNormals() const
{
	std::vector<repo_vector_t> *vertices = new std::vector<repo_vector_t>();
	if (hasField(REPO_NODE_LABEL_VERTICES_COUNT)
		&& hasField(REPO_NODE_LABEL_NORMALS))
	{
		const uint32_t verticesSize = getField(REPO_NODE_LABEL_VERTICES_COUNT).numberInt();
		getBinaryFieldAsVector(getField(REPO_NODE_LABEL_NORMALS), verticesSize, vertices);
	}

	return vertices;
}


std::vector<repo_vector2d_t>* MeshNode::getUVChannels() const
{
	std::vector<repo_vector2d_t> *channels = new std::vector<repo_vector2d_t>();
	if (hasField(REPO_NODE_LABEL_VERTICES_COUNT) 
		&& hasField(REPO_NODE_LABEL_UV_CHANNELS_COUNT)
		&& hasField(REPO_NODE_LABEL_UV_CHANNELS))
	{
		const uint32_t uvChannelSize = getField(REPO_NODE_LABEL_VERTICES_COUNT).numberInt() 
			* getField(REPO_NODE_LABEL_UV_CHANNELS_COUNT).numberInt();
		getBinaryFieldAsVector(getField(REPO_NODE_LABEL_UV_CHANNELS), uvChannelSize, channels);
	}

	return channels;
}

std::vector<repo_face_t>* MeshNode::getFaces() const
{
	std::vector<repo_face_t> *faces = new std::vector<repo_face_t>();
	if (hasField(REPO_NODE_LABEL_FACES)
		&& hasField(REPO_NODE_LABEL_FACES_BYTE_COUNT)
		&& hasField(REPO_NODE_LABEL_FACES_COUNT))
	{
		int32_t facesCount     = getField(REPO_NODE_LABEL_FACES_COUNT).numberInt();
		int32_t facesByteCount = getField(REPO_NODE_LABEL_FACES_BYTE_COUNT).numberInt();

		faces->resize(facesCount);
		std::vector <uint32_t> *serializedFaces = new std::vector<uint32_t>();
		
		serializedFaces->resize(facesByteCount / sizeof(uint32_t));

		getBinaryFieldAsVector(getField(REPO_NODE_LABEL_FACES), facesByteCount, serializedFaces);

		// Retrieve numbers of vertices for each face and subsequent
		// indices into the vertex array.
		// In API level 1, mesh is represented as
		// [n1, v1, v2, ..., n2, v1, v2...]
		
		unsigned int counter = 0;
		int mNumIndicesIndex = 0;
		while (counter < facesCount)
		{
			int mNumIndices = serializedFaces->at(mNumIndicesIndex);
			repo_face_t face;
			face.numIndices = mNumIndices;
			uint32_t *indices = new uint32_t[mNumIndices];
			for (int i = 0; i < mNumIndices; ++i)
				indices[i] = serializedFaces->at(mNumIndicesIndex + 1 + i);
			face.indices = indices;
			(*faces)[counter] = face;
			mNumIndicesIndex = mNumIndicesIndex + mNumIndices + 1;
			++counter;
		}

		// Memory cleanup
		if (serializedFaces)
			delete serializedFaces;

	}

	return faces;
}