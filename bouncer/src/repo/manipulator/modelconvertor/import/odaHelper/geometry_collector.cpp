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
	if(idxToMat.find(checkSum) == idxToMat.end()) {
		idxToMat[checkSum] = { 
			repo::core::model::RepoBSONFactory::makeMaterialNode(material), 
			createTextureNode(material.texturePath) 
		};

		if (missingTexture)
			this->missingTextures = true;
	}
	currMat = checkSum;
}

void repo::manipulator::modelconvertor::odaHelper::GeometryCollector::setCurrentMeta(const std::pair<std::vector<std::string>, std::vector<std::string>>& meta)
{
	currentMeta = meta;
}

repo::core::model::RepoNodeSet repo::manipulator::modelconvertor::odaHelper::GeometryCollector::getMetaNodes()
{
	return metaSet;
}

mesh_data_t GeometryCollector::createMeshEntry() {
	mesh_data_t entry;
	entry.matIdx = currMat;
	entry.name = nextMeshName;
	entry.groupName = nextGroupName;
	entry.layerName = nextLayer.empty() ? "UnknownLayer" : nextLayer;
	entry.metaValues = currentMeta;

	return entry;

}

void  GeometryCollector::startMeshEntry() {

	nextMeshName = nextMeshName.empty() ? std::to_string(std::time(0)) : nextMeshName;
	nextGroupName = nextGroupName.empty() ? nextMeshName : nextGroupName;
	nextLayer = nextLayer.empty() ? nextMeshName : nextLayer;

	if (meshData.find(nextGroupName) == meshData.end()) {
		meshData[nextGroupName] = std::unordered_map<std::string, std::unordered_map<int, mesh_data_t>>();
	}

	if (meshData[nextGroupName].find(nextLayer) == meshData[nextGroupName].end()) {
		meshData[nextGroupName][nextLayer] = std::unordered_map<int, mesh_data_t>();
	}

	if (meshData[nextGroupName][nextLayer].find(currMat) == meshData[nextGroupName][nextLayer].end()) {
		meshData[nextGroupName][nextLayer][currMat] = createMeshEntry();
	}
	currentEntry = &meshData[nextGroupName][nextLayer][currMat];

}

void  GeometryCollector::stopMeshEntry() {
	if(currentEntry)
		currentEntry->vToVIndex.clear();		
	nextMeshName = "";
}

void GeometryCollector::addFace(
	const std::vector<repo::lib::RepoVector3D64> &vertices, 
	const repo::lib::RepoVector3D64& normal,
	const std::vector<repo::lib::RepoVector2D>& uvCoords) 
{
	if (!meshData.size()) startMeshEntry();
	repo_face_t face;
	for (auto i = 0; i < vertices.size(); ++i) {
		auto& v = vertices[i];
		int vertIdx = 0;
		if (currentEntry->vToVIndex.find(v) == currentEntry->vToVIndex.end()) {
			//..insert new vertex along with index and normal
			currentEntry->vToVIndex.insert(
				std::pair<repo::lib::RepoVector3D64, 
				std::pair<int, repo::lib::RepoVector3D64>>(
					v, 
					std::pair<int, repo::lib::RepoVector3D64>(currentEntry->rawVertices.size(), normal))
			);
			vertIdx = currentEntry->rawVertices.size();
			currentEntry->rawVertices.push_back(v);
			currentEntry->rawNormals.push_back(normal);
			if (i < uvCoords.size())
				currentEntry->uvCoords.push_back(uvCoords[i]);

			if (currentEntry->boundingBox.size()) {
				currentEntry->boundingBox[0][0] = currentEntry->boundingBox[0][0] > v.x ? (float) v.x : currentEntry->boundingBox[0][0];
				currentEntry->boundingBox[0][1] = currentEntry->boundingBox[0][1] > v.y ? (float) v.y : currentEntry->boundingBox[0][1];
				currentEntry->boundingBox[0][2] = currentEntry->boundingBox[0][2] > v.z ? (float) v.z : currentEntry->boundingBox[0][2];

				currentEntry->boundingBox[1][0] = currentEntry->boundingBox[1][0] < v.x ? (float) v.x : currentEntry->boundingBox[1][0];
				currentEntry->boundingBox[1][1] = currentEntry->boundingBox[1][1] < v.y ? (float) v.y : currentEntry->boundingBox[1][1];
				currentEntry->boundingBox[1][2] = currentEntry->boundingBox[1][2] < v.z ? (float) v.z : currentEntry->boundingBox[1][2];
			}
			else {
				currentEntry->boundingBox.push_back({ (float)v.x, (float)v.y, (float)v.z });
				currentEntry->boundingBox.push_back({ (float)v.x, (float)v.y, (float)v.z });
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
			auto iterators = currentEntry->vToVIndex.equal_range(v);

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
				currentEntry->vToVIndex.insert(
					std::pair<repo::lib::RepoVector3D64,
					std::pair<int, repo::lib::RepoVector3D64>>(
						v,
						std::pair<int, repo::lib::RepoVector3D64>(currentEntry->rawVertices.size(), normal))
				);

				vertIdx = currentEntry->rawVertices.size();
				currentEntry->rawVertices.push_back(v);
				currentEntry->rawNormals.push_back(normal);
			}
		}

		face.push_back(vertIdx);
	}
	currentEntry->faces.push_back(face);
}


repo::core::model::RepoNodeSet GeometryCollector::getMeshNodes(const repo::core::model::TransformationNode& root) {
	repo::core::model::RepoNodeSet res;
	auto dummyCol = std::vector<repo_color4d_t>();
	auto dummyOutline = std::vector<std::vector<float>>();

	std::unordered_map<std::string, repo::core::model::TransformationNode*> layerToTrans;

	auto rootId = root.getSharedID();
	repoDebug << "Mesh data: " << meshData.size();
	for (const auto &meshGroupEntry : meshData) {
		for (const auto &meshLayerEntry : meshGroupEntry.second) {
			for (const auto &meshMatEntry : meshLayerEntry.second) {
				if (!meshMatEntry.second.rawVertices.size()) continue;

				auto uvChannels = meshMatEntry.second.uvCoords.size() ? 
					std::vector<std::vector<repo::lib::RepoVector2D>>{meshMatEntry.second.uvCoords} :
					std::vector<std::vector<repo::lib::RepoVector2D>>();


				if (layerToTrans.find(meshLayerEntry.first) == layerToTrans.end()) {
					layerToTrans[meshLayerEntry.first] = createTransNode(meshLayerEntry.first, rootId);
					transNodes.insert(layerToTrans[meshLayerEntry.first]);
				}

				std::vector<repo::lib::RepoVector3D> vertices32;
				vertices32.reserve(meshMatEntry.second.rawVertices.size());

				std::vector<repo::lib::RepoVector3D> normals32;
				normals32.reserve(meshMatEntry.second.rawNormals.size());

				for (int i = 0; i < meshMatEntry.second.rawVertices.size(); ++i) {
					auto v = meshMatEntry.second.rawVertices[i];
					vertices32.push_back({ (float)(v.x - minMeshBox[0]), (float)(v.y - minMeshBox[1]), (float)(v.z - minMeshBox[2]) });
					if (i < meshMatEntry.second.rawNormals.size()) {
						auto n = meshMatEntry.second.rawNormals[i];
						normals32.push_back({ (float)(n.x), (float)(n.y), (float)(n.z) });
					}
				}

				auto meshNode = repo::core::model::RepoBSONFactory::makeMeshNode(
					vertices32,
					meshMatEntry.second.faces,
					normals32,
					meshMatEntry.second.boundingBox,
					uvChannels,
					dummyCol,
					dummyOutline,
					meshGroupEntry.first,
					{ layerToTrans[meshLayerEntry.first]->getSharedID() }
				);

				auto& metaValues = meshMatEntry.second.metaValues;

				if (!metaValues.first.empty())
				{
					metaSet.insert(new repo::core::model::MetadataNode(repo::core::model::RepoBSONFactory::makeMetaDataNode(metaValues.first, metaValues.second, meshMatEntry.second.name,
					{
						meshNode.getSharedID()
					})));
				}

				if (matToMeshes.find(meshMatEntry.second.matIdx) == matToMeshes.end()) {
					matToMeshes[meshMatEntry.second.matIdx] = std::vector<repo::lib::RepoUUID>();
				}
				matToMeshes[meshMatEntry.second.matIdx].push_back(meshNode.getSharedID());

				res.insert(new repo::core::model::MeshNode(meshNode));
			}
		}
	}

	transNodes.insert(new repo::core::model::TransformationNode(root));
	return res;
}

repo::core::model::TransformationNode*  GeometryCollector::createTransNode(
	const std::string &name,
	const repo::lib::RepoUUID &parentId) {	
	return new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), name, { parentId }));
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
		0,
		REPO_NODE_API_LEVEL_1
	);

	delete[] memblock;

	return texnode;
}