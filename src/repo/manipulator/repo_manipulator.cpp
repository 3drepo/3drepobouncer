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
#include "../core/model/bson/repo_bson_factory.h"
#include "modelconvertor/export/repo_model_export_assimp.h"

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
	//FIXME: we should have a database manager class that will instantiate new handlers/give existing handlers
	repo::core::handler::AbstractDatabaseHandler *handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(
		errMsg, address, port, maxConnections, dbName, username, password, pwDigested);

	return handler != 0;
}

bool RepoManipulator::connectAndAuthenticateWithAdmin(
	std::string       &errMsg,
	const std::string &address,
	const uint32_t    &port,
	const uint32_t    &maxConnections,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested
	)
{
	//FIXME: we should have a database manager class that will instantiate new handlers/give existing handlers
	repo::core::handler::AbstractDatabaseHandler *handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(
		errMsg, address, port, maxConnections, 
		repo::core::handler::MongoDatabaseHandler::getAdminDatabaseName(), 
		username, password, pwDigested);

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

repo::manipulator::graph::RepoScene* RepoManipulator::createFederatedScene(
	const std::map<repo::core::model::bson::TransformationNode, repo::core::model::bson::ReferenceNode> &fedMap)
{

	repo::core::model::bson::RepoNodeSet transNodes;
	repo::core::model::bson::RepoNodeSet refNodes;
	repo::core::model::bson::RepoNodeSet emptySet;

	repo::core::model::bson::TransformationNode rootNode =
		repo::core::model::bson::RepoBSONFactory::makeTransformationNode(
		repo::core::model::bson::TransformationNode::identityMat(), "<root>");

	transNodes.insert(new repo::core::model::bson::TransformationNode(rootNode));

	for (const auto &pair : fedMap)
	{

		transNodes.insert(new repo::core::model::bson::TransformationNode(
			pair.first.cloneAndAddParent(rootNode.getSharedID())
			)
			);
		refNodes.insert(new repo::core::model::bson::ReferenceNode(
			pair.second.cloneAndAddParent(pair.first.getSharedID())
			)
			);
	}

	repo::manipulator::graph::RepoScene *scene =
		new repo::manipulator::graph::RepoScene(emptySet, emptySet, emptySet, emptySet, emptySet, transNodes, refNodes);

	return scene;
}

void RepoManipulator::commitScene(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON 	  *cred,
	repo::manipulator::graph::RepoScene           *scene)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);

	std::string msg;
	if (handler && scene && scene->commit(handler, msg, cred->getStringField("user")))
	{
		BOOST_LOG_TRIVIAL(info) << "Scene successfully committed to the database";
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Error committing scene to the database : " << msg;
	}
		
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
		vector = handler->getAllFromCollectionTailable(database, collection, skip);
	return vector;
}

std::vector<repo::core::model::bson::RepoBSON>
	RepoManipulator::getAllFromCollectionTailable(
		const std::string                             &databaseAd,
		const repo::core::model::bson::RepoBSON*	  cred,
		const std::string                             &database,
		const std::string                             &collection,
		const std::list<std::string>				  &fields,
		const std::string							  &sortField,
		const int									  &sortOrder,
		const uint64_t                                &skip)
{
	std::vector<repo::core::model::bson::RepoBSON> vector;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		vector = handler->getAllFromCollectionTailable(database, collection, skip, fields, sortField, sortOrder);
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

std::map<std::string, std::list<std::string>>
	RepoManipulator::getDatabasesWithProjects(
		const std::string                             &databaseAd,
		const repo::core::model::bson::RepoBSON*	  cred,
		const std::list<std::string> &databases)
{
	std::map<std::string, std::list<std::string>> list;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		list = handler->getDatabasesWithProjects(databases);

	return list;
}

std::list<std::string> RepoManipulator::getAdminDatabaseRoles(
	const std::string  &databaseAd)
{
	std::list<std::string> roles;

	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		roles = handler->getAdminDatabaseRoles();

	return roles;
}

std::list<std::string> RepoManipulator::getStandardDatabaseRoles(
	const std::string  &databaseAd)
{
	std::list<std::string> roles;

	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		roles = handler->getStandardDatabaseRoles();

	return roles;
}

std::string RepoManipulator::getNameOfAdminDatabase(
	const std::string                             &databaseAd) const
{
	
	//FIXME: at the moment we only have mongo. But if we have
	//different database types then this would not work
	return  repo::core::handler::MongoDatabaseHandler::getAdminDatabaseName();
}

repo::manipulator::graph::RepoScene* 
	RepoManipulator::loadSceneFromFile(
	const std::string &filePath, 
	      std::string &msg,
    const repo::manipulator::modelconvertor::ModelImportConfig *config)
{
	
	repo::manipulator::graph::RepoScene* scene = nullptr;

	repo::manipulator::modelconvertor::AssimpModelImport*
		modelConvertor = new repo::manipulator::modelconvertor::AssimpModelImport(config);



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

void RepoManipulator::insertUser(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred,
	const repo::core::model::bson::RepoUser       &user)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->insertUser(user, errMsg))
		{
			BOOST_LOG_TRIVIAL(info) << "User added successfully.";
		}
		else
		{
			BOOST_LOG_TRIVIAL(error) << "Failed to add user : " << errMsg;
		}
	}
		
}

void RepoManipulator::removeUser(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred,
	const repo::core::model::bson::RepoUser       &user)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->dropUser(user, errMsg))
		{
			BOOST_LOG_TRIVIAL(info) << "User removed successfully.";
		}
		else
		{
			BOOST_LOG_TRIVIAL(error) << "Failed to remove user : " << errMsg;
		}
	}

}

void RepoManipulator::updateUser(
	const std::string                             &databaseAd,
	const repo::core::model::bson::RepoBSON*	  cred,
	const repo::core::model::bson::RepoUser       &user)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->updateUser(user, errMsg))
		{
			BOOST_LOG_TRIVIAL(info) << "User updated successfully.";
		}
		else
		{
			BOOST_LOG_TRIVIAL(error) << "Failed to update user : " << errMsg;
		}
	}

}

bool RepoManipulator::saveSceneToFile(
	const std::string &filePath,
	const repo::manipulator::graph::RepoScene* scene)
{
	modelconvertor::AssimpModelExport modelExport;
	return modelExport.exportToFile(scene, filePath);
}

