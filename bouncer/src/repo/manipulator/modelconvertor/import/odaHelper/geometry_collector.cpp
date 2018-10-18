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

void  GeometryCollector::startMeshEntry() {
	mesh_data_t entry;
	std::string meshName = nextMeshName.empty() ?  "mesh_" + std::to_string(meshData.size()) : nextMeshName;
	entry.matIdx = currMat;
	entry.name = meshName;
	meshData.push_back(entry);
}

void  GeometryCollector::stopMeshEntry() {
	if (meshData.size()) {
		meshData.back().vToVIndex.clear();
		if (!meshData.back().rawVertices.size()) {
			meshData.pop_back();
			nextMeshName = "";
		}		
	}
	
}

void GeometryCollector::addFace(const std::vector<repo::lib::RepoVector3D64> &vertices) {
	if (!meshData.size()) startMeshEntry();
	repo_face_t face;
	for (const auto &v : vertices) {
		auto chkSum = v.checkSum();
		if (meshData.back().vToVIndex.find(chkSum) == meshData.back().vToVIndex.end()) {
			meshData.back().vToVIndex[chkSum] = meshData.back().rawVertices.size();
			meshData.back().rawVertices.push_back(v);

			if (meshData.back().boundingBox.size()) {
				meshData.back().boundingBox[0][0] = meshData.back().boundingBox[0][0] > v.x ? (float) v.x : meshData.back().boundingBox[0][0];
				meshData.back().boundingBox[0][1] = meshData.back().boundingBox[0][1] > v.y ? (float) v.y : meshData.back().boundingBox[0][1];
				meshData.back().boundingBox[0][2] = meshData.back().boundingBox[0][2] > v.z ? (float) v.z : meshData.back().boundingBox[0][2];

				meshData.back().boundingBox[1][0] = meshData.back().boundingBox[1][0] < v.x ? (float) v.x : meshData.back().boundingBox[1][0];
				meshData.back().boundingBox[1][1] = meshData.back().boundingBox[1][1] < v.y ? (float) v.y : meshData.back().boundingBox[1][1];
				meshData.back().boundingBox[1][2] = meshData.back().boundingBox[1][2] < v.z ? (float) v.z : meshData.back().boundingBox[1][2];
			}
			else {
				meshData.back().boundingBox.push_back({ (float)v.x, (float)v.y, (float)v.z });
				meshData.back().boundingBox.push_back({ (float)v.x, (float)v.y, (float)v.z });
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
		face.push_back(meshData.back().vToVIndex[chkSum]);

	}
	meshData.back().faces.push_back(face);

}


std::vector<repo::core::model::MeshNode> GeometryCollector::getMeshes() {
	std::vector<repo::core::model::MeshNode> res;
	repoInfo << "getting meshes:" << meshData.size();
	res.reserve(meshData.size());
	auto dummyUV = std::vector<std::vector<repo::lib::RepoVector2D>>();
	auto dummyCol = std::vector<repo_color4d_t>();
	auto dummyOutline = std::vector<std::vector<float>>();

	for (const auto &meshEntry : meshData) {
		std::vector<repo::lib::RepoVector3D> vertices32;
		vertices32.reserve(meshEntry.rawVertices.size());

		for (const auto &v : meshEntry.rawVertices) {
			vertices32.push_back({ (float)(v.x - minMeshBox[0]), (float)(v.y - minMeshBox[1]), (float)(v.z - minMeshBox[2]) });
		}
		auto meshNode = repo::core::model::RepoBSONFactory::makeMeshNode(
			vertices32,
			meshEntry.faces,
			std::vector<repo::lib::RepoVector3D>(),
			meshEntry.boundingBox,
			dummyUV,
			dummyCol,
			dummyOutline,
			meshEntry.name
		);
		res.push_back(meshNode);
		if (matToMeshes.find(meshEntry.matIdx) == matToMeshes.end()) {
			matToMeshes[meshEntry.matIdx] =  std::vector<repo::lib::RepoUUID>();
		}
		matToMeshes[meshEntry.matIdx].push_back(meshNode.getSharedID());
	}
	

	return res;
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