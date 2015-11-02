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


#include "repo_controller.h"

#include "manipulator/modelconvertor/import/repo_model_import_assimp.h"
#include "manipulator/modelconvertor/export/repo_model_export_assimp.h"




using namespace repo;

RepoController::RepoController(
	std::vector<lib::RepoAbstractListener*> listeners,
	const uint32_t &numConcurrentOps,
	const uint32_t &numDbConn) :
	numDBConnections(numDbConn)
{

	for (uint32_t i = 0; i < numConcurrentOps; i++)
	{
		repoTrace << "Instantiating worker pool with " << numConcurrentOps << " workers";
		manipulator::RepoManipulator* worker = new manipulator::RepoManipulator();
		workerPool.push(worker);
	}

	if (listeners.size() > 0)
	{
		subscribeToLogger(listeners);
	}

}


RepoController::~RepoController()
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

RepoToken* RepoController::authenticateToAdminDatabaseMongo(
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
		cred = worker->createCredBSON(dbFullAd, username, password, pwDigested);


	if (cred || username.empty())
	{
		token = new RepoToken(cred, dbFullAd, worker->getNameOfAdminDatabase(dbFullAd));

		repoInfo << "Successfully connected to the " << dbFullAd;
		if (!username.empty())
			repoInfo << username << " is authenticated to " << dbFullAd;
	}

	workerPool.push(worker);
	return token;
}

RepoToken* RepoController::authenticateMongo(
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
		cred = worker->createCredBSON(dbFullAd, username, password, pwDigested);
	workerPool.push(worker);

	if (cred || username.empty())
	{
		token = new RepoToken(cred, dbFullAd, dbName);
	}
	return token ;
}

bool RepoController::testConnection(const repo::RepoCredentials &credentials)
{
    std::string errMsg;
	bool isConnected = false;
    RepoToken* token = nullptr;
	if (token = authenticateMongo(
		errMsg,
		credentials.getHost(),
		credentials.getPort(),
		credentials.getAuthenticationDatabase(),
		credentials.getUsername(),
		credentials.getPassword(),
		false))
	{
		repoTrace << "Connection established.";
		disconnectFromDatabase(token);
		isConnected = true;
		delete token;
	}
    else
    {
        //connection/authentication failed
        repoError << "Failed to connect/authenticate.";
        repoError << errMsg;
    }
	return isConnected;
}

void RepoController::commitScene(
	const RepoToken                     *token,
	repo::core::model::RepoScene        *scene,
	const std::string                   &owner)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->commitScene(token->databaseAd, token->credentials, scene, owner);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to commit to the database without a database connection!";
	}
}

uint64_t RepoController::countItemsInCollection(
	const RepoToken            *token,
	const std::string    &database,
	const std::string    &collection)
{
	uint64_t numItems = 0;
	repoTrace << "Controller: Counting number of items in the collection";

	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		std::string errMsg;
		numItems = worker->countItemsInCollection(token->databaseAd, token->credentials, database, collection, errMsg);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch database without a database connection!";
	}

	return numItems;
}

void RepoController::disconnectFromDatabase(const RepoToken* token)
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

repo::core::model::RepoScene* RepoController::fetchScene(
	const RepoToken      *token,
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

		scene = worker->fetchScene(token->databaseAd, token->credentials,
			database, collection, stringToUUID(uuid), headRevision, lightFetch);

		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch scene without a database connection!";
	}

	return scene;
}

std::vector < repo::core::model::RepoBSON >
	RepoController::getAllFromCollectionContinuous(
	    const RepoToken      *token,
		const std::string    &database,
		const std::string    &collection,
		const uint64_t       &skip)
{
	repoTrace << "Controller: Fetching BSONs from "
		<< database << "."  << collection << "....";
	std::vector<repo::core::model::RepoBSON> vector;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		vector = worker->getAllFromCollectionTailable(token->databaseAd, token->credentials,
			database, collection, skip);

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
	RepoController::getAllFromCollectionContinuous(
		const RepoToken              *token,
		const std::string            &database,
		const std::string            &collection,
		const std::list<std::string> &fields,
		const std::string            &sortField,
		const int                    &sortOrder,
		const uint64_t               &skip)
{
	repoTrace << "Controller: Fetching BSONs from "
		<< database << "." << collection << "....";
	std::vector<repo::core::model::RepoBSON> vector;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		vector = worker->getAllFromCollectionTailable(token->databaseAd, token->credentials,
			database, collection, fields, sortField, sortOrder, skip);

		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch data from a collection without a database connection!";
	}

	repoTrace << "Obtained " << vector.size() << " bson objects.";

	return vector;
}

std::list<std::string> RepoController::getDatabases(const RepoToken *token)
{
	repoTrace << "Controller: Fetching Database....";
	std::list<std::string> list;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		if (token->databaseName == worker->getNameOfAdminDatabase(token->databaseAd))
		{
			list = worker->fetchDatabases(token->databaseAd, token->credentials);
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

std::list<std::string>  RepoController::getCollections(
	const RepoToken       *token,
	const std::string     &databaseName
	)
{
	std::list<std::string> list;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		list = worker->fetchCollections(token->databaseAd, token->credentials, databaseName);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to fetch collections without a database connection!";
	}

	return list;

}

repo::core::model::CollectionStats RepoController::getCollectionStats(
	const RepoToken      *token,
	const std::string    &database,
	const std::string    &collection)
{
	repo::core::model::CollectionStats stats;

	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		std::string errMsg;
		stats = worker->getCollectionStats(token->databaseAd,
			token->credentials, database, collection, errMsg);
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
	RepoController::getDatabasesWithProjects(
		const RepoToken *token,
		const std::list<std::string> &databases)
{
	std::map<std::string, std::list<std::string>> map;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		map = worker->getDatabasesWithProjects(token->databaseAd,
			token->credentials, databases);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";

	}

	return map;
}

void RepoController::insertRole(
	const RepoToken                   *token,
	const repo::core::model::RepoRole &role
	)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->insertRole(token->databaseAd,
			token->credentials, role);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";

	}
}

void RepoController::insertUser(
	const RepoToken                          *token,
	const repo::core::model::RepoUser  &user)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->insertUser(token->databaseAd,
			token->credentials, user);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";

	}

}

bool RepoController::removeCollection(
	const RepoToken             *token,
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
			token->credentials, databaseName, collectionName, errMsg);
		workerPool.push(worker);
	}
	else
	{
		errMsg = "Trying to fetch collections without a database connection!";
		repoError << errMsg;

	}

	return success;
}


bool RepoController::removeDatabase(
	const RepoToken       *token,
	const std::string     &databaseName,
	std::string			  &errMsg
	)
{
	bool success = false;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->dropDatabase(token->databaseAd,
			token->credentials, databaseName, errMsg);
		workerPool.push(worker);
	}
	else
	{
		errMsg = "Trying to fetch collections without a database connection!";
		repoError << errMsg;

	}

	return success;
}

void RepoController::removeDocument(
	const RepoToken                          *token,
	const std::string                        &databaseName,
	const std::string                        &collectionName,
	const repo::core::model::RepoBSON  &bson)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->removeDocument(token->databaseAd,
			token->credentials, databaseName, collectionName, bson);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to delete a document without a database connection!";
	}

}

void RepoController::removeRole(
	const RepoToken                          *token,
	const repo::core::model::RepoRole        &role)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->removeRole(token->databaseAd,
			token->credentials, role);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";

	}
}

void RepoController::removeUser(
	const RepoToken                          *token,
	const repo::core::model::RepoUser  &user)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->removeUser(token->databaseAd,
			token->credentials, user);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";

	}
}

void RepoController::updateRole(
	const RepoToken                          *token,
	const repo::core::model::RepoRole        &role)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->updateRole(token->databaseAd,
			token->credentials, role);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";

	}
}

void RepoController::updateUser(
	const RepoToken                          *token,
	const repo::core::model::RepoUser  &user)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->updateUser(token->databaseAd,
			token->credentials, user);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to insert a user without a database connection!";

	}
}


void RepoController::upsertDocument(
	const RepoToken                          *token,
	const std::string                        &databaseName,
	const std::string                        &collectionName,
	const repo::core::model::RepoBSON  &bson)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->upsertDocument(token->databaseAd,
			token->credentials, databaseName, collectionName, bson);
		workerPool.push(worker);
	}
	else
	{
		repoError << "Trying to upsert a document without a database connection!";
	}
}

void RepoController::setLoggingLevel(const repo::lib::RepoLog::RepoLogLevel &level)
{

	repo::lib::RepoLog::getInstance().setLoggingLevel(level);

}

void RepoController::logToFile(const std::string &filePath)
{
	repo::lib::RepoLog::getInstance().logToFile(filePath);
}

void RepoController::subscribeToLogger(
	std::vector<lib::RepoAbstractListener*> listeners)
{
	repo::lib::RepoLog::getInstance().subscribeListeners(listeners);
}



repo::core::model::RepoScene* RepoController::createFederatedScene(
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

repo::core::model::RepoScene* RepoController::createMapScene(
	const repo::core::model::MapNode &mapNode)
{
	manipulator::RepoManipulator* worker = workerPool.pop();
	repo::core::model::RepoScene* scene = worker->createMapScene(mapNode);
	workerPool.push(worker);

	return scene;
}

std::list<std::string> RepoController::getAdminDatabaseRoles(const RepoToken *token)
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

std::string RepoController::getNameOfAdminDatabase(const RepoToken *token)
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

std::list<std::string> RepoController::getStandardDatabaseRoles(const RepoToken *token)
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

std::string RepoController::getSupportedExportFormats()
{
	//This needs to be updated if we support more than assimp
	return repo::manipulator::modelconvertor::AssimpModelExport::getSupportedFormats();
}

std::string RepoController::getSupportedImportFormats()
{
	//This needs to be updated if we support more than assimp
	return repo::manipulator::modelconvertor::AssimpModelImport::getSupportedFormats();
}


repo::core::model::RepoNodeSet RepoController::loadMetadataFromFile(
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
	RepoController::loadSceneFromFile(
	const std::string                                          &filePath,
	const repo::manipulator::modelconvertor::ModelImportConfig *config)
{

	std::string errMsg;
	repo::core::model::RepoScene *scene = nullptr;

	if (!filePath.empty())
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		scene = worker->loadSceneFromFile(filePath, errMsg, config);
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

void RepoController::saveOriginalFiles(
	const RepoToken                    *token,
	const repo::core::model::RepoScene *scene,
	const std::string                   &directory)
{
	bool success = true;
	if (scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		worker->saveOriginalFiles(token->databaseAd, token->credentials, scene, directory);
		workerPool.push(worker);

	}
	else{
		repoError << "RepoController::saveSceneToFile: NULL pointer to scene!";
	}

}

bool RepoController::saveSceneToFile(
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
		repoError << "RepoController::saveSceneToFile: NULL pointer to scene!";
		success = false;
	}

	return success;
}
