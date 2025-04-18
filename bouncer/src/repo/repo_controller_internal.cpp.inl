/**
*  Copyright (C) 2015 3D Repo Ltd
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

#pragma once

#include "repo_controller.cpp.inl"
#include "error_codes.h"
#include "repo/core/model/bson/repo_bson.h"

#include "manipulator/modelconvertor/import/repo_model_import_assimp.h"

RepoController::_RepoControllerImpl::_RepoControllerImpl(
	std::vector<lib::RepoAbstractListener*> listeners,
	const uint32_t &numConcurrentOps,
	const uint32_t &numDbConn) :
	numDBConnections(numDbConn)
{
	for (uint32_t i = 0; i < numConcurrentOps; i++)
	{
		manipulator::RepoManipulator* worker = new manipulator::RepoManipulator();
		workerPool.push(worker);
	}

	if (listeners.size() > 0)
	{
		subscribeToLogger(listeners);
	}

	licenseValidator.activate();
}

RepoController::_RepoControllerImpl::~_RepoControllerImpl()
{
	std::vector<manipulator::RepoManipulator*> workers = workerPool.empty();
	std::vector<manipulator::RepoManipulator*>::iterator it;
	for (it = workers.begin(); it != workers.end(); ++it)
	{
		manipulator::RepoManipulator* man = *it;
		if (man)
			delete man;
	}
}

RepoController::RepoToken* RepoController::_RepoControllerImpl::init(
	std::string       &errMsg,
	const lib::RepoConfig  &config
)
{
	RepoToken *token = nullptr;
	if (config.validate()) {
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->init(errMsg, config, numDBConnections);
		token = new RepoController::RepoToken(config);
		auto dbConf = config.getDatabaseConfig();
		const std::string dbFullAd = dbConf.connString.empty() ? dbConf.addr + ":" + std::to_string(dbConf.port) : dbConf.connString;
		repoInfo << "Successfully connected to the " << dbFullAd;
		if (!dbConf.username.empty())
			repoInfo << dbConf.username << " is authenticated to " << dbFullAd;
		workerPool.push(worker);
	}
	else {
		errMsg = "Invalid configuration.";
	}

	return token;
}

uint8_t RepoController::_RepoControllerImpl::commitScene(
	const RepoController::RepoToken                     *token,
	repo::core::model::RepoScene        *scene,
	const std::string                   &owner,
	const std::string                      &tag,
	const std::string                      &desc,
	const repo::lib::RepoUUID           &revId)
{
	uint8_t errCode = REPOERR_UNKNOWN_ERR;
	if (scene)
	{
		if (!(scene->getDatabaseName().empty() || scene->getProjectName().empty()))
		{
			if (token)
			{
				manipulator::RepoManipulator* worker = workerPool.pop();
				errCode = worker->commitScene(
					token->getDatabaseUsername(),
					scene,
					owner.empty() ? "ANONYMOUS USER" : owner,
					tag,
					desc,
					revId);
				workerPool.push(worker);
			}
			else
			{
				repoError << "Trying to commit to the database without a database connection!";
			}
		}
		else
		{
			repoError << "Trying to commit a scene without specifying database/project names.";
		}
	}
	else
	{
		repoError << "Trying to commit an empty scene into the database";
	}
	return errCode;
}

repo::core::model::RepoScene* RepoController::_RepoControllerImpl::fetchScene(
	const RepoController::RepoToken      *token,
	const std::string    &database,
	const std::string    &collection,
	const std::string    &uuid,
	const bool           &headRevision,
	const bool           &ignoreRefScene,
	const bool           &skeletonFetch,
	const std::vector<repo::core::model::ModelRevisionNode::UploadStatus> &includeStatus)
{
	repo::core::model::RepoScene* scene = 0;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		scene = worker->fetchScene(
			database, collection, repo::lib::RepoUUID(uuid), headRevision, ignoreRefScene, skeletonFetch, includeStatus);

		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch scene without a database connection!";
	}

	return scene;
}

bool RepoController::_RepoControllerImpl::generateAndCommitSelectionTree(
	const RepoController::RepoToken         *token,
	repo::core::model::RepoScene            *scene)
{
	bool success = false;

	if (token && scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		if (scene->isRevisioned() && !scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT))
		{
			repoInfo << "Unoptimised scene not loaded, trying loading unoptimised scene...";
			worker->fetchScene( scene);
		}

		success = worker->generateAndCommitSelectionTree(scene);
		workerPool.push(worker);
	}

	return success;
}

std::vector < repo::core::model::RepoBSON >
RepoController::_RepoControllerImpl::getAllFromCollectionContinuous(
	const RepoController::RepoToken      *token,
	const std::string    &database,
	const std::string    &collection,
	const uint64_t       &skip,
	const uint32_t       &limit)
{
	repoTrace << "Controller: Fetching BSONs from "
		<< database << "." << collection << "....";
	std::vector<repo::core::model::RepoBSON> vector;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		vector = worker->getAllFromCollectionTailable(database, collection, skip, limit);

		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch data from a collection without a database connection!";
	}

	repoTrace << "Obtained " << vector.size() << " bson objects.";

	return vector;
}

std::vector < repo::core::model::RepoBSON >
RepoController::_RepoControllerImpl::getAllFromCollectionContinuous(
	const RepoController::RepoToken              *token,
	const std::string            &database,
	const std::string            &collection,
	const std::list<std::string> &fields,
	const std::string            &sortField,
	const int                    &sortOrder,
	const uint64_t               &skip,
	const uint32_t               &limit)
{
	repoTrace << "Controller: Fetching BSONs from "
		<< database << "." << collection << "....";
	std::vector<repo::core::model::RepoBSON> vector;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		vector = worker->getAllFromCollectionTailable(
			database, collection, fields, sortField, sortOrder, skip, limit);

		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch data from a collection without a database connection!";
	}

	repoTrace << "Obtained " << vector.size() << " bson objects.";

	return vector;
}

void RepoController::_RepoControllerImpl::setLoggingLevel(const repo::lib::RepoLog::RepoLogLevel &level)
{
	repo::lib::RepoLog::getInstance().setLoggingLevel(level);
}

void RepoController::_RepoControllerImpl::logToFile(const std::string &filePath)
{
	repo::lib::RepoLog::getInstance().logToFile(filePath);
}

void RepoController::_RepoControllerImpl::subscribeToLogger(
	std::vector<lib::RepoAbstractListener*> listeners)
{
	repo::lib::RepoLog::getInstance().subscribeListeners(listeners);
}

repo::core::model::RepoScene* RepoController::_RepoControllerImpl::createFederatedScene(
	const std::map<repo::core::model::ReferenceNode, std::string> &fedMap)
{
	repo::core::model::RepoScene* scene = nullptr;
	if (fedMap.size() > 0)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		scene = worker->createFederatedScene(fedMap);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to federate a new scene graph with no references!";
	}

	return scene;
}

bool RepoController::_RepoControllerImpl::generateAndCommitRepoBundlesBuffer(
	const RepoController::RepoToken* token,
	repo::core::model::RepoScene* scene)
{
	bool success;
	if (success = token && scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->generateAndCommitRepoBundlesBuffer(scene);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Failed to generate RepoBundles Buffer.";
	}
	return success;
}

std::shared_ptr<repo::lib::repo_partitioning_tree_t>
RepoController::_RepoControllerImpl::getScenePartitioning(
	const repo::core::model::RepoScene *scene,
	const uint32_t                     &maxDepth
)
{
	std::shared_ptr<repo::lib::repo_partitioning_tree_t> partition(nullptr);

	if (scene && scene->getRoot(scene->getViewGraph()))
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		partition = worker->getScenePartitioning(scene, maxDepth);
		workerPool.push(worker);
	}
	else
		repoError << "Trying to partition an empty scene!";

	return partition;
}

std::string RepoController::_RepoControllerImpl::getSupportedImportFormats()
{
	//This needs to be updated if we support more than assimp
	return repo::manipulator::modelconvertor::AssimpModelImport::getSupportedFormats();
}

repo::core::model::RepoNodeSet RepoController::_RepoControllerImpl::loadMetadataFromFile(
	const std::string &filePath,
	const char        &delimiter)
{
	repo::core::model::RepoNodeSet metadata;

	if (!filePath.empty())
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		metadata = worker->loadMetadataFromFile(filePath, delimiter);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to load from an empty file path!";
	}

	return metadata;
}

bool RepoController::_RepoControllerImpl::isVREnabled(const RepoToken *token,
	const repo::core::model::RepoScene *scene)
{
	bool result = false;
	if (scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		result = worker->isVREnabled(scene);
		workerPool.push(worker);
	}
	else {
		repoError << "RepoController::_RepoControllerImpl::isVREnabled: NULL pointer to scene!";
	}
	return result;
}

repo::core::model::RepoScene*
RepoController::_RepoControllerImpl::loadSceneFromFile(
	const std::string                                          &filePath,
	uint8_t													 &err,
	const repo::manipulator::modelconvertor::ModelImportConfig &config)
{
	repo::core::model::RepoScene *scene = nullptr;

	if (!filePath.empty())
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		scene = worker->loadSceneFromFile(filePath, err, config);
		workerPool.push(worker);
		if (!scene)
			repoError << "Failed to load scene from file - error code: " << std::to_string(err);
	}
	else
	{
		repoError << "Trying to load from an empty file path!";
	}

	return scene;
}

void
RepoController::_RepoControllerImpl::processDrawingRevision(
	const RepoController::RepoToken* token,
	const std::string& teamspace,
	const repo::lib::RepoUUID revision,
	uint8_t& err,
	const std::string &imagePath)
{
	manipulator::RepoManipulator* worker = workerPool.pop();
	worker->processDrawingRevision(teamspace, revision, err, imagePath);
	workerPool.push(worker);
}

void RepoController::_RepoControllerImpl::updateRevisionStatus(
	repo::core::model::RepoScene* scene,
	const repo::core::model::ModelRevisionNode::UploadStatus& status)
{
	manipulator::RepoManipulator* worker = workerPool.pop();
	worker->updateRevisionStatus(scene, status);
	workerPool.push(worker);
}

std::string RepoController::_RepoControllerImpl::getVersion()
{
	std::stringstream ss;
	ss << BOUNCER_VMAJOR << "." << BOUNCER_VMINOR;
	return ss.str();
}