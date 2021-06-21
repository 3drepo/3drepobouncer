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

#include "repo_controller_internal.cpp.inl" //Inner class implementation

using namespace repo;

RepoController::RepoController(
	std::vector<lib::RepoAbstractListener*> listeners,
	const uint32_t &numConcurrentOps,
	const uint32_t &numDbConn)
{
	//RepoController follows the Pimpl idiom http://www.gotw.ca/gotw/028.htm
	//This is done to avoid high dependencies on other headers for library users
	//Actual implementations are in _RepoControllerImpl
	impl = new RepoController::_RepoControllerImpl(listeners, numConcurrentOps, numDbConn);
}

RepoController::~RepoController()
{
	if (impl) delete impl;
}

void RepoController::addAlias(
	RepoController::RepoToken         *token,
	const std::string &alias)
{
	if (token)
		token->alias = alias;
}

RepoController::RepoToken* RepoController::init(
	std::string            &errMsg,
	const lib::RepoConfig  &config
)
{
	return impl->init(errMsg, config);
}

bool RepoController::commitAssetBundleBuffers(
	const RepoController::RepoToken *token,
	repo::core::model::RepoScene    *scene,
	const repo_web_buffers_t &buffers)
{
	return impl->commitAssetBundleBuffers(token, scene, buffers);
}

uint8_t RepoController::commitScene(
	const RepoController::RepoToken    *token,
	repo::core::model::RepoScene        *scene,
	const std::string                   &owner,
	const std::string                      &tag,
	const std::string                      &desc,
	const repo::lib::RepoUUID           &revId)
{
	return impl->commitScene(token, scene, owner, tag, desc, revId);
}

uint64_t RepoController::countItemsInCollection(
	const RepoController::RepoToken            *token,
	const std::string    &database,
	const std::string    &collection)
{
	return impl->countItemsInCollection(token, database, collection);
}

void RepoController::destroyToken(RepoController::RepoToken* token)
{
	if (token) delete token;
}

void RepoController::disconnectFromDatabase(const RepoController::RepoToken* token)
{
	impl->disconnectFromDatabase(token);
}

repo::core::model::RepoScene* RepoController::fetchScene(
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
	return impl->fetchScene(token, database, collection, uuid, headRevision, lightFetch, ignoreRefScene, skeletonFetch, includeStatus);
}

bool RepoController::generateAndCommitSelectionTree(
	const RepoController::RepoToken                         *token,
	repo::core::model::RepoScene            *scene)
{
	return impl->generateAndCommitSelectionTree(token, scene);
}

bool RepoController::generateAndCommitStashGraph(
	const RepoController::RepoToken              *token,
	repo::core::model::RepoScene* scene
)
{
	return impl->generateAndCommitStashGraph(token, scene);
}

std::vector < repo::core::model::RepoBSON >
RepoController::getAllFromCollectionContinuous(
	const RepoController::RepoToken      *token,
	const std::string    &database,
	const std::string    &collection,
	const uint64_t       &skip,
	const uint32_t       &limit)
{
	return impl->getAllFromCollectionContinuous(token, database, collection, skip, limit);
}

std::vector < repo::core::model::RepoBSON >
RepoController::getAllFromCollectionContinuous(
	const RepoController::RepoToken              *token,
	const std::string            &database,
	const std::string            &collection,
	const std::list<std::string> &fields,
	const std::string            &sortField,
	const int                    &sortOrder,
	const uint64_t               &skip,
	const uint32_t               &limit)
{
	return impl->getAllFromCollectionContinuous(token, database, collection, fields, sortField, sortOrder, skip, limit);
}

std::vector < repo::core::model::RepoRole > RepoController::getRolesFromDatabase(
	const RepoController::RepoToken              *token,
	const std::string            &database,
	const uint64_t               &skip,
	const uint32_t               &limit)
{
	return impl->getRolesFromDatabase(token, database, skip, limit);
}

std::list<std::string> RepoController::getDatabases(const RepoController::RepoToken *token)
{
	return impl->getDatabases(token);
}

std::map<std::string, std::list<std::string>>
RepoController::getDatabasesWithProjects(
	const RepoController::RepoToken  *token,
	const std::list<std::string>     &databases)
{
	return impl->getDatabasesWithProjects(token, databases);
}

bool RepoController::insertBinaryFileToDatabase(
	const RepoController::RepoToken            *token,
	const std::string          &database,
	const std::string          &collection,
	const std::string          &name,
	const std::vector<uint8_t> &rawData,
	const std::string          &mimeType)
{
	return impl->insertBinaryFileToDatabase(token, database, collection, name, rawData, mimeType);
}

void RepoController::insertRole(
	const RepoController::RepoToken   *token,
	const repo::core::model::RepoRole &role
)
{
	impl->insertRole(token, role);
}

void RepoController::insertUser(
	const RepoController::RepoToken    *token,
	const repo::core::model::RepoUser  &user)
{
	impl->insertUser(token, user);
}

bool RepoController::removeCollection(
	const RepoController::RepoToken             *token,
	const std::string     &databaseName,
	const std::string     &collectionName,
	std::string			  &errMsg
)
{
	return impl->removeCollection(token, databaseName, collectionName, errMsg);
}

bool RepoController::removeDatabase(
	const RepoController::RepoToken       *token,
	const std::string     &databaseName,
	std::string			  &errMsg
)
{
	return impl->removeDatabase(token, databaseName, errMsg);
}

void RepoController::removeDocument(
	const RepoController::RepoToken          *token,
	const std::string                        &databaseName,
	const std::string                        &collectionName,
	const repo::core::model::RepoBSON        &bson)
{
	impl->removeDocument(token, databaseName, collectionName, bson);
}

bool RepoController::removeProject(
	const RepoController::RepoToken          *token,
	const std::string                        &databaseName,
	const std::string                        &projectName,
	std::string								 &errMsg)
{
	return impl->removeProject(token, databaseName, projectName, errMsg);
}

void RepoController::removeRole(
	const RepoController::RepoToken          *token,
	const repo::core::model::RepoRole        &role)
{
	impl->removeRole(token, role);
}

void RepoController::removeUser(
	const RepoController::RepoToken    *token,
	const repo::core::model::RepoUser  &user)
{
	impl->removeUser(token, user);
}

void RepoController::updateRole(
	const RepoController::RepoToken          *token,
	const repo::core::model::RepoRole        &role)
{
	impl->updateRole(token, role);
}

void RepoController::updateUser(
	const RepoController::RepoToken    *token,
	const repo::core::model::RepoUser  &user)
{
	impl->updateUser(token, user);
}

void RepoController::upsertDocument(
	const RepoController::RepoToken          *token,
	const std::string                        &databaseName,
	const std::string                        &collectionName,
	const repo::core::model::RepoBSON        &bson)
{
	impl->upsertDocument(token, databaseName, collectionName, bson);
}

void RepoController::setLoggingLevel(const repo::lib::RepoLog::RepoLogLevel &level)
{
	impl->setLoggingLevel(level);
}

void RepoController::logToFile(const std::string &filePath)
{
	impl->logToFile(filePath);
}

repo::core::model::RepoScene* RepoController::createFederatedScene(
	const std::map<repo::core::model::TransformationNode, repo::core::model::ReferenceNode> &fedMap)
{
	return impl->createFederatedScene(fedMap);
}

bool RepoController::generateAndCommitGLTFBuffer(
	const RepoController::RepoToken    *token,
	repo::core::model::RepoScene *scene)
{
	return impl->generateAndCommitGLTFBuffer(token, scene);
}

bool RepoController::generateAndCommitSRCBuffer(
	const RepoController::RepoToken    *token,
	repo::core::model::RepoScene *scene)
{
	return impl->generateAndCommitSRCBuffer(token, scene);
}

repo_web_buffers_t RepoController::generateGLTFBuffer(
	repo::core::model::RepoScene *scene)
{
	return impl->generateGLTFBuffer(scene);
}

repo_web_buffers_t RepoController::generateSRCBuffer(
	repo::core::model::RepoScene *scene)
{
	return impl->generateSRCBuffer(scene);
}

std::list<std::string> RepoController::getAdminDatabaseRoles(const RepoController::RepoToken *token)
{
	return impl->getAdminDatabaseRoles(token);
}

std::string RepoController::getNameOfAdminDatabase(const RepoController::RepoToken *token)
{
	return impl->getNameOfAdminDatabase(token);
}

std::shared_ptr<repo_partitioning_tree_t>
RepoController::getScenePartitioning(
	const repo::core::model::RepoScene *scene,
	const uint32_t                     &maxDepth
)
{
	return impl->getScenePartitioning(scene, maxDepth);
}

std::list<std::string> RepoController::getStandardDatabaseRoles(const RepoController::RepoToken *token)
{
	return impl->getStandardDatabaseRoles(token);
}

std::string RepoController::getSupportedImportFormats()
{
	return impl->getSupportedImportFormats();
}

std::string RepoController::getSupportedExportFormats()
{
	return impl->getSupportedExportFormats();
}

std::vector<std::shared_ptr<repo::core::model::MeshNode>> RepoController::initialiseAssetBuffer(
	const RepoController::RepoToken                    *token,
	repo::core::model::RepoScene *scene,
	std::unordered_map<std::string, std::vector<uint8_t>> &jsonFiles,
	repo::core::model::RepoUnityAssets &unityAssets,
	std::vector<std::vector<uint16_t>> &serialisedFaceBuf,
	std::vector<std::vector<std::vector<float>>> &idMapBuf,
	std::vector<std::vector<std::vector<repo_mesh_mapping_t>>> &meshMappings)
{
	return impl->initialiseAssetBuffer(token, scene, jsonFiles, unityAssets, serialisedFaceBuf, idMapBuf, meshMappings);
}

repo::core::model::RepoNodeSet RepoController::loadMetadataFromFile(
	const std::string &filePath,
	const char        &delimiter)
{
	return impl->loadMetadataFromFile(filePath, delimiter);
}

repo::core::model::RepoScene*
RepoController::loadSceneFromFile(
	const std::string                                          &filePath,
	uint8_t                                                    &err,
	const repo::manipulator::modelconvertor::ModelImportConfig &config)
{
	return impl->loadSceneFromFile(filePath, err, config);
}

bool RepoController::saveOriginalFiles(
	const RepoController::RepoToken    *token,
	const repo::core::model::RepoScene *scene,
	const std::string                   &directory)
{
	return impl->saveOriginalFiles(token, scene, directory);
}

bool RepoController::saveOriginalFiles(
	const RepoController::RepoToken     *token,
	const std::string                   &database,
	const std::string                   &project,
	const std::string                   &directory)
{
	return impl->saveOriginalFiles(token, database, project, directory);
}

bool RepoController::saveSceneToFile(
	const std::string &filePath,
	const repo::core::model::RepoScene* scene)
{
	return impl->saveSceneToFile(filePath, scene);
}

void RepoController::reduceTransformations(
	const RepoController::RepoToken              *token,
	repo::core::model::RepoScene *scene)
{
	impl->reduceTransformations(token, scene);
}

void RepoController::compareScenes(
	const RepoController::RepoToken    *token,
	repo::core::model::RepoScene       *base,
	repo::core::model::RepoScene       *compare,
	repo_diff_result_t                         &baseResults,
	repo_diff_result_t                         &compResults,
	const repo::DiffMode                     &diffMode
)
{
	impl->compareScenes(token, base, compare, baseResults, compResults, diffMode);
}

bool RepoController::isVREnabled(const RepoController::RepoToken *token,
	const repo::core::model::RepoScene *scene)
{
	return impl->isVREnabled(token, scene);
}

std::string RepoController::getVersion()
{
	return impl->getVersion();
}