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

bool RepoController::_RepoControllerImpl::commitAssetBundleBuffers(
	const RepoController::RepoToken *token,
	repo::core::model::RepoScene    *scene,
	const repo_web_buffers_t &buffers)
{
	bool success = false;
	manipulator::RepoManipulator* worker = workerPool.pop();
	success = worker->commitAssetBundleBuffers(token->databaseAd,
		token->getCredentials(),
		token->bucketName,
		token->bucketRegion,
		scene,
		buffers);
	workerPool.push(worker);

	return success;
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

uint64_t RepoController::_RepoControllerImpl::countItemsInCollection(
	const RepoController::RepoToken            *token,
	const std::string    &database,
	const std::string    &collection)
{
	uint64_t numItems = 0;
	repoTrace << "Controller: Counting number of items in the collection";

	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		std::string errMsg;
		numItems = worker->countItemsInCollection(token->databaseAd, token->getCredentials(), database, collection, errMsg);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch database without a database connection!";
	}

	return numItems;
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
	const bool           &lightFetch,
	const bool           &ignoreRefScene,
	const bool           &skeletonFetch,
	const std::vector<repo::core::model::RevisionNode::UploadStatus> &includeStatus)
{
	repo::core::model::RepoScene* scene = 0;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		scene = worker->fetchScene(token->databaseAd, token->getCredentials(),
			database, collection, repo::lib::RepoUUID(uuid), headRevision, lightFetch, ignoreRefScene, skeletonFetch, includeStatus);

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

bool RepoController::_RepoControllerImpl::generateAndCommitStashGraph(
	const RepoController::RepoToken              *token,
	repo::core::model::RepoScene* scene
)
{
	bool success = false;

	if (token && scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		if (scene->isRevisioned() && !scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT))
		{
			//If the unoptimised graph isn't fetched, try to fetch full scene before beginning
			//This should be safe considering if it has not loaded the unoptimised graph it shouldn't have
			//any uncommited changes.
			repoInfo << "Unoptimised scene not loaded, trying loading unoptimised scene...";
			worker->fetchScene(token->databaseAd, token->getCredentials(), scene);
		}

		success = worker->generateAndCommitStashGraph(token->databaseAd, token->getCredentials(),
			scene);

		workerPool.push(worker);
	}
	else
	{
		repoError << "Failed to generate stash graph: nullptr to scene or token!";
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

std::vector < repo::core::model::RepoRole > RepoController::_RepoControllerImpl::getRolesFromDatabase(
	const RepoController::RepoToken              *token,
	const std::string            &database,
	const uint64_t               &skip,
	const uint32_t               &limit)
{
	auto bsons = getAllFromCollectionContinuous(token, database, REPO_SYSTEM_ROLES, skip, limit);
	std::vector<repo::core::model::RepoRole> roles;
	for (const auto &b : bsons)
	{
		roles.push_back(repo::core::model::RepoRole(b));
	}

	return roles;
}
std::list<std::string> RepoController::_RepoControllerImpl::getDatabases(const RepoController::RepoToken *token)
{
	repoTrace << "Controller: Fetching Database....";
	std::list<std::string> list;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		if (token->databaseName == worker->getNameOfAdminDatabase(token->databaseAd))
		{
			list = worker->fetchDatabases(token->databaseAd, token->getCredentials());
		}
		else
		{
			//If the user is only authenticated against a single
			//database then just return the database he/she is authenticated against.
			list.push_back(token->databaseName);
		}
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch database without a database connection!";
	}

	return list;
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

std::map<std::string, std::list<std::string>>
RepoController::_RepoControllerImpl::getDatabasesWithProjects(
	const RepoController::RepoToken *token,
	const std::list<std::string> &databases)
{
	std::map<std::string, std::list<std::string> > map;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		map = worker->getDatabasesWithProjects(token->databaseAd,
			token->getCredentials(), databases);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to get database listings without a database connection!";
	}

	return map;
}

bool RepoController::_RepoControllerImpl::insertBinaryFileToDatabase(
	const RepoController::RepoToken            *token,
	const std::string          &database,
	const std::string          &collection,
	const std::string          &name,
	const std::vector<uint8_t> &rawData,
	const std::string          &mimeType)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		return worker->insertBinaryFileToDatabase(token->databaseAd,
			token->getCredentials(),
			token->bucketName,
			token->bucketRegion,
			database,
			collection,
			name,
			rawData,
			mimeType);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to save a binary file without a database connection!";
		return false;
	}
}

void RepoController::_RepoControllerImpl::insertRole(
	const RepoController::RepoToken                   *token,
	const repo::core::model::RepoRole &role
)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->insertRole(token->databaseAd,
			token->getCredentials(), role);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a role without a database connection!";
	}
}

void RepoController::_RepoControllerImpl::insertUser(
	const RepoController::RepoToken                          *token,
	const repo::core::model::RepoUser  &user)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->insertUser(token->databaseAd,
			token->getCredentials(), user);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";
	}
}

bool RepoController::_RepoControllerImpl::removeCollection(
	const RepoController::RepoToken             *token,
	const std::string     &databaseName,
	const std::string     &collectionName,
	std::string			  &errMsg
)
{
	bool success = false;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->dropCollection(token->databaseAd,
			token->getCredentials(), databaseName, collectionName, errMsg);
		workerPool.push(worker);
	}
	else
	{
		errMsg = "Trying to fetch collections without a database connection!";
		repoError << errMsg;
	}

	return success;
}

bool RepoController::_RepoControllerImpl::removeDatabase(
	const RepoController::RepoToken       *token,
	const std::string     &databaseName,
	std::string			  &errMsg
)
{
	bool success = false;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->dropDatabase(token->databaseAd,
			token->getCredentials(), databaseName, errMsg);
		workerPool.push(worker);
	}
	else
	{
		errMsg = "Trying to fetch collections without a database connection!";
		repoError << errMsg;
	}

	return success;
}

void RepoController::_RepoControllerImpl::removeDocument(
	const RepoController::RepoToken                          *token,
	const std::string                        &databaseName,
	const std::string                        &collectionName,
	const repo::core::model::RepoBSON  &bson)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->removeDocument(token->databaseAd,
			token->getCredentials(), databaseName, collectionName, bson);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to delete a document without a database connection!";
	}
}

bool RepoController::_RepoControllerImpl::removeProject(
	const RepoController::RepoToken                          *token,
	const std::string                        &databaseName,
	const std::string                        &projectName,
	std::string								 &errMsg)
{
	bool result = false;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		result = worker->removeProject(token->databaseAd,
			token->getCredentials(), databaseName, projectName, errMsg);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to delete a document without a database connection!";
	}
	return result;
}

void RepoController::_RepoControllerImpl::removeRole(
	const RepoController::RepoToken                          *token,
	const repo::core::model::RepoRole        &role)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->removeRole(token->databaseAd,
			token->getCredentials(), role);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";
	}
}

void RepoController::_RepoControllerImpl::removeUser(
	const RepoController::RepoToken                          *token,
	const repo::core::model::RepoUser  &user)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->removeUser(token->databaseAd,
			token->getCredentials(), user);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";
	}
}

void RepoController::_RepoControllerImpl::updateRole(
	const RepoController::RepoToken                          *token,
	const repo::core::model::RepoRole        &role)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->updateRole(token->databaseAd,
			token->getCredentials(), role);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";
	}
}

void RepoController::_RepoControllerImpl::updateUser(
	const RepoController::RepoToken                          *token,
	const repo::core::model::RepoUser  &user)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->updateUser(token->databaseAd,
			token->getCredentials(), user);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";
	}
}

void RepoController::_RepoControllerImpl::upsertDocument(
	const RepoController::RepoToken                          *token,
	const std::string                        &databaseName,
	const std::string                        &collectionName,
	const repo::core::model::RepoBSON  &bson)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->upsertDocument(token->databaseAd,
			token->getCredentials(), databaseName, collectionName, bson);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to upsert a document without a database connection!";
	}
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
	const std::map<repo::core::model::TransformationNode, repo::core::model::ReferenceNode> &fedMap)
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

bool RepoController::_RepoControllerImpl::generateAndCommitGLTFBuffer(
	const RepoController::RepoToken *token,
	repo::core::model::RepoScene    *scene)
{
	bool success;
	if (success = token && scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->generateAndCommitGLTFBuffer(token->databaseAd,
			token->getCredentials(),
			token->bucketName,
			token->bucketRegion,
			scene);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Failed to generate GLTF Buffer.";
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

repo_web_buffers_t RepoController::_RepoControllerImpl::generateGLTFBuffer(
	repo::core::model::RepoScene *scene)
{
	repo_web_buffers_t buffer;
	if (scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		buffer = worker->generateGLTFBuffer(scene);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Failed to generate SRC Buffer.";
	}
	return buffer;
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

std::list<std::string> RepoController::_RepoControllerImpl::getAdminDatabaseRoles(const RepoController::RepoToken *token)
{
	std::list<std::string> roles;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		roles = worker->getAdminDatabaseRoles(token->databaseAd);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to get database roles without a database connection!";
	}

	return roles;
}

std::string RepoController::_RepoControllerImpl::getNameOfAdminDatabase(const RepoController::RepoToken *token)
{
	std::string name;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		name = worker->getNameOfAdminDatabase(token->databaseAd);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to get database roles without a token!";
	}
	return name;
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

std::list<std::string> RepoController::_RepoControllerImpl::getStandardDatabaseRoles(const RepoController::RepoToken *token)
{
	std::list<std::string> roles;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		roles = worker->getStandardDatabaseRoles(token->databaseAd);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to get database roles without a token!";
	}

	return roles;
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

std::vector<std::shared_ptr<repo::core::model::MeshNode>> RepoController::_RepoControllerImpl::initialiseAssetBuffer(
	const RepoController::RepoToken                    *token,
	repo::core::model::RepoScene *scene,
	std::unordered_map<std::string, std::vector<uint8_t>> &jsonFiles,
	repo::core::model::RepoUnityAssets &unityAssets,
	std::vector<std::vector<uint16_t>> &serialisedFaceBuf,
	std::vector<std::vector<std::vector<float>>> &idMapBuf,
	std::vector<std::vector<std::vector<repo_mesh_mapping_t>>> &meshMappings)
{
	std::vector<std::shared_ptr<repo::core::model::MeshNode>> res;
	if (scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		res = worker->initialiseAssetBuffer(token->databaseAd, token->getCredentials(), scene, jsonFiles, unityAssets, serialisedFaceBuf, idMapBuf, meshMappings);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to generate Asset buffer without a scene";
	}

	return res;
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

void RepoController::_RepoControllerImpl::compareScenes(
	const RepoController::RepoToken                    *token,
	repo::core::model::RepoScene       *base,
	repo::core::model::RepoScene       *compare,
	repo_diff_result_t &baseResults,
	repo_diff_result_t &compResults,
	const repo::DiffMode       &diffMode
)
{
	//We only do reduction optimisations on the unoptimised graph
	const repo::core::model::RepoScene::GraphType gType = repo::core::model::RepoScene::GraphType::DEFAULT;
	if (token && base && compare)
	{
		//If the unoptimised graph isn't fetched, try to fetch full scene before beginning
		//This should be safe considering if it has not loaded the unoptimised graph it shouldn't have
		//any uncommited changes.
		if (base->isRevisioned() && !base->hasRoot(gType))
		{
			repoInfo << "Unoptimised base scene not loaded, trying loading unoptimised scene...";
			manipulator::RepoManipulator* worker = workerPool.pop();
			worker->fetchScene(token->databaseAd, token->getCredentials(), base);
			workerPool.push(worker);
		}

		if (compare->isRevisioned() && !compare->hasRoot(gType))
		{
			repoInfo << "Unoptimised compare scene not loaded, trying loading unoptimised scene...";
			manipulator::RepoManipulator* worker = workerPool.pop();
			worker->fetchScene(token->databaseAd, token->getCredentials(), compare);
			workerPool.push(worker);
		}
	}

	if (base && base->hasRoot(gType) && compare && compare->hasRoot(gType))
	{
		repoInfo << "Comparing scenes...";
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->compareScenes(base, compare, baseResults, compResults, diffMode, gType);
		workerPool.push(worker);
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