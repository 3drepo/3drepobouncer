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

void SceneManager::commitScene(
	repo::core::model::RepoScene* scene,
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
	const repo::manipulator::modelutility::WebBufferConfig& config
) {
	try {
		scene->commit(handler);
		repoInfo << "Scene successfully committed to the database";

		repoInfo << "Generating Selection Tree JSON...";
		scene->updateRevisionStatus(handler.get(), repo::core::model::ModelRevisionNode::UploadStatus::GEN_SEL_TREE);
		generateAndCommitSelectionTree(scene, handler.get());

		repoInfo << "Generating Repo Bundles...";
		scene->updateRevisionStatus(handler.get(), repo::core::model::ModelRevisionNode::UploadStatus::GEN_WEB_STASH);
		generateWebViewBuffers(scene, repo::manipulator::modelconvertor::ExportType::REPO, handler.get(), config);

		scene->updateRevisionStatus(handler.get(), repo::core::model::ModelRevisionNode::UploadStatus::COMPLETE);
	}
	catch(...)
	{
		scene->addErrorStatusToProjectSettings(handler.get());
		std::rethrow_exception(std::current_exception());
	}
}

repo::core::model::RepoScene* SceneManager::fetchScene(
	repo::core::handler::AbstractDatabaseHandler  *handler,
	const std::string                             &database,
	const std::string                             &project,
	const repo::lib::RepoUUID                     &uuid,
	const bool                                    &headRevision,
	const bool                                    &skeletonFetch,
	const std::vector<repo::core::model::ModelRevisionNode::UploadStatus> &includeStatus)
{
	if (!handler) {
		throw repo::lib::RepoException("Failed to retrieve database handler to perform the operation");
	}

	auto scene = new repo::core::model::RepoScene();

	if (headRevision) {
		repoInfo << "Fetching head " << uuid.toString() << " of " << database << "." << project;

		using namespace repo::core::handler::database;

		query::RepoQueryBuilder query;
		if (includeStatus.size())
		{
			std::vector<int> statuses;
			std::transform(includeStatus.begin(), includeStatus.end(), std::back_inserter(statuses), [](auto s) { return (int)s; });
			query.append(
				query::Or(
					query::Eq(REPO_NODE_REVISION_LABEL_INCOMPLETE, statuses),
					query::Exists(REPO_NODE_REVISION_LABEL_INCOMPLETE, false)
				)
			);
		}
		else
		{
			query.append(
				query::Exists(REPO_NODE_REVISION_LABEL_INCOMPLETE, false)
			);
		}
		query.append(query::Eq(REPO_NODE_LABEL_SHARED_ID, uuid));

		auto bson = handler->findOneByCriteria(
			database,
			project + "." + REPO_COLLECTION_HISTORY,
			query,
			REPO_NODE_REVISION_LABEL_TIMESTAMP
		);

		if (bson.isEmpty()) {
			throw repo::lib::RepoException("Failed to retrieve head revision for " + database + "." + project + " with branch id " + uuid.toString());
		}

		scene->setRevision(database, project, bson.getUUIDField(REPO_NODE_LABEL_ID));
	}
	else {
		repoInfo << "Fetching revision " << uuid.toString() << " of " << database << "." << project;
		scene->setRevision(database, project, uuid);
	}

	if (skeletonFetch) {
		scene->skipLoadingExtFiles();
	}

	scene->loadScene(handler);

	return scene;
}

void SceneManager::fetchScene(
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::core::model::RepoScene              *scene)
{
	if (!handler || !scene || !scene->isRevisioned()) {
		throw repo::lib::RepoException("Failed to retrieve database handler or scene is empty or not revisioned");
	}
	scene->loadScene(handler);
}

bool SceneManager::generateWebViewBuffers(
	repo::core::model::RepoScene *scene,
	const repo::manipulator::modelconvertor::ExportType &exType,
	repo::core::handler::AbstractDatabaseHandler *handler,
	const repo::manipulator::modelutility::WebBufferConfig& config
)
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
				handler,
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

		repo::manipulator::modeloptimizer::MultipartOptimizer mpOpt(handler, exporter.get(), config.splitByFloor);
		mpOpt.processScene(
			scene->getDatabaseName(),
			scene->getProjectName(),
			scene->getRevisionID()
		);

		return true;
	}
	else
	{
		repoError << "Failed to generate web buffers: scene is empty or not revisioned!";
	}

	return false;
}

bool SceneManager::generateAndCommitSelectionTree(
	repo::core::model::RepoScene					*scene,
	repo::core::handler::AbstractDatabaseHandler	*handler)
{
	bool success = false;
	if (success = scene && scene->isRevisioned() && handler)
	{
		SelectionTreeMaker treeMaker(scene, handler);
		auto buffer = treeMaker.getSelectionTreeAsBuffer();

		if (success = buffer.size())
		{
			std::string databaseName = scene->getDatabaseName();
			std::string projectName = scene->getProjectName();
			std::string errMsg;
			std::string fileNamePrefix = scene->getRevisionID().toString() + "/";

			for (const auto & file : buffer)
			{
				std::string fileName = fileNamePrefix + file.first;

				if (handler && handler->getFileManager()->uploadFileAndCommit(
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