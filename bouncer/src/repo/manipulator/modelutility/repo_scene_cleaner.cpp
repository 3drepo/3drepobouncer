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
#include "../../core/model/bson/repo_node_revision.h"
#include "../../core/model/collection/repo_scene.h"
#include "repo_scene_manager.h"

using namespace repo::manipulator::modelutility;

SceneCleaner::SceneCleaner(
	const std::string                            &dbName,
	const std::string                            &projectName,
	repo::core::handler::AbstractDatabaseHandler *handler
	)
	: dbName(dbName)
	, projectName(projectName)
	, handler(handler)
{
}

bool SceneCleaner::cleanUpRevision(
	const repo::core::model::RevisionNode &revNode)
{
	bool success = false;
	repo::core::model::RevisionNode::UploadStatus status = revNode.getUploadStatus();
	repo::core::model::RepoScene *scene = nullptr;
	repoUUID revID = revNode.getUniqueID();
	SceneManager manager;
	switch (status)
	{
	case repo::core::model::RevisionNode::UploadStatus::GEN_DEFAULT:
		//corrupted scene import, delete the revision
		success = removeRevision(revNode);
		break;
	case repo::core::model::RevisionNode::UploadStatus::GEN_REPO_STASH:
	{
		//corrupted repo stash, try to regenerate it
		scene = manager.fetchScene(handler, dbName, projectName, revID, false);
		if (!(success = manager.generateStashGraph(scene, handler)))
		{
			//generate the rest if it is successful, only break if it failed
			break;
		}
	}
	case repo::core::model::RevisionNode::UploadStatus::GEN_WEB_STASH:
	{
		//corrupted web stash, try to regenerate it
		if (!scene)
			scene = manager.fetchScene(handler, dbName, projectName, revID, false);
		repo_web_buffers_t buffers;
		if (!(success = manager.generateWebViewBuffers(scene, repo::manipulator::modelconvertor::WebExportType::SRC, buffers, handler)))
		{
			//generate the rest if it is successful, only break if it failed
			break;
		}
	}
	case repo::core::model::RevisionNode::UploadStatus::GEN_SEL_TREE:
		//corrupted selection tree, try to regenerate it
		if (!scene)
			scene = manager.fetchScene(handler, dbName, projectName, revID, false);
		success = manager.generateAndCommitSelectionTree(scene, handler);

		break;
	default:
		repoError << "Unrecognised upload status: " << (int)status;
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
			handler->dropRawFile(dbName, collection, fname.second, errMsg);
	}
}

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

	repo::core::model::RepoBSON criteria = BSON(REPO_NODE_LABEL_ID << BSON("$in" << currentField));
	std::string errMsg;
	//clean up scene
	handler->dropDocuments(criteria, dbName, projectName + "." + REPO_COLLECTION_SCENE, errMsg);

	//remove the original files attached to the revision
	auto orgFileNames = revNode.getOrgFiles();
	for (const auto &file : orgFileNames)
		handler->dropRawFile(dbName, projectName + "." + REPO_COLLECTION_HISTORY, file, errMsg);
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