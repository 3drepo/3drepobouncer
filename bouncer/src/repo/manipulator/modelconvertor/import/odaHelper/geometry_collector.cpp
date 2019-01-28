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


void GeometryCollector::setCurrentMaterial(const repo_material_t &material) {

	auto checkSum = material.checksum();
	if(idxToMat.find(checkSum) == idxToMat.end()) {
		idxToMat[checkSum] = repo::core::model::RepoBSONFactory::makeMaterialNode(material);
	}
	currMat = checkSum;	
}

mesh_data_t GeometryCollector::createMeshEntry() {
	mesh_data_t entry;
	entry.matIdx = currMat;
	entry.name = nextMeshName;
	entry.groupName = nextGroupName;
	entry.layerName = nextLayer.empty() ? "UnknownLayer" : nextLayer;

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

void GeometryCollector::addFace(const std::vector<repo::lib::RepoVector3D64> &vertices) {
	if (!meshData.size()) startMeshEntry();
	repo_face_t face;
	for (const auto &v : vertices) {
		auto chkSum = v.checkSum();
		if (currentEntry->vToVIndex.find(chkSum) == currentEntry->vToVIndex.end()) {
			currentEntry->vToVIndex[chkSum] = currentEntry->rawVertices.size();
			currentEntry->rawVertices.push_back(v);

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
		face.push_back(currentEntry->vToVIndex[chkSum]);

	}
	currentEntry->faces.push_back(face);

}


repo::core::model::RepoNodeSet GeometryCollector::getMeshNodes() {
	repo::core::model::RepoNodeSet res;
	auto dummyUV = std::vector<std::vector<repo::lib::RepoVector2D>>();
	auto dummyCol = std::vector<repo_color4d_t>();
	auto dummyOutline = std::vector<std::vector<float>>();

	std::unordered_map<std::string, repo::core::model::TransformationNode*> layerToTrans;

	auto root = repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), "rootNode");
	auto rootId = root.getSharedID();
	for (const auto &meshGroupEntry : meshData) {
		for (const auto &meshLayerEntry : meshGroupEntry.second) {
			for (const auto &meshMatEntry : meshLayerEntry.second) {
				if (!meshMatEntry.second.rawVertices.size()) continue;

				if (layerToTrans.find(meshLayerEntry.first) == layerToTrans.end()) {
					layerToTrans[meshLayerEntry.first] = createTransNode(layerIDToName[meshLayerEntry.first], rootId);
					transNodes.insert(layerToTrans[meshLayerEntry.first]);
				}

				std::vector<repo::lib::RepoVector3D> vertices32;
				vertices32.reserve(meshMatEntry.second.rawVertices.size());

				for (const auto &v : meshMatEntry.second.rawVertices) {
					vertices32.push_back({ (float)(v.x - minMeshBox[0]), (float)(v.y - minMeshBox[1]), (float)(v.z - minMeshBox[2]) });
				}

				auto meshNode = repo::core::model::RepoBSONFactory::makeMeshNode(
					vertices32,
					meshMatEntry.second.faces,
					std::vector<repo::lib::RepoVector3D>(),
					meshMatEntry.second.boundingBox,
					dummyUV,
					dummyCol,
					dummyOutline,
					meshGroupEntry.first,
					{ layerToTrans[meshLayerEntry.first]->getSharedID() }
				);

				if (idToMeta.find(meshGroupEntry.first) != idToMeta.end()) {
					metaNodes.insert(createMetaNode(meshGroupEntry.first, { meshNode.getSharedID() }, idToMeta[meshGroupEntry.first]));
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

repo::core::model::MetadataNode*  GeometryCollector::createMetaNode(
	const std::string &name,
	const repo::lib::RepoUUID &parentId,
	const  std::unordered_map<std::string, std::string> &metaValues
) {
	return new repo::core::model::MetadataNode(repo::core::model::RepoBSONFactory::makeMetaDataNode(metaValues, name, {parentId}));
}

repo::core::model::TransformationNode*  GeometryCollector::createTransNode(
	const std::string &name,
	const repo::lib::RepoUUID &parentId
) {	
	auto transNode = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), name, { parentId }));
	if (idToMeta.find(name) != idToMeta.end()) {
		metaNodes.insert(createMetaNode(name, transNode->getSharedID(), idToMeta[name]));
	}
	return transNode;
}



repo::core::model::RepoNodeSet GeometryCollector::getMaterialNodes() {
	repo::core::model::RepoNodeSet matSet;
	for (const auto &matPair : idxToMat) {
		auto matIdx = matPair.first;
		if (matToMeshes.find(matIdx) != matToMeshes.end()) {
			matSet.insert(new repo::core::model::MaterialNode(matPair.second.cloneAndAddParent(matToMeshes[matIdx])));
		}
		else {
			repoDebug << "Did not find matTo Meshes: " << matIdx;
		}
	}
	return matSet;
}
