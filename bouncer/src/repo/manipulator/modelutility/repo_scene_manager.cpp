/**
*  Copyright (C) 2016 3D Repo Ltd
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
#include "repo_scene_manager.h"
#include "repo/core/model/bson/repo_bson_builder.h"
#include "repo/core/model/bson/repo_bson_ref.h"
#include "repo/core/model/bson/repo_bson_teamspace.h"
#include "../../error_codes.h"
#include "../modeloptimizer/repo_optimizer_multipart.h"
#include "../modelutility/repo_maker_selection_tree.h"

#ifdef REPO_ASSET_GENERATOR_SUPPORT
#include <submodules/asset_generator/src/repo_model_export_repobundle.h>
#endif

using namespace repo::lib;
using namespace repo::manipulator::modelutility;


uint8_t SceneManager::commitScene(
	repo::core::model::RepoScene									*scene,
	const std::string												&owner,
	const std::string												&tag,
	const std::string												&desc,
	const repo::lib::RepoUUID										&revId,
	repo::core::handler::AbstractDatabaseHandler					*handler,
	repo::core::handler::fileservice::FileManager					*fileManager
) {
	uint8_t errCode = REPOERR_UPLOAD_FAILED;
	std::string msg;
	if (handler && scene)
	{
		errCode = scene->commit(handler, fileManager, msg, owner, desc, tag, revId);
		if (errCode == REPOERR_OK) {
			repoInfo << "Scene successfully committed to the database";
			bool success = true;
			bool isFederation = scene->getAllReferences(repo::core::model::RepoScene::GraphType::DEFAULT).size();

			if (success)
			{
				// Loading the full scene is required for the tree generation, for now.
				if (!isFederation && !scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT).size())
				{
					// Scene is just a container - load all the actual meshes
					std::string msg;
					scene->loadScene(handler, msg);
				}

				repoInfo << "Generating Selection Tree JSON...";
				scene->updateRevisionStatus(handler, repo::core::model::ModelRevisionNode::UploadStatus::GEN_SEL_TREE);
				if (generateAndCommitSelectionTree(scene, handler, fileManager))
					repoInfo << "Selection Tree Stored into the database";
				else
					repoError << "failed to commit selection tree";
			}

			if (success && !isFederation)
			{
				repoInfo << "Generating Repo Bundles...";
				scene->updateRevisionStatus(handler, repo::core::model::ModelRevisionNode::UploadStatus::GEN_WEB_STASH);
				if (success = generateWebViewBuffers(scene, repo::manipulator::modelconvertor::ExportType::REPO, handler))
					repoInfo << "Repo Bundles for Stash stored into the database";
				else
					repoError << "failed to commit Repo Bundles";
			}

			if (success) {
				errCode = REPOERR_OK;
				scene->updateRevisionStatus(handler, repo::core::model::ModelRevisionNode::UploadStatus::COMPLETE);
			}
			else {
				errCode = REPOERR_UPLOAD_FAILED;
			}
		}
	}
	else
	{
		if (!handler)
			msg += "Failed to connect to database";
		if (!scene)
			msg += "Trying to commit a scene that does not exist!";
		repoError << "Error committing scene to the database : " << msg;
		errCode = REPOERR_UPLOAD_FAILED;
	}

	if (errCode != REPOERR_OK)
	{
		scene->addErrorStatusToProjectSettings(handler);
	}

	return errCode;
}

repo::core::model::RepoScene* SceneManager::fetchScene(
	repo::core::handler::AbstractDatabaseHandler  *handler,
	const std::string                             &database,
	const std::string                             &project,
	const repo::lib::RepoUUID                     &uuid,
	const bool                                    &headRevision,
	const bool                                    &ignoreRefScenes,
	const bool                                    &skeletonFetch,
	const std::vector<repo::core::model::ModelRevisionNode::UploadStatus> &includeStatus)
{
	repo::core::model::RepoScene* scene = nullptr;
	if (handler)
	{
		//not setting a scene if we don't have a handler since we
		//can't retreive anything from the database.
		scene = new repo::core::model::RepoScene(database, project);
		if (scene)
		{
			if (skeletonFetch)
				scene->skipLoadingExtFiles();
			if (ignoreRefScenes)
				scene->ignoreReferenceScene();
			if (headRevision)
				scene->setBranch(uuid);
			else
				scene->setRevision(uuid);

			std::string errMsg;
			if (scene->loadRevision(handler, errMsg, includeStatus))
			{
				repoInfo << "Loaded" <<
					(headRevision ? (" head revision of branch " + uuid.toString())
						: (" revision " + uuid.toString()))
					<< " of " << database << "." << project;

				if (scene->loadScene(handler, errMsg))
				{
					repoTrace << "Loaded Scene";
				}
				else
				{
					delete scene;
					scene = nullptr;
				}
			}
			else
			{
				repoError << "Failed to load revision for "
					<< database << "." << project << " : " << errMsg;
				delete scene;
				scene = nullptr;
			}
		}
		else {
			repoError << "Failed to create a RepoScene(out of memory?)!";
		}
	}

	return scene;
}

void SceneManager::fetchScene(
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::core::model::RepoScene              *scene)
{
	if (scene)
	{
		if (scene->isRevisioned())
		{
			if (!handler)
			{
				repoError << "Failed to retrieve database handler to perform the operation!";
			}

			std::string errMsg;
			if (!scene->hasRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED) && scene->loadStash(handler, errMsg))
			{
				repoTrace << "Stash Loaded";
			}
			else
			{
				if (!errMsg.empty())
					repoWarning << "Error loading stash for " << scene->getDatabaseName() << "." << scene->getProjectName() << " : " << errMsg;
			}

			errMsg.clear();

			if (!scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT) && scene->loadScene(handler, errMsg))
			{
				repoTrace << "Scene Loaded";
			}
			else
			{
				if (!errMsg.empty())
					repoError << "Error loading scene: " << errMsg;
			}
		}
	}
	else
	{
		repoError << "Cannot populate a scene that doesn't exist. Use the other function if you wish to fully load a scene from scratch";
	}
}

bool SceneManager::generateWebViewBuffers(
	repo::core::model::RepoScene									*scene,
	const repo::manipulator::modelconvertor::ExportType				&exType,
	repo::core::handler::AbstractDatabaseHandler					*handler)
{
	bool validScene =
		scene
		&& scene->isRevisioned()
		&& scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT);
		
	if (validScene)
	{
		auto database = scene->getDatabaseName();
		auto collection = scene->getProjectName();
		auto revId = scene->getRevisionID();

		// Get world offset
		std::vector<double> worldOffset;
		auto bson = handler->findOneByUniqueID(database, collection + "." + REPO_COLLECTION_HISTORY, revId);
		if (bson.hasField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT))
		{
			worldOffset = bson.getDoubleVectorField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT);
		}
		else {
			worldOffset = std::vector<double>({ 0, 0, 0 });
		}

		// Initialise exporter		
		std::unique_ptr<repo::manipulator::modelconvertor::AbstractModelExport> exporter = nullptr;

		switch (exType)
		{
		case repo::manipulator::modelconvertor::ExportType::REPO:
#ifdef REPO_ASSET_GENERATOR_SUPPORT
			exporter = std::make_unique<repo::manipulator::modelconvertor::RepoBundleExport>(
				database,
				collection,
				revId,
				worldOffset
			);
#else
			repoError << "Bouncer must be built with REPO_ASSET_GENERATOR_SUPPORT ON in order to generate Repo Bundles.";
			return false;
#endif // REPO_ASSETGENERATOR
			break;
		default:
			repoError << "Unknown export type with enum:  " << (uint16_t)exType;
			return false;
		}


		repo::manipulator::modeloptimizer::MultipartOptimizer mpOpt;
		return mpOpt.processScene(
			scene->getDatabaseName(),
			scene->getProjectName(),
			scene->getRevisionID(),
			handler,
			exporter.get() 
		);
	}
	else
	{
		repoError << "Failed to generate web buffers: scene is empty or not revisioned!";
	}

	return false;
}

bool SceneManager::generateAndCommitSelectionTree(
	repo::core::model::RepoScene					*scene,
	repo::core::handler::AbstractDatabaseHandler	*handler,
	repo::core::handler::fileservice::FileManager	*fileManager)
{
	bool success = false;
	if (success = scene && scene->isRevisioned() && handler)
	{
		SelectionTreeMaker treeMaker(scene);
		auto buffer = treeMaker.getSelectionTreeAsBuffer();

		if (success = buffer.size())
		{
			std::string databaseName = scene->getDatabaseName();
			std::string projectName = scene->getProjectName();
			std::string errMsg;
			std::string fileNamePrefix = "/" + databaseName + "/" + projectName + "/revision/"
				+ scene->getRevisionID().toString() + "/";

			for (const auto & file : buffer)
			{
				std::string fileName = fileNamePrefix + file.first;

				if (handler && fileManager->uploadFileAndCommit(
					databaseName,
					projectName + "." + REPO_COLLECTION_STASH_JSON,
					fileName,
					file.second))
				{
					repoInfo << "File (" << fileName << ") added successfully to file storage.";
				}
				else
				{
					repoError << "Failed to add file  (" << fileName << ") to file storage: " << errMsg;
					success = false;
					break;
				}
			}
		}
		else
		{
			repoError << "Failed to generate selection tree: JSON file buffer is empty!";
		}
	}
	return success;
}

bool isAddOnEnabled(repo::core::handler::AbstractDatabaseHandler *handler, const std::string &database, const std::string addOn) {

	auto teamspace = repo::core::model::RepoTeamspace(handler->findOneByUniqueID(database, "teamspace", database));
	return teamspace.isAddOnEnabled(addOn);
}

bool SceneManager::isVrEnabled(
	const repo::core::model::RepoScene           *scene,
	repo::core::handler::AbstractDatabaseHandler *handler) const
{
	auto dbName = scene->getDatabaseName();
	return isAddOnEnabled(handler, dbName, REPO_USER_LABEL_VR_ENABLED);
}

bool SceneManager::removeStashGraph(repo::core::model::RepoScene *scene)
{
	if (scene)
	{
		scene->clearStash();
		return true;
	}
	else
	{
		repoError << "Trying to remove a stashGraph from scene but the scene is empty!";
		return false;
	}
}