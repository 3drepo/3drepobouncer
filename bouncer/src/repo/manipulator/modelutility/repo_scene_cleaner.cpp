/**
*  Copyright (C) 2016 3D Repo Ltd
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
#include "repo_scene_cleaner.h"
#include "../../core/model/bson/repo_bson_builder.h"
#include "../../core/model/bson/repo_node_revision.h"
#include "../../core/model/collection/repo_scene.h"
#include "repo_scene_manager.h"

using namespace repo::manipulator::modelutility;

SceneCleaner::SceneCleaner(
	const std::string                                      &dbName,
	const std::string                                      &projectName,
	repo::core::handler::AbstractDatabaseHandler           *handler,
	repo::core::handler::fileservice::FileManager		   *fileManager) :
	dbName(dbName),
	projectName(projectName),
	handler(handler),
	fileManager (fileManager)
{
}

bool SceneCleaner::cleanUpRevision(
	const repo::core::model::RevisionNode &revNode)
{
	bool success = false;
	repo::core::model::RevisionNode::UploadStatus status = revNode.getUploadStatus();
	repo::core::model::RepoScene *scene = nullptr;
	repo::lib::RepoUUID revID = revNode.getUniqueID();
	SceneManager manager;
	if (status != repo::core::model::RevisionNode::UploadStatus::COMPLETE)
	{
		// upload incomplete, delete the revision
		success = removeRevision(revNode);
	}

	return success;
}

bool SceneCleaner::execute()
{
	bool success = handler;
	if (!handler)
	{
		repoError << "Failed to instantiate scene cleaner: null pointer to the database!";
	}

#ifdef FILESERVICE_SUPPORT
	if (!fileManager)
	{
		repoError << "Failed to instantiate scene cleaner: null pointer to the file service manager!";
	}
#endif

	if (!(success &= !(dbName.empty() || projectName.empty())))
	{
		repoError << "Failed to instantiate scene cleaner: database name or project name is empty!";
	}

	if (success)
	{
		auto unfinishedRevisions = getIncompleteRevisions();
		repoInfo << unfinishedRevisions.size() << " incomplete revisions found.";
		for (const auto &bson : unfinishedRevisions)
		{
			success &= cleanUpRevision(repo::core::model::RevisionNode(bson));
		}
	}

	return success;
}

std::vector<repo::core::model::RepoBSON> SceneCleaner::getIncompleteRevisions()
{
	repo::core::model::RepoBSON criteria = BSON(REPO_NODE_REVISION_LABEL_INCOMPLETE << BSON("$exists" << true));
	return handler->findAllByCriteria(dbName, projectName + "." + REPO_COLLECTION_HISTORY, criteria);
}

void SceneCleaner::removeAllGridFSReference(
	const repo::core::model::RepoBSON &criteria
	)
{
	//find all bson objects that has a ext ref
	const std::string collection = projectName + "." + REPO_COLLECTION_SCENE;
	auto results = handler->findAllByCriteria(dbName, collection, criteria);

	for (const auto &res : results)
	{
		auto node = repo::core::model::RepoNode(res);
		auto fnames = node.getFileList();
		std::string errMsg;
		for (const auto fname : fnames)
		{
			handler->dropRawFile(dbName, collection, fname.second, errMsg);
		}
	}
}

#ifdef FILESERVICE_SUPPORT
void SceneCleaner::removeCollectionFiles(
	const std::vector<std::string> &documentIds,
	const std::string &collection)
{
	for (const auto fname : documentIds)
	{
		fileManager->deleteFileAndRef(handler, dbName, collection, fname);
	}
}

void SceneCleaner::removeAllFiles(
	const std::vector<std::string> &documentIds
	)
{
	removeCollectionFiles(documentIds, projectName + "." + REPO_COLLECTION_STASH_JSON);
	removeCollectionFiles(documentIds, projectName + "." + REPO_COLLECTION_STASH_UNITY);
}
#endif

bool SceneCleaner::removeRevision(
	const repo::core::model::RevisionNode &revNode)
{
	repoInfo << "Removing revision with id: " << revNode.getUniqueID();
	//FIXME: avoid using mongo object field
	auto currentField = mongo::BSONArray(revNode.getObjectField(REPO_NODE_REVISION_LABEL_CURRENT_UNIQUE_IDS));
	//find and delete all gridfs entries
	//FIXME: it's not easy to keep the bsonarray as array. need to construct it into a embedded bson (or we need to inherit mongo::bsonarray)
	auto gridFSCriteria = BSON(REPO_NODE_LABEL_ID << BSON("$in" << currentField) << REPO_LABEL_OVERSIZED_FILES << BSON("$exists" << true));
	removeAllGridFSReference(gridFSCriteria);

	core::model::RepoBSONBuilder builder;
	builder.append(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_MESH);
	builder.append(REPO_NODE_STASH_REF, revNode.getUniqueID());
	auto revisionMeshes = handler->findAllByCriteria(dbName, projectName + "." + REPO_COLLECTION_STASH_REPO, builder.obj());

	std::vector<std::string> documentIds;
	documentIds.push_back("revision/" + revNode.getUniqueID().toString() + "/" + REPO_DOCUMENT_ID_SUFFIX_FULLTREE);
	documentIds.push_back("revision/" + revNode.getUniqueID().toString() + "/" + REPO_DOCUMENT_ID_SUFFIX_IDMAP);
	documentIds.push_back("revision/" + revNode.getUniqueID().toString() + "/" + REPO_DOCUMENT_ID_SUFFIX_IDTOMESHES);
	documentIds.push_back("revision/" + revNode.getUniqueID().toString() + "/" + REPO_DOCUMENT_ID_SUFFIX_TREEPATH);
	documentIds.push_back("revision/" + revNode.getUniqueID().toString() + "/" + REPO_DOCUMENT_ID_SUFFIX_UNITYASSETS);
	for (const auto &mesh : revisionMeshes)
	{
		auto node = repo::core::model::RepoNode(mesh);
		auto meshId = node.getUniqueID().toString();
		documentIds.push_back(meshId + REPO_DOCUMENT_ID_SUFFIX_UNITY_JSON);
		documentIds.push_back(meshId + REPO_DOCUMENT_ID_SUFFIX_UNITY3D);
		documentIds.push_back(meshId + REPO_DOCUMENT_ID_SUFFIX_UNITY3D_WIN);
	}

#ifdef FILESERVICE_SUPPORT
	removeAllFiles(documentIds);
#endif

	repo::core::model::RepoBSON criteria = BSON(REPO_NODE_LABEL_ID << BSON("$in" << currentField));
	std::string errMsg;
	//clean up scene
	handler->dropDocuments(criteria, dbName, projectName + "." + REPO_COLLECTION_SCENE, errMsg);

	//remove the original files attached to the revision
	auto orgFileNames = revNode.getOrgFiles();
	for (const auto &file : orgFileNames)
	{
		handler->dropRawFile(dbName, projectName + "." + REPO_COLLECTION_HISTORY, file, errMsg);
#ifdef FILESERVICE_SUPPORT
		fileManager->deleteFileAndRef(handler, dbName, projectName + "." + REPO_COLLECTION_HISTORY, file);
#endif
	}
	//delete the revision itself
	handler->dropDocument(revNode, dbName, projectName + "." + REPO_COLLECTION_HISTORY, errMsg);

	if (errMsg.empty())
		return true;
	else
	{
		repoError << "Failed to clean up revision " << revNode.getUniqueID() << " : " << errMsg;
		return false;
	}
}

SceneCleaner::~SceneCleaner()
{
}
