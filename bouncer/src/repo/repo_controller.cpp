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

void RepoController::destroyToken(RepoController::RepoToken* token)
{
	if (token) delete token;
}

repo::core::model::RepoScene* RepoController::fetchScene(
	const RepoController::RepoToken      *token,
	const std::string    &database,
	const std::string    &collection,
	const std::string    &uuid,
	const bool           &headRevision,
	const bool           &ignoreRefScene,
	const bool           &skeletonFetch,
	const std::vector<repo::core::model::ModelRevisionNode::UploadStatus> &includeStatus)
{
	return impl->fetchScene(token, database, collection, uuid, headRevision, ignoreRefScene, skeletonFetch, includeStatus);
}

bool RepoController::generateAndCommitSelectionTree(
	const RepoController::RepoToken                         *token,
	repo::core::model::RepoScene            *scene)
{
	return impl->generateAndCommitSelectionTree(token, scene);
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

void RepoController::setLoggingLevel(const repo::lib::RepoLog::RepoLogLevel &level)
{
	impl->setLoggingLevel(level);
}

void RepoController::logToFile(const std::string &filePath)
{
	impl->logToFile(filePath);
}

repo::core::model::RepoScene* RepoController::createFederatedScene(
	const std::map<repo::core::model::ReferenceNode, std::string> &fedMap)
{
	return impl->createFederatedScene(fedMap);
}

bool RepoController::generateAndCommitRepoBundlesBuffer(
	const RepoController::RepoToken* token,
	repo::core::model::RepoScene* scene)
{
	return impl->generateAndCommitRepoBundlesBuffer(token, scene);
}

std::shared_ptr<repo::lib::repo_partitioning_tree_t>
RepoController::getScenePartitioning(
	const repo::core::model::RepoScene *scene,
	const uint32_t                     &maxDepth
)
{
	return impl->getScenePartitioning(scene, maxDepth);
}

std::string RepoController::getSupportedImportFormats()
{
	return impl->getSupportedImportFormats();
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

void RepoController::processDrawingRevision(
	const RepoController::RepoToken* token,
	const std::string& teamspace,
	const repo::lib::RepoUUID revision,
	uint8_t& err,
	const std::string &imagePath)
{
	return impl->processDrawingRevision(token, teamspace, revision, err, imagePath);
}

void RepoController::updateRevisionStatus(
	repo::core::model::RepoScene* scene,
	const repo::core::model::ModelRevisionNode::UploadStatus& status)
{
	impl->updateRevisionStatus(scene, status);
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