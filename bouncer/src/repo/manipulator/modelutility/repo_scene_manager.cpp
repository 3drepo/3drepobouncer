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
#include "../modelconvertor/export/repo_model_export_src.h"
#include "../modeloptimizer/repo_optimizer_multipart.h"
#include "../modelutility/repo_maker_selection_tree.h"

#ifdef REPO_ASSET_GENERATOR_SUPPORT
#include <submodules/asset_generator/src/repo_model_export_repobundle.h>
#endif

using namespace repo::manipulator::modelutility;

bool SceneManager::commitWebBuffers(
	repo::core::model::RepoScene                          *scene,
	const std::string                                     &geoStashExt,
	const repo_web_buffers_t                              &resultBuffers,
	repo::core::handler::AbstractDatabaseHandler          *handler,
	repo::core::handler::fileservice::FileManager         *fileManager)
{
	bool success = true;
	std::string jsonStashExt = REPO_COLLECTION_STASH_JSON;
	std::string repoAssetsStashExt = REPO_COLLECTION_STASH_BUNDLE;
	std::string databaseName = scene->getDatabaseName();
	std::string projectName = scene->getProjectName();

	//Upload the files
	for (const auto &bufferPair : resultBuffers.geoFiles)
	{
		if (success &= fileManager->uploadFileAndCommit(databaseName, projectName + "." + geoStashExt, bufferPair.first, bufferPair.second, {}, repo::core::handler::fileservice::FileManager::Encoding::Gzip)) // Web geometry files are gzipped by default
		{
			repoInfo << "File (" << bufferPair.first << ") added successfully to file storage.";
		}
		else
		{
			repoError << "Failed to add file  (" << bufferPair.first << ") to file storage";
		}
	}

	for (const auto &bufferPair : resultBuffers.jsonFiles)
	{
		std::string errMsg;
		std::string fileName = bufferPair.first;

		if (success &= fileManager->uploadFileAndCommit(databaseName, projectName + "." + jsonStashExt, bufferPair.first, bufferPair.second))
		{
			repoInfo << "File (" << fileName << ") added successfully to file storage.";
		}
		else
		{
			repoError << "Failed to add file  (" << fileName << ") to  file storage.";
		}
	}

	std::string errMsg;

	if (!resultBuffers.repoAssets.isEmpty())
	{
		handler->upsertDocument(databaseName, projectName + "." + repoAssetsStashExt, resultBuffers.repoAssets, true);
	}

	return success;
}

uint8_t SceneManager::commitScene(
	repo::core::model::RepoScene                          *scene,
	const std::string									  &owner,
	const std::string									  &tag,
	const std::string									  &desc,
	const repo::lib::RepoUUID              &revId,
	repo::core::handler::AbstractDatabaseHandler          *handler,
	repo::core::handler::fileservice::FileManager         *fileManager
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

			if (!isFederation && !(success = scene->hasRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED))) { // Make sure to check if we are looking at a federation before updating success with the state of the stash graph
				repoInfo << "Optimised scene not found. Attempt to generate...";

				if (!scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT).size())
				{
					// Scene is just a container - load all the actual meshes
					std::string msg;
					scene->loadScene(handler, msg);
				}

				success = generateStashGraph(scene);
			}

			if (success)
			{
				repoInfo << "Generating Selection Tree JSON...";
				scene->updateRevisionStatus(handler, repo::core::model::ModelRevisionNode::UploadStatus::GEN_SEL_TREE);
				if (generateAndCommitSelectionTree(scene, handler, fileManager))
					repoInfo << "Selection Tree Stored into the database";
				else
					repoError << "failed to commit selection tree";
			}

			if (success && !isFederation)
			{
				if (shouldGenerateSrcFiles(scene, handler))
				{
					repoInfo << "Generating SRC stashes...";
					scene->updateRevisionStatus(handler, repo::core::model::ModelRevisionNode::UploadStatus::GEN_WEB_STASH);
					repo_web_buffers_t buffers;
					if (success = generateWebViewBuffers(scene, repo::manipulator::modelconvertor::WebExportType::SRC, buffers, handler, fileManager))
						repoInfo << "SRC Stashes Stored into the database";
					else
						repoError << "failed to commit SRC";
				}
			}

			if (success && !isFederation)
			{
				repoInfo << "Generating Repo Bundles...";
				scene->updateRevisionStatus(handler, repo::core::model::ModelRevisionNode::UploadStatus::GEN_WEB_STASH);
				repo_web_buffers_t buffers;
				if (success = generateWebViewBuffers(scene, repo::manipulator::modelconvertor::WebExportType::REPO, buffers, handler, fileManager))
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
	repo::core::handler::AbstractDatabaseHandler *handler,
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

bool SceneManager::generateStashGraph(
	repo::core::model::RepoScene              *scene
)
{
	bool success = false;
	if (success = (scene && scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT)))
	{
		removeStashGraph(scene);

		repoInfo << "Generating stash graph...";

		repo::manipulator::modeloptimizer::MultipartOptimizer mpOpt;
		if (!(success = mpOpt.apply(scene)))
		{
			repoError << "Failed to generate stash graph with MultipartOptimizer.";
		}
	}
	else
	{
		repoError << "Failed to generate stash graph: nullptr to scene or empty scene graph!";
	}

	return success;
}

bool SceneManager::generateWebViewBuffers(
	repo::core::model::RepoScene                           *scene,
	const repo::manipulator::modelconvertor::WebExportType &exType,
	repo_web_buffers_t                                     &resultBuffers,
	repo::core::handler::AbstractDatabaseHandler           *handler,
	repo::core::handler::fileservice::FileManager         *fileManager)
{
	bool success = false;
	if (success = (scene&& scene->isRevisioned()))
	{
		bool toCommit = handler;

		std::string geoStashExt;
		std::string jsonStashExt = REPO_COLLECTION_STASH_JSON;

		switch (exType)
		{
		case repo::manipulator::modelconvertor::WebExportType::SRC:
			geoStashExt = REPO_COLLECTION_STASH_SRC;
			resultBuffers = generateSRCBuffer(scene);
			break;
		case repo::manipulator::modelconvertor::WebExportType::REPO:
			geoStashExt = REPO_COLLECTION_STASH_BUNDLE;
			resultBuffers = generateRepoBundleBuffer(scene);
			break;
		default:
			repoError << "Unknown export type with enum:  " << (uint16_t)exType;
			return false;
		}

		if (success = resultBuffers.geoFiles.size())
		{
			if (toCommit)
			{
				success = commitWebBuffers(scene, geoStashExt, resultBuffers, handler, fileManager);
			}
		}
		else
		{
			repoError << "Failed to generate web buffers: no geometry file generated";
		}
	}
	else
	{
		repoError << "Failed to generate web buffers: scene is empty or not revisioned!";
	}

	return success;
}

bool SceneManager::generateAndCommitSelectionTree(
	repo::core::model::RepoScene                           *scene,
	repo::core::handler::AbstractDatabaseHandler           *handler,
	repo::core::handler::fileservice::FileManager         *fileManager)
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

repo::repo_web_buffers_t SceneManager::generateSRCBuffer(
	repo::core::model::RepoScene *scene)
{
	repo_web_buffers_t result;
	repo::manipulator::modelconvertor::SRCModelExport srcExport(scene);
	if (srcExport.isOk())
	{
		repoTrace << "Conversion succeed.. exporting as buffer..";
		result = srcExport.getAllFilesExportedAsBuffer();
	}
	else
		repoError << "Export to SRC failed.";

	return result;
}

repo::repo_web_buffers_t SceneManager::generateRepoBundleBuffer(
	repo::core::model::RepoScene* scene)
{
	repo_web_buffers_t result;
#ifdef REPO_ASSET_GENERATOR_SUPPORT
	repo::manipulator::modelconvertor::RepoBundleExport bundleExport(scene);
	if (bundleExport.isOk()) {
		repoTrace << "Exporting Repo Bundles as buffer...";
		result = bundleExport.getAllFilesExportedAsBuffer();
	}
	else
	{
		repoError << "Export of Repo Bundles failed.";
	}
#else
	repoError << "Bouncer must be built with REPO_ASSET_GENERATOR_SUPPORT ON in order to generate Repo Bundles.";
#endif // REPO_ASSETGENERATOR

	return result;
}

bool isAddOnEnabled(repo::core::handler::AbstractDatabaseHandler *handler, const std::string &database, const std::string addOn) {

	auto teamspace = repo::core::model::RepoTeamspace(handler->findOneByUniqueID(database, "teamspace", database));
	return teamspace.isAddOnEnabled(addOn);
}

bool SceneManager::isVrEnabled(
	const repo::core::model::RepoScene                 *scene,
	repo::core::handler::AbstractDatabaseHandler *handler) const
{
	auto dbName = scene->getDatabaseName();
	return isAddOnEnabled(handler, dbName, REPO_USER_LABEL_VR_ENABLED);
}

bool SceneManager::shouldGenerateSrcFiles(
	const repo::core::model::RepoScene* scene,
	repo::core::handler::AbstractDatabaseHandler* handler) const
{
	//only generate SRC files if the user has the src flag enabled and there are meshes

	if (isAddOnEnabled(handler, scene->getDatabaseName(), REPO_USER_LABEL_SRC_ENABLED))
	{
		auto meshes = scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT);
		for (const repo::core::model::RepoNode* node : meshes)
		{
			auto mesh = dynamic_cast<const repo::core::model::MeshNode*>(node);
			if (!mesh)
			{
				continue;
			}

			if (mesh->getPrimitive() == repo::core::model::MeshNode::Primitive::TRIANGLES)
			{
				return true; // only need one triangle mesh to warrant generating srcs
			}
		}
	}

	return false;
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