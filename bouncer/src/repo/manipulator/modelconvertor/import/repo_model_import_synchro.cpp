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
#include "../../../lib/repo_property_tree.h"
#include "../../../error_codes.h"

using namespace repo::manipulator::modelconvertor;

const std::string RESOURCE_ID_NAME = "Resource ID";
const std::string DEFAULT_SEQUENCE_NAME = "Unnamed Sequence";


const static std::string SEQ_CACHE_LABEL_TRANSPARENCY = "transparency";
const static std::string SEQ_CACHE_LABEL_COLOR = "color";
const static std::string SEQ_CACHE_LABEL_VALUE = "value";
const static std::string SEQ_CACHE_LABEL_SHARED_IDS = "shared_ids";
const static std::string SEQ_CACHE_LABEL_CAMERA = "camera";
const static std::string SEQ_CACHE_LABEL_POSITION = "position";
const static std::string SEQ_CACHE_LABEL_DIRECTION = "direction";
const static std::string SEQ_CACHE_LABEL_FORWARD = "forward";
const static std::string SEQ_CACHE_LABEL_UP = "up";
const static std::string SEQ_CACHE_LABEL_FOV = "fov";
const static std::string SEQ_CACHE_LABEL_PERSPECTIVE = "perspective";

class SynchroModelImport::CameraChange{
public:
	CameraChange(
		const repo::lib::RepoVector3D64 &_position,
		const repo::lib::RepoVector3D &_forward,
		const repo::lib::RepoVector3D &_up,
		const float &_fov,
		const bool _isPerspective
	) :
		position(_position),
		forward(_forward),
		up(_up),
		fov(_fov),
		isPerspective(_isPerspective) {}

	const repo::lib::RepoVector3D64 position;
	const repo::lib::RepoVector3D forward, up;
	const float fov;
	const bool isPerspective;
};

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

uint32_t SynchroModelImport::colourIn32Bit(const std::vector<float> &color) const {
	uint8_t bitColor[4];
	bitColor[0] = color[0] * 255;
	bitColor[1] = color[1] * 255;
	bitColor[2] = color[2] * 255;
	bitColor[3] = 0;

	return *(uint32_t*)&bitColor;
}

std::vector<float> SynchroModelImport::colourFrom32Bit(const uint32_t &color) const {
	auto colorArr = (uint8_t*)&color;
	return{ (float)colorArr[0] / 255.f, (float)colorArr[1] / 255.f, (float)colorArr[2] / 255.f };
}


std::pair<std::string, std::vector<uint8_t>> SynchroModelImport::generateCache(
	const std::unordered_map<repo::lib::RepoUUID, std::pair<float, float>, repo::lib::RepoUUIDHasher> &meshAlphaState,
	const std::unordered_map<repo::lib::RepoUUID, std::pair<uint32_t, std::vector<float>>, repo::lib::RepoUUIDHasher> &meshColourState,
	const std::unordered_map<repo::lib::RepoUUID, std::vector<double>, repo::lib::RepoUUIDHasher> &transformState,
	const std::unordered_map<repo::lib::RepoUUID, std::pair<repo::lib::RepoVector3D64, repo::lib::RepoVector3D64>, repo::lib::RepoUUIDHasher> &clipState,
	const std::shared_ptr<CameraChange> &cam) {

	std::vector<repo::lib::PropertyTree> transparencyStates, colourStates, transformationStates, clipPlaneStates;
	std::unordered_map<float, std::vector<std::string>> transToIDs;
	std::unordered_map<uint32_t, std::vector<std::string>> colorToIDs;

	for (const auto &entry : meshAlphaState) {
		auto value = entry.second.second;
		if (value != entry.second.first) {
			if (transToIDs.find(value) == transToIDs.end()) {
				transToIDs[value] = { entry.first.toString() };
			}
			else {
				transToIDs[value].push_back(entry.first.toString());
			}
		}
	}

	for (const auto &entry : meshColourState) {
		if (!entry.second.second.size()) continue;
		auto value = colourIn32Bit(entry.second.second);
		if (value != entry.second.first) {
			if (colorToIDs.find(value) == colorToIDs.end()) {
				colorToIDs[value] = { entry.first.toString() };
			}
			else {
				colorToIDs[value].push_back(entry.first.toString());
			}
		}
	}

	for (const auto &entry : transformState) {
		repo::lib::PropertyTree transformTree;
		transformTree.addToTree(SEQ_CACHE_LABEL_VALUE, entry.second);
		std::vector<repo::lib::RepoUUID> idArr = {entry.first};
		transformTree.addToTree(SEQ_CACHE_LABEL_SHARED_IDS, idArr);
		transformationStates.push_back(transformTree);
	}	

	for (const auto &entry : clipState) {
		repo::lib::PropertyTree clipTree, valueTree;
		valueTree.addToTree(SEQ_CACHE_LABEL_POSITION, entry.second.first);
		valueTree.addToTree(SEQ_CACHE_LABEL_DIRECTION, entry.second.second);
		clipTree.mergeSubTree(SEQ_CACHE_LABEL_VALUE, valueTree);
		std::vector<repo::lib::RepoUUID> idArr = { entry.first };
		clipTree.addToTree(SEQ_CACHE_LABEL_SHARED_IDS, idArr);
		clipPlaneStates.push_back(clipTree);
	}

	for (const auto &entry : transToIDs) {
		repo::lib::PropertyTree transTree;
		transTree.addToTree(SEQ_CACHE_LABEL_VALUE, entry.first);
		transTree.addToTree(SEQ_CACHE_LABEL_SHARED_IDS, entry.second);
		transparencyStates.push_back(transTree);
	}

	for (const auto &entry : colorToIDs) {
		repo::lib::PropertyTree colTree;
		colTree.addToTree(SEQ_CACHE_LABEL_VALUE, colourFrom32Bit(entry.first));
		colTree.addToTree(SEQ_CACHE_LABEL_SHARED_IDS, entry.second);
		colourStates.push_back(colTree);
	}
	
	repo::lib::PropertyTree bufferTree;
	if (transparencyStates.size()) bufferTree.addArrayObjects(SEQ_CACHE_LABEL_TRANSPARENCY, transparencyStates);
	if (colourStates.size()) bufferTree.addArrayObjects(SEQ_CACHE_LABEL_COLOR, colourStates);
	if (transformationStates.size()) bufferTree.addArrayObjects(SEQ_CACHE_LABEL_COLOR, transformationStates);
	if (clipPlaneStates.size()) bufferTree.addArrayObjects(SEQ_CACHE_LABEL_COLOR, clipPlaneStates);


	if (cam) {
		repo::lib::PropertyTree camTree;
		camTree.addToTree(SEQ_CACHE_LABEL_POSITION, cam->position);
		camTree.addToTree(SEQ_CACHE_LABEL_FORWARD, cam->forward);
		camTree.addToTree(SEQ_CACHE_LABEL_UP, cam->up);
		camTree.addToTree(SEQ_CACHE_LABEL_PERSPECTIVE, cam->isPerspective ? "true" : "false");
		camTree.addToTree(SEQ_CACHE_LABEL_FOV, cam->fov);
		bufferTree.mergeSubTree(SEQ_CACHE_LABEL_CAMERA, camTree);
	}

	std::stringstream ss;
	bufferTree.write_json(ss);
	auto data = ss.str();
	
	//FIXME: Refactor to prop tree
	std::vector<uint8_t> binData;
	binData.resize(data.size());
	memcpy(binData.data(), data.c_str(), data.size());

	return {repo::lib::RepoUUID::createUUID().toString(), binData};

}

void SynchroModelImport::updateFrameState(
	const std::vector<std::shared_ptr<synchro_reader::AnimationTask>> &tasks,
	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> &resourceIDsToSharedIDs,
	std::unordered_map<repo::lib::RepoUUID, std::pair<float, float>, repo::lib::RepoUUIDHasher> &meshAlphaState,
	std::unordered_map<repo::lib::RepoUUID, std::pair<uint32_t, std::vector<float>>, repo::lib::RepoUUIDHasher> &meshColourState,
	std::unordered_map<repo::lib::RepoUUID, std::vector<double>, repo::lib::RepoUUIDHasher> &transformState,
	std::unordered_map<repo::lib::RepoUUID, std::pair<repo::lib::RepoVector3D64, repo::lib::RepoVector3D64>, repo::lib::RepoUUIDHasher> &clipState,
	std::shared_ptr<CameraChange> &cam

	) {
	for (const auto &task : tasks) {
		switch (task->getType()) {
			case synchro_reader::AnimationTask::TaskType::CAMERA:
				{
					auto camChange = std::dynamic_pointer_cast<const synchro_reader::CameraChange>(task);
					cam = std::make_shared<CameraChange>(
							repo::lib::RepoVector3D64(camChange->position.x, camChange->position.y, camChange->position.z),
							repo::lib::RepoVector3D(camChange->forward.x, camChange->forward.y, camChange->forward.z),
							repo::lib::RepoVector3D(camChange->up.x, camChange->up.y, camChange->up.z),
							camChange->fov,
							camChange->isPerspective
						);

				}
				break;
		case synchro_reader::AnimationTask::TaskType::CLIP:
		{
			auto clipTask = std::dynamic_pointer_cast<const synchro_reader::ClipTask>(task);
			auto meshes = resourceIDsToSharedIDs[clipTask->resourceID];
			if (clipTask->reset) {
				for (const auto &mesh : meshes)
					clipState.erase(mesh);
			}
			else {
				repo::lib::RepoVector3D64 position = { clipTask->position.x, clipTask->position.y, clipTask->position.z};
				repo::lib::RepoVector3D64 dir = { clipTask->dir.x, clipTask->dir.y, clipTask->dir.z };
				auto clip = std::pair<repo::lib::RepoVector3D64, repo::lib::RepoVector3D64>(position, dir);
				for (const auto &mesh : meshes)
					clipState[mesh] = clip;
			}
			
		}
		break;
		case synchro_reader::AnimationTask::TaskType::COLOR:
		{
			auto colourTask = std::dynamic_pointer_cast<const synchro_reader::ColourTask>(task);

			if (resourceIDsToSharedIDs.find(colourTask->resourceID) != resourceIDsToSharedIDs.end()) {
				auto color = colourTask->color;
				auto colour32Bit = colourIn32Bit(color);
				auto meshes = resourceIDsToSharedIDs[colourTask->resourceID];
				for (const auto mesh : meshes) {
					meshColourState[mesh].second = meshColourState[mesh].first == colour32Bit ? std::vector<float>() : color;
				}
			}
		}
		break;
		case synchro_reader::AnimationTask::TaskType::TRANSFORMATION:
		{
			auto transTask = std::dynamic_pointer_cast<const synchro_reader::TransformationTask>(task);
			auto meshes = resourceIDsToSharedIDs[transTask->resourceID];
			if (transTask->reset) {
				for (const auto &mesh : meshes)
					transformState.erase(mesh);
			}
			else {
				for (const auto &mesh : meshes)
					transformState[mesh] = transTask->trans;
			}

		}
		break;
		case synchro_reader::AnimationTask::TaskType::VISIBILITY:
		{
			auto visibilityTask = std::dynamic_pointer_cast<const synchro_reader::VisibilityTask>(task);
			if (resourceIDsToSharedIDs.find(visibilityTask->resourceID) != resourceIDsToSharedIDs.end()) {
				auto visibility = visibilityTask->visibility / 100.;
				auto meshes = resourceIDsToSharedIDs[visibilityTask->resourceID];
				for (const auto mesh : meshes) {
					meshAlphaState[mesh].second = meshAlphaState[mesh].first == visibility ? -1 : visibility;
				}
			}
		}
		break;
		}
	}

}

void SynchroModelImport::addTasks(
	std::unordered_map<std::string, std::shared_ptr<repo::core::model::RepoSequence::Task>> &currentTasks,
	std::vector<std::string> &toAdd,
	std::map<std::string, synchro_reader::Task> &tasks,
	std::unordered_map<std::string, repo::lib::RepoUUID> &taskIDtoRepoID
	) {
	for (const auto &entry : toAdd) {
		std::list<std::string> hierachy;
		auto currentEntry = entry;
		while(!currentEntry.empty()) {
			hierachy.push_front(currentEntry);
			currentEntry = tasks[currentEntry].parentTask;
		}

		auto taskList = &currentTasks;
		
		for (const auto &taskID : hierachy) {
			if (taskList->find(taskID) == taskList->end()) {
				repo::core::model::RepoSequence::Task newTask;
				newTask.name = tasks[taskID].name;
				newTask.startTime = tasks[taskID].startTime;
				newTask.endTime = tasks[taskID].endTime;
				newTask.id = taskIDtoRepoID[taskID];
				(*taskList)[taskID] = std::make_shared<repo::core::model::RepoSequence::Task>(newTask);
			}
			
			taskList = &((*taskList)[taskID]->childTasks);
			
		}
	}
}

void SynchroModelImport::removeTasks(
	std::unordered_map<std::string, std::shared_ptr<repo::core::model::RepoSequence::Task>> &currentTasks,
	std::vector<std::string> &toRemove,
	std::map<std::string, synchro_reader::Task> &tasks
) {
	for (const auto &entry : toRemove) {
		std::list<std::string> hierachy;
		auto currentEntry = tasks[entry].parentTask;
		while (!currentEntry.empty()) {
			hierachy.push_front(currentEntry);
			currentEntry = tasks[currentEntry].parentTask;
		}

		auto taskList = &currentTasks;

		for (const auto &taskID : hierachy) {
			if (taskList->find(taskID) != taskList->end()) {
				taskList = &((*taskList)[taskID]->childTasks);
			}
		}
		if (taskList->find(entry) != taskList->end()) {
			taskList->erase(entry);
		}
	}
}

repo::core::model::RepoScene* SynchroModelImport::generateRepoScene() {

	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> resourceIDsToSharedIDs;

	auto scene = constructScene(resourceIDsToSharedIDs);

	repoInfo << "Getting animations... ";
	auto animInfo = reader->getAnimation();


	auto animation = animInfo.first;
	auto taskInfo = reader->getTasks();

	std::unordered_map<repo::lib::RepoUUID, std::pair<float, float>, repo::lib::RepoUUIDHasher> meshAlphaState;
	std::unordered_map<repo::lib::RepoUUID, std::pair<uint32_t, std::vector<float>>, repo::lib::RepoUUIDHasher> meshColourState;
	std::unordered_map<std::string, std::vector<uint8_t>> stateBuffers;
	std::vector<repo::core::model::RepoSequence::FrameData> frameData;
	std::set<repo::lib::RepoUUID> defaultInvisible;
	std::unordered_map<std::string, repo::lib::RepoUUID> taskIDtoRepoID;
	std::vector<repo::core::model::RepoTask> taskBSONs;

	for (const auto &task : taskInfo.tasks) {
		std::vector<repo::lib::RepoUUID> parents;
		auto parentID = task.second.parentTask;
		if (!parentID.empty()) {
			if (taskIDtoRepoID.find(parentID) == taskIDtoRepoID.end()) {
				taskIDtoRepoID[parentID] = repo::lib::RepoUUID::createUUID();
			}

			parents.push_back(taskIDtoRepoID[parentID]);
		}

		auto taskID = task.second.id;
		if(taskIDtoRepoID.find(taskID) == taskIDtoRepoID.end())
			taskIDtoRepoID[taskID] = repo::lib::RepoUUID::createUUID();

		std::vector<repo::lib::RepoUUID> relatedEntities;
		for (const auto &resourceID : task.second.relatedResources) {
			if (resourceIDsToSharedIDs.find(resourceID) != resourceIDsToSharedIDs.end()) {
				relatedEntities.insert(relatedEntities.end(), resourceIDsToSharedIDs[resourceID].begin(), resourceIDsToSharedIDs[resourceID].end());
			}
		}
		taskBSONs.push_back(repo::core::model::RepoBSONFactory::makeTask(task.second.name, task.second.data, relatedEntities, parents, taskIDtoRepoID[taskID]));
	}


	auto meshes = scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT);
	for (const auto &lastStateEntry : animInfo.second) {
		if (resourceIDsToSharedIDs.find(lastStateEntry.first) == resourceIDsToSharedIDs.end()) continue;
		for (const auto &id : resourceIDsToSharedIDs[lastStateEntry.first]) {
			float defaultAlpha = 1;

			auto material = scene->getChildrenNodesFiltered(repo::core::model::RepoScene::GraphType::DEFAULT, id, repo::core::model::NodeType::MATERIAL);
			std::vector<float> materialCol = { 0,0,0 };
			if (material.size()) {
				auto materialNode = (repo::core::model::MaterialNode*)material[0];
				materialCol = materialNode->getMaterialStruct().diffuse;
				defaultAlpha = materialNode->getMaterialStruct().opacity;
			}

			if (!lastStateEntry.second) {
				defaultInvisible.insert(scene->getNodeBySharedID(repo::core::model::RepoScene::GraphType::DEFAULT, id)->getUniqueID());
				defaultAlpha = 0;
			}

			meshColourState[id] = { colourIn32Bit(materialCol), std::vector<float >() };
			meshAlphaState[id] = { defaultAlpha, -1 };
		}
	}

	std::string validCache;
	std::shared_ptr<CameraChange> cam = nullptr;

	std::unordered_map<repo::lib::RepoUUID, std::vector<double>, repo::lib::RepoUUIDHasher> transformState;
	std::unordered_map<repo::lib::RepoUUID, std::pair<repo::lib::RepoVector3D64, repo::lib::RepoVector3D64>, repo::lib::RepoUUIDHasher> clipState;

	auto currentFrame = animation.frames.begin();
	auto currentStart = taskInfo.taskStartDates.begin();
	auto currentEnd = taskInfo.taskEndDates.begin();

	std::unordered_map<std::string, std::shared_ptr<repo::core::model::RepoSequence::Task>> currentTasks;
	while (currentFrame != animation.frames.end() || currentStart != taskInfo.taskStartDates.end() || currentEnd != taskInfo.taskEndDates.end()) {
		auto currentTime = currentFrame != animation.frames.end() ? currentFrame->first : -1;

		currentTime = std::min(currentTime, currentStart != taskInfo.taskStartDates.end() ? currentStart->first : -1);

		currentTime = std::min(currentTime, currentEnd != taskInfo.taskEndDates.end() ? currentEnd->first : -1);

		if (currentFrame != animation.frames.end() && currentFrame->first == currentTime) {
			updateFrameState(currentFrame->second, resourceIDsToSharedIDs, meshAlphaState, meshColourState, transformState, clipState, cam);
			auto cacheData = generateCache(meshAlphaState, meshColourState, transformState, clipState, cam);
			validCache = cacheData.first;
			stateBuffers[validCache] = cacheData.second;
			currentFrame++;
		}

		if (currentStart != taskInfo.taskStartDates.end() && currentStart->first == currentTime) {
			addTasks(currentTasks, currentStart->second, taskInfo.tasks, taskIDtoRepoID);
			currentStart++;
		}

		if (currentEnd != taskInfo.taskEndDates.end() && currentEnd->first == currentTime) {
			removeTasks(currentTasks, currentEnd->second, taskInfo.tasks);
			currentEnd++;
		}


		repo::core::model::RepoSequence::FrameData data;
		data.ref = validCache;
		data.timestamp = currentTime;
		data.currentTasks = currentTasks;
		frameData.push_back(data);		
	}



	std::string animationName = animation.name.empty() ? DEFAULT_SEQUENCE_NAME : animation.name;
	auto sequence = repo::core::model::RepoBSONFactory::makeSequence(frameData, animationName);
	repoInfo << "Animation constructed, number of frames: " << frameData.size();
	scene->addSequence(sequence, stateBuffers, taskBSONs);
	scene->setDefaultInvisible(defaultInvisible);
	repoInfo << "#default invisible: " << defaultInvisible.size();


	return scene;
}
#endif