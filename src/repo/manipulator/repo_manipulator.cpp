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

/**
* Repo Manipulator which handles all the manipulation
*/

#include "repo_manipulator.h"

using namespace repo::manipulator;

RepoManipulator::RepoManipulator()
{
}


RepoManipulator::~RepoManipulator()
{
}

bool RepoManipulator::connectAndAuthenticate(
	std::string       &errMsg,
	const std::string &address,
	const uint32_t    &port,
	const uint32_t    &maxConnections,
	const std::string &dbName,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested
	)
{
	repo::core::handler::AbstractDatabaseHandler *handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(
		errMsg, address, port, maxConnections, dbName, username, password, pwDigested);

	return handler != 0;
}

repo::core::model::bson::RepoBSON* RepoManipulator::createCredBSON(
	const std::string &databaseAd,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested)
{
	core::model::bson::RepoBSON* bson = 0;

	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		bson = handler->createBSONCredentials(databaseAd, username, password, pwDigested);

	return bson;
}

uint64_t RepoManipulator::countItemsInCollection(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred,
	const std::string                             &database,
	const std::string                             &collection,
	std::string                                   &errMsg)
{

	uint64_t numItems;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);

	if (handler)
		numItems = handler->countItemsInCollection(database, collection, errMsg);

	return numItems;
}

bool RepoManipulator::dropCollection(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred,
	const std::string                             &databaseName,
	const std::string                             &collectionName,
	std::string			                          &errMsg
	)
{
	bool success = false;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		success = handler->dropCollection(databaseName, collectionName, errMsg);
	else
		errMsg = "Unable to locate database handler for " + databaseAd + ". Try reauthenticating.";

	return success;
}

bool RepoManipulator::dropDatabase(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred,
	const std::string                             &databaseName,
	std::string			                          &errMsg
	)
{
	bool success = false;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		success = handler->dropDatabase(databaseName, errMsg);
	else
		errMsg = "Unable to locate database handler for " + databaseAd + ". Try reauthenticating.";

	return success;
}

std::list<std::string> RepoManipulator::fetchDatabases(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred
	)
{
	std::list<std::string> list;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		list = handler->getDatabases();

	return list;
}

std::list<std::string> RepoManipulator::fetchCollections(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred,
	const std::string                             &database
	)
{

	std::list<std::string> list;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		list = handler->getCollections(database);

	return list;
}

repo::manipulator::graph::RepoScene* RepoManipulator::fetchScene(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred,
	const std::string                             &database,
	const std::string                             &project,
	const repoUUID                                &uuid,
	const bool                                    &headRevision)
{
	graph::RepoScene* scene = nullptr;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		//not setting a scene if we don't have a handler since we 
		//retreive anything from the database.
		scene = new graph::RepoScene(database, project);
		if (scene)
		{
			if (headRevision)
				scene->setBranch(uuid);
			else
				scene->setRevision(uuid);

			std::string errMsg;
			if (scene->loadRevision(handler, errMsg))
			{
				BOOST_LOG_TRIVIAL(trace) << "Loaded " <<
					(headRevision ? ("head revision of branch" + UUIDtoString(uuid))
					: ("revision " + UUIDtoString(uuid)))
					<< " of " << database << "." << project;

				if (scene->loadScene(handler, errMsg))
				{
					BOOST_LOG_TRIVIAL(trace) << "Loaded Scene";
				}
				else{
					delete scene;
					scene = nullptr;
				}
			}
			else
			{
				BOOST_LOG_TRIVIAL(error) << "Failed to load revision for " 
				 << database << "." << project << " : " << errMsg;
				delete scene;
				scene = nullptr;
			}
		}
		else{
			BOOST_LOG_TRIVIAL(error) << "Failed to create a RepoScene(out of memory?)!";
		}
	}

	return scene;
}

std::vector<repo::core::model::bson::RepoBSON>
	RepoManipulator::getAllFromCollectionTailable(
		const std::string                             &databaseAd,
		const repo::core::model::bson::RepoBSON*	  cred,
		const std::string                             &database,
		const std::string                             &collection,
		const uint64_t                                &skip)
{
	std::vector<repo::core::model::bson::RepoBSON> vector;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		vector =  handler->getAllFromCollectionTailable(database, collection);
	return vector;
}

repo::core::model::bson::CollectionStats RepoManipulator::getCollectionStats(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred,
	const std::string                             &database,
	const std::string                             &collection,
	std::string                                   &errMsg)
{

	repo::core::model::bson::CollectionStats stats;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		stats = handler->getCollectionStats(database, collection, errMsg);

	return stats;
}

repo::manipulator::graph::RepoScene* 
	RepoManipulator::loadSceneFromFile(
	const std::string &filePath, 
	      std::string &msg)
{
	
	repo::manipulator::graph::RepoScene* scene = nullptr;

	repo::manipulator::modelconvertor::AssimpModelImport*
		modelConvertor = new repo::manipulator::modelconvertor::AssimpModelImport();



	if (modelConvertor)
	{
		if (modelConvertor->importModel(filePath, msg))
		{
			scene = modelConvertor->generateRepoScene();
		}

		delete modelConvertor;
	}
	else
	{
		msg += "Unable to instantiate a new modelConvertor (out of memory?)";
	}

	return scene;
}
