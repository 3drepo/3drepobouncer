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
#include <memory>
#include "repo_model_import_synchro.h"

#include "../../../core/model/bson/repo_bson_builder.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../lib/repo_log.h"
#include "../../../error_codes.h"

using namespace repo::manipulator::modelconvertor;

const std::string RESOURCE_ID_NAME = "Resource ID";
const std::string DEFAULT_SEQUENCE_NAME = "Unnamed Sequence";

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

repo::core::model::MeshNode* SynchroModelImport::createMeshNode(
	const repo::core::model::MeshNode &templateMesh, 
	const std::vector<double> &transformation, 
	const std::vector<double> &offset, 
	const repo::lib::RepoUUID &parentID) {
	auto trans = transformation;
	trans[3] -= offset[0];
	trans[7] -= offset[1];
	trans[11] -= offset[2];
	auto matrix = repo::lib::RepoMatrix(trans);
	return new repo::core::model::MeshNode(
		templateMesh.cloneAndApplyTransformation(matrix).cloneAndAddParent(parentID, true, true));
}

repo::core::model::RepoScene* SynchroModelImport::constructScene( 
	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> &resourceIDsToSharedIDs
) {
	repo::core::model::RepoNodeSet transNodes, matNodes, textNodes, meshNodes, metaNodes;
	std::unordered_map<std::string, repo::lib::RepoUUID> synchroIDToRepoID;
	std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode*, repo::lib::RepoUUIDHasher> repoIDToNode;

	auto matPairs = generateMatNodes(synchroIDToRepoID, repoIDToNode);
	matNodes = matPairs.first;
	textNodes = matPairs.second;

	auto identity = repo::lib::RepoMatrix();
	auto root = createTransNode(identity, reader->getProjectName());
	transNodes.insert(root);

	std::unordered_map<repo::lib::RepoUUID, std::vector<repo::lib::RepoUUID>, repo::lib::RepoUUIDHasher> nodeToParents;
	std::unordered_map<repo::lib::RepoUUID, std::string, repo::lib::RepoUUIDHasher> nodeToSynchroParent;

	auto meshNodeTemplates = createMeshTemplateNodes();
	std::vector<synchro_reader::Vector3D> bbox;
	for (const auto entity : reader->getEntities(bbox)) {
		auto trans = createTransNode(identity, entity.second.name);
		auto resourceID = entity.second.resourceID;
		transNodes.insert(trans);
		repoIDToNode[trans->getUniqueID()] = trans;
		synchroIDToRepoID[entity.second.id] = trans->getUniqueID();
		nodeToSynchroParent[trans->getUniqueID()] = entity.second.parentID;
		auto meta = entity.second.metadata;		
		meta[RESOURCE_ID_NAME] = resourceID;
		if (meta.size() > 1 || !resourceID.empty()) {
			metaNodes.insert(createMetaNode(meta, entity.second.name, { trans->getSharedID() }));
		}

		if (!resourceID.empty() && resourceIDsToSharedIDs.find(resourceID) == resourceIDsToSharedIDs.end()) {
			resourceIDsToSharedIDs[resourceID] = {};
		}


		for (const auto meshEntry : entity.second.meshes) {
			auto meshID = meshEntry.geoId;
			auto matID = meshEntry.matId;

			if (meshNodeTemplates.find(meshID) == meshNodeTemplates.end() || synchroIDToRepoID.find(matID) == synchroIDToRepoID.end()) {
				repoDebug << "Cannot find mesh/material entry. Skipping..";
				continue;
			}

			auto mesh = createMeshNode(meshNodeTemplates[meshID],
				meshEntry.transformation, { bbox[0].x, bbox[0].y, bbox[0].z }, trans->getSharedID());

			auto matNode = repoIDToNode[synchroIDToRepoID[matID]];
			auto matNodeID = matNode->getUniqueID();

			if (nodeToParents.find(matNodeID) == nodeToParents.end())
				nodeToParents[matNodeID] = { mesh->getSharedID() };
			else
				nodeToParents[matNodeID].push_back(mesh->getSharedID());

			meshNodes.insert(mesh);
			synchroIDToRepoID[meshID] = mesh->getUniqueID();
			repoIDToNode[mesh->getUniqueID()] = mesh;
			if (!resourceID.empty())
				resourceIDsToSharedIDs[resourceID].push_back(mesh->getSharedID());
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
	std::vector<double> offset = { origin.x + bbox[0].x, origin.y + bbox[0].y, origin.z + bbox[0].z };
	repoInfo << "Setting Global Offset: " << offset[0] << ", " << offset[1] << ", " << offset[2];
	scene->setWorldOffset(offset);

	return scene;
}

repo::core::model::RepoScene* SynchroModelImport::generateRepoScene() {
	
	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> resourceIDsToSharedIDs;

	auto scene = constructScene(resourceIDsToSharedIDs);

	repoInfo << "Getting animations... ";
	auto animation = reader->getAnimation();
	

	std::map<uint64_t, std::vector<repo::lib::RepoUUID>> frameToTasks;
	std::vector<repo::core::model::RepoTask> tasks;


	for (const auto &frame : animation.frames) {
		frameToTasks[frame.first] = std::vector < repo::lib::RepoUUID>();
		std::unordered_map<uint32_t, std::vector<repo::lib::RepoUUID>> colorTasks;
		std::unordered_map<float, std::vector<repo::lib::RepoUUID>> visibilityTasks;
		for (const auto &task : frame.second) {
			switch (task->getType()) {
			case synchro_reader::AnimationTask::TaskType::CAMERA:
				{
					auto camTask = std::dynamic_pointer_cast<const synchro_reader::CameraChange>(task);
					auto taskBSON = repo::core::model::RepoBSONFactory::makeCameraTask(
						camTask->fov, camTask->isPerspective,
						{ (float)camTask->position.x, (float)camTask->position.y, (float)camTask->position.z },
						{ (float)camTask->forward.x, (float)camTask->forward.y, (float)camTask->forward.z },
						{ (float)camTask->up.x, (float)camTask->up.y, (float)camTask->up.z }
					);

					tasks.push_back(taskBSON);
					frameToTasks[frame.first].push_back(taskBSON.getUUIDField(REPO_LABEL_ID));
				}
				break;
			case synchro_reader::AnimationTask::TaskType::COLOR:
				{
					auto colourTask = std::dynamic_pointer_cast<const synchro_reader::ColourTask>(task);

					if (resourceIDsToSharedIDs.find(colourTask->resourceID) != resourceIDsToSharedIDs.end()) {
						auto color = colourTask->color;
						uint8_t bitColor[4];
						bitColor[0] = color[0] * 255;
						bitColor[1] = color[1] * 255;
						bitColor[2] = color[2] * 255;
						bitColor[3] = 0;

						auto colour32Bit = *(uint32_t*)&bitColor;

						if (colorTasks.find(colour32Bit) == colorTasks.end()) {
							colorTasks[colour32Bit] = resourceIDsToSharedIDs[colourTask->resourceID];
						}
						else {
							colorTasks[colour32Bit].insert(
								colorTasks[colour32Bit].end(),
								resourceIDsToSharedIDs[colourTask->resourceID].begin(),
								resourceIDsToSharedIDs[colourTask->resourceID].end());
						}					
					}
				}
				break;
			case synchro_reader::AnimationTask::TaskType::VISIBILITY:
				{
					auto visibilityTask = std::dynamic_pointer_cast<const synchro_reader::VisibilityTask>(task);
					if (resourceIDsToSharedIDs.find(visibilityTask->resourceID) != resourceIDsToSharedIDs.end()) {
						auto visibility = visibilityTask->visibility / 100.;
						if (visibilityTasks.find(visibility) == visibilityTasks.end()) {
							visibilityTasks[visibility] = resourceIDsToSharedIDs[visibilityTask->resourceID];
						}
						else {
							visibilityTasks[visibility].insert(
								visibilityTasks[visibility].end(),
								resourceIDsToSharedIDs[visibilityTask->resourceID].begin(), 
								resourceIDsToSharedIDs[visibilityTask->resourceID].end());
						}
					}
				}
				break;
			}
		}

		for (const auto &colorEntry : colorTasks) {
			if (!colorEntry.second.size()) continue;
			auto color32Bit = colorEntry.first;
			auto colorArr = (uint8_t*)&color32Bit;

			auto colorVector = repo::lib::RepoVector3D((float)colorArr[0]/255.f, (float)colorArr[1] / 255.f , (float)colorArr[2] / 255.f );

			auto taskBSON = repo::core::model::RepoBSONFactory::makeColorTask( colorVector, colorEntry.second);

			tasks.push_back(taskBSON);
			frameToTasks[frame.first].push_back(taskBSON.getUUIDField(REPO_LABEL_ID));
		}

		for (const auto &visibilityEntry : visibilityTasks) {
			if (!visibilityEntry.second.size()) continue;
			auto taskBSON = repo::core::model::RepoBSONFactory::makeVisibilityTask(visibilityEntry.first, visibilityEntry.second);

			tasks.push_back(taskBSON);
			frameToTasks[frame.first].push_back(taskBSON.getUUIDField(REPO_LABEL_ID));			
		}
	}
	std::string animationName = animation.name.empty() ? DEFAULT_SEQUENCE_NAME : animation.name;
	auto sequence = repo::core::model::RepoBSONFactory::makeSequence(frameToTasks, animationName);
	repoInfo << "Animation constructed, number of frames: " << frameToTasks.size() << "# tasks:" << tasks.size();
	scene->addSequence(sequence, tasks);
	return scene;
}
#endif