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
#include "../../../lib/datastructure/repo_matrix.h"
#include "../../../error_codes.h"

using namespace repo::manipulator::modelconvertor;

const std::string RESOURCE_ID_NAME = "Resource ID";
const std::string DEFAULT_SEQUENCE_NAME = "Unnamed Sequence";

const static std::string SEQ_CACHE_LABEL_TRANSPARENCY = "transparency";
const static std::string SEQ_CACHE_LABEL_COLOR = "color";
const static std::string SEQ_CACHE_LABEL_TRANSFORMATION = "transformation";
const static std::string SEQ_CACHE_LABEL_CLIP = "clip";
const static std::string SEQ_CACHE_LABEL_VALUE = "value";
const static std::string SEQ_CACHE_LABEL_SHARED_IDS = "shared_ids";
const static std::string SEQ_CACHE_LABEL_CAMERA = "camera";
const static std::string SEQ_CACHE_LABEL_POSITION = "position";
const static std::string SEQ_CACHE_LABEL_DIRECTION = "direction";
const static std::string SEQ_CACHE_LABEL_FORWARD = "forward";
const static std::string SEQ_CACHE_LABEL_UP = "up";
const static std::string SEQ_CACHE_LABEL_FOV = "fov";
const static std::string SEQ_CACHE_LABEL_PERSPECTIVE = "perspective";

class SynchroModelImport::CameraChange {
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

bool SynchroModelImport::importModel(std::string filePath, uint8_t &errCode) {
	orgFile = filePath;
	reader = std::make_shared<synchro_reader::SynchroReader>(filePath, settings.getTimeZone());
	repoInfo << "=== IMPORTING MODEL WITH SYNCHRO MODEL CONVERTOR (animations: " << settings.shouldImportAnimations() << ") ===";
	repoInfo << "Sequence timezone is set to : " << (settings.getTimeZone().empty() ? "UTC" : settings.getTimeZone());
	std::string msg;
	auto synchroErrCode = reader->init(msg);
	if (synchroErrCode != synchro_reader::SynchroError::ERR_OK) {
		if (synchroErrCode == synchro_reader::SynchroError::ERR_UNSUPPORTED_VERSION_PREV ||
			synchroErrCode == synchro_reader::SynchroError::ERR_UNSUPPORTED_VERSION_FUTURE
			)
			errCode = REPOERR_UNSUPPORTED_VERSION;
		else
			errCode = REPOERR_LOAD_SCENE_FAIL;
		repoError << msg;
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

	return{ matNodes, textNodes };
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
				normals.push_back({ (float)meshDetails.normals[i].x, (float)meshDetails.normals[i].y, (float)meshDetails.normals[i].z });
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
			uvs.push_back({ (float)uv.x, (float)uv.y });
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
	//trans[3] -= offset[0];
	//trans[7] -= offset[1];
	//trans[11] -= offset[2];
	auto matrix = repo::lib::RepoMatrix(trans);
	return new repo::core::model::MeshNode(
		templateMesh.cloneAndApplyTransformation(matrix).cloneAndAddParent(parentID, true, true));
}

std::vector<repo::lib::RepoUUID> SynchroModelImport::findRecursiveMeshSharedIDs(
	repo::core::model::RepoScene* &scene,
	const repo::lib::RepoUUID &id
) const {
	auto node = scene->getNodeByUniqueID(repo::core::model::RepoScene::GraphType::DEFAULT, id);

	if (node->getTypeAsEnum() == repo::core::model::NodeType::MESH) {
		return { node->getSharedID() };
	}

	auto allMeshes = scene->getAllDescendantsByType(repo::core::model::RepoScene::GraphType::DEFAULT, node->getSharedID(), repo::core::model::NodeType::MESH);
	std::vector<repo::lib::RepoUUID> results;

	for (const auto &mesh : allMeshes) {
		results.push_back(mesh->getSharedID());
	}

	return results;
}

void SynchroModelImport::determineMeshesInResources(
	repo::core::model::RepoScene* &scene,
	const std::unordered_map<repo::lib::RepoUUID, std::vector<repo::lib::RepoUUID>, repo::lib::RepoUUIDHasher> &entityNodesToMeshesSharedID,
	const std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> &resourceIDsToEntityNodes,
	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> &resourceIDsToSharedIDs) {
	for (const auto &entry : resourceIDsToEntityNodes) {
		resourceIDsToSharedIDs[entry.first] = {};
		for (const auto &nodeId : entry.second) {
			auto meshes = findRecursiveMeshSharedIDs(scene, nodeId);
			resourceIDsToSharedIDs[entry.first].insert(resourceIDsToSharedIDs[entry.first].end(), meshes.begin(), meshes.end());
		}
	}
}

void SynchroModelImport::determineUnits(const synchro_reader::Units &units) {
	switch (units) {
	case synchro_reader::Units::CENTIMETRES:
		modelUnits = ModelUnits::CENTIMETRES;
		break;
	case synchro_reader::Units::FEET:
		modelUnits = ModelUnits::FEET;
		break;
	case synchro_reader::Units::INCHES:
		modelUnits = ModelUnits::INCHES;
		break;
	case synchro_reader::Units::METRES:
		modelUnits = ModelUnits::METRES;
		break;
	case synchro_reader::Units::MILIMETRES:
		modelUnits = ModelUnits::MILLIMETRES;
		break;
	default:
		modelUnits = ModelUnits::UNKNOWN;
	}
}

repo::core::model::RepoScene* SynchroModelImport::constructScene(
	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> &resourceIDsToSharedIDs,
	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> &resourceIDsToTransIDs
) {
	repo::core::model::RepoNodeSet transNodes, matNodes, textNodes, meshNodes, metaNodes;
	std::unordered_map<std::string, repo::lib::RepoUUID> synchroIDToRepoID;
	std::unordered_map<repo::lib::RepoUUID, repo::core::model::RepoNode*, repo::lib::RepoUUIDHasher> repoIDToNode;
	repoInfo << "Generating materials.... ";
	auto matPairs = generateMatNodes(synchroIDToRepoID, repoIDToNode);
	matNodes = matPairs.first;
	textNodes = matPairs.second;

	auto identity = repo::lib::RepoMatrix();
	determineUnits(reader->getUnits());
	auto root = createTransNode(identity, reader->getProjectName());
	transNodes.insert(root);

	std::unordered_map<repo::lib::RepoUUID, std::vector<repo::lib::RepoUUID>, repo::lib::RepoUUIDHasher> nodeToParents;
	std::unordered_map<repo::lib::RepoUUID, std::string, repo::lib::RepoUUIDHasher> nodeToSynchroParent;
	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> resourceIDsToEntityNodes;
	std::unordered_map<repo::lib::RepoUUID, std::vector<repo::lib::RepoUUID>, repo::lib::RepoUUIDHasher> entityNodesToMeshesSharedID;

	repoInfo << "Generating mesh templates...";
	auto meshNodeTemplates = createMeshTemplateNodes();
	std::vector<synchro_reader::Vector3D> bbox;
	repoInfo << "Reading entities ";
	for (const auto entity : reader->getEntities(bbox)) {
		auto trans = createTransNode(identity, entity.second.name);
		auto resourceID = entity.second.resourceID;
		transNodes.insert(trans);
		auto transUniqueID = trans->getUniqueID();
		if (!resourceID.empty()) {
			if (resourceIDsToTransIDs.find(resourceID) == resourceIDsToTransIDs.end())
				resourceIDsToTransIDs[resourceID] = { transUniqueID };
			else
				resourceIDsToTransIDs[resourceID].push_back(transUniqueID);
		}
		repoIDToNode[transUniqueID] = trans;
		synchroIDToRepoID[entity.second.id] = transUniqueID;
		nodeToSynchroParent[transUniqueID] = entity.second.parentID;
		auto meta = entity.second.metadata;
		meta[RESOURCE_ID_NAME] = resourceID;

		std::vector<repo::lib::RepoUUID> metaParents = { trans->getSharedID() };

		entityNodesToMeshesSharedID[transUniqueID] = {};
		if (resourceIDsToEntityNodes.find(resourceID) == resourceIDsToEntityNodes.end()) {
			resourceIDsToEntityNodes[resourceID] = { transUniqueID };
		}
		else {
			resourceIDsToEntityNodes[resourceID].push_back(transUniqueID);
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
			entityNodesToMeshesSharedID[transUniqueID].push_back(mesh->getSharedID());
			synchroIDToRepoID[meshID] = mesh->getUniqueID();
			repoIDToNode[mesh->getUniqueID()] = mesh;
			if (!resourceID.empty())
				resourceIDsToSharedIDs[resourceID].push_back(mesh->getSharedID());
		}

		if (meta.size() > 1 || !resourceID.empty()) {
			metaNodes.insert(createMetaNode(meta, entity.second.name, metaParents));
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

	/*std::vector<double> offset = { origin.x + bbox[0].x, origin.y + bbox[0].y, origin.z + bbox[0].z };*/
	std::vector<double> offset = { origin.x, origin.y, origin.z };
	repoInfo << "Setting Global Offset: " << offset[0] << ", " << offset[1] << ", " << offset[2];
	scene->setWorldOffset(offset);

	// Gather all meshes belong to a certain resource ID
	determineMeshesInResources(scene, entityNodesToMeshesSharedID, resourceIDsToEntityNodes, resourceIDsToSharedIDs);
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

std::string SynchroModelImport::generateCache(
	const std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> &resourceIDsToSharedIDs,
	const std::unordered_map<float, std::set<std::string>> &alphaValueToIDs,
	const std::unordered_map<repo::lib::RepoUUID, std::pair<uint32_t, std::vector<float>>, repo::lib::RepoUUIDHasher> &meshColourState,
	const std::unordered_map<std::string, std::vector<double>> &resourceIDTransState,
	const std::unordered_map<repo::lib::RepoUUID, std::pair<repo::lib::RepoVector3D64, repo::lib::RepoVector3D64>, repo::lib::RepoUUIDHasher> &clipState,
	const std::shared_ptr<CameraChange> &cam,
	std::unordered_map<std::string, std::vector<uint8_t>> &stateBuffers) {
	std::vector<repo::lib::PropertyTree> transparencyStates, colourStates, transformationStates, clipPlaneStates;
	std::unordered_map<float, std::vector<std::string>> transToIDs;
	std::unordered_map<uint32_t, std::vector<std::string>> colorToIDs;

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
	if (settings.shouldImportAnimations()) {
		for (const auto &entry : resourceIDTransState) {
			if (resourceIDsToSharedIDs.find(entry.first) != resourceIDsToSharedIDs.end()
				&& resourceIDsToSharedIDs.at(entry.first).size()) {
				repo::lib::PropertyTree transformTree;
				transformTree.addToTree(SEQ_CACHE_LABEL_VALUE, entry.second);
				std::vector<repo::lib::RepoUUID> idArr = resourceIDsToSharedIDs.at(entry.first);
				transformTree.addToTree(SEQ_CACHE_LABEL_SHARED_IDS, idArr);
				transformationStates.push_back(transformTree);
			}
		}
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

	for (const auto &entry : alphaValueToIDs) {
		if (!entry.second.size()) continue;
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
	if (transformationStates.size()) bufferTree.addArrayObjects(SEQ_CACHE_LABEL_TRANSFORMATION, transformationStates);
	if (clipPlaneStates.size()) bufferTree.addArrayObjects(SEQ_CACHE_LABEL_CLIP, clipPlaneStates);

	if (cam) {
		repo::lib::PropertyTree camTree;
		camTree.addToTree(SEQ_CACHE_LABEL_POSITION, cam->position);
		camTree.addToTree(SEQ_CACHE_LABEL_FORWARD, cam->forward);
		camTree.addToTree(SEQ_CACHE_LABEL_UP, cam->up);
		camTree.addToTree(SEQ_CACHE_LABEL_PERSPECTIVE, cam->isPerspective ? "true" : "false");
		camTree.addToTree(SEQ_CACHE_LABEL_FOV, cam->fov);
		bufferTree.mergeSubTree(SEQ_CACHE_LABEL_CAMERA, camTree);
	}

	auto id = repo::lib::RepoUUID::createUUID().toString();

	stateBuffers[id] = bufferTree.writeJsonToBuffer();
	return id;
}

repo::lib::RepoMatrix64 SynchroModelImport::convertMatrixTo3DRepoWorld(
	const repo::lib::RepoMatrix64 &matrix,
	const std::vector<double> &offset) {
	std::vector<double> toDX = {
		1, 0, 0, 0,
		0, 0, -1, 0,
		0, 1, 0, 0,
		0, 0, 0,  1
	};
	std::vector<double> toGL = {
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, -1, 0, 0,
		0, 0, 0,  1
	};

	std::vector<double> toWorld = {
		1, 0, 0, offset[0],
		0, 1, 0, offset[1],
		0, 0, 1, offset[2],
		0, 0, 0,  1
	};

	repo::lib::RepoMatrix64 matDX(toDX);
	repo::lib::RepoMatrix64 matGL(toGL);
	repo::lib::RepoMatrix64 matToWorld(toWorld);
	auto fromWorld = matToWorld.invert();

	return matToWorld * matGL * matrix * matDX * fromWorld;
}

void SynchroModelImport::updateFrameState(
	const std::vector<std::shared_ptr<synchro_reader::AnimationTask>> &tasks,
	const std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> &resourceIDsToSharedIDs,
	const std::unordered_map<std::string, repo::lib::RepoMatrix64> &resourceIDLastTrans,
	std::unordered_map<float, std::set<std::string>> &alphaValueToIDs,
	std::unordered_map<repo::lib::RepoUUID, std::pair<float, float>, repo::lib::RepoUUIDHasher> &meshAlphaState,
	std::unordered_map<repo::lib::RepoUUID, std::pair<uint32_t, std::vector<float>>, repo::lib::RepoUUIDHasher> &meshColourState,
	std::unordered_map<std::string, std::vector<double>> &resourceIDTransState,
	std::unordered_map<repo::lib::RepoUUID, std::pair<repo::lib::RepoVector3D64, repo::lib::RepoVector3D64>, repo::lib::RepoUUIDHasher> &clipState,
	std::shared_ptr<CameraChange> &cam,
	std::set<std::string> &transformingResource,
	const std::vector<double> &offset

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
			auto meshes = resourceIDsToSharedIDs.at(clipTask->resourceID);
			if (clipTask->reset) {
				for (const auto &mesh : meshes)
					clipState.erase(mesh);
			}
			else {
				repo::lib::RepoVector3D64 position = { clipTask->position.x, clipTask->position.y, clipTask->position.z };
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
				bool reset = !color.size();
				auto colour32Bit = reset ? 0 : colourIn32Bit(color);
				auto meshes = resourceIDsToSharedIDs.at(colourTask->resourceID);
				for (const auto &mesh : meshes) {
					meshColourState[mesh].second = reset || meshColourState[mesh].first == colour32Bit ? std::vector<float>() : color;
				}
			}
		}
		break;
		case synchro_reader::AnimationTask::TaskType::TRANSFORMATION:
		{
			if (settings.shouldImportAnimations()) {
				auto transTask = std::dynamic_pointer_cast<const synchro_reader::TransformationTask>(task);

				auto meshes = resourceIDsToSharedIDs.at(transTask->resourceID);
				transformingResource.insert(transTask->resourceID);
				repo::lib::RepoMatrix64 matrix(transTask->trans);

				bool isTransforming = true;

				if (resourceIDLastTrans.find(transTask->resourceID) != resourceIDLastTrans.end()) {
					if (transTask->reset) {
						//The resource has a default animation state, so we actually have to move it to the inverse matrix
						matrix = repo::lib::RepoMatrix64();
					}
					else if (matrix.equals(resourceIDLastTrans.at(transTask->resourceID))) {
						//it's moving to the final state
						resourceIDTransState.erase(transTask->resourceID);
						isTransforming = false;
					}
				}
				else if (transTask->reset) {
					resourceIDTransState.erase(transTask->resourceID);
					isTransforming = false;
				}

				if (isTransforming) {
					if (resourceIDLastTrans.find(transTask->resourceID) != resourceIDLastTrans.end()) {
						//the geometry takes the transformation of the final frame as the default position. undo it before applying a new transformation.
						matrix = matrix * resourceIDLastTrans.at(transTask->resourceID).invert();
					}

					matrix = convertMatrixTo3DRepoWorld(matrix, offset);

					if (matrix.isIdentity()) {
						//We're moving it to the final state. undo the transformation
						resourceIDTransState.erase(transTask->resourceID);
					}
					else {
						resourceIDTransState[transTask->resourceID] = matrix.getData();
					}
				}
			}
			else {
				repoInfo << "Transformation found but it's disabled";
			}
		}
		break;
		case synchro_reader::AnimationTask::TaskType::VISIBILITY:
		{
			auto visibilityTask = std::dynamic_pointer_cast<const synchro_reader::VisibilityTask>(task);
			if (resourceIDsToSharedIDs.find(visibilityTask->resourceID) != resourceIDsToSharedIDs.end()) {
				auto visibility = visibilityTask->visibility;
				auto meshes = resourceIDsToSharedIDs.at(visibilityTask->resourceID);

				for (const auto mesh : meshes) {
					auto previousState = meshAlphaState[mesh].second;
					auto meshStr = mesh.toString();
					if (alphaValueToIDs.find(previousState) != alphaValueToIDs.end())
						alphaValueToIDs[previousState].erase(meshStr);
					if (visibility == -1) {
						meshAlphaState[mesh].second = meshAlphaState[mesh].first;
					}
					else {
						meshAlphaState[mesh].second = visibility;
						if (alphaValueToIDs.find(visibility) == alphaValueToIDs.end()) {
							alphaValueToIDs[visibility] = { meshStr };
						}
						else {
							alphaValueToIDs[visibility].insert(meshStr);
						}
					}
				}
			}
		}
		break;
		}
	}
}

repo::lib::PropertyTree SynchroModelImport::createTaskTree(
	const SequenceTask &task,
	const std::unordered_map<repo::lib::RepoUUID, std::set<SequenceTask, SequenceTaskComparator>, repo::lib::RepoUUIDHasher> &taskToChildren
) {
	repo::lib::PropertyTree taskTree;
	taskTree.addToTree(TASK_ID, task.id);
	taskTree.addToTree(TASK_NAME, task.name);
	taskTree.addToTree(TASK_START_DATE, task.startTime);
	taskTree.addToTree(TASK_END_DATE, task.endTime);

	if (taskToChildren.find(task.id) != taskToChildren.end()) {
		std::vector<repo::lib::PropertyTree> childrenTrees;
		for (const auto subTask : taskToChildren.at(task.id)) {
			childrenTrees.push_back(createTaskTree(subTask, taskToChildren));
		}

		taskTree.addArrayObjects(TASK_CHILDREN, childrenTrees);
	}

	return taskTree;
}

std::vector<uint8_t> SynchroModelImport::generateTaskCache(
	const std::set<SequenceTask, SequenceTaskComparator> &rootTasks,
	const std::unordered_map<repo::lib::RepoUUID, std::set<SequenceTask, SequenceTaskComparator>, repo::lib::RepoUUIDHasher> &taskToChildren
) {
	std::vector<repo::lib::PropertyTree> tasks;

	for (const auto &task : rootTasks) {
		tasks.push_back(createTaskTree(task, taskToChildren));
	}

	repo::lib::PropertyTree root;
	root.addArrayObjects("activities", tasks);
	return root.writeJsonToBuffer();
}

std::pair<uint64_t, uint64_t> SynchroModelImport::generateTaskInformation(
	const synchro_reader::TasksInformation &taskInfo,
	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> &resourceIDsToSharedIDs,
	repo::core::model::RepoScene* &scene,
	const repo::lib::RepoUUID &sequenceID
) {
	std::unordered_map<std::string, repo::lib::RepoUUID> taskIDtoRepoID;
	std::unordered_map<repo::lib::RepoUUID, std::set<SequenceTask, SequenceTaskComparator>, repo::lib::RepoUUIDHasher> taskToChildren;
	std::set<SequenceTask, SequenceTaskComparator> rootTasks;
	std::vector<repo::core::model::RepoTask> taskBSONs;

	uint64_t firstTS = (uint64_t)std::numeric_limits<uint64_t>::max;
	uint64_t lastTS = 0;
	for (const auto &task : taskInfo.tasks) {
		auto parentID = task.second.parentTask;
		repo::lib::RepoUUID parentUUID;

		auto taskID = task.second.id;
		if (taskIDtoRepoID.find(taskID) == taskIDtoRepoID.end()) {
			taskIDtoRepoID[taskID] = repo::lib::RepoUUID::createUUID();
		}
		auto startTime = task.second.startTime;
		auto endTime = task.second.endTime;

		SequenceTask taskItem = {
			taskIDtoRepoID[taskID] ,
			task.second.name,
			startTime * 1000,
			endTime * 1000
		};
		firstTS = std::min(startTime * 1000, firstTS);
		lastTS = std::max(endTime * 1000, lastTS);

		if (parentID.empty()) {
			rootTasks.insert(taskItem);
		}
		else {
			if (taskIDtoRepoID.find(parentID) == taskIDtoRepoID.end()) {
				parentUUID = repo::lib::RepoUUID::createUUID();
				taskIDtoRepoID[parentID] = parentUUID;
				taskToChildren[parentUUID] = {};
			}

			taskToChildren[taskIDtoRepoID[parentID]].insert(taskItem);
		}

		std::vector<repo::lib::RepoUUID> relatedEntities;
		for (const auto &resourceID : task.second.relatedResources) {
			if (resourceIDsToSharedIDs.find(resourceID) != resourceIDsToSharedIDs.end()) {
				relatedEntities.insert(relatedEntities.end(), resourceIDsToSharedIDs[resourceID].begin(), resourceIDsToSharedIDs[resourceID].end());
			}
		}
		taskBSONs.push_back(repo::core::model::RepoBSONFactory::makeTask(task.second.name, startTime * 1000, endTime * 1000, sequenceID, task.second.data, relatedEntities, parentUUID, taskIDtoRepoID[taskID]));
	}

	scene->addSequenceTasks(taskBSONs, generateTaskCache(rootTasks, taskToChildren));

	return { firstTS, lastTS };
}

repo::core::model::RepoScene* SynchroModelImport::generateRepoScene(uint8_t &errMsg) {
	std::unordered_map<std::string, std::vector<repo::lib::RepoUUID>> resourceIDsToSharedIDs, resourceIDsToTransIDs;

	repo::core::model::RepoScene* scene = nullptr;
	try {
		repoInfo << "Constructing scene...";
		scene = constructScene(resourceIDsToSharedIDs, resourceIDsToTransIDs);

		const auto sequenceID = repo::lib::RepoUUID::createUUID();

		repoInfo << "Getting tasks... ";

		auto taskFrame = generateTaskInformation(reader->getTasks(), resourceIDsToSharedIDs, scene, sequenceID);

		auto firstFrame = taskFrame.first;
		auto lastFrame = taskFrame.second;

		repoInfo << "Getting animations... ";
		auto animation = reader->getAnimation();

		std::unordered_map<repo::lib::RepoUUID, std::pair<float, float>, repo::lib::RepoUUIDHasher> meshAlphaState;
		std::unordered_map<repo::lib::RepoUUID, std::pair<uint32_t, std::vector<float>>, repo::lib::RepoUUIDHasher> meshColourState;
		std::unordered_map<std::string, std::vector<double>> resourceIDTransState;
		std::unordered_map<std::string, repo::lib::RepoMatrix64> resourceIDLastTrans;
		std::unordered_map<std::string, std::vector<uint8_t>> stateBuffers;
		std::vector<repo::core::model::RepoSequence::FrameData> frameData;
		std::set<repo::lib::RepoUUID> defaultInvisible;
		auto origin = reader->getGlobalOffset();
		std::vector<double> offset = { origin.x, origin.z , -origin.y };

		for (const auto &lastStateEntry : animation.lastFrameVisibility) {
			if (resourceIDsToSharedIDs.find(lastStateEntry.first) == resourceIDsToSharedIDs.end()) {
				continue;
			};

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
				}

				meshColourState[id] = { colourIn32Bit(materialCol), std::vector<float >() };
				meshAlphaState[id] = { defaultAlpha, defaultAlpha };
			}
		}

		for (const auto &lastStateEntry : animation.lastTransformation) {
			auto resourceID = lastStateEntry.first;
			if (resourceIDsToTransIDs.find(resourceID) == resourceIDsToTransIDs.end()) {
				continue;
			};

			auto matrix = repo::lib::RepoMatrix64(lastStateEntry.second);
			if (!matrix.isIdentity()) {
				resourceIDLastTrans[resourceID] = matrix;
				for (const auto transId : resourceIDsToTransIDs[resourceID]) {
					auto node = (repo::core::model::TransformationNode*) scene->getNodeByUniqueID(
						repo::core::model::RepoScene::GraphType::DEFAULT, transId);

					node->swap(node->cloneAndApplyTransformation(matrix.getData()));
				}

				auto matInverse = matrix.invert();
				resourceIDTransState[lastStateEntry.first] = convertMatrixTo3DRepoWorld(matInverse, offset).getData();
			}
		}

		std::shared_ptr<CameraChange> cam = nullptr;
		std::unordered_map<float, std::set<std::string>> alphaValueToIDs;
		std::unordered_map<repo::lib::RepoUUID, std::pair<repo::lib::RepoVector3D64, repo::lib::RepoVector3D64>, repo::lib::RepoUUIDHasher> clipState;

		std::set<std::string> transformingResources;

		int count = 0;
		auto total = animation.frames.size();
		int step = total > 10 ? total / 10 : 1;

		if (animation.frames.begin()->first > firstFrame &&
			resourceIDTransState.size()) {
			//First animation frame is bigger than the task frame
			//And we have animations... need to reset the state of the transforms.
			repo::core::model::RepoSequence::FrameData data;
			data.ref = generateCache(resourceIDsToSharedIDs, alphaValueToIDs, meshColourState, resourceIDTransState, clipState, cam, stateBuffers);
			data.timestamp = firstFrame;
			frameData.push_back(data);
		}

		for (const auto &currentFrame : animation.frames) {
			auto currentTime = currentFrame.first;
			firstFrame = std::min(firstFrame, currentTime * 1000);
			lastFrame = std::max(lastFrame, currentTime * 1000);

			updateFrameState(currentFrame.second, resourceIDsToSharedIDs, resourceIDLastTrans, alphaValueToIDs, meshAlphaState, meshColourState, resourceIDTransState, clipState, cam, transformingResources, offset);
			repo::core::model::RepoSequence::FrameData data;
			data.ref = generateCache(resourceIDsToSharedIDs, alphaValueToIDs, meshColourState, resourceIDTransState, clipState, cam, stateBuffers);
			data.timestamp = currentTime;
			frameData.push_back(data);
			if (++count % step == 0) {
				repoInfo << "Processed " << count << " of " << total << " frames";
			};
		}

		repoInfo << "transforming Mesh: " << transformingResources.size();
		for (const auto &resourceID : transformingResources) {
			for (const auto &mesh : resourceIDsToSharedIDs[resourceID]) {
				auto meshNode = (repo::core::model::MeshNode*) scene->getNodeBySharedID(repo::core::model::RepoScene::GraphType::DEFAULT, mesh);
				meshNode->swap(meshNode->cloneAndNoteGrouping(resourceID));
			}
		}

		std::string animationName = animation.name.empty() ? DEFAULT_SEQUENCE_NAME : animation.name;
		auto sequence = repo::core::model::RepoBSONFactory::makeSequence(frameData, animationName, sequenceID, firstFrame, lastFrame);

		if (sequence.objsize() > REPO_MAX_OBJ_SIZE) {
			errMsg = REPOERR_SYNCHRO_SEQUENCE_TOO_BIG;
			delete scene;
			scene = nullptr;
		}
		else {
			repoInfo << "Animation constructed, number of frames: " << frameData.size();
			scene->addSequence(sequence, stateBuffers);

			scene->setDefaultInvisible(defaultInvisible);
			repoInfo << "#default invisible: " << defaultInvisible.size();
		}
	}
	catch (const std::exception &e) {
		std::string error(e.what());
		if (error.find("BufBuilder") != std::string::npos) {
			errMsg = scene ? REPOERR_SYNCHRO_SEQUENCE_TOO_BIG : REPOERR_MAX_NODES_EXCEEDED;
		}
		else {
			errMsg = REPOERR_LOAD_SCENE_FAIL;
		}

		if (scene)
			delete scene;
		return nullptr;
	}

	return scene;
}
#endif