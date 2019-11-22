/**
*  Copyright (C) 2019 3D Repo Ltd
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

#ifdef SYNCHRO_SUPPORT
#include "repo_model_import_synchro.h"

#include "../../../core/model/bson/repo_bson_builder.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../lib/repo_log.h"
#include "../../../error_codes.h"

using namespace repo::manipulator::modelconvertor;

const std::string RESOURCE_ID_NAME = "Resource ID";

bool SynchroModelImport::importModel(std::string filePath, uint8_t &errMsg) {

	orgFile = filePath;
	reader = std::make_shared<synchro_reader::SPMReader>(filePath);
	repoInfo << "=== IMPORTING MODEL WITH SYNCHRO MODEL CONVERTOR ===";
	if (!reader->init()) {
		errMsg = REPOERR_LOAD_SCENE_FAIL;
		return false;
	}
	repoInfo << "Initialisation successful";
	
	return true;
}

std::pair<repo::core::model::RepoNodeSet, repo::core::model::RepoNodeSet> SynchroModelImport::generateMatNodes(
	std::unordered_map<std::string, repo::lib::RepoUUID> &synchroIDtoRepoID,
	std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode*, repo::lib::RepoUUIDHasher> &repoIDToNode) {

	repo::core::model::RepoNodeSet matNodes, textNodes;

	for (const auto matEntry : reader->getMaterials()) {
		auto mat = matEntry.second;
		repo_material_t material;
		material.diffuse = mat.diffuse;
		material.specular = mat.specular;
		material.opacity = 1 - mat.transparency;
		auto matNode = new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(material, "material"));
		matNodes.insert(matNode);
		synchroIDtoRepoID[mat.matID] = matNode->getUniqueID();

		auto textBuff = mat.texture.texture;
		if (textBuff.size()) {
			auto textNode = new repo::core::model::TextureNode(repo::core::model::RepoBSONFactory::makeTextureNode("texture", (char*)textBuff.data(),
				textBuff.size(), matEntry.second.texture.width, matEntry.second.texture.height).cloneAndAddParent(matNode->getSharedID()));
			textNodes.insert(textNode);
			repoIDToNode[textNode->getUniqueID()] = textNode;
		}

		repoIDToNode[matNode->getUniqueID()] = matNode;

	}

	return{ matNodes, textNodes};
}

repo::core::model::MetadataNode* SynchroModelImport::createMetaNode(
	const std::unordered_map<std::string, std::string> &metadata,
	const std::string &name,	
	const std::vector<repo::lib::RepoUUID> &parents) {

	return new repo::core::model::MetadataNode(repo::core::model::RepoBSONFactory::makeMetaDataNode(metadata, name, parents));
}


repo::core::model::TransformationNode* SynchroModelImport::createTransNode(
	const repo::lib::RepoMatrix &matrix,
	const std::string &name,
	const std::vector<repo::lib::RepoUUID> &parents) {

	return new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode(matrix, name, parents));
}

std::unordered_map<std::string, repo::core::model::MeshNode> SynchroModelImport::createMeshTemplateNodes() {
	std::unordered_map<std::string, repo::core::model::MeshNode> res;
	for (const auto meshEntry : reader->getMeshes()) {
		auto meshDetails = meshEntry.second;

		std::vector<std::vector<float>> bbox;
		std::vector<repo::lib::RepoVector3D> vertices, normals;
		std::vector<repo::lib::RepoVector2D> uvs;
		std::vector<repo_face_t> faces;
		for (int i = 0; i < meshDetails.vertices.size(); ++i) {
			if (meshDetails.normals.size() > i) {
				normals.push_back({ (float) meshDetails.normals[i].x, (float)meshDetails.normals[i].y, (float)meshDetails.normals[i].z });
			}
			vertices.push_back({ (float)meshDetails.vertices[i].x, (float)meshDetails.vertices[i].y, (float)meshDetails.vertices[i].z });
			if (bbox.size()) {
				bbox[0][0] = meshDetails.vertices[i].x < bbox[0][0] ? meshDetails.vertices[i].x : bbox[0][0];
				bbox[0][1] = meshDetails.vertices[i].y < bbox[0][1] ? meshDetails.vertices[i].y : bbox[0][1];
				bbox[0][2] = meshDetails.vertices[i].z < bbox[0][2] ? meshDetails.vertices[i].z : bbox[0][2];

				bbox[1][0] = meshDetails.vertices[i].x < bbox[1][0] ? meshDetails.vertices[i].x : bbox[1][0];
				bbox[1][1] = meshDetails.vertices[i].y < bbox[1][1] ? meshDetails.vertices[i].y : bbox[1][1];
				bbox[1][2] = meshDetails.vertices[i].z < bbox[1][2] ? meshDetails.vertices[i].z : bbox[1][2];
			}
			else {
				bbox = { { vertices[i].x , vertices[i].y, vertices[i].z },{ vertices[i].x , vertices[i].y, vertices[i].z } };
			}
		}

		for (int i = 0; i < meshDetails.faces.size(); i += 3) {

			faces.push_back({ (uint32_t)meshDetails.faces[i],(uint32_t)meshDetails.faces[i + 1],(uint32_t)meshDetails.faces[i + 2] });
		}


		for (const auto uv : meshDetails.uv) {
			uvs.push_back({ (float) uv.x, (float)uv.y });
		}		
		res[meshDetails.geoID] = repo::core::model::RepoBSONFactory::makeMeshNode(vertices, faces, normals, bbox);		
	}

	return res;		
}


repo::core::model::RepoScene* SynchroModelImport::generateRepoScene() {
	
	repo::core::model::RepoNodeSet transNodes, matNodes, textNodes, meshNodes, metaNodes;
	std::unordered_map<std::string, repo::lib::RepoUUID> synchroIDToRepoID;
	std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode*, repo::lib::RepoUUIDHasher> repoIDToNode;

	auto matPairs = generateMatNodes(synchroIDToRepoID, repoIDToNode);
	matNodes = matPairs.first;
	textNodes = matPairs.second;

	auto identity = repo::lib::RepoMatrix();	
	auto root = createTransNode(identity, reader->getProjectName());
	transNodes.insert(root);
	
	std::unordered_map < std::string, std::unordered_map<std::string, repo::lib::RepoUUID>> meshMatNodeMap;
	std::unordered_map<repo::lib::RepoUUID, std::vector<repo::lib::RepoUUID>, repo::lib::RepoUUIDHasher> nodeToParents;
	std::unordered_map<repo::lib::RepoUUID, std::string, repo::lib::RepoUUIDHasher> nodeToSynchroParent;

	auto meshNodeTemplates = createMeshTemplateNodes();

	for (const auto entity : reader->getEntities()) {
		auto trans = createTransNode(identity, entity.second.name);
		transNodes.insert(trans);
		repoIDToNode[trans->getUniqueID()] = trans;
		synchroIDToRepoID[entity.second.id] = trans->getUniqueID();
		nodeToSynchroParent[trans->getUniqueID()] = entity.second.parentID;
		auto meta = entity.second.metadata;
		meta[RESOURCE_ID_NAME] = entity.second.resourceID;
		metaNodes.insert(createMetaNode(meta, entity.second.name, { trans->getSharedID() }));


		for (const auto meshEntry : entity.second.meshes) {
			auto meshID = meshEntry.geoId;
			auto matID = meshEntry.matId;

			if (meshNodeTemplates.find(meshID) == meshNodeTemplates.end() || synchroIDToRepoID.find(matID) == synchroIDToRepoID.end()) {
				repoDebug << "Cannot find mesh/material entry. Skipping..";
				continue;
			}

			if (meshMatNodeMap.find(meshID) == meshMatNodeMap.end()) {
				meshMatNodeMap[meshID] = std::unordered_map<std::string, repo::lib::RepoUUID>();
			}

			if (meshMatNodeMap[meshID].find(matID) == meshMatNodeMap[meshID].end()) {
				auto mesh = new repo::core::model::MeshNode(meshNodeTemplates[meshID].cloneAndChangeIdentity());
				meshMatNodeMap[meshID][matID] = mesh->getUniqueID();

				auto matNode = repoIDToNode[synchroIDToRepoID[matID]];
				auto matNodeID = matNode->getUniqueID();

				if (nodeToParents.find(matNodeID) == nodeToParents.end())
					nodeToParents[matNodeID] = { mesh->getSharedID() };
				else
					nodeToParents[matNodeID].push_back(mesh->getSharedID());

				meshNodes.insert(mesh);
				synchroIDToRepoID[meshID] = mesh->getUniqueID();
				repoIDToNode[mesh->getUniqueID()] = mesh;
			}

			//Add transformation as parent of the mesh entry
			auto repoMat = repo::lib::RepoMatrix(meshEntry.transformation);
			auto meshTrans = new repo::core::model::TransformationNode(
				repo::core::model::RepoBSONFactory::makeTransformationNode(
					repoMat,
					entity.second.name,
					{ trans->getSharedID() }));

			transNodes.insert(meshTrans);


			auto mesh = meshMatNodeMap[meshID][matID];
			if (nodeToParents.find(mesh) == nodeToParents.end()) {
				nodeToParents[mesh] = { meshTrans->getSharedID() };
			}
			else {
				nodeToParents[mesh].push_back(meshTrans->getSharedID());
			}

		}
	}

	auto rootSharedID = root->getSharedID();
	for (const auto entry : nodeToSynchroParent) {
		auto nodeID = entry.first;
		auto synchroParent = entry.second;
		auto parentSharedID = rootSharedID;
		if (synchroIDToRepoID.find(synchroParent) != synchroIDToRepoID.end()) {
			parentSharedID = repoIDToNode[synchroIDToRepoID[synchroParent]]->getSharedID();			
		}
		*repoIDToNode[nodeID] = repoIDToNode[nodeID]->cloneAndAddParent(parentSharedID);
	}

	for (auto entry : nodeToParents) {
		auto nodeID = entry.first;
		auto parents = entry.second;
		*repoIDToNode[nodeID] = repoIDToNode[nodeID]->cloneAndAddParent(parents);
	}

	repo::core::model::RepoNodeSet dummy;
	repoInfo << "Creating scene with : " << transNodes.size() << " transformations, "
		<< meshNodes.size() << " meshes, "
		<< matNodes.size() << " materials, "
		<< textNodes.size() << " textures, "
		<< metaNodes.size() << " metadata ";
	auto scene = new repo::core::model::RepoScene({ orgFile }, dummy, meshNodes, matNodes, metaNodes, textNodes, transNodes);
	auto origin = reader->getGlobalOffset();
	repoInfo << "Setting Global Offset: " << origin.x << ", " << origin.y << ", " << origin.z;
	scene->setWorldOffset({ origin.x, origin.y, origin.z });

	return scene;
}
#endif