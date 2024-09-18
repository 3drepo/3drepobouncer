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
#include "modelconvertor/import/repo_drawing_import_manager.h"
#include "modelconvertor/import/repo_model_import_manager.h"
#include "modelconvertor/export/repo_model_export_assimp.h"
#include "modelconvertor/import/repo_metadata_import_csv.h"
#include "modelutility/repo_scene_manager.h"
#include "modelutility/spatialpartitioning/repo_spatial_partitioner_rdtree.h"
#include "modelutility/repo_drawing_manager.h"
#include "modeloptimizer/repo_optimizer_trans_reduction.h"
#include "repo_manipulator.h"

using namespace repo::manipulator;

RepoManipulator::RepoManipulator()
{
}

RepoManipulator::~RepoManipulator()
{
}

bool RepoManipulator::connectAndAuthenticateWithAdmin(
	std::string& errMsg,
	const std::string& address,
	const uint32_t& port,
	const uint32_t& maxConnections,
	const std::string& username,
	const std::string& password,
	const bool& pwDigested
)
{
	//FIXME: we should have a database manager class that will instantiate new handlers/give existing handlers
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(
			errMsg, address, port, maxConnections,
			repo::core::handler::MongoDatabaseHandler::getAdminDatabaseName(),
			username, password, pwDigested);

	return handler != 0;
}

bool RepoManipulator::connectAndAuthenticateWithAdmin(
	std::string& errMsg,
	const std::string& connString,
	const uint32_t& maxConnections,
	const std::string& username,
	const std::string& password,
	const bool& pwDigested
)
{
	//FIXME: we should have a database manager class that will instantiate new handlers/give existing handlers
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(
			errMsg, connString, maxConnections,
			repo::core::handler::MongoDatabaseHandler::getAdminDatabaseName(),
			username, password, pwDigested);

	return handler != 0;
}

repo::core::model::RepoBSON* RepoManipulator::createCredBSON(
	const std::string& databaseAd,
	const std::string& username,
	const std::string& password,
	const bool& pwDigested)
{
	core::model::RepoBSON* bson =
		repo::core::handler::MongoDatabaseHandler::createBSONCredentials(databaseAd, username, password, pwDigested);

	return bson;
}

repo::core::model::RepoScene* RepoManipulator::createFederatedScene(
	const std::map<repo::core::model::ReferenceNode, std::string>& fedMap)
{
	repo::core::model::RepoNodeSet transNodes;
	repo::core::model::RepoNodeSet refNodes;
	repo::core::model::RepoNodeSet emptySet;

	auto rootNode = new repo::core::model::TransformationNode(
		repo::core::model::RepoBSONFactory::makeTransformationNode(
			repo::lib::RepoMatrix(), "Federation"));

	transNodes.insert(rootNode);

	std::map<std::string, repo::core::model::TransformationNode*> groupNameToNode;

	for (const auto& pair : fedMap)
	{
		auto parentNode = rootNode;
		if (!pair.second.empty()) {
			if (groupNameToNode.find(pair.second) == groupNameToNode.end()) {
				groupNameToNode[pair.second] = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode(
					repo::lib::RepoMatrix(), pair.second, { rootNode->getSharedID() }));
				transNodes.insert(groupNameToNode[pair.second]);
			}

			parentNode = groupNameToNode[pair.second];
		}

		refNodes.insert(new repo::core::model::ReferenceNode(
			pair.first.cloneAndAddParent(parentNode->getSharedID())
		));
	}
	//federate scene has no referenced files
	std::vector<std::string> empty;
	repo::core::model::RepoScene* scene =
		new repo::core::model::RepoScene(empty, emptySet, emptySet, emptySet, emptySet, emptySet, transNodes, refNodes);

	return scene;
}

uint8_t RepoManipulator::commitScene(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& bucketName,
	const std::string& bucketRegion,
	repo::core::model::RepoScene* scene,
	const std::string& owner,
	const std::string& tag,
	const std::string& desc,
	const repo::lib::RepoUUID& revId)
{
	repoLog("Manipulator: Committing model to database");
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	auto manager = repo::core::handler::fileservice::FileManager::getManager();
	std::string projOwner = owner.empty() ? cred->getStringField("user") : owner;

	//Check if database exists
	std::string dbName = scene->getDatabaseName();
	std::string projectName = scene->getProjectName();
	if (dbName.empty() || projectName.empty())
	{
		repoError << "Failed to commit scene : database name or project name is empty!";
		return REPOERR_UPLOAD_FAILED;
	}

	modelutility::SceneManager sceneManager;
	return sceneManager.commitScene(scene, projOwner, tag, desc, revId, handler, manager);
}

uint64_t RepoManipulator::countItemsInCollection(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& database,
	const std::string& collection,
	std::string& errMsg)
{
	uint64_t numItems;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);

	if (handler)
		numItems = handler->countItemsInCollection(database, collection, errMsg);

	return numItems;
}

void RepoManipulator::disconnectFromDatabase(const std::string& databaseAd)
{
	//FIXME: can only kill mongo here, but this is suppose to be a quick fix
	core::handler::MongoDatabaseHandler::disconnectHandler();
}

bool RepoManipulator::dropCollection(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& databaseName,
	const std::string& collectionName,
	std::string& errMsg
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

std::list<std::string> RepoManipulator::fetchDatabases(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred
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
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& database
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
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& database,
	const std::string& project,
	const repo::lib::RepoUUID& uuid,
	const bool& headRevision,
	const bool& ignoreRefScene,
	const bool& skeletonFetch,
	const std::vector<repo::core::model::RevisionNode::UploadStatus>& includeStatus)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	modelutility::SceneManager sceneManager;
	return sceneManager.fetchScene(handler, database, project, uuid, headRevision, ignoreRefScene, skeletonFetch, includeStatus);
}

void RepoManipulator::fetchScene(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	repo::core::model::RepoScene* scene,
	const bool& ignoreRefScene,
	const bool& skeletonFetch)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	modelutility::SceneManager sceneManager;
	return sceneManager.fetchScene(handler, scene);
}

bool RepoManipulator::generateAndCommitRepoBundlesBuffer(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& bucketName,
	const std::string& bucketRegion,
	repo::core::model::RepoScene* scene)
{
	repo_web_buffers_t buffers;
	return generateAndCommitWebViewBuffer(databaseAd, cred, bucketName, bucketRegion, scene,
		buffers, modelconvertor::WebExportType::REPO);
}

bool RepoManipulator::generateAndCommitSRCBuffer(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& bucketName,
	const std::string& bucketRegion,
	repo::core::model::RepoScene* scene)
{
	repo_web_buffers_t buffers;
	return generateAndCommitWebViewBuffer(databaseAd, cred, bucketName, bucketRegion, scene,
		buffers, modelconvertor::WebExportType::SRC);
}

bool RepoManipulator::generateAndCommitSelectionTree(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& bucketName,
	const std::string& bucketRegion,
	repo::core::model::RepoScene* scene
)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	auto manager = repo::core::handler::fileservice::FileManager::getManager();

	modelutility::SceneManager SceneManager;

	return SceneManager.generateAndCommitSelectionTree(scene, handler, manager);
}

bool RepoManipulator::generateStashGraph(
	repo::core::model::RepoScene* scene
)
{
	modelutility::SceneManager SceneManager;
	return SceneManager.generateStashGraph(scene);
}

bool RepoManipulator::generateAndCommitWebViewBuffer(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& bucketName,
	const std::string& bucketRegion,
	repo::core::model::RepoScene* scene,
	repo_web_buffers_t& buffers,
	const modelconvertor::WebExportType& exType)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	auto manager = repo::core::handler::fileservice::FileManager::getManager();
	modelutility::SceneManager SceneManager;
	if (!scene->hasRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED)) {
		SceneManager.generateStashGraph(scene);
	}
	return SceneManager.generateWebViewBuffers(scene, exType, buffers, handler, manager);
}

repo_web_buffers_t RepoManipulator::generateSRCBuffer(
	repo::core::model::RepoScene* scene)
{
	repo_web_buffers_t buffers;
	modelutility::SceneManager SceneManager;
	SceneManager.generateWebViewBuffers(scene, modelconvertor::WebExportType::SRC, buffers, nullptr);
	return buffers;
}

std::vector<repo::core::model::RepoBSON>
RepoManipulator::getAllFromCollectionTailable(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& database,
	const std::string& collection,
	const uint64_t& skip,
	const uint32_t& limit)
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
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& database,
	const std::string& collection,
	const std::list<std::string>& fields,
	const std::string& sortField,
	const int& sortOrder,
	const uint64_t& skip,
	const uint32_t& limit)
{
	std::vector<repo::core::model::RepoBSON> vector;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		vector = handler->getAllFromCollectionTailable(database, collection, skip, limit, fields, sortField, sortOrder);
	return vector;
}

std::map<std::string, std::list<std::string>>
RepoManipulator::getDatabasesWithProjects(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::list<std::string>& databases)
{
	std::map<std::string, std::list<std::string>> list;
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		list = handler->getDatabasesWithProjects(databases);

	return list;
}

std::list<std::string> RepoManipulator::getAdminDatabaseRoles(
	const std::string& databaseAd)
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
	const repo::core::model::RepoScene* scene,
	const uint32_t& maxDepth
)
{
	modelutility::RDTreeSpatialPartitioner partitioner(scene, maxDepth);
	return partitioner.partitionScene();
}

std::list<std::string> RepoManipulator::getStandardDatabaseRoles(
	const std::string& databaseAd)
{
	std::list<std::string> roles;

	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	if (handler)
		roles = handler->getStandardDatabaseRoles();

	return roles;
}

std::string RepoManipulator::getNameOfAdminDatabase(
	const std::string& databaseAd) const
{
	//FIXME: at the moment we only have mongo. But if we have
	//different database types then this would not work
	return  repo::core::handler::MongoDatabaseHandler::getAdminDatabaseName();
}

repo::core::model::RepoNodeSet
RepoManipulator::loadMetadataFromFile(
	const std::string& filePath,
	const char& delimiter)
{
	repo::manipulator::modelconvertor::MetadataImportCSV metaImport;
	std::vector<std::string> tmp;

	return metaImport.readMetadata(filePath, tmp, delimiter);
}

repo::core::model::RepoScene*
RepoManipulator::loadSceneFromFile(
	const std::string& filePath,
	uint8_t& error,
	const repo::manipulator::modelconvertor::ModelImportConfig& config)
{
	repo::manipulator::modelconvertor::ModelImportManager manager;
	auto scene = manager.ImportFromFile(filePath, config, error);
	if (scene) {
		if (generateStashGraph(scene)) {
			repoTrace << "Stash graph generated.";
			error = REPOERR_OK;
		}
		else
		{
			repoError << "Error generating stash graph";
			error = REPOERR_STASH_GEN_FAIL;
			delete scene;
			scene = nullptr;
		}
	}

	return scene;
}

void RepoManipulator::processDrawingRevision(
	const std::string databaseAd,
	const std::string& teamspace,
	const repo::lib::RepoUUID revision,
	uint8_t& error,
	const std::string& imagePath)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);

	// get the drawing node that holds the ref

	auto manager = repo::manipulator::modelutility::DrawingManager();
	auto revisionNode = manager.fetchRevision(handler, teamspace, revision);

	auto fileNodeIds = revisionNode.getFiles();

	if (fileNodeIds.size() < 1)
	{
		repoError << "Drawing revision " << revision.toString() << " does not contain an rFile. Unable to process drawing.";
		error = REPOERR_GET_FILE_FAILED;
		return;
	}

	// The ODA importers supported (DWG, DGN) require actual files and do not
	// have overloads for memory streams, so resolve the ref node to a path on
	// a locally accessible filesystem.

	auto fileManager = repo::core::handler::fileservice::FileManager::getManager();
	auto refNodeId = fileNodeIds[0]; // We do not expect drawing revision nodes to have multiple rFile entries
	auto refNode = fileManager->getFileRef(
		teamspace,
		REPO_COLLECTION_DRAWINGS,
		refNodeId
	);
	auto fullpath = fileManager->getFilePath(refNode);

	// The DrawingImportManager will select the correct importer to convert the
	// drawing, and return the contents along with calibration and any other
	// metadata in the DrawingImageInfo.

	repo::manipulator::modelconvertor::DrawingImportManager importer;
	repo::manipulator::modelutility::DrawingImageInfo drawing;
	drawing.name = refNode.getFileName();
	if (imagePath.empty()) {
		importer.importFromFile(
			drawing,
			fullpath,
			revisionNode.getFormat(),
			error
		);
	}
	else {
		std::ifstream input(imagePath, std::ios::binary);
		input.unsetf(std::ios_base::skipws);
		std::istream_iterator<uint8_t> start(input), end;
		std::vector<uint8_t> buffer(start, end);

		drawing.data = buffer;
		error = REPOERR_OK;
	}

	if (error == REPOERR_OK) {
		error = manager.commitImage(handler, fileManager, teamspace, revisionNode, drawing);
	}
}

bool RepoManipulator::hasCollection(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& dbName,
	const std::string& project)
{
	std::list<std::string> dList = { dbName };
	auto databaseList = getDatabasesWithProjects(databaseAd, cred, dList);
	if (!databaseList.size()) return false;
	auto collectionList = databaseList.begin()->second;
	auto findIt = std::find(collectionList.begin(), collectionList.end(), project);
	return findIt != collectionList.end();
}

bool RepoManipulator::hasDatabase(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& dbName)
{
	auto databaseList = fetchDatabases(databaseAd, cred);
	auto findIt = std::find(databaseList.begin(), databaseList.end(), dbName);
	return findIt != databaseList.end();
}

bool RepoManipulator::init(
	std::string& errMsg,
	const repo::lib::RepoConfig& config,
	const int& nDbConnections
) {
	auto dbConf = config.getDatabaseConfig();
	bool success = true;
	if (dbConf.connString.empty()) {
		success = connectAndAuthenticateWithAdmin(errMsg, dbConf.addr, dbConf.port, nDbConnections, dbConf.username, dbConf.password);
	}
	else {
		success = connectAndAuthenticateWithAdmin(errMsg, dbConf.connString, nDbConnections, dbConf.username, dbConf.password);
	}

	if (success) {
		repo::core::handler::AbstractDatabaseHandler* handler =
			repo::core::handler::MongoDatabaseHandler::getHandler(dbConf.addr);
		success = (bool)repo::core::handler::fileservice::FileManager::instantiateManager(config, handler);
	}

	return success;
}

bool RepoManipulator::isVREnabled(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const repo::core::model::RepoScene* scene) const
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
	repo::core::model::RepoScene* scene,
	const repo::core::model::RepoScene::GraphType& gType)
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
	else {
		repoError << "RepoController::reduceTransformations: NULL pointer to scene/ Scene is not loaded!";
	}
}

bool RepoManipulator::saveOriginalFiles(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const repo::core::model::RepoScene* scene,
	const std::string& directory)
{
	repo::core::handler::AbstractDatabaseHandler* handler =
		repo::core::handler::MongoDatabaseHandler::getHandler(databaseAd);
	auto manager = repo::core::handler::fileservice::FileManager::getManager();
	bool success = false;
	if (handler && scene)
	{
		std::string errMsg;

		const std::vector<std::string> files = scene->getOriginalFiles();
		if (success = files.size() > 0)
		{
			boost::filesystem::path dir(directory);

			for (const std::string& file : files)
			{
				std::vector<uint8_t> rawFile = manager->getFile(scene->getDatabaseName(),
					scene->getProjectName() + "." + REPO_COLLECTION_RAW, file);

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
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& database,
	const std::string& project,
	const std::string& directory)
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
	const std::string& filePath,
	const repo::core::model::RepoScene* scene)
{
	modelconvertor::AssimpModelExport modelExport(scene);
	return modelExport.exportToFile(filePath);
}

void RepoManipulator::upsertDocument(
	const std::string& databaseAd,
	const repo::core::model::RepoBSON* cred,
	const std::string& databaseName,
	const std::string& collectionName,
	const repo::core::model::RepoBSON& bson)
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