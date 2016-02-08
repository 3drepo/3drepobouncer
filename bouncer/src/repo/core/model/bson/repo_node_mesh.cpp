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
	std::vector<repo_vector_t> vertices = getVertices();


	RepoBSONBuilder builder;

	if (vertices.size())
	{ 
		std::vector<repo_vector_t> resultVertice;
		resultVertice.reserve(vertices.size());
		for (const repo_vector_t &v : vertices)
		{
			resultVertice.push_back(multiplyMatVec(matrix, v));
		}



		builder.appendBinary(REPO_NODE_MESH_LABEL_VERTICES, resultVertice.data(), resultVertice.size() * sizeof(repo_vector_t));

		std::vector < repo_vector_t >normals = getNormals();
		if (normals.size())
		{
			std::vector<repo_vector_t> resultNormals;

			const std::vector<float> normMat = transposeMat(invertMat(matrix));
			for (const repo_vector_t &n : normals) {
				repo_vector_t transedNormal = multiplyMatVecFake3x3(matrix, n);
				normalize(transedNormal);
				resultNormals.push_back(transedNormal);
			}

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
		std::set<std::string> fields;
		mapArray.getFieldNames(fields);
		index = fields.size();
	}

	for (uint32_t i = 0; i < vec.size(); ++i)
	{
		mapbuilder << std::to_string(index+i) << meshMappingAsBSON(vec[i]);
	}
	//append the rest of the array onto this new map bson
	if (!overwrite) mapbuilder.appendElementsUnique(mapArray);

	builder.appendArray(REPO_NODE_MESH_LABEL_MERGE_MAP, mapbuilder.obj());

	//append the rest of the mesh onto this new bson
	builder.appendElementsUnique(*this);

	
	return MeshNode(builder.obj(), bigFiles);
}

MeshNode MeshNode::cloneAndRemapMeshMapping(
	const size_t verticeThreshold,
	std::vector<uint16_t> &newFaces,
	std::vector<std::vector<float>> &idMapBuf,
	std::unordered_map<repoUUID, std::vector<uint32_t>, RepoUUIDHasher> &splitMap) const
{
	//A lot easier if we can directly manipulate the binaries without 
	//keep having to modify the mongo buffer, so call cloneAndShrink
	MeshNode mesh = cloneAndShrink();
	std::vector<repo_mesh_mapping_t> newMappings;

	std::vector<repo_mesh_mapping_t> mappings = mesh.getMeshMapping();

	std::sort(mappings.begin(), mappings.end(),
		[](repo_mesh_mapping_t const& a, repo_mesh_mapping_t const& b) { return a.vertFrom < b.vertFrom; });
	auto it = bigFiles.find(REPO_NODE_MESH_LABEL_VERTICES);
	size_t totalVertices, totalFaces;
	
	std::vector<repo_vector_t> vertices = mesh.getVertices();
	std::vector<repo_face_t> faces = mesh.getFaces();
	std::vector<uint32_t> newFacesRepoBuf;



	totalVertices = vertices.size();
	totalFaces    = faces.size();


	size_t subMeshVFrom =  mappings[0].vertFrom, subMeshFFrom = mappings[0].triFrom;
	size_t subMeshVTo, subMeshFTo;
	std::vector<float> bboxMin;
	std::vector<float> bboxMax;

	//Running subMesh Totals
	size_t runningVTotal = 0, runningFTotal = 0;
	size_t runningIdx = 0;
	size_t fullFaceIndex = 0;

	idMapBuf.resize(1);
	idMapBuf.back().clear();

	for (const auto &mapping : mappings)
	{
		splitMap[mapping.mesh_id] = std::vector < uint32_t >() ;

		size_t smVertices = mapping.vertTo  -  mapping.vertFrom;
		size_t smFaces = mapping.triTo - mapping.triFrom;
		size_t currentVFrom = mapping.vertFrom;
		
		if (runningVTotal && runningVTotal + smVertices > verticeThreshold)
		{
			//==================== END OF SUB MESH =====================

			//NOTE: meshIDs and materials ID are not reliable due to a single 
			//      submesh here can be multiple mesh of original scene
			repo_mesh_mapping_t newMap;
			newMap.vertFrom = subMeshVFrom;
			newMap.vertTo = subMeshVTo;
			newMap.triFrom = subMeshFFrom;
			newMap.triTo = subMeshFTo;
			newMap.mesh_id = mapping.mesh_id;
			newMap.material_id = mapping.material_id;

			if (bboxMin.size() && bboxMax.size())
			{
				newMap.min.x = bboxMin[0];
				newMap.min.y = bboxMin[1];
				newMap.min.z = bboxMin[2];

				newMap.max.x = bboxMax[0];
				newMap.max.y = bboxMax[1];
				newMap.max.z = bboxMax[2];
			}
			else
			{
				//Shouldn't ever happen, but not fatal as bbox isn't used in current context
				repoError << "Bounding box was not recorded using a mesh remap!";
			}
				
			newMappings.push_back(newMap);			
			//================== START OF SUB MESH ========================

			//reset counters
			subMeshVFrom = currentVFrom;
			subMeshFFrom = mapping.triFrom;
			subMeshVTo = 0;
			subMeshFTo = 0;
			runningVTotal = 0;
			runningFTotal = 0;
			bboxMin.clear();
			bboxMax.clear();
			idMapBuf.resize(idMapBuf.size() + 1);
			idMapBuf.back().clear();
		}//if (subMeshVTotal + smVertices > verticeThreshold)

		if (smVertices > verticeThreshold)
		{
			//=============CURRENT MAPPING EXCEED THRESHOLD ==============

			//If the current subMesh in question is already bigger than the 
			//threshold already, we need to split this submesh into 2 mappings
			repoTrace << "Limit exceeded splitting large meshes into smaller meshes";
			//Never had a model that has a submesh that is THIS big
			repoDebug << "[WARNING] Entering untested code in cloneAndRemapMeshMapping()";

			std::unordered_map<uint32_t, uint32_t> reIndexMap; //Vertices may be moved during the operation
			std::vector<repo_vector_t> newVertices;
			newVertices.reserve(mapping.vertTo - mapping.vertFrom);

			for (uint32_t faceIdx = 0; faceIdx < smFaces; ++faceIdx)
			{
				uint32_t nVerticesInFace = faces.at(fullFaceIndex).size();
				if (nVerticesInFace != 3)
				{
					repoError << "Non triangulated face with " << nVerticesInFace << " vertices.";
					--runningFTotal;
					--subMeshFFrom;
				}
				else if (runningVTotal + nVerticesInFace > verticeThreshold)
				{
					//With this face included, the running total will exceed the threshold
					//so close up the current subMesh and start a new one

					if (faceIdx)
						splitMap[mapping.mesh_id].push_back(newMappings.size());

					//==================== END OF SUB MESH =====================

					//NOTE: meshIDs and materials ID are not reliable due to a single 
					//      submesh here can be multiple mesh of original scene
					repo_mesh_mapping_t newMap;
					newMap.vertFrom = subMeshVFrom;
					newMap.vertTo = newMap.vertFrom + runningVTotal;

					newMap.triTo = mapping.triFrom + faceIdx;
					newMap.triFrom = subMeshFFrom;

					newMap.mesh_id = mapping.mesh_id;
					newMap.material_id = mapping.material_id;

					if (bboxMin.size() && bboxMax.size())
					{
						newMap.min.x = bboxMin[0];
						newMap.min.y = bboxMin[1];
						newMap.min.z = bboxMin[2];

						newMap.max.x = bboxMax[0];
						newMap.max.y = bboxMax[1];
						newMap.max.z = bboxMax[2];
					}
					else
					{
						//Shouldn't ever happen, but not fatal as bbox isn't used in current context
						repoError << "Bounding box was not recorded using a mesh remap!";
					}
					idMapBuf.back().resize(runningVTotal);
					float runningIdx_f = runningIdx;
					std::fill(idMapBuf.back().begin(), idMapBuf.back().end(), runningIdx_f);
					++runningIdx;
					newMappings.push_back(newMap);
					reIndexMap.clear();

					//================== START OF SUB MESH ========================

					//reset counters
					currentVFrom += runningVTotal;
					subMeshVFrom = currentVFrom;
					subMeshFFrom = mapping.triFrom + faceIdx;
					subMeshVTo = 0;
					subMeshFTo = 0;
					runningVTotal = 0;
					runningFTotal = 0;
					bboxMin.clear();
					bboxMax.clear();
					idMapBuf.resize(idMapBuf.size() + 1);
					idMapBuf.back().clear();
		

				}//else if (runningVTotal + nVerticesInFace > verticeThreshold)


				//==================== ADD CURRENT FACE TO SUB MESH =====================
				newFacesRepoBuf.push_back(nVerticesInFace); 
				for (uint32_t vIdx = 0; vIdx < nVerticesInFace; ++vIdx)
				{
					uint32_t verticeIndex = faces[fullFaceIndex][vIdx];

					if (reIndexMap.find(verticeIndex) == reIndexMap.end())
					{
						reIndexMap[verticeIndex] = runningVTotal;
						newFaces.push_back(runningVTotal);
						newFacesRepoBuf.push_back(runningVTotal);
						//Update Bounding box
						repo_vector_t vertex = vertices[verticeIndex];
						if (bboxMin.size())
						{
							if (bboxMin[0] > vertex.x)
								bboxMin[0] = vertex.x;

							if (bboxMin[1] > vertex.y)
								bboxMin[1] = vertex.y;

							if (bboxMin[2] > vertex.z)
								bboxMin[2] = vertex.z;
						}
						else
						{
							bboxMin = { vertex.x, vertex.y, vertex.z };
						}


						if (bboxMax.size())
						{
							if (bboxMax[0] < vertex.x)
								bboxMax[0] = vertex.x;

							if (bboxMax[1] < vertex.y)
								bboxMax[1] = vertex.y;

							if (bboxMax[2] < vertex.z)
								bboxMax[2] = vertex.z;
						}
						else
						{
							bboxMax = { vertex.x, vertex.y, vertex.z };
						}

						++runningVTotal;
						newVertices.push_back(vertex);


					}
					else
					{
						newFacesRepoBuf.push_back(reIndexMap[verticeIndex]);
						newFaces.push_back(reIndexMap[verticeIndex]);
					}

				}//for (uint32_t vIdx = 0; vIdx < nVerticesInFace; ++vIdx)


				++fullFaceIndex;
				++runningFTotal;

			}//for (uint32_t faceIdx = 0; faceIdx < smFaces; ++faceIdx)

			//Update the subMesh info to include this face


			//Modify the vertices as it may be rearranged within this region
			std::copy(newVertices.begin(), newVertices.end(), vertices.begin() + mapping.vertFrom);

			//Update the subMesh info of the last submesh occupied by this mapping
			subMeshVTo = subMeshVFrom + runningVTotal;
			subMeshFTo = mapping.triTo;

			//runningVTotal = subMeshVTo - subMeshVFrom;
			//runningFTotal = subMeshFTo - subMeshFFrom;

			if (runningFTotal)
				splitMap[mapping.mesh_id].push_back(newMappings.size());

		/*	uint32_t idMapLength = idMapBuf.back().size();
			idMapBuf.back().resize(idMapLength + smVertices);
			float runningIdx_f = runningIdx;
			std::fill(idMapBuf.back().begin() + idMapLength, idMapBuf.back().end(), runningIdx_f);
			++runningIdx;*/

			if (runningVTotal)
			{
				idMapBuf.back().resize(runningVTotal);
				float runningIdx_f = runningIdx;
				std::fill(idMapBuf.back().begin(), idMapBuf.back().end(), runningIdx_f);
				++runningIdx;
			}

			

		}//if (smVertices > verticeThreshold)
		else
		{
			for (uint32_t faceIdx = 0; faceIdx < smFaces; ++faceIdx)
			{
				uint32_t nVerticesInFace = faces[fullFaceIndex].size();
				if (nVerticesInFace != 3)
				{
					repoError << "Non triangulated face with " << nVerticesInFace << " vertices.";
				}
				else
				{
					newFacesRepoBuf.push_back(nVerticesInFace);
					for (uint32_t compIdx = 0; compIdx < nVerticesInFace; ++compIdx)
					{
						uint32_t indexVal = faces[fullFaceIndex][compIdx];
						// Take currentMeshVFrom from Index Value to reset to zero start,
						// then add back in the current running total to append after
						// pervious mesh.

						indexVal += (runningVTotal - currentVFrom);
						newFaces.push_back(indexVal);
						newFacesRepoBuf.push_back(indexVal);
					}
				}

				++fullFaceIndex;
			}


			//Update the subMesh info of the last submesh occupied by this mapping
			subMeshVTo = mapping.vertTo;
			subMeshFTo = mapping.triTo;

			//Update Bounding box
			if (bboxMin.size())
			{
				if (bboxMin[0] > mapping.min.x)
					bboxMin[0] = mapping.min.x;

				if (bboxMin[1] > mapping.min.y)
					bboxMin[1] = mapping.min.y;

				if (bboxMin[2] > mapping.min.z)
					bboxMin[2] = mapping.min.z;
			}
			else
			{
				bboxMin = { mapping.min.x, mapping.min.y, mapping.min.z };
			}


			if (bboxMax.size())
			{
				if (bboxMax[0] < mapping.max.x)
					bboxMax[0] = mapping.max.x;

				if (bboxMax[1] < mapping.max.y)
					bboxMax[1] = mapping.max.y;

				if (bboxMax[2] < mapping.max.z)
					bboxMax[2] = mapping.max.z;
			}
			else
			{
				bboxMax = { mapping.max.x, mapping.max.y, mapping.max.z };
			}

			uint32_t idMapLength = idMapBuf.back().size();
			idMapBuf.back().resize(idMapLength + smVertices);
			float runningIdx_f = runningIdx;
			std::fill(idMapBuf.back().begin() + idMapLength, idMapBuf.back().end(), runningIdx_f);

			++runningIdx;

			runningVTotal += smVertices;
			runningFTotal += smFaces;

			splitMap[mapping.mesh_id].push_back(newMappings.size());

		}//else of (smVertices > verticeThreshold)

	}//for (const auto &mapping : mappings)
	
	repoDebug << "Last sub mesh : " << newMappings.size() << " running VTotal : " << runningVTotal << " running FTotal" << runningFTotal;
	//Finish off the last subMesh
	if (runningVTotal )
	{
		repo_mesh_mapping_t newMap;
		newMap.vertFrom = subMeshVFrom;
		newMap.vertTo = subMeshVTo;
		newMap.triFrom = subMeshFFrom;
		newMap.triTo = subMeshFTo;
		newMap.mesh_id = mappings.back().mesh_id;
		newMap.material_id = mappings.back().material_id;

		if (bboxMin.size() && bboxMax.size())
		{
			newMap.min.x = bboxMin[0];
			newMap.min.y = bboxMin[1];
			newMap.min.z = bboxMin[2];

			newMap.max.x = bboxMax[0];
			newMap.max.y = bboxMax[1];
			newMap.max.z = bboxMax[2];
		}
		else
		{
			//Shouldn't ever happen, but not fatal as bbox isn't used in current context
			repoError << "Bounding box was not recorded using a mesh remap!";
		}
		newMappings.push_back(newMap);
	}
	
	//rebuild the mesh with new faces and vertices and mappings

	auto binBuffers = mesh.getFilesMapping();
	
	binBuffers[REPO_NODE_MESH_LABEL_VERTICES].second.resize(vertices.size() * sizeof(*vertices.data()));
	memcpy(binBuffers[REPO_NODE_MESH_LABEL_VERTICES].second.data(), vertices.data(), vertices.size()*sizeof(*vertices.data()));
	


	binBuffers[REPO_NODE_MESH_LABEL_FACES].second.resize(newFacesRepoBuf.size()*sizeof(*newFacesRepoBuf.data()));
	memcpy(binBuffers[REPO_NODE_MESH_LABEL_FACES].second.data(), newFacesRepoBuf.data(), newFacesRepoBuf.size()*sizeof(*newFacesRepoBuf.data()));

	auto returnMesh =  MeshNode(mesh.cloneAndUpdateMeshMapping(newMappings, true), binBuffers);
	return returnMesh;

}

std::vector<repo_vector_t> MeshNode::getBoundingBox() const
{	
	RepoBSON bbArr = getObjectField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);

	std::vector<repo_vector_t> bbox = getBoundingBox(bbArr);

	return bbox;
}

std::vector<repo_vector_t> MeshNode::getBoundingBox(RepoBSON &bbArr)
{
	std::vector<repo_vector_t> bbox;
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
					repo_vector_t vector;
					vector.x = bbVectorBson.getField("0").Double();
					vector.y = bbVectorBson.getField("1").Double();
					vector.z = bbVectorBson.getField("2").Double();
					
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

std::vector<repo_vector_t> MeshNode::getVertices() const
{
	std::vector<repo_vector_t> vertices;
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
		mappings.resize(fields.size());
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

			RepoBSON boundingBox = mappingObj.getObjectField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);
			
			std::vector<repo_vector_t> bboxVec = getBoundingBox(boundingBox);
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

std::vector<repo_vector_t> MeshNode::getNormals() const
{
	std::vector<repo_vector_t> normals;
	if (hasBinField(REPO_NODE_MESH_LABEL_NORMALS))
	{
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_NORMALS, normals);
	}

	return normals;
}


std::vector<repo_vector2d_t> MeshNode::getUVChannels() const
{
	std::vector<repo_vector2d_t> channels;
	if (hasField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT))
	{
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_UV_CHANNELS, channels);		
	}



	return channels;
}

std::vector<std::vector<repo_vector2d_t>> MeshNode::getUVChannelsSeparated() const
{
	std::vector<std::vector<repo_vector2d_t>> channels;

	std::vector<repo_vector2d_t> serialisedChannels = getUVChannels();

	if (serialisedChannels.size())
	{
		//get number of channels and split the serialised.
		uint32_t nChannels = getField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT).numberInt();
		uint32_t vecPerChannel = serialisedChannels.size() / nChannels;
		channels.reserve(nChannels);
		for (uint32_t i = 0; i < nChannels; i++)
		{
			channels.push_back(std::vector<repo_vector2d_t>());
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
		int32_t facesCount = getField(REPO_NODE_MESH_LABEL_FACES_COUNT).numberInt();
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


	std::vector<repo_vector_t> vertices, vertices2, normals, normals2;
	std::vector<repo_vector2d_t> uvChannels, uvChannels2;
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
