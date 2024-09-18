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

#include "manipulator/modelconvertor/import/repo_model_import_assimp.h"
#include "manipulator/modelconvertor/export/repo_model_export_assimp.h"

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

	repo::core::handler::MongoDatabaseHandler::disconnectHandler();
}

RepoController::RepoToken* RepoController::_RepoControllerImpl::init(
	std::string       &errMsg,
	const lib::RepoConfig  &config
)
{
	RepoToken *token = nullptr;
	if (config.validate()) {
		manipulator::RepoManipulator* worker = workerPool.pop();

		auto dbConf = config.getDatabaseConfig();

		//FIXME : this should just use the dbConf struct...
		const bool success = worker->init(errMsg, config, numDBConnections);

		if (success)
		{
			const std::string dbFullAd = dbConf.connString.empty() ? dbConf.addr + ":" + std::to_string(dbConf.port) : dbConf.connString;
			token = new RepoController::RepoToken(config);
			repoInfo << "Successfully connected to the " << dbFullAd;
			if (!dbConf.username.empty())
				repoInfo << dbConf.username << " is authenticated to " << dbFullAd;
		}

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
				errCode = worker->commitScene(token->databaseAd,
					token->getCredentials(),
					token->bucketName,
					token->bucketRegion,
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

void RepoController::_RepoControllerImpl::disconnectFromDatabase(const RepoController::RepoToken* token)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		worker->disconnectFromDatabase(token->databaseAd);

		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to disconnect from database with an invalid token!";
	}
}

repo::core::model::RepoScene* RepoController::_RepoControllerImpl::fetchScene(
	const RepoController::RepoToken      *token,
	const std::string    &database,
	const std::string    &collection,
	const std::string    &uuid,
	const bool           &headRevision,
	const bool           &ignoreRefScene,
	const bool           &skeletonFetch,
	const std::vector<repo::core::model::RevisionNode::UploadStatus> &includeStatus)
{
	repo::core::model::RepoScene* scene = 0;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		scene = worker->fetchScene(token->databaseAd, token->getCredentials(),
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
			worker->fetchScene(token->databaseAd, token->getCredentials(), scene);
		}

		success = worker->generateAndCommitSelectionTree(token->databaseAd,
			token->getCredentials(),
			token->bucketName,
			token->bucketRegion,
			scene);
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

		vector = worker->getAllFromCollectionTailable(token->databaseAd, token->getCredentials(),
			database, collection, skip, limit);

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
		vector = worker->getAllFromCollectionTailable(token->databaseAd, token->getCredentials(),
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

std::list<std::string>  RepoController::_RepoControllerImpl::getCollections(
	const RepoController::RepoToken       *token,
	const std::string     &databaseName
)
{
	std::list<std::string> list;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		list = worker->fetchCollections(token->databaseAd, token->getCredentials(), databaseName);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch collections without a database connection!";
	}

	return list;
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
		success = worker->generateAndCommitRepoBundlesBuffer(token->databaseAd,
			token->getCredentials(),
			token->bucketName,
			token->bucketRegion,
			scene);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Failed to generate RepoBundles Buffer.";
	}
	return success;
}

bool RepoController::_RepoControllerImpl::generateAndCommitSRCBuffer(
	const RepoController::RepoToken *token,
	repo::core::model::RepoScene    *scene)
{
	bool success;
	if (success = token && scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->generateAndCommitSRCBuffer(token->databaseAd,
			token->getCredentials(),
			token->bucketName,
			token->bucketRegion,
			scene);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Failed to generate SRC Buffer.";
	}
	return success;
}

repo_web_buffers_t RepoController::_RepoControllerImpl::generateSRCBuffer(
	repo::core::model::RepoScene *scene)
{
	repo_web_buffers_t buffer;
	if (scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		buffer = worker->generateSRCBuffer(scene);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Failed to generate SRC Buffer.";
	}
	return buffer;
}

std::shared_ptr<repo_partitioning_tree_t>
RepoController::_RepoControllerImpl::getScenePartitioning(
	const repo::core::model::RepoScene *scene,
	const uint32_t                     &maxDepth
)
{
	std::shared_ptr<repo_partitioning_tree_t> partition(nullptr);

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

std::string RepoController::_RepoControllerImpl::getSupportedExportFormats()
{
	//This needs to be updated if we support more than assimp
	return repo::manipulator::modelconvertor::AssimpModelExport::getSupportedFormats();
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

		result = worker->isVREnabled(token->databaseAd, token->getCredentials(), scene);
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
	worker->processDrawingRevision(token->databaseAd, teamspace, revision, err, imagePath);
	workerPool.push(worker);
}

bool RepoController::_RepoControllerImpl::saveOriginalFiles(
	const RepoController::RepoToken                    *token,
	const repo::core::model::RepoScene *scene,
	const std::string                   &directory)
{
	bool success = false;
	if (scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		success = worker->saveOriginalFiles(token->databaseAd, token->getCredentials(), scene, directory);
		workerPool.push(worker);
	}
	else {
		repoError << "RepoController::_RepoControllerImpl::saveSceneToFile: NULL pointer to scene!";
	}
	return success;
}

bool RepoController::_RepoControllerImpl::saveOriginalFiles(
	const RepoController::RepoToken                    *token,
	const std::string                   &database,
	const std::string                   &project,
	const std::string                   &directory)
{
	bool success = false;
	if (!(database.empty() || project.empty()))
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		success = worker->saveOriginalFiles(token->databaseAd, token->getCredentials(), database, project, directory);
		workerPool.push(worker);
	}
	else {
		repoError << "RepoController::_RepoControllerImpl::saveSceneToFile: NULL pointer to scene!";
	}

	return success;
}

bool RepoController::_RepoControllerImpl::saveSceneToFile(
	const std::string &filePath,
	const repo::core::model::RepoScene* scene)
{
	bool success = true;
	if (scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		worker->saveSceneToFile(filePath, scene);
		workerPool.push(worker);
	}
	else {
		repoError << "RepoController::_RepoControllerImpl::saveSceneToFile: NULL pointer to scene!";
		success = false;
	}

	return success;
}

void RepoController::_RepoControllerImpl::reduceTransformations(
	const RepoController::RepoToken              *token,
	repo::core::model::RepoScene *scene)
{
	//We only do reduction optimisations on the unoptimised graph
	const repo::core::model::RepoScene::GraphType gType = repo::core::model::RepoScene::GraphType::DEFAULT;
	if (token && scene && scene->isRevisioned() && !scene->hasRoot(gType))
	{
		//If the unoptimised graph isn't fetched, try to fetch full scene before beginning
		//This should be safe considering if it has not loaded the unoptimised graph it shouldn't have
		//any uncommited changes.
		repoInfo << "Unoptimised scene not loaded, trying loading unoptimised scene...";
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->fetchScene(token->databaseAd, token->getCredentials(), scene);
		workerPool.push(worker);
	}

	if (scene && scene->hasRoot(gType))
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		size_t transNodes_pre = scene->getAllTransformations(gType).size();
		try {
			worker->reduceTransformations(scene, gType);
		}
		catch (const std::exception &e)
		{
			repoError << "Caught exception whilst trying to optimise graph : " << e.what();
		}

		workerPool.push(worker);
		repoInfo << "Optimization completed. Number of transformations has been reduced from "
			<< transNodes_pre << " to " << scene->getAllTransformations(gType).size();
	}
	else {
		repoError << "RepoController::_RepoControllerImpl::reduceTransformations: NULL pointer to scene/ Scene is not loaded!";
	}
}

std::string RepoController::_RepoControllerImpl::getVersion()
{
	std::stringstream ss;
	ss << BOUNCER_VMAJOR << "." << BOUNCER_VMINOR;
	return ss.str();
}