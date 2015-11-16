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

#include <boost/filesystem.hpp>

#include "repo_manipulator.h"
#include "../lib/repo_log.h"
#include "../core/model/bson/repo_bson_factory.h"
#include "diff/repo_diff_sharedid.h"
#include "modelconvertor/export/repo_model_export_assimp.h"
#include "modelconvertor/import/repo_metadata_import_csv.h"
#include "modeloptimizer/repo_optimizer_trans_reduction.h"


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

repo::core::model::RepoBSON* RepoManipulator::createCredBSON(
	const std::string &databaseAd,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested)
{
	core::model::RepoBSON* bson = 0;

	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		bson = handler->createBSONCredentials(databaseAd, username, password, pwDigested);

	return bson;
}

repo::core::model::RepoScene* RepoManipulator::createFederatedScene(
	const std::map<repo::core::model::TransformationNode, repo::core::model::ReferenceNode> &fedMap)
{

	repo::core::model::RepoNodeSet transNodes;
	repo::core::model::RepoNodeSet refNodes;
	repo::core::model::RepoNodeSet emptySet;

	repo::core::model::TransformationNode rootNode =
		repo::core::model::RepoBSONFactory::makeTransformationNode(
		repo::core::model::TransformationNode::identityMat(), "<root>");

	transNodes.insert(new repo::core::model::TransformationNode(rootNode));

	for (const auto &pair : fedMap)
	{

		transNodes.insert(new repo::core::model::TransformationNode(
			pair.first.cloneAndAddParent(rootNode.getSharedID())
			)
			);
		refNodes.insert(new repo::core::model::ReferenceNode(
			pair.second.cloneAndAddParent(pair.first.getSharedID())
			)
			);
	}
	//federate scene has no referenced files
	std::vector<std::string> empty;
	repo::core::model::RepoScene *scene =
		new repo::core::model::RepoScene(empty, emptySet, emptySet, emptySet, emptySet, emptySet, transNodes, refNodes);

	return scene;
}

repo::core::model::RepoScene* RepoManipulator::createMapScene(
	const repo::core::model::MapNode &mapNode)
{
	repo::core::model::RepoNodeSet transNodes;
	repo::core::model::RepoNodeSet mapNodes;
	repo::core::model::RepoNodeSet emptySet;

	repo::core::model::TransformationNode rootNode =
		repo::core::model::RepoBSONFactory::makeTransformationNode(
		repo::core::model::TransformationNode::identityMat(), "<root>");

	transNodes.insert(new repo::core::model::TransformationNode(rootNode));

	mapNodes.insert(new repo::core::model::MapNode(mapNode.cloneAndAddParent(rootNode.getSharedID())));


	//federate scene has no referenced files
	std::vector<std::string> empty;
	repo::core::model::RepoScene *scene =
		new repo::core::model::RepoScene(empty, emptySet, emptySet, emptySet, emptySet, emptySet, transNodes, emptySet, mapNodes);

	return scene;
}

void RepoManipulator::commitScene(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON 	  *cred,
	repo::core::model::RepoScene           *scene,
	const std::string                      &owner)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);

	std::string msg;
	if (handler && scene && scene->commit(handler, msg, owner.empty() ? cred->getStringField("user") : owner))
	{
		repoInfo << "Scene successfully committed to the database";
		if (scene->commitStash(handler, msg))
		{
			repoInfo << "Commited scene stash successfully.";
		}
		else
		{
			repoError << "Failed to commit scene stash : " << msg;
		}


	}
	else
	{
		repoError << "Error committing scene to the database : " << msg;
	}


}

void RepoManipulator::compareScenesByIDs(
	repo::core::model::RepoScene       *base,
	repo::core::model::RepoScene       *compare,
	std::vector<repoUUID>              &added,
	std::vector<repoUUID>              &deleted,
	std::vector<repoUUID>              &modified)
{
	diff::AbstractDiff *diff =  new diff::DiffBySharedID(base, compare);

	if (diff)
	{
		std::string msg;

		if (diff->isOk(msg))
		{
			added = diff->getNodesAdded();
			modified = diff->getNodesModified();
			deleted = diff->getNodesDeleted();
		}
		else
		{
			repoError << "Error on scene comparator: " << msg;
		}

		delete diff;
	}
	else
	{
		repoError << "Failed to instantiate 3D Diff comparator - out of memory?";
	}
	
}

uint64_t RepoManipulator::countItemsInCollection(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
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
	const repo::core::model::RepoBSON*	  cred,
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
	const repo::core::model::RepoBSON*	  cred,
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
	const repo::core::model::RepoBSON*	  cred
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
	const repo::core::model::RepoBSON*	  cred,
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

repo::core::model::RepoScene* RepoManipulator::fetchScene(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const std::string                             &database,
	const std::string                             &project,
	const repoUUID                                &uuid,
	const bool                                    &headRevision,
	const bool                                    &lightFetch)
{
	repo::core::model::RepoScene* scene = nullptr;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		//not setting a scene if we don't have a handler since we
		//can't retreive anything from the database.
		scene = new repo::core::model::RepoScene(database, project);
		if (scene)
		{
			if (headRevision)
				scene->setBranch(uuid);
			else
				scene->setRevision(uuid);

			std::string errMsg;
			if (scene->loadRevision(handler, errMsg))
			{
				repoTrace << "Loaded " <<
					(headRevision ? ("head revision of branch" + UUIDtoString(uuid))
					: ("revision " + UUIDtoString(uuid)))
					<< " of " << database << "." << project;
				if (lightFetch)
				{


						if (scene->loadStash(handler, errMsg))
						{
							repoTrace << "Stash Loaded";
						}
						else
						{
							//failed to load stash isn't critical, give it a warning instead of returning false
							repoWarning << "Error loading stash" << errMsg;
							if (scene->loadScene(handler, errMsg))
							{
								repoTrace << "Scene Loaded";
							}
							else{
								delete scene;
								scene = nullptr;
							}
						}
				}
				else
				{
					if (scene->loadScene(handler, errMsg))
					{
						repoTrace << "Loaded Scene";

						if (scene->loadStash(handler, errMsg))
						{
							repoTrace << "Stash Loaded";
						}
						else
						{
							//failed to load stash isn't critical, give it a warning instead of returning false
							repoWarning << "Error loading stash" << errMsg;
						}
					}
					else{
						delete scene;
						scene = nullptr;
					}
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
		else{
			repoError << "Failed to create a RepoScene(out of memory?)!";
		}
	}

	return scene;
}

void RepoManipulator::fetchScene(
	const std::string                     &databaseAd,
	const repo::core::model::RepoBSON     *cred,
	repo::core::model::RepoScene          *scene)
{
	if (scene)
	{
		if (scene->isRevisioned())
		{
			repo::core::handler::AbstractDatabaseHandler* handler =
				repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
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
					repoError << "Error loading stash: " << errMsg;
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

std::vector<repo::core::model::RepoBSON>
	RepoManipulator::getAllFromCollectionTailable(
		const std::string                             &databaseAd,
		const repo::core::model::RepoBSON*	  cred,
		const std::string                             &database,
		const std::string                             &collection,
		const uint64_t                                &skip,
		const uint32_t								  &limit)
{
	std::vector<repo::core::model::RepoBSON> vector;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		vector = handler->getAllFromCollectionTailable(database, collection, skip, limit);
	return vector;
}

std::vector<repo::core::model::RepoBSON>
	RepoManipulator::getAllFromCollectionTailable(
		const std::string                             &databaseAd,
		const repo::core::model::RepoBSON*	  cred,
		const std::string                             &database,
		const std::string                             &collection,
		const std::list<std::string>				  &fields,
		const std::string							  &sortField,
		const int									  &sortOrder,
		const uint64_t                                &skip,
		const uint32_t								  &limit)
{
	std::vector<repo::core::model::RepoBSON> vector;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		vector = handler->getAllFromCollectionTailable(database, collection, skip, limit, fields, sortField, sortOrder);
	return vector;
}

repo::core::model::CollectionStats RepoManipulator::getCollectionStats(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const std::string                             &database,
	const std::string                             &collection,
	std::string                                   &errMsg)
{

	repo::core::model::CollectionStats stats;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		stats = handler->getCollectionStats(database, collection, errMsg);

	return stats;
}

std::map<std::string, std::list<std::string>>
	RepoManipulator::getDatabasesWithProjects(
		const std::string                             &databaseAd,
		const repo::core::model::RepoBSON*	  cred,
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

repo::core::model::RepoNodeSet
	RepoManipulator::loadMetadataFromFile(
		const std::string &filePath,
		const char        &delimiter)
{
	repo::manipulator::modelconvertor::MetadataImportCSV metaImport;
	std::vector<std::string> tmp;

	return metaImport.readMetadata(filePath, tmp, delimiter);
}

repo::core::model::RepoScene*
	RepoManipulator::loadSceneFromFile(
	const std::string &filePath,
	      std::string &msg,
    const repo::manipulator::modelconvertor::ModelImportConfig *config)
{

	repo::core::model::RepoScene* scene = nullptr;

	repo::manipulator::modelconvertor::AssimpModelImport*
		modelConvertor = new repo::manipulator::modelconvertor::AssimpModelImport(config);



	if (modelConvertor)
	{
		repoTrace << "Importing model...";
		if (modelConvertor->importModel(filePath, msg))
		{
			repoTrace << "model Imported, generating Repo Scene";
			scene = modelConvertor->generateRepoScene();
		}
		else
		{
			repoError << "Failed to import model : " << msg;
		}

		delete modelConvertor;
	}
	else
	{
		msg += "Unable to instantiate a new modelConvertor (out of memory?)";
	}

	return scene;
}

void RepoManipulator::insertRole(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON	          *cred,
	const repo::core::model::RepoRole             &role)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->insertRole(role, errMsg))
		{
			repoInfo << "Role added successfully.";
		}
		else
		{
			repoError << "Failed to add role : " << errMsg;
		}
	}

}

void RepoManipulator::insertUser(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const repo::core::model::RepoUser       &user)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->insertUser(user, errMsg))
		{
			repoInfo << "User added successfully.";
		}
		else
		{
			repoError << "Failed to add user : " << errMsg;
		}
	}

}

void RepoManipulator::reduceTransformations(
	repo::core::model::RepoScene *scene)
{
	if (scene && scene->hasRoot())
	{
		modeloptimizer::TransformationReductionOptimizer optimizer;

		optimizer.apply(scene);
		//clear stash, it is not guaranteed to be relevant now.
		//The user needs to regenerate the stash if they want one for this version
		if (scene->isRevisioned())
		{
			scene->clearStash();
			//FIXME: There is currently no way to regenerate the stash. Give this warning message. 
			//In the future we may want to autogenerate the stash upon commit.
			repoWarning << "There is no stash associated with this optimised graph. Viewing may be impossible/slow should you commit this scene.";
		}

	}
	else{
		repoError << "RepoController::reduceTransformations: NULL pointer to scene/ Scene is not loaded!";
	}
}

void RepoManipulator::removeDocument(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const std::string                             &databaseName,
	const std::string                             &collectionName,
	const repo::core::model::RepoBSON       &bson)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->dropDocument(bson, databaseName, collectionName, errMsg))
		{
			repoInfo << "Document removed successfully.";
		}
		else
		{
			repoError << "Failed to remove document : " << errMsg;
		}
	}

}

void RepoManipulator::removeRole(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const repo::core::model::RepoRole       &role)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->dropRole(role, errMsg))
		{
			repoInfo << "Role removed successfully.";
		}
		else
		{
			repoError << "Failed to remove role : " << errMsg;
		}
	}

}

void RepoManipulator::removeUser(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const repo::core::model::RepoUser       &user)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->dropUser(user, errMsg))
		{
			repoInfo << "User removed successfully.";
		}
		else
		{
			repoError << "Failed to remove user : " << errMsg;
		}
	}

}

void RepoManipulator::saveOriginalFiles(
	const std::string                    &databaseAd,
	const repo::core::model::RepoBSON	 *cred,
	const repo::core::model::RepoScene   *scene,
	const std::string                    &directory)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler && scene)
	{
		std::string errMsg;

		const std::vector<std::string> files = scene->getOriginalFiles();
		if (files.size() > 0)
		{
			boost::filesystem::path dir(directory);
			/*if (boost::filesystem::create_directory(dir))
			{
				repoTrace << "Directory created: " << directory;
			}*/

			for (const std::string &file : files)
			{
				std::vector<uint8_t> rawFile = handler->getRawFile(scene->getDatabaseName(),
					scene->getProjectName() + "." + scene->getRawExtension(), file);
				if (rawFile.size() > 0)
				{
					boost::filesystem::path filePath(file);
					boost::filesystem::path fullPath = dir / filePath;


					std::ofstream out(fullPath.string(), std::ofstream::binary);
					if (out.good())
					{
						out.write((char*)rawFile.data(), rawFile.size());
						out.close();
					}
					else
					{
						repoError << " Failed to open file to write: " << fullPath.string();
					}
				}
				else
				{
					repoWarning << "Unable to read file " << file << " from the database. Skipping...";
				}
			}
		}
		

	}
}

bool RepoManipulator::saveSceneToFile(
	const std::string &filePath,
	const repo::core::model::RepoScene* scene)
{
	modelconvertor::AssimpModelExport modelExport;
	return modelExport.exportToFile(scene, filePath);
}

void RepoManipulator::updateRole(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const repo::core::model::RepoRole       &role)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->updateRole(role, errMsg))
		{
			repoInfo << "Role updated successfully.";
		}
		else
		{
			repoError << "Failed to update role : " << errMsg;
		}
	}

}

void RepoManipulator::updateUser(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const repo::core::model::RepoUser       &user)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->updateUser(user, errMsg))
		{
			repoInfo << "User updated successfully.";
		}
		else
		{
			repoError << "Failed to update user : " << errMsg;
		}
	}

}

void RepoManipulator::upsertDocument(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const std::string                             &databaseName,
	const std::string                             &collectionName,
	const repo::core::model::RepoBSON       &bson)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->upsertDocument(databaseName, collectionName, bson, true, errMsg))
		{
			repoInfo << "Document updated successfully.";
		}
		else
		{
			repoError << "Failed to remove document : " << errMsg;
		}
	}
}


