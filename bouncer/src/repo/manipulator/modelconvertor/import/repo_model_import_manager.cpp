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

#include "repo_model_import_manager.h"
#include "repo/lib/repo_utils.h"
#include "repo/error_codes.h"
#include "repo_model_import_assimp.h"
#include "repo_model_import_ifc.h"
#include "repo_model_import_3drepo.h"
#include "repo_model_import_oda.h"
#include "repo_model_import_synchro.h"
#include "repo_model_units.h"
#include <boost/filesystem.hpp>
#include <repo/core/handler/repo_database_handler_abstract.h>
#include <repo/core/handler/database/repo_query.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_node.h>

using namespace repo::manipulator::modelconvertor;

repo::core::model::RepoScene* ModelImportManager::ImportFromFile(
	const std::string &file,
	const repo::manipulator::modelconvertor::ModelImportConfig &config,
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
	uint8_t &error
) const {
	if (!repo::lib::doesFileExist(file)) {
		error = REPOERR_MODEL_FILE_READ;
		repoError << "Cannot find file: " << file;
		return nullptr;
	}
	auto modelConvertor = chooseModelConvertor(file, config);
	if (!modelConvertor) {
		error = REPOERR_FILE_TYPE_NOT_SUPPORTED;
		repoError << "Cannot find file: " << file;
		return nullptr;
	}

	repoTrace << "Importing model and generating Repo Scene";
	repo::core::model::RepoScene* scene = modelConvertor->importModel(file, handler, error);
	if (scene) {

		scene->setDatabaseAndProjectName(config.getDatabaseName(), config.getProjectName());

		auto fileUnits = modelConvertor->getUnits();
		auto targetUnits = config.getTargetUnits();

		auto unitsScale = determineScaleFactor(fileUnits, targetUnits);
		repoInfo << "File units: " << toUnitsString(fileUnits) << "\tTarget units: " << toUnitsString(targetUnits);

		if (unitsScale != 1.0) {
			repoInfo << "Applying scaling factor of " << unitsScale << " to convert " << toUnitsString(fileUnits) << " to " << toUnitsString(targetUnits);
			scene->applyScaleFactor(unitsScale);
		}

		if (!scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT)) {
			delete scene;
			scene = nullptr;
			error = REPOERR_NO_MESHES;
		}
		else {
			if (modelConvertor->requireReorientation()) {
				repoTrace << "rotating model by 270 degress on the x axis...";
				scene->reorientateDirectXModel();
			}

			error = REPOERR_OK;
		}

		// Apply the in-leaf node metadata shim, on the scene directly or the db,
		// depending on whether this was a streamed import or not.
		if (scene) {
			repoInfo << "Connecting metadata within leaf nodes...";
			if (scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT).size()) {
				connectMetadataNodes(scene);
			}
			else {
				connectMetadataNodes(scene, handler);
			}
		}
	}

	return scene;
}

std::shared_ptr<AbstractModelImport> ModelImportManager::chooseModelConvertor(
	const std::string &file,
	const repo::manipulator::modelconvertor::ModelImportConfig &config
) const {
	std::string fileExt = repo::lib::getExtension(file);
	repo::lib::toUpper(fileExt);

	std::shared_ptr<AbstractModelImport> modelConvertor = nullptr;

	//NOTE: IFC and ODA checks needs to be done before assimp otherwise assimp will try to import IFC/DXF
	if (fileExt == ".IFC")
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::IFCModelImport(config));
	else if (repo::manipulator::modelconvertor::OdaModelImport::isSupportedExts(fileExt))
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::OdaModelImport(config));
	else if (repo::manipulator::modelconvertor::AssimpModelImport::isSupportedExts(fileExt))
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::AssimpModelImport(config));
	else if (fileExt == ".BIM")
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::RepoModelImport(config));
	else if (fileExt == ".SPM")
		modelConvertor = std::shared_ptr<AbstractModelImport>(new repo::manipulator::modelconvertor::SynchroModelImport(config));

	return modelConvertor;
}

void ModelImportManager::connectMetadataNodes(repo::core::model::RepoScene* scene) const
{
	for (auto& metadata : scene->getAllMetadata(repo::core::model::RepoScene::GraphType::DEFAULT)) {
		for (auto& parent : scene->getParentNodesFiltered(repo::core::model::RepoScene::GraphType::DEFAULT, metadata, repo::core::model::NodeType::TRANSFORMATION)) {

			if (scene->getChildrenNodesFiltered(repo::core::model::RepoScene::GraphType::DEFAULT,
				parent->getSharedID(),
				repo::core::model::NodeType::TRANSFORMATION).size())
			{
				continue;
			}

			bool isLeafNode = true;

			repo::core::model::RepoNodeSet meshNodes;

			for (auto& mesh : scene->getChildrenNodesFiltered(repo::core::model::RepoScene::GraphType::DEFAULT,
				parent->getSharedID(),
				repo::core::model::NodeType::MESH))
			{
				if (mesh->getName().size()) {
					isLeafNode = false;
					break;
				}

				meshNodes.insert(mesh);
			}

			if (!isLeafNode) {
				continue;
			}

			scene->addInheritance(repo::core::model::RepoScene::GraphType::DEFAULT,
				meshNodes,
				metadata);
		}
	}
}

void ModelImportManager::connectMetadataNodes(repo::core::model::RepoScene* scene,
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler) const
{
	// This is a shim that parents metadata nodes to any hidden meshes of leaf
	// nodes for an already committed scene.

	using namespace repo::core::handler::database;

	query::RepoQueryBuilder filter;
	filter.append(query::Eq(REPO_NODE_REVISION_ID, scene->getRevisionID()));
	filter.append(query::Eq(REPO_NODE_LABEL_TYPE, {
		std::string(REPO_NODE_TYPE_TRANSFORMATION),
		std::string(REPO_NODE_TYPE_MESH),
		std::string(REPO_NODE_TYPE_METADATA)
	}));

	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.includeField(REPO_NODE_LABEL_ID);
	projection.includeField(REPO_NODE_LABEL_SHARED_ID);
	projection.includeField(REPO_NODE_LABEL_PARENTS);
	projection.includeField(REPO_NODE_LABEL_TYPE);
	projection.includeField(REPO_NODE_LABEL_NAME);

	struct TransformationNode {
		std::set<repo::lib::RepoUUID> meshSharedIds;
		std::set<repo::lib::RepoUUID> metadataUniqueIds;

		bool isLeafNode;

		TransformationNode()
			:isLeafNode(true)
		{
		}
	};

	// Indexed by shared id, as is used to determine relationships within the db
	std::unordered_map<repo::lib::RepoUUID, TransformationNode, repo::lib::RepoUUIDHasher> nodes;

	auto cursor = handler->findCursorByCriteria(scene->getDatabaseName(), scene->getProjectName() + ".scene", filter, projection);
	for (auto bson : *cursor)
	{
		auto node = repo::core::model::RepoNode(bson);

		auto type = bson.getStringField(REPO_NODE_LABEL_TYPE);
		if (type == REPO_NODE_TYPE_TRANSFORMATION) {
			for (auto& p : node.getParentIDs()) {
				auto& n = nodes[p];
				n.isLeafNode = false; // Nodes that have transformations as children cannot be leaf nodes
			}
		}
		else if (type == REPO_NODE_TYPE_MESH) {
			for (auto& p : node.getParentIDs()) {
				auto& n = nodes[p];
				if (node.getName().size()) {
					n.isLeafNode = false; // Nodes that have named meshes as children cannot be leaf nodes
				}
				else
				{
					n.meshSharedIds.insert(node.getSharedID());
				}
			}
		}
		else if (type == REPO_NODE_TYPE_METADATA) {
			for (auto& p : node.getParentIDs()) {
				auto& n = nodes[p];
				n.metadataUniqueIds.insert(node.getUniqueID());
			}
		}
	}

	// The nodes map now has everything required to work out which metadata
	// nodes need additional parents.

	auto context = handler->getBulkWriteContext(scene->getDatabaseName(), scene->getProjectName() + ".scene");
	for (auto& p : nodes) {
		auto& t = p.second;
		if (t.isLeafNode) {
			for (auto& m : t.metadataUniqueIds) {
				context->updateDocument(query::AddParent(m, t.meshSharedIds));
			}
		}
	}
}
