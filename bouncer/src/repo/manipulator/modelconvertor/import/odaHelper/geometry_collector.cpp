/**
*  Copyright (C) 2018 3D Repo Ltd
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

#include "geometry_collector.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

using namespace repo::manipulator::modelconvertor::odaHelper;

GeometryCollector::GeometryCollector()
{
}

GeometryCollector::~GeometryCollector()
{
}

repo::core::model::CameraNode generateCameraNode(camera_t camera, repo::lib::RepoUUID parentID)
{
	auto node = repo::core::model::RepoBSONFactory::makeCameraNode(camera.aspectRatio, camera.farClipPlane, camera.nearClipPlane, camera.FOV, camera.eye, camera.pos, camera.up, camera.name);
	node = node.cloneAndAddParent(parentID);
	return node;
}

void GeometryCollector::addCameraNode(camera_t node)
{
	cameras.push_back(node);
}

repo::core::model::RepoNodeSet GeometryCollector::getCameraNodes(repo::lib::RepoUUID parentID)
{
	repo::core::model::RepoNodeSet camerasNodeSet;
	for (auto& camera : cameras)
		camerasNodeSet.insert(new repo::core::model::CameraNode(generateCameraNode(camera, parentID)));
	return camerasNodeSet;
}

bool GeometryCollector::hasCameraNodes()
{
	return cameras.size();
}

repo::core::model::TransformationNode GeometryCollector::createRootNode()
{
	return repo::core::model::RepoBSONFactory::makeTransformationNode(rootMatrix, "rootNode");
}

void GeometryCollector::setCurrentMaterial(const repo_material_t &material, bool missingTexture) {
	auto checkSum = material.checksum();
	if (idxToMat.find(checkSum) == idxToMat.end()) {
		idxToMat[checkSum] = {
			repo::core::model::RepoBSONFactory::makeMaterialNode(material),
			createTextureNode(material.texturePath)
		};

		if (missingTexture)
			this->missingTextures = true;
	}
	currMat = checkSum;
}

repo::core::model::RepoNodeSet repo::manipulator::modelconvertor::odaHelper::GeometryCollector::getMetaNodes()
{
	return metaNodes;
}

mesh_data_t GeometryCollector::createMeshEntry(uint32_t format) {
	mesh_data_t entry;
	entry.matIdx = currMat;
	entry.name = nextMeshName;
	entry.groupName = nextGroupName;
	entry.layerName = nextLayer.empty() ? "UnknownLayer" : nextLayer;
	entry.format = format;
	return entry;
}

void GeometryCollector::startMeshEntry() {
	nextMeshName = nextMeshName.empty() ? boost::lexical_cast<std::string>(uuidGenerator()) : nextMeshName;
	nextGroupName = nextGroupName.empty() ? nextMeshName : nextGroupName;
	nextLayer = nextLayer.empty() ? nextMeshName : nextLayer;

	if (meshData.find(nextGroupName) == meshData.end()) {
		meshData[nextGroupName] = std::unordered_map<std::string, std::unordered_map<int, std::vector<mesh_data_t>>>();
	}

	if (meshData[nextGroupName].find(nextLayer) == meshData[nextGroupName].end()) {
		meshData[nextGroupName][nextLayer] = std::unordered_map<int, std::vector<mesh_data_t>>();
	}

	if (meshData[nextGroupName][nextLayer].find(currMat) == meshData[nextGroupName][nextLayer].end()) {
		meshData[nextGroupName][nextLayer][currMat] = std::vector<mesh_data_t>();
	}

	currentEntry = &meshData[nextGroupName][nextLayer][currMat];
}

void  GeometryCollector::stopMeshEntry() {
	nextMeshName = "";
	currentEntry = nullptr;
	currentMesh = nullptr;
}

uint32_t GeometryCollector::getMeshFormat(bool hasUvs, bool hasNormals, int faceSize)
{
	uint32_t vBit = 1;
	uint32_t fBit = faceSize << 8;
	uint32_t nBit = hasNormals ? 1 : 0 << 16;
	uint32_t uBit = hasUvs ? 1 : 0 << 17;
	return vBit | fBit | nBit | uBit;
}

mesh_data_t* GeometryCollector::startOrContinueMeshByFormat(uint32_t format)
{
	if (!currentMesh || currentMesh->format != format)
	{
		currentMesh = nullptr;

		if (!currentEntry)
		{
			startMeshEntry();
		}

		for (auto meshDataIterator = currentEntry->begin(); meshDataIterator != currentEntry->end(); meshDataIterator++)
		{
			if (meshDataIterator->format == format)
			{
				currentMesh = meshDataIterator._Ptr;
			}
		}

		if (!currentMesh)
		{
			currentEntry->push_back(createMeshEntry(format));
			currentMesh = &currentEntry->back();
		}
	}

	return currentMesh;
}

void GeometryCollector::addFace(
	const std::vector<repo::lib::RepoVector3D64>& vertices)
{
	if (!vertices.size())
	{
		repoError << "Vertices size[" << vertices.size() << "] is unsupported. A face must have more than 0 vertices";
	}

	auto meshData = startOrContinueMeshByFormat(getMeshFormat(false, false, vertices.size()));

	repo_face_t face;
	for (auto i = 0; i < vertices.size(); ++i) 
	{
		auto& v = vertices[i];
		int vertIdx = 0;

		// match the vertex to generate an index for this face
		if (meshData->vToVIndex.find(v) == meshData->vToVIndex.end())
		{
			vertIdx = meshData->rawVertices.size();
			meshData->rawVertices.push_back(v);

			meshData->vToVIndex.insert(
				std::pair<repo::lib::RepoVector3D64,
				std::pair<int, repo::lib::RepoVector3D64>>(
					v,
					std::pair<int,
					repo::lib::RepoVector3D64>(
						vertIdx,
						repo::lib::RepoVector3D64())));


			// if the vertex has not yet been seen, also update the bounding box

			if (!meshData->boundingBox.size())
			{
				meshData->boundingBox.push_back({ (float)v.x, (float)v.y, (float)v.z });
				meshData->boundingBox.push_back({ (float)v.x, (float)v.y, (float)v.z });
			}
			else
			{
				meshData->boundingBox[0][0] = meshData->boundingBox[0][0] > v.x ? (float)v.x : meshData->boundingBox[0][0];
				meshData->boundingBox[0][1] = meshData->boundingBox[0][1] > v.y ? (float)v.y : meshData->boundingBox[0][1];
				meshData->boundingBox[0][2] = meshData->boundingBox[0][2] > v.z ? (float)v.z : meshData->boundingBox[0][2];

				meshData->boundingBox[1][0] = meshData->boundingBox[1][0] < v.x ? (float)v.x : meshData->boundingBox[1][0];
				meshData->boundingBox[1][1] = meshData->boundingBox[1][1] < v.y ? (float)v.y : meshData->boundingBox[1][1];
				meshData->boundingBox[1][2] = meshData->boundingBox[1][2] < v.z ? (float)v.z : meshData->boundingBox[1][2];
			}

			if (minMeshBox.size()) {
				minMeshBox[0] = v.x < minMeshBox[0] ? v.x : minMeshBox[0];
				minMeshBox[1] = v.y < minMeshBox[1] ? v.y : minMeshBox[1];
				minMeshBox[2] = v.z < minMeshBox[2] ? v.z : minMeshBox[2];
			}
			else {
				minMeshBox = { v.x, v.y, v.z };
			}
		}
		{
			auto iterators = meshData->vToVIndex.equal_range(v);
			vertIdx = iterators.first->second.first;
		}

		face.push_back(vertIdx);
	}

	meshData->faces.push_back(face);
}

void GeometryCollector::addFace(
	const std::vector<repo::lib::RepoVector3D64> &vertices,
	const repo::lib::RepoVector3D64& normal,
	const std::vector<repo::lib::RepoVector2D>& uvCoords)
{
	if (!vertices.size())
	{
		repoError << "Vertices size[" << vertices.size() << "] is unsupported. A face must have more than 0 vertices";
		exit(-1);
	}

	auto meshData = startOrContinueMeshByFormat(getMeshFormat(uvCoords.size(), true, vertices.size()));

	if ((uvCoords.size() > 0) && uvCoords.size() != vertices.size()) {
		repoError << "Vertices size[" << vertices.size() << "] and UV size [" << uvCoords.size() << "] mismatched!";
		exit(-1);
	}

	// Todo: this method attempts to generate indices by matching vertex based on attributes, but doesn't yet match the uv coordinates.
	// ISSUE #432 has been raised about this.
	// To fix this properly, merge this function and the addFace overload above, and update the matching code to support an arbitrary number
	// of flat attributes.

	repo_face_t face;
	for (auto i = 0; i < vertices.size(); ++i) {
		auto& v = vertices[i];
		int vertIdx = 0;
		if (meshData->vToVIndex.find(v) == meshData->vToVIndex.end()) {
			//..insert new vertex along with index and normal
			meshData->vToVIndex.insert(
				std::pair<repo::lib::RepoVector3D64,
				std::pair<int, repo::lib::RepoVector3D64>>(
					v,
					std::pair<int, repo::lib::RepoVector3D64>(meshData->rawVertices.size(), normal))
			);
			vertIdx = meshData->rawVertices.size();
			meshData->rawVertices.push_back(v);
			meshData->rawNormals.push_back(normal);
			if (i < uvCoords.size())
				meshData->uvCoords.push_back(uvCoords[i]);

			if (meshData->boundingBox.size()) {
				meshData->boundingBox[0][0] = meshData->boundingBox[0][0] > v.x ? (float)v.x : meshData->boundingBox[0][0];
				meshData->boundingBox[0][1] = meshData->boundingBox[0][1] > v.y ? (float)v.y : meshData->boundingBox[0][1];
				meshData->boundingBox[0][2] = meshData->boundingBox[0][2] > v.z ? (float)v.z : meshData->boundingBox[0][2];

				meshData->boundingBox[1][0] = meshData->boundingBox[1][0] < v.x ? (float)v.x : meshData->boundingBox[1][0];
				meshData->boundingBox[1][1] = meshData->boundingBox[1][1] < v.y ? (float)v.y : meshData->boundingBox[1][1];
				meshData->boundingBox[1][2] = meshData->boundingBox[1][2] < v.z ? (float)v.z : meshData->boundingBox[1][2];
			}
			else {
				meshData->boundingBox.push_back({ (float)v.x, (float)v.y, (float)v.z });
				meshData->boundingBox.push_back({ (float)v.x, (float)v.y, (float)v.z });
			}

			if (minMeshBox.size()) {
				minMeshBox[0] = v.x < minMeshBox[0] ? v.x : minMeshBox[0];
				minMeshBox[1] = v.y < minMeshBox[1] ? v.y : minMeshBox[1];
				minMeshBox[2] = v.z < minMeshBox[2] ? v.z : minMeshBox[2];
			}
			else {
				minMeshBox = { v.x, v.y, v.z };
			}
		}
		else
		{
			//..if the vertex already exists - receive all entries of this vertex in multimap
			bool normalFound = false;
			auto iterators = meshData->vToVIndex.equal_range(v);

			for (auto it = iterators.first; it != iterators.second; it++)
			{
				//..try to find a point with the same normal
				auto result = it->second.second.dotProduct(normal);
				if (!compare(result, 1.0))
				{
					vertIdx = it->second.first;
					normalFound = true;
					break;
				}
			}

			if (!normalFound)
			{
				//.. in case the normal for this point doesn't exist yet - we should add it
				//.. as duplicated and add new normal and index
				meshData->vToVIndex.insert(
					std::pair<repo::lib::RepoVector3D64,
					std::pair<int, repo::lib::RepoVector3D64>>(
						v,
						std::pair<int, repo::lib::RepoVector3D64>(meshData->rawVertices.size(), normal))
				);

				vertIdx = meshData->rawVertices.size();
				meshData->rawVertices.push_back(v);
				meshData->rawNormals.push_back(normal);
				if (i < uvCoords.size())
					meshData->uvCoords.push_back(uvCoords[i]);
			}
		}

		face.push_back(vertIdx);
	}

	meshData->faces.push_back(face);
}

repo::core::model::RepoNodeSet GeometryCollector::getMeshNodes(const repo::core::model::TransformationNode& root) {
	repo::core::model::RepoNodeSet res;
	auto dummyCol = std::vector<repo_color4d_t>();
	auto dummyOutline = std::vector<std::vector<float>>();

	std::unordered_map<std::string, repo::core::model::TransformationNode*> layerToTrans;

	int numEntries = 0;
	for (const auto& meshGroupEntry : meshData) {
		for (const auto& meshLayerEntry : meshGroupEntry.second) {
			for (const auto& meshMatEntry : meshLayerEntry.second) {
				for (const auto& meshData : meshMatEntry.second) {
					numEntries++;
				}
			}
		}
	}

	repoInfo << "Collecting " << numEntries << " mesh nodes...";

	auto rootId = root.getSharedID();
	for (const auto& meshGroupEntry : meshData) {
		for (const auto& meshLayerEntry : meshGroupEntry.second) {
			for (const auto& meshMatEntry : meshLayerEntry.second) {
				for (const auto& meshData : meshMatEntry.second) {

					if (!meshData.rawVertices.size()) {
						continue;
					}

					auto uvChannels = meshData.uvCoords.size() ?
						std::vector<std::vector<repo::lib::RepoVector2D>>{meshData.uvCoords} :
						std::vector<std::vector<repo::lib::RepoVector2D>>();

					if (layerToTrans.find(meshLayerEntry.first) == layerToTrans.end()) {
						layerToTrans[meshLayerEntry.first] = createTransNode(layerIDToName[meshLayerEntry.first], meshLayerEntry.first, rootId);
						transNodes.insert(layerToTrans[meshLayerEntry.first]);
					}

					std::vector<repo::lib::RepoVector3D> vertices32;
					vertices32.reserve(meshData.rawVertices.size());

					std::vector<repo::lib::RepoVector3D> normals32;
					normals32.reserve(meshData.rawNormals.size());

					for (int i = 0; i < meshData.rawVertices.size(); ++i) {
						auto& v = meshData.rawVertices[i];
						vertices32.push_back({ (float)(v.x - minMeshBox[0]), (float)(v.y - minMeshBox[1]), (float)(v.z - minMeshBox[2]) });
						if (i < meshData.rawNormals.size()) {
							auto& n = meshData.rawNormals[i];
							normals32.push_back({ (float)(n.x), (float)(n.y), (float)(n.z) });
						}
					}

					auto meshNode = repo::core::model::RepoBSONFactory::makeMeshNode(
						vertices32,
						meshData.faces,
						normals32,
						meshData.boundingBox,
						uvChannels,
						dummyCol,
						dummyOutline,
						meshGroupEntry.first,
						{ layerToTrans[meshLayerEntry.first]->getSharedID() }
					);

					if (idToMeta.find(meshGroupEntry.first) != idToMeta.end()) {
						metaNodes.insert(createMetaNode(meshGroupEntry.first, { meshNode.getSharedID() }, idToMeta[meshGroupEntry.first]));
					}

					if (matToMeshes.find(meshData.matIdx) == matToMeshes.end()) {
						matToMeshes[meshData.matIdx] = std::vector<repo::lib::RepoUUID>();
					}
					matToMeshes[meshData.matIdx].push_back(meshNode.getSharedID());

					res.insert(new repo::core::model::MeshNode(meshNode));
				}
			}
		}
	}

	transNodes.insert(new repo::core::model::TransformationNode(root));
	return res;
}

repo::core::model::MetadataNode*  GeometryCollector::createMetaNode(
	const std::string &name,
	const repo::lib::RepoUUID &parentId,
	const  std::unordered_map<std::string, std::string> &metaValues
) {
	return new repo::core::model::MetadataNode(repo::core::model::RepoBSONFactory::makeMetaDataNode(metaValues, name, { parentId }));
}

repo::core::model::TransformationNode*  GeometryCollector::createTransNode(
	const std::string &name,
	const std::string &id,
	const repo::lib::RepoUUID &parentId)
{
	auto transNode = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), name, { parentId }));
	if (idToMeta.find(id) != idToMeta.end()) {
		metaNodes.insert(createMetaNode(name, transNode->getSharedID(), idToMeta[id]));
	}
	return transNode;
}

void repo::manipulator::modelconvertor::odaHelper::GeometryCollector::setRootMatrix(repo::lib::RepoMatrix matrix)
{
	rootMatrix = matrix;
}

bool GeometryCollector::hasMissingTextures()
{
	return missingTextures;
}

void GeometryCollector::getMaterialAndTextureNodes(repo::core::model::RepoNodeSet& materials, repo::core::model::RepoNodeSet& textures) {
	materials.clear();
	textures.clear();

	for (const auto &matPair : idxToMat) {
		auto matIdx = matPair.first;
		if (matToMeshes.find(matIdx) != matToMeshes.end()) {
			auto& materialNode = matPair.second.first;
			auto& textureNode = matPair.second.second;

			auto matNode = new repo::core::model::MaterialNode(materialNode.cloneAndAddParent(matToMeshes[matIdx]));
			materials.insert(matNode);

			//FIXME: Mat shared ID is known at the point of creating texture node. We shouldn't be cloning here.
			if (!textureNode.isEmpty()) {
				auto texNode = new repo::core::model::TextureNode(textureNode.cloneAndAddParent(matNode->getSharedID()));
				textures.insert(texNode);
			}
		}
		else {
			repoDebug << "Did not find matTo Meshes: " << matIdx;
		}
	}
}

repo::core::model::TextureNode GeometryCollector::createTextureNode(const std::string& texturePath)
{
	std::ifstream::pos_type size;
	std::ifstream file(texturePath, std::ios::in | std::ios::binary | std::ios::ate);
	char *memblock = nullptr;
	if (!file.is_open())
		return repo::core::model::TextureNode();

	size = file.tellg();
	memblock = new char[size];
	file.seekg(0, std::ios::beg);
	file.read(memblock, size);
	file.close();

	auto texnode = repo::core::model::RepoBSONFactory::makeTextureNode(
		texturePath,
		(const char*)memblock,
		size,
		1,
		0
	);

	delete[] memblock;

	return texnode;
}