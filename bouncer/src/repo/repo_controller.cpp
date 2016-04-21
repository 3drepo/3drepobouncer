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

RepoController::RepoToken* RepoController::authenticateToAdminDatabaseMongo(
	std::string       &errMsg,
	const std::string &address,
	const int         &port,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested
	)
{
	return impl->authenticateToAdminDatabaseMongo(errMsg, address, port, username, password, pwDigested);
}

RepoController::RepoToken* RepoController::authenticateMongo(
	std::string       &errMsg,
	const std::string &address,
	const uint32_t    &port,
	const std::string &dbName,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested
	)
{
	return impl->authenticateMongo(errMsg, address, port, dbName, username, password, pwDigested);
}

bool RepoController::testConnection(const repo::RepoCredentials &credentials)
{
	return impl->testConnection(credentials);
}

void RepoController::commitScene(
	const RepoController::RepoToken    *token,
	repo::core::model::RepoScene        *scene,
	const std::string                   &owner)
{
	impl->commitScene(token, scene, owner);
}

uint64_t RepoController::countItemsInCollection(
	const RepoController::RepoToken            *token,
	const std::string    &database,
	const std::string    &collection)
{
	return impl->countItemsInCollection(token, database, collection);
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
	const bool           &lightFetch)
{
	return impl->fetchScene(token, database, collection, uuid, headRevision, lightFetch);
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

std::vector < repo::core::model::RepoRoleSettings > RepoController::getRoleSettingsFromDatabase(
	const RepoController::RepoToken              *token,
	const std::string            &database,
	const uint64_t               &skip,
	const uint32_t               &limit)
{
	return impl->getRoleSettingsFromDatabase(token, database, skip, limit);
}

repo::core::model::RepoRoleSettings RepoController::getRoleSettings(
	const RepoController::RepoToken *token,
	const repo::core::model::RepoRole &role)
{
	return getRoleSettings(token, role.getDatabase(), role.getName());
}

repo::core::model::RepoRoleSettings RepoController::getRoleSettings(
	const RepoController::RepoToken *token,
	const std::string &database,
	const std::string &uniqueRoleName)
{
	return impl->getRoleSettings(token, database, uniqueRoleName);
}

std::list<std::string> RepoController::getDatabases(const RepoController::RepoToken *token)
{
	return impl->getDatabases(token);
}

std::list<std::string>  RepoController::getCollections(
	const RepoController::RepoToken       *token,
	const std::string     &databaseName
	)
{
	return impl->getCollections(token, databaseName);
}

repo::core::model::CollectionStats RepoController::getCollectionStats(
	const RepoController::RepoToken      *token,
	const std::string                    &database,
	const std::string                    &collection)
{
	return impl->getCollectionStats(token, database, collection);
}

std::string RepoController::getHostAndPort(const RepoController::RepoToken *token)
{
	return token->getDatabaseHostPort();
}

std::map<std::string, std::list<std::string>>
RepoController::getDatabasesWithProjects(
const RepoController::RepoToken  *token,
const std::list<std::string>     &databases)
{
	return impl->getDatabasesWithProjects(token, databases);
}

void RepoController::insertBinaryFileToDatabase(
	const RepoController::RepoToken            *token,
	const std::string          &database,
	const std::string          &collection,
	const std::string          &name,
	const std::vector<uint8_t> &rawData,
	const std::string          &mimeType)
{
	impl->insertBinaryFileToDatabase(token, database, collection, name, rawData, mimeType);
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

repo::core::model::RepoScene* RepoController::createMapScene(
	const repo::core::model::MapNode &mapNode)
{
	return impl->createMapScene(mapNode);
}

bool RepoController::generateAndCommitGLTFBuffer(
	const RepoController::RepoToken    *token,
	const repo::core::model::RepoScene *scene)
{
	return impl->generateAndCommitGLTFBuffer(token, scene);
}

bool RepoController::generateAndCommitSRCBuffer(
	const RepoController::RepoToken    *token,
	const repo::core::model::RepoScene *scene)
{
	return impl->generateAndCommitSRCBuffer(token, scene);
}

repo_web_buffers_t RepoController::generateGLTFBuffer(
	const repo::core::model::RepoScene *scene)
{
	return impl->generateGLTFBuffer(scene);
}

repo_web_buffers_t RepoController::generateSRCBuffer(
	const repo::core::model::RepoScene *scene)
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

std::shared_ptr<PartitioningTree>
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

std::string RepoController::getSupportedExportFormats()
{
	return impl->getSupportedExportFormats();
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
const bool                                                 &applyReduction,
const bool                                                 &rotateModel,
const repo::manipulator::modelconvertor::ModelImportConfig *config)
{
	return impl->loadSceneFromFile(filePath, applyReduction, rotateModel, config);
}

void RepoController::saveOriginalFiles(
	const RepoController::RepoToken    *token,
	const repo::core::model::RepoScene *scene,
	const std::string                   &directory)
{
	return impl->saveOriginalFiles(token, scene, directory);
}

void RepoController::saveOriginalFiles(
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
	DiffResult                         &baseResults,
	DiffResult                         &compResults,
	const DiffMode                     &diffMode
	)
{
	impl->compareScenes(token, base, compare, baseResults, compResults, diffMode);
}

std::string RepoController::getVersion()
{
	return impl->getVersion();
}