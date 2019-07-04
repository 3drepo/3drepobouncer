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
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

#include "../core/handler/repo_database_handler_mongo.h"
#include "../core/handler/fileservice/repo_file_manager.h"
#include "../core/model/bson/repo_bson_factory.h"
#include "../error_codes.h"
#include "../lib/repo_log.h"
#include "../lib/repo_config.h"
#include "diff/repo_diff_name.h"
#include "diff/repo_diff_sharedid.h"
#include "modelconvertor/import/repo_model_import_assimp.h"
#include "modelconvertor/import/repo_model_import_ifc.h"
#include "modelconvertor/import/repo_model_import_3drepo.h"
#include "modelconvertor/import/repo_model_import_oda.h"
#include "modelconvertor/export/repo_model_export_assimp.h"
#include "modelconvertor/export/repo_model_export_asset.h"
#include "modelconvertor/import/repo_metadata_import_csv.h"
#include "modeloptimizer/repo_optimizer_trans_reduction.h"
#include "modeloptimizer/repo_optimizer_ifc.h"
#include "modelutility/repo_scene_manager.h"
#include "modelutility/spatialpartitioning/repo_spatial_partitioner_rdtree.h"
#include "statistics/repo_statistics_generator.h"
#include "repo_manipulator.h"

using namespace repo::manipulator;

RepoManipulator::RepoManipulator()
{
}

RepoManipulator::~RepoManipulator()
{
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
	core::model::RepoBSON* bson =
		repo::core::handler::MongoDatabaseHandler::createBSONCredentials(databaseAd, username, password, pwDigested);

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
		repo::lib::RepoMatrix(), "<root>");

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

bool RepoManipulator::commitAssetBundleBuffers(
	const std::string                     &databaseAd,
	const repo::core::model::RepoBSON     *cred,
	const std::string                     &bucketName,
	const std::string                     &bucketRegion,
	repo::core::model::RepoScene          *scene,
	const repo_web_buffers_t              &buffers)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	auto manager = repo::core::handler::fileservice::FileManager::getManager();
	modelutility::SceneManager SceneManager;
	return SceneManager.commitWebBuffers(scene, scene->getUnityExtension(), buffers, handler, manager, true);
}

bool RepoManipulator::commitScene(
	const std::string                      &databaseAd,
	const repo::core::model::RepoBSON      *cred,
	const std::string                      &bucketName,
	const std::string                      &bucketRegion,
	repo::core::model::RepoScene           *scene,
	const std::string                      &owner,
	const std::string                      &tag,
	const std::string                      &desc)
{
	repoLog("Manipulator: Committing model to database");
	bool success = false;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	auto manager = repo::core::handler::fileservice::FileManager::getManager();
	std::string projOwner = owner.empty() ? cred->getStringField("user") : owner;

	std::string msg;
	//Check if database exists
	std::string dbName = scene->getDatabaseName();
	std::string projectName = scene->getProjectName();
	if (dbName.empty() || projectName.empty())
	{
		repoError << "Failed to commit scene : database name or project name is empty!";
	}

	if (handler && scene && scene->commit(handler, manager, msg, projOwner, desc, tag))
	{
		repoInfo << "Scene successfully committed to the database";
		if (!(success = (scene->getAllReferences(repo::core::model::RepoScene::GraphType::DEFAULT).size())))
		{
			if (!scene->hasRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED)){
				repoInfo << "Optimised scene not found. Attempt to generate...";
				success = generateAndCommitStashGraph(databaseAd, cred, scene);
			}
			else if (success = scene->commitStash(handler, msg))
			{
				repoInfo << "Commited scene stash successfully.";
			}
			else
			{
				repoError << "Failed to commit scene stash : " << msg;
			}			
		}

		if (success)
		{
			repoInfo << "Generating Selection Tree JSON...";
			if (generateAndCommitSelectionTree(databaseAd, cred, bucketName, bucketRegion, scene))
				repoInfo << "Selection Tree Stored into the database";
			else
				repoError << "failed to commit selection tree";
		}
	}
	else
	{
		if (!handler)
			msg += "Failed to connect to database";
		if (!scene)
			msg += "Trying to commit a scene that does not exist!";
		repoError << "Error committing scene to the database : " << msg;
	}

	return success;
}

void RepoManipulator::compareScenes(
	repo::core::model::RepoScene                  *base,
	repo::core::model::RepoScene                  *compare,
	repo_diff_result_t           &baseResults,
	repo_diff_result_t           &compResults,
	const repo::DiffMode					          &diffMode,
	const repo::core::model::RepoScene::GraphType &gType)
{
	diff::AbstractDiff *diff = nullptr;

	switch (diffMode)
	{
	case repo::DiffMode::DIFF_BY_ID:
		diff = new diff::DiffBySharedID(base, compare, gType);
		break;
	case repo::DiffMode::DIFF_BY_NAME:
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
			baseResults = diff->getrepo_diff_result_tForBase();
			compResults = diff->getrepo_diff_result_tForComp();
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

void RepoManipulator::disconnectFromDatabase(const std::string &databaseAd)
{
	//FIXME: can only kill mongo here, but this is suppose to be a quick fix
	core::handler::MongoDatabaseHandler::disconnectHandler();
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
	{
		success = handler->dropDatabase(databaseName, errMsg);

		//remove all roles belonging to this database
		repo::core::model::RepoBSON criteria = BSON(REPO_ROLE_LABEL_DATABASE << databaseName);
		for (const auto role : handler->findAllByCriteria(REPO_ADMIN, REPO_SYSTEM_ROLES, criteria))
		{
			removeRole(databaseAd, cred, repo::core::model::RepoRole(role));
		}

		//remove privileges associated with this db
		std::string fieldName = REPO_ROLE_LABEL_PRIVILEGES + std::string(".") + REPO_ROLE_LABEL_RESOURCE + "." + REPO_ROLE_LABEL_DATABASE;
		repo::core::model::RepoBSON criteria2 = BSON(fieldName << databaseName);
		for (const auto role : handler->findAllByCriteria(REPO_ADMIN, REPO_SYSTEM_ROLES, criteria2))
		{
			auto roleBson = repo::core::model::RepoRole(role);
			auto privileges = roleBson.getPrivileges();
			int index = 0;
			while (index < privileges.size())
			{
				if (privileges[index].database == databaseName)
				{
					privileges.erase(privileges.begin() + index);
				}
				else
				{
					index++;
				}
			}

			if (privileges.size())
			{
				updateRole(databaseAd, cred, roleBson.cloneAndUpdatePrivileges(privileges));
			}
			else
			{
				removeRole(databaseAd, cred, roleBson);
			}
		}
	}
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
	const repo::core::model::RepoBSON             *cred,
	const std::string                             &database,
	const std::string                             &project,
	const repo::lib::RepoUUID                                &uuid,
	const bool                                    &headRevision,
	const bool                                    &lightFetch,
	const bool                                    &ignoreRefScene,
	const bool                                    &skeletonFetch)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	modelutility::SceneManager sceneManager;
	return sceneManager.fetchScene(handler, database, project, uuid, headRevision, lightFetch, ignoreRefScene, skeletonFetch);
}

void RepoManipulator::fetchScene(
	const std::string                     &databaseAd,
	const repo::core::model::RepoBSON     *cred,
	repo::core::model::RepoScene          *scene,
	const bool                            &ignoreRefScene,
	const bool                            &skeletonFetch)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	modelutility::SceneManager sceneManager;
	return sceneManager.fetchScene(handler, scene);
}

repo::core::model::RepoRole RepoManipulator::findRole(
	const std::string                      &databaseAd,
	const repo::core::model::RepoBSON 	   *cred,
	const std::string                      &dbName,
	const std::string                      &roleName
	)
{
	repo::core::model::RepoRole role;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (!handler)
	{
		repoError << "Failed to retrieve database handler to perform the operation!";
	}
	else
	{
		repo::core::model::RepoBSONBuilder builder;
		builder.append(REPO_LABEL_ROLE, roleName);

		role = repo::core::model::RepoRole(
			handler->findOneByCriteria(REPO_ADMIN, REPO_SYSTEM_ROLES, builder.obj()));
	}

	return role;
}

repo::core::model::RepoUser RepoManipulator::findUser(
	const std::string                      &databaseAd,
	const repo::core::model::RepoBSON 	   *cred,
	const std::string                      &username
	)
{
	repo::core::model::RepoUser user;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (!handler)
	{
		repoError << "Failed to retrieve database handler to perform the operation!";
	}
	else
	{
		repo::core::model::RepoBSONBuilder builder;
		builder.append(REPO_LABEL_USER, username);

		user = repo::core::model::RepoUser(
			handler->findOneByCriteria(REPO_ADMIN, REPO_SYSTEM_USERS, builder.obj()));
	}

	return user;
}

bool RepoManipulator::generateAndCommitGLTFBuffer(
	const std::string                     &databaseAd,
	const repo::core::model::RepoBSON     *cred,
	const std::string                     &bucketName,
	const std::string                     &bucketRegion,
	repo::core::model::RepoScene          *scene)
{
	repo_web_buffers_t buffers;
	return generateAndCommitWebViewBuffer(databaseAd, cred, bucketName, bucketRegion, scene,
		buffers, modelconvertor::WebExportType::GLTF);
}

bool RepoManipulator::generateAndCommitSRCBuffer(
	const std::string                     &databaseAd,
	const repo::core::model::RepoBSON     *cred,
	const std::string                     &bucketName,
	const std::string                     &bucketRegion,
	repo::core::model::RepoScene          *scene)
{
	repo_web_buffers_t buffers;
	return generateAndCommitWebViewBuffer(databaseAd, cred, bucketName, bucketRegion, scene,
		buffers, modelconvertor::WebExportType::SRC);
}

bool RepoManipulator::generateAndCommitSelectionTree(
	const std::string                     &databaseAd,
	const repo::core::model::RepoBSON     *cred,
	const std::string                     &bucketName,
	const std::string                     &bucketRegion,
	repo::core::model::RepoScene          *scene
	)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	auto manager = repo::core::handler::fileservice::FileManager::getManager();

	modelutility::SceneManager SceneManager;

	return SceneManager.generateAndCommitSelectionTree(scene, handler, manager);
}

bool RepoManipulator::removeStashGraphFromDatabase(
	const std::string                         &databaseAd,
	const repo::core::model::RepoBSON         *cred,
	repo::core::model::RepoScene* scene
	)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	modelutility::SceneManager SceneManager;
	return SceneManager.removeStashGraph(scene, handler);
}

bool RepoManipulator::generateStashGraph(
	repo::core::model::RepoScene              *scene
	)
{
	modelutility::SceneManager SceneManager;
	return SceneManager.generateStashGraph(scene, nullptr);
}

bool RepoManipulator::generateAndCommitStashGraph(
	const std::string                         &databaseAd,
	const repo::core::model::RepoBSON         *cred,
	repo::core::model::RepoScene              *scene
	)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	modelutility::SceneManager SceneManager;
	return SceneManager.generateStashGraph(scene, handler);
}

bool RepoManipulator::generateAndCommitWebViewBuffer(
	const std::string                     &databaseAd,
	const repo::core::model::RepoBSON     *cred,
	const std::string                     &bucketName,
	const std::string                     &bucketRegion,
	repo::core::model::RepoScene          *scene,
	repo_web_buffers_t                    &buffers,
	const modelconvertor::WebExportType   &exType)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	auto manager = repo::core::handler::fileservice::FileManager::getManager();
	modelutility::SceneManager SceneManager;
	return SceneManager.generateWebViewBuffers(scene, exType, buffers, handler, manager);
}

repo_web_buffers_t RepoManipulator::generateGLTFBuffer(
	repo::core::model::RepoScene *scene)
{
	repo_web_buffers_t buffers;
	modelutility::SceneManager SceneManager;
	SceneManager.generateWebViewBuffers(scene, modelconvertor::WebExportType::GLTF, buffers, nullptr);
	return buffers;
}

repo_web_buffers_t RepoManipulator::generateSRCBuffer(
	repo::core::model::RepoScene *scene)
{
	repo_web_buffers_t buffers;
	modelutility::SceneManager SceneManager;
	SceneManager.generateWebViewBuffers(scene, modelconvertor::WebExportType::SRC, buffers, nullptr);
	return buffers;
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

repo::core::model::DatabaseStats RepoManipulator::getDatabaseStats(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const std::string                             &database,
	std::string                                   &errMsg)
{
	repo::core::model::DatabaseStats stats;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		stats = handler->getDatabaseStats(database, errMsg);

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

void RepoManipulator::getDatabaseStatistics(
	const std::string                     &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const std::string &outputFilePath)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	StatisticsGenerator statGen(handler);
	statGen.getDatabaseStatistics(outputFilePath);
}

void RepoManipulator::getUserList(
	const std::string                     &databaseAd,
	const repo::core::model::RepoBSON*	  cred,
	const std::string &outputFilePath)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	StatisticsGenerator statGen(handler);
	statGen.getUserList(outputFilePath);
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

std::shared_ptr<repo_partitioning_tree_t>
RepoManipulator::getScenePartitioning(
const repo::core::model::RepoScene *scene,
const uint32_t                     &maxDepth
)
{
	modelutility::RDTreeSpatialPartitioner partitioner(scene, maxDepth);
	return partitioner.partitionScene();
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
uint8_t &error,
const bool &applyReduction,
const bool &rotateModel,
const repo::manipulator::modelconvertor::ModelImportConfig *config)
{
	repo::core::model::RepoScene* scene = nullptr;

	boost::filesystem::path filePathP(filePath);
	std::string fileExt = filePathP.extension().string();

	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);
	
	repo::manipulator::modelconvertor::AbstractModelImport* modelConvertor = nullptr;

	bool useAssimpImporter = repo::manipulator::modelconvertor::AssimpModelImport::isSupportedExts(fileExt);
	bool useIFCImporter = fileExt == ".IFC" && (!config || config->getUseIFCOpenShell());
	bool useRepoImporter = fileExt == ".BIM";
	bool useOdaImporter = repo::manipulator::modelconvertor::OdaModelImport::isSupportedExts(fileExt);

	if (useIFCImporter)
	{
		modelConvertor = new repo::manipulator::modelconvertor::IFCModelImport(config);
	}
	else if (useRepoImporter)
	{
		modelConvertor = new repo::manipulator::modelconvertor::RepoModelImport(config);
	}
	else if (useOdaImporter)
	{
		modelConvertor = new repo::manipulator::modelconvertor::OdaModelImport(); //FIXME: take in config like everything else.
	}
	else if (useAssimpImporter)
	{
		modelConvertor = new repo::manipulator::modelconvertor::AssimpModelImport(config);
	}
	else
	{
		error = REPOERR_FILE_TYPE_NOT_SUPPORTED;
		return nullptr;
	}

	if (modelConvertor)
	{
		repoTrace << "Importing model...";
		if (modelConvertor->importModel(filePath, error))
		{
			repoTrace << "model Imported, generating Repo Scene";
			if ((scene = modelConvertor->generateRepoScene()))
			{
				if (scene->exceedsMaximumNodes()) {
					delete scene;
					error = REPOERR_MAX_NODES_EXCEEDED;
					return nullptr;
				}

				if (!scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT).size()) {

					delete scene;
					error = REPOERR_NO_MESHES;
					return nullptr;
				}
				if (rotateModel || useIFCImporter || useOdaImporter)
				{
					repoTrace << "rotating model by 270 degress on the x axis...";
					scene->reorientateDirectXModel();
				}

				if (applyReduction && !useOdaImporter)
				{
					repoTrace << "Scene generated. Applying transformation reduction optimizer";
					modeloptimizer::TransformationReductionOptimizer optimizer;
					optimizer.apply(scene);
				}

				//Generate stash
				repoInfo << "Generating stash graph for optimised viewing...";
				if (generateStashGraph(scene))
				{
					repoTrace << "Stash graph generated.";
				}
				else
				{
					repoError << "Error generating stash graph";
					error = REPOERR_STASH_GEN_FAIL;
					delete scene;
					return nullptr;
				}
				error = REPOERR_OK;
			}
			else
			{
				error = REPOERR_UNKNOWN_ERR;

			}
		}
		delete modelConvertor;
	}
	else
		error = REPOERR_UNKNOWN_ERR;
	return scene;
}

bool RepoManipulator::hasCollection(
	const std::string                      &databaseAd,
	const repo::core::model::RepoBSON 	   *cred,
	const std::string                      &dbName,
	const std::string                      &project)
{
	std::list<std::string> dList = { dbName };
	auto databaseList = getDatabasesWithProjects(databaseAd, cred, dList);
	if (!databaseList.size()) return false;
	auto collectionList = databaseList.begin()->second;
	auto findIt = std::find(collectionList.begin(), collectionList.end(), project);
	return findIt != collectionList.end();
}

bool RepoManipulator::hasDatabase(
	const std::string                      &databaseAd,
	const repo::core::model::RepoBSON 	   *cred,
	const std::string &dbName)
{
	auto databaseList = fetchDatabases(databaseAd, cred);
	auto findIt = std::find(databaseList.begin(), databaseList.end(), dbName);
	return findIt != databaseList.end();
}

bool RepoManipulator::init(
	std::string       &errMsg,
	const repo::lib::RepoConfig  &config,
	const int            &nDbConnections
) {
	auto dbConf = config.getDatabaseConfig();
	bool success = true;
	if (success = connectAndAuthenticateWithAdmin(errMsg, dbConf.addr, dbConf.port, nDbConnections, dbConf.username, dbConf.password)) {
		repo::core::handler::AbstractDatabaseHandler* handler =
			repo::core::handler::MongoDatabaseHandler::getHandler(dbConf.addr);
		success = (bool) repo::core::handler::fileservice::FileManager::instantiateManager(config, handler);
	}

	return success;
}

std::vector<std::shared_ptr<repo::core::model::MeshNode>> RepoManipulator::initialiseAssetBuffer(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON	          *cred,
	repo::core::model::RepoScene *scene,
	std::unordered_map<std::string, std::vector<uint8_t>> &jsonFiles,
	repo::core::model::RepoUnityAssets &unityAssets,
	std::vector<std::vector<uint16_t>> &serialisedFaceBuf,
	std::vector<std::vector<std::vector<float>>> &idMapBuf,
	std::vector<std::vector<std::vector<repo_mesh_mapping_t>>> &meshMappings)
{
	bool vrEnabled = false;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
	{
		modelutility::SceneManager sceneManager;
		vrEnabled = sceneManager.isVrEnabled(scene, handler);

		scene->updateRevisionStatus(handler, repo::core::model::RevisionNode::UploadStatus::GEN_WEB_STASH);
	}
	repo::manipulator::modelconvertor::AssetModelExport assetExport(scene, vrEnabled);
	jsonFiles = assetExport.getJSONFilesAsBuffer();
	unityAssets = assetExport.getUnityAssets();
	return assetExport.getReorganisedMeshes(serialisedFaceBuf, idMapBuf, meshMappings);
}

bool RepoManipulator::insertBinaryFileToDatabase(
	const std::string                             &databaseAd,
	const repo::core::model::RepoBSON             *cred,
	const std::string                             &bucketName,
	const std::string                             &bucketRegion,
	const std::string                             &database,
	const std::string                             &collection,
	const std::string                             &name,
	const std::vector<uint8_t>                    &rawData,
	const std::string                             &mimeType)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	auto manager = repo::core::handler::fileservice::FileManager::getManager();
	bool success = false;
	if (handler && manager)
	{
		if (success = manager->uploadFileAndCommit(database, collection, name, rawData))
		{
			repoInfo << "File (" << name << ") added successfully to storage.";
		}
		else
		{
			repoError << "Failed to add file (" << name << ") to storage.";
		}

	}

	return success;
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

bool RepoManipulator::isVREnabled(
	const std::string                       &databaseAd,
	const repo::core::model::RepoBSON       *cred,
	const repo::core::model::RepoScene      *scene) const
{
	modelutility::SceneManager manager;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	bool isVREnabled = false;
	if (handler)
	{
		isVREnabled = manager.isVrEnabled(scene, handler);
	}
	return isVREnabled;
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

bool RepoManipulator::removeProject(
	const std::string                        &databaseAd,
	const repo::core::model::RepoBSON        *cred,
	const std::string                        &databaseName,
	const std::string                        &projectName,
	std::string								 &errMsg
	)
{
	bool success = true;
	//Remove entry from project settings
	repo::core::model::RepoBSON criteria = BSON(REPO_LABEL_ID << projectName);
	removeDocument(databaseAd, cred, databaseName, REPO_COLLECTION_SETTINGS, criteria);

	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);

	//Remove all the collections
	for (const auto &ext : repo::core::model::RepoScene::getProjectExtensions())
	{
		std::string collectionName = projectName + "." + ext;
		bool droppedCol = dropCollection(databaseAd, cred, databaseName, collectionName, errMsg);
		//if drop collection failed with no errMsg = failed because the collection didn't exist  should be considered as a success
		if (!(!droppedCol && errMsg.empty()))
		{
			success &= droppedCol;
		}

		//find all roles with a privilege of this collection and remove it
		repo::core::model::RepoBSON privCriteria = BSON("privileges" <<
			BSON("$elemMatch" <<
			BSON("resource" << BSON("db" << databaseName << "collection" << collectionName))
			)
			);

		//FIXME: should get this from handler to ensure it's correct for non mongo databases (future proof)
		auto results = handler->findAllByCriteria(REPO_ADMIN, REPO_SYSTEM_ROLES, privCriteria);

		for (const auto &roleBSON : results)
		{
			repo::core::model::RepoRole role = repo::core::model::RepoRole(roleBSON);
			auto privilegesMap = role.getPrivilegesMapped();
			privilegesMap.erase(databaseName + "." + collectionName);

			std::vector<repo::core::model::RepoPrivilege> privilegesUpdated;
			boost::copy(
				privilegesMap | boost::adaptors::map_values,
				std::back_inserter(privilegesUpdated));
			role = role.cloneAndUpdatePrivileges(privilegesUpdated);

			success &= handler->updateRole(role, errMsg);
		}
		if (!success) break;
	}

	return success;
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

bool RepoManipulator::saveOriginalFiles(
	const std::string                    &databaseAd,
	const repo::core::model::RepoBSON	 *cred,
	const repo::core::model::RepoScene   *scene,
	const std::string                    &directory)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	bool success = false;
	if (handler && scene)
	{
		std::string errMsg;

		const std::vector<std::string> files = scene->getOriginalFiles();
		if (success = files.size() > 0)
		{
			boost::filesystem::path dir(directory);

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
						success = false;
					}
				}
				else
				{
					repoWarning << "Unable to read file " << file << " from the database. Skipping...";
				}
			}
		}
	}

	return success;
}

bool RepoManipulator::saveOriginalFiles(
	const std::string                    &databaseAd,
	const repo::core::model::RepoBSON	 *cred,
	const std::string                    &database,
	const std::string                    &project,
	const std::string                    &directory)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	bool success = false;
	auto scene = new repo::core::model::RepoScene(database, project);
	std::string errMsg;
	if (scene && scene->loadRevision(handler, errMsg))
	{
		success = saveOriginalFiles(databaseAd, cred, scene, directory);
	}
	else
	{
		repoError << "Failed to fetch project from the database!" << (errMsg.empty() ? "" : errMsg);
	}
	return success;
}

bool RepoManipulator::saveSceneToFile(
	const std::string &filePath,
	const repo::core::model::RepoScene* scene)
{
	modelconvertor::AssimpModelExport modelExport(scene);
	return modelExport.exportToFile(filePath);
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
