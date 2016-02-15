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
#include "diff/repo_diff_name.h"
#include "modelconvertor/export/repo_model_export_assimp.h"
#include "modelconvertor/export/repo_model_export_src.h"
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
	const std::string                      &databaseAd,
	const repo::core::model::RepoBSON 	   *cred,
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

			repoInfo << "Generating SRC encoding for web viewing...";
			if (generateAndCommitSRCBuffer(databaseAd, cred, scene))
			{
				repoInfo << "SRC file stored into the database";
			}			
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

void RepoManipulator::compareScenes(
	repo::core::model::RepoScene                  *base,
	repo::core::model::RepoScene                  *compare,
	repo::manipulator::diff::DiffResult           &baseResults,
	repo::manipulator::diff::DiffResult           &compResults,
	const diff::Mode					          &diffMode,
	const repo::core::model::RepoScene::GraphType &gType)
{
	diff::AbstractDiff *diff = nullptr; 
	
	switch (diffMode)
	{
	case diff::Mode::DIFF_BY_ID:
		diff = new diff::DiffBySharedID(base, compare, gType);
		break;
	case diff::Mode::DIFF_BY_NAME:
		diff = new diff::DiffByName(base, compare, gType);
		break;
	default:
		repoError << "Unknown diff mode: " << (int)diffMode;
	}
	

	if (diff)
	{
		std::string msg;

		if (diff->isOk(msg))
		{
			baseResults = diff->getDiffResultForBase();
			compResults = diff->getDiffResultForComp();
		}
		else
		{
			repoError << "Error on scene comparator: " << msg;
		}

		delete diff;
	}
	else
	{
		repoError << "Failed to instantiate 3D Diff comparator (unsupported diff mode/out of memory?)";
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
				repoInfo << "Loaded " <<
					(headRevision ? (" head revision of branch " + UUIDtoString(uuid))
					: (" revision " + UUIDtoString(uuid)))
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

bool RepoManipulator::generateAndCommitSRCBuffer(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON	          *cred,
	const repo::core::model::RepoScene            *scene)
{
	bool success;
	modelconvertor::repo_src_export_t v = generateSRCBuffer(scene);
	if (success = (v.srcFiles.size() + v.x3dFiles.size() + v.jsonFiles.size()))
	{
		repo::core::handler::AbstractDatabaseHandler* handler =
			repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
		if (success = handler)
		{

			for (const auto bufferPair : v.srcFiles)
			{
				std::string databaseName = scene->getDatabaseName();
				std::string projectName = scene->getProjectName();
				std::string prefix = "/" + databaseName + "/" + projectName + "/";
				std::string errMsg;
				//FIXME: constant value somewhere for .stash.src?
				std::string fileName = prefix+bufferPair.first;
				if (handler->insertRawFile(scene->getDatabaseName(), scene->getProjectName() + ".stash.src", fileName, bufferPair.second,
					errMsg, "binary/octet-stream"))
				{
					repoInfo << "File ("<<fileName <<") added successfully.";
				}
				else
				{
					repoError << "Failed to add file  ("<<fileName <<"): " << errMsg;
				}
			}		
			for (const auto bufferPair : v.x3dFiles)
			{
				std::string databaseName = scene->getDatabaseName();
				std::string projectName = scene->getProjectName();
				std::string errMsg;
				//FIXME: constant value somewhere for .stash.x3d?
				std::string fileName = bufferPair.first;
				if (handler->insertRawFile(scene->getDatabaseName(), scene->getProjectName() + ".stash.x3d", fileName, bufferPair.second,
					errMsg, "binary/octet-stream"))
				{
					repoInfo << "File (" << fileName << ") added successfully.";
				}
				else
				{
					repoError << "Failed to add file  (" << fileName << "): " << errMsg;
				}
			}

			for (const auto bufferPair : v.jsonFiles)
			{
				std::string databaseName = scene->getDatabaseName();
				std::string projectName = scene->getProjectName();
				std::string errMsg;
				//FIXME: constant value somewhere for .stash.x3d?
				std::string fileName = bufferPair.first;
				if (handler->insertRawFile(scene->getDatabaseName(), scene->getProjectName() + ".stash.json_mpc", fileName, bufferPair.second,
					errMsg, "binary/octet-stream"))
				{
					repoInfo << "File (" << fileName << ") added successfully.";
				}
				else
				{
					repoError << "Failed to add file  (" << fileName << "): " << errMsg;
				}
			}
		}
	}

	return success;

}

modelconvertor::repo_gltf_export_t RepoManipulator::generateGLTFBuffer(
	const repo::core::model::RepoScene *scene)
{

	modelconvertor::repo_gltf_export_t result;
	modelconvertor::GLTFModelExport gltfExport(scene);
	if (gltfExport.isOk())
	{
		repoTrace << "Conversion succeed.. exporting as buffer..";
		result = gltfExport.getAllFilesExportedAsBuffer();
	}
	else
		repoError << "Export to GLTF failed.";

	return result;
}


modelconvertor::repo_src_export_t RepoManipulator::generateSRCBuffer(
	const repo::core::model::RepoScene *scene)
{

	modelconvertor::repo_src_export_t result;
	modelconvertor::SRCModelExport srcExport(scene);
	if (srcExport.isOk())
	{
		repoTrace << "Conversion succeed.. exporting as buffer..";
		result = srcExport.getAllFilesExportedAsBuffer();
	}
	else
		repoError << "Export to SRC failed.";

	return result;
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

repo::core::model::RepoRoleSettings RepoManipulator::getRoleSettingByName(
	const std::string                   &databaseAd,
	const repo::core::model::RepoBSON	*cred,
	const std::string					&database,
	const std::string					&uniqueRoleName
	)
{

	repo::core::model::RepoRoleSettings settings;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	repo::core::model::RepoBSONBuilder builder;
	builder << REPO_LABEL_ID << uniqueRoleName;
	if (handler)
		settings = repo::core::model::RepoRoleSettings(
			handler->findOneByCriteria(database, REPO_COLLECTION_SETTINGS_ROLES, builder.obj()));
	return settings;
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
	const bool &applyReduction,
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
			if ((scene = modelConvertor->generateRepoScene()) && applyReduction)
			{
				repoTrace << "Scene generated. Applying transformation reduction optimizer";
				modeloptimizer::TransformationReductionOptimizer optimizer;
				optimizer.apply(scene);
			}
			
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

void RepoManipulator::insertBinaryFileToDatabase(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON	          *cred,
	const std::string                             &database,
	const std::string                             &collection,
	const std::string                             &name,
	const std::vector<uint8_t>                    &rawData,
	const std::string                             &mimeType)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		std::string errMsg;
		if (handler->insertRawFile(database, collection, name, rawData, errMsg, mimeType))
		{
			repoInfo << "File ("<< name <<") added successfully.";
		}
		else
		{
			repoError << "Failed to add file ("<< name <<"): " << errMsg;
		}
	}
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
	repo::core::model::RepoScene *scene,
	const repo::core::model::RepoScene::GraphType &gType)
{
	if (scene && scene->hasRoot(gType))
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


