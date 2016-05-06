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

RepoController::RepoToken* RepoController::_RepoControllerImpl::authenticateToAdminDatabaseMongo(
	std::string       &errMsg,
	const std::string &address,
	const int         &port,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested
	)
{
	manipulator::RepoManipulator* worker = workerPool.pop();

	core::model::RepoBSON* cred = 0;
	RepoToken *token = 0;

	std::string dbFullAd = address + ":" + std::to_string(port);

	bool success = worker->connectAndAuthenticateWithAdmin(errMsg, address, port,
		numDBConnections, username, password, pwDigested);

	if (success && !username.empty())
		cred = worker->createCredBSON("admin", username, password, pwDigested);

	if (success && (cred || username.empty()))
	{
		token = new RepoController::RepoToken(*cred, address, port, worker->getNameOfAdminDatabase(dbFullAd));
		if (cred) delete cred;
		repoInfo << "Successfully connected to the " << dbFullAd;
		if (!username.empty())
			repoInfo << username << " is authenticated to " << dbFullAd;
	}

	workerPool.push(worker);
	return token;
}

RepoController::RepoToken* RepoController::_RepoControllerImpl::authenticateMongo(
	std::string       &errMsg,
	const std::string &address,
	const uint32_t    &port,
	const std::string &dbName,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested
	)
{
	manipulator::RepoManipulator* worker = workerPool.pop();

	core::model::RepoBSON* cred = 0;
	RepoToken *token = 0;

	std::string dbFullAd = address + ":" + std::to_string(port);

	bool success = worker->connectAndAuthenticate(errMsg, address, port,
		numDBConnections, dbName, username, password, pwDigested);

	if (success && !username.empty())
		cred = worker->createCredBSON(dbName, username, password, pwDigested);
	workerPool.push(worker);

	if (cred || username.empty())
	{
		token = new RepoController::RepoToken(*cred, address, port, dbName);
		if (cred) delete cred;
	}
	return token;
}

bool RepoController::_RepoControllerImpl::authenticateMongo(
	std::string                       &errMsg,
	const RepoController::RepoToken   *token
	)
{
	bool success = false;
	if (success = token && token->valid())
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->connectAndAuthenticate(errMsg, token->databaseHost, token->databasePort,
			numDBConnections, token->databaseName, token->getCredentials());
		workerPool.push(worker);
	}
	else
	{
		errMsg = "Token is invalid!";
	}

	return success;
}

bool RepoController::_RepoControllerImpl::testConnection(const RepoController::RepoToken *token)
{
	std::string errMsg;
	bool isConnected = false;
	if (authenticateMongo(
		errMsg,
		token))
	{
		repoTrace << "Connection established.";
		disconnectFromDatabase(token);
		isConnected = true;
	}
	else
	{
		//connection/authentication failed
		repoError << "Failed to connect/authenticate: " << errMsg;
	}
	return isConnected;
}

void  RepoController::_RepoControllerImpl::cleanUp(
	const RepoController::RepoToken        *token,
	const std::string                      &dbName,
	const std::string                      &projectName
	)
{
	if (!token)
	{
		repoError << "Failed to clean up project: empty token to database";
		return;
	}

	if (dbName.empty() || projectName.empty())
	{
		repoError << "Failed to clean up project: database or project name is empty!";
		return;
	}

	manipulator::RepoManipulator* worker = workerPool.pop();
	worker->cleanUp(token->databaseAd, token->getCredentials(), dbName, projectName);
	workerPool.push(worker);
}

RepoController::RepoToken* RepoController::_RepoControllerImpl::createToken(
	const std::string &alias,
	const std::string &address,
	const int         &port,
	const std::string &dbName,
	const std::string &username,
	const std::string &password
	)
{
	manipulator::RepoManipulator* worker = workerPool.pop();

	RepoController::RepoToken *token = nullptr;

	std::string dbFullAd = address + ":" + std::to_string(port);
	repo::core::model::RepoBSON *cred = nullptr;
	if (!username.empty())
		cred = worker->createCredBSON(dbName, username, password, false);
	workerPool.push(worker);

	if (cred || username.empty())
	{
		token = new RepoController::RepoToken(cred ? *cred : repo::core::model::RepoBSON(), address, port, dbName, alias);
		if (cred)delete cred;
	}

	return token && token->valid() ? token : nullptr;
}

RepoController::RepoToken* RepoController::_RepoControllerImpl::createToken(
	const std::string &alias,
	const std::string &address,
	const int         &port,
	const std::string &dbName,
	const RepoController::RepoToken *credToken
	)
{
	RepoController::RepoToken *token = nullptr;
	std::string dbFullAd = address + ":" + std::to_string(port);

	token = new RepoController::RepoToken(credToken->credentials, address, port, dbName, alias);

	return token && token->valid() ? token : nullptr;
}

void RepoController::_RepoControllerImpl::commitScene(
	const RepoController::RepoToken                     *token,
	repo::core::model::RepoScene        *scene,
	const std::string                   &owner)
{
	if (scene)
	{
		if (!(scene->getDatabaseName().empty() || scene->getProjectName().empty()))
		{
			if (token)
			{
				std::string sceneOwner = owner;
				if (!token->getCredentials())
				{
					sceneOwner = "ANONYMOUS USER";
				}
				manipulator::RepoManipulator* worker = workerPool.pop();
				worker->commitScene(token->databaseAd, token->getCredentials(), scene, sceneOwner);
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
	const bool           &lightFetch)
{
	repo::core::model::RepoScene* scene = 0;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		scene = worker->fetchScene(token->databaseAd, token->getCredentials(),
			database, collection, stringToUUID(uuid), headRevision, lightFetch);

		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch scene without a database connection!";
	}

	return scene;
}

bool RepoController::_RepoControllerImpl::generateAndCommitSelectionTree(
	const RepoController::RepoToken                               *token,
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

		success = worker->generateAndCommitSelectionTree(token->databaseAd, token->getCredentials(), scene);
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

std::vector < repo::core::model::RepoRoleSettings > RepoController::_RepoControllerImpl::getRoleSettingsFromDatabase(
	const RepoController::RepoToken              *token,
	const std::string            &database,
	const uint64_t               &skip,
	const uint32_t               &limit)
{
	auto bsons = getAllFromCollectionContinuous(token, database, REPO_COLLECTION_SETTINGS_ROLES, skip, limit);
	std::vector<repo::core::model::RepoRoleSettings > roleSettings;
	for (const auto &b : bsons)
	{
		roleSettings.push_back(repo::core::model::RepoRoleSettings(b));
	}
	return roleSettings;
}

repo::core::model::RepoRoleSettings RepoController::_RepoControllerImpl::getRoleSettings(
	const RepoController::RepoToken *token,
	const std::string &database,
	const std::string &uniqueRoleName)
{
	repo::core::model::RepoRoleSettings res;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		res = worker->getRoleSettingByName(token->databaseAd, token->getCredentials(), database, uniqueRoleName);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to retrieve Role setting from database without a valid token!";
	}
	return res;
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

repo::core::model::CollectionStats RepoController::_RepoControllerImpl::getCollectionStats(
	const RepoController::RepoToken      *token,
	const std::string    &database,
	const std::string    &collection)
{
	repo::core::model::CollectionStats stats;

	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		std::string errMsg;
		stats = worker->getCollectionStats(token->databaseAd,
			token->getCredentials(), database, collection, errMsg);
		workerPool.push(worker);

		if (!errMsg.empty())
			repoError << errMsg;
	}
	else
	{
		repoError << "Trying to get collections stats without a database connection!";
	}

	return stats;
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

void RepoController::_RepoControllerImpl::insertBinaryFileToDatabase(
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
		worker->insertBinaryFileToDatabase(token->databaseAd,
			token->getCredentials(), database, collection, name, rawData, mimeType);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to save a binary file without a database connection!";
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

repo::core::model::RepoScene* RepoController::_RepoControllerImpl::createMapScene(
	const repo::core::model::MapNode &mapNode)
{
	manipulator::RepoManipulator* worker = workerPool.pop();
	repo::core::model::RepoScene* scene = worker->createMapScene(mapNode);
	workerPool.push(worker);

	return scene;
}

bool RepoController::_RepoControllerImpl::generateAndCommitGLTFBuffer(
	const RepoController::RepoToken                    *token,
	const repo::core::model::RepoScene *scene)
{
	bool success;
	if (success = token && scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->generateAndCommitGLTFBuffer(token->databaseAd, token->getCredentials(), scene);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Failed to generate GLTF Buffer.";
	}
	return success;
}

bool RepoController::_RepoControllerImpl::generateAndCommitSRCBuffer(
	const RepoController::RepoToken                    *token,
	const repo::core::model::RepoScene *scene)
{
	bool success;
	if (success = token && scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->generateAndCommitSRCBuffer(token->databaseAd, token->getCredentials(), scene);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Failed to generate SRC Buffer.";
	}
	return success;
}

repo_web_buffers_t RepoController::_RepoControllerImpl::generateGLTFBuffer(
	const repo::core::model::RepoScene *scene)
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
	const repo::core::model::RepoScene *scene)
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

repo::core::model::RepoScene*
RepoController::_RepoControllerImpl::loadSceneFromFile(
const std::string                                          &filePath,
const bool                                                 &applyReduction,
const bool                                                 &rotateModel,
const repo::manipulator::modelconvertor::ModelImportConfig *config)
{
	std::string errMsg;
	repo::core::model::RepoScene *scene = nullptr;

	if (!filePath.empty())
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		scene = worker->loadSceneFromFile(filePath, errMsg, applyReduction, rotateModel, config);
		workerPool.push(worker);
		if (!scene)
			repoError << "Failed ot load scene from file: " << errMsg;
	}
	else
	{
		repoError << "Trying to load from an empty file path!";
	}

	return scene;
}

void RepoController::_RepoControllerImpl::saveOriginalFiles(
	const RepoController::RepoToken                    *token,
	const repo::core::model::RepoScene *scene,
	const std::string                   &directory)
{
	if (scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		worker->saveOriginalFiles(token->databaseAd, token->getCredentials(), scene, directory);
		workerPool.push(worker);
	}
	else{
		repoError << "RepoController::_RepoControllerImpl::saveSceneToFile: NULL pointer to scene!";
	}
}

void RepoController::_RepoControllerImpl::saveOriginalFiles(
	const RepoController::RepoToken                    *token,
	const std::string                   &database,
	const std::string                   &project,
	const std::string                   &directory)
{
	if (!(database.empty() || project.empty()))
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		worker->saveOriginalFiles(token->databaseAd, token->getCredentials(), database, project, directory);
		workerPool.push(worker);
	}
	else{
		repoError << "RepoController::_RepoControllerImpl::saveSceneToFile: NULL pointer to scene!";
	}
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
	else{
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
		try{
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
	else{
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
	else{
		repoError << "RepoController::_RepoControllerImpl::reduceTransformations: NULL pointer to scene/ Scene is not loaded!";
	}
}

std::string RepoController::_RepoControllerImpl::getVersion()
{
	std::stringstream ss;
	ss << BOUNCER_VMAJOR << "." << BOUNCER_VMINOR;
	return ss.str();
}