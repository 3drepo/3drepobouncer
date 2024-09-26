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
	nextMeshName = nextMeshName.empty() ? repo::lib::RepoUUID::createUUID().toString() : nextMeshName;
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
				currentMesh = &(*meshDataIterator);
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
	addFace(vertices, boost::optional<const repo::lib::RepoVector3D64&>(), boost::optional<const std::vector<repo::lib::RepoVector2D>&>());
}

void GeometryCollector::addFace(
	const std::vector<repo::lib::RepoVector3D64>& vertices,
	const repo::lib::RepoVector3D64& normal,
	const std::vector<repo::lib::RepoVector2D>& uvCoords)
{
	addFace(vertices, boost::optional<const repo::lib::RepoVector3D64&>(normal), boost::optional<const std::vector<repo::lib::RepoVector2D>&>(uvCoords)); // make the boost option type explicit so we get the correct overload
}

void GeometryCollector::addFace(
	const std::vector<repo::lib::RepoVector3D64>& vertices,
	boost::optional<const repo::lib::RepoVector3D64&> normal,
	boost::optional<const std::vector<repo::lib::RepoVector2D>&> uvCoords
)
{
	if (!vertices.size())
	{
		repoError << "Vertices size [" << vertices.size() << "] is unsupported. A face must have more than 0 vertices.";
		errorCode = REPOERR_GEOMETRY_ERROR;
		return;
	}

	bool hasNormals = (bool)normal;
	bool hasUvs = (bool)uvCoords && (*uvCoords).size();

	auto meshData = startOrContinueMeshByFormat(getMeshFormat(hasUvs, hasNormals, vertices.size()));

	repo_face_t face;
	for (auto i = 0; i < vertices.size(); ++i) {
		auto& v = vertices[i];

		VertexMap::result_t vertexReference;
		if (hasNormals)
		{
			if (hasUvs)
			{
				auto& uv = (*uvCoords)[i];
				vertexReference = meshData->vertexMap.find(v, *normal, uv);
			}
			else
			{
				vertexReference = meshData->vertexMap.find(v, *normal);
			}
		}
		else if (hasUvs)
		{
			repoError << "Face has uvs but no normals. This is not supported. Faces that have uvs must also have a normal.";
			errorCode = REPOERR_GEOMETRY_ERROR;
			return;
		}
		else
		{
			vertexReference = meshData->vertexMap.find(v);
		}

		if (vertexReference.added)
		{
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

		face.push_back(vertexReference.index);
	}

	meshData->faces.push_back(face);
}

repo::core::model::TransformationNode* GeometryCollector::ensureParentNodeExists(
	const std::string &layerId,
	const repo::lib::RepoUUID &rootId,
	std::unordered_map<std::string, repo::core::model::TransformationNode*> &layerToTrans

) {
	if (layerToTrans.find(layerId) == layerToTrans.end()) {
		auto parent = rootId;
		if (layerIDToParent.find(layerId) != layerIDToParent.end()) {
			parent = ensureParentNodeExists(layerIDToParent[layerId], rootId, layerToTrans)->getSharedID();
		}
		layerToTrans[layerId] = createTransNode(layerIDToName[layerId], layerId, parent);
		transNodes.insert(layerToTrans[layerId]);
	}
	return layerToTrans[layerId];
}

repo::core::model::RepoNodeSet GeometryCollector::getMeshNodes(const repo::core::model::TransformationNode& root) {
	repo::core::model::RepoNodeSet res;
	auto dummyOutline = std::vector<std::vector<float>>();

	std::unordered_map<std::string, repo::core::model::TransformationNode*> layerToTrans;
	std::unordered_map < repo::core::model::MetadataNode*, std::vector<repo::lib::RepoUUID>>  metaNodeToParents;

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
					if (!meshData.vertexMap.vertices.size()) {
						continue;
					}

					if (meshData.vertexMap.uvs.size() && (meshData.vertexMap.uvs.size() != meshData.vertexMap.vertices.size()))
					{
						repoError << "Vertices size [" << meshData.vertexMap.vertices.size() << "] does not match the uvs size [" << meshData.vertexMap.uvs.size() << "]. Skipping...";
						errorCode = REPOERR_GEOMETRY_ERROR;
						continue;
					}

					auto uvChannels = meshData.vertexMap.uvs.size() ?
						std::vector<std::vector<repo::lib::RepoVector2D>>{meshData.vertexMap.uvs} :
						std::vector<std::vector<repo::lib::RepoVector2D>>();

					ensureParentNodeExists(meshLayerEntry.first, rootId, layerToTrans);

					std::vector<repo::lib::RepoVector3D> normals32;

					if (meshData.vertexMap.normals.size()) {
						if ((meshData.vertexMap.normals.size() != meshData.vertexMap.vertices.size()))
						{
							repoError << "Vertices size [" << meshData.vertexMap.vertices.size() << "] does not match the Normals size [" << meshData.vertexMap.uvs.size() << "]. At this point the normals must be defined per-vertex. Skipping...";
							errorCode = REPOERR_GEOMETRY_ERROR;
							continue;
						}

						normals32.reserve(meshData.vertexMap.normals.size());

						for (int i = 0; i < meshData.vertexMap.vertices.size(); ++i) {
							auto& n = meshData.vertexMap.normals[i];
							normals32.push_back({ (float)(n.x), (float)(n.y), (float)(n.z) });
						}
					}

					std::vector<repo::lib::RepoVector3D> vertices32;
					vertices32.reserve(meshData.vertexMap.vertices.size());
					bool partialObject = meshGroupEntry.first == meshLayerEntry.first;
					auto parentId = layerToTrans[meshLayerEntry.first]->getSharedID();

					for (int i = 0; i < meshData.vertexMap.vertices.size(); ++i) {
						auto& v = meshData.vertexMap.vertices[i];
						vertices32.push_back({ (float)(v.x - minMeshBox[0]), (float)(v.y - minMeshBox[1]), (float)(v.z - minMeshBox[2]) });
					}

					auto meshNode = repo::core::model::RepoBSONFactory::makeMeshNode(
						vertices32,
						meshData.faces,
						normals32,
						meshData.boundingBox,
						uvChannels,
						partialObject ? "" : meshGroupEntry.first,
						{ parentId }
					);

					if (idToMeta.find(meshGroupEntry.first) != idToMeta.end()) {
						auto itPtr = elementToMetaNode.find(meshGroupEntry.first);
						auto metaParent = partialObject ? parentId : meshNode.getSharedID();
						if (itPtr == elementToMetaNode.end()) {
							auto metaNode = createMetaNode(meshGroupEntry.first, {}, idToMeta[meshGroupEntry.first]);
							elementToMetaNode[meshGroupEntry.first] = metaNode;
							metaNodes.insert(metaNode);
							metaNodeToParents[metaNode] = { metaParent };
						}
						else {
							metaNodeToParents[itPtr->second].push_back(metaParent);
						}
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
	for (auto &metaEntry : metaNodeToParents) {
		auto metaNode = metaEntry.first;
		auto parentSet = metaEntry.second;
		*metaNode = metaNode->cloneAndAddParent(parentSet);
	}

	transNodes.insert(new repo::core::model::TransformationNode(root));
	return res;
}

repo::core::model::MetadataNode*  GeometryCollector::createMetaNode(
	const std::string &name,
	const repo::lib::RepoUUID &parentId,
	const  std::unordered_map<std::string, repo::lib::RepoVariant> &metaValues
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
		if (elementToMetaNode.find(id) == elementToMetaNode.end()) {
			auto metaNode = createMetaNode(name, transNode->getSharedID(), idToMeta[id]);
			metaNodes.insert(metaNode);
			elementToMetaNode[id] = metaNode;
		}
		else {
			*elementToMetaNode[id] = elementToMetaNode[id]->cloneAndAddParent(transNode->getSharedID());
		}
	}
	return transNode;
}

void repo::manipulator::modelconvertor::odaHelper::GeometryCollector::setRootMatrix(repo::lib::RepoMatrix matrix)
{
	rootMatrix = matrix;
}

int GeometryCollector::getErrorCode()
{
	return errorCode;
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