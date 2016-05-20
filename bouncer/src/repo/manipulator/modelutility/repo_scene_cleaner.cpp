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
	repoInfo << "Removing revision with id: " << revNode.getUniqueID();
	//FIXME: avoid using mongo object field
	auto currentField = mongo::BSONArray(revNode.getObjectField(REPO_NODE_REVISION_LABEL_CURRENT_UNIQUE_IDS));
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
	//Stash isn't commited until the revision flag is set to complete, so there shouldn't be anything to clean up
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

SceneCleaner::~SceneCleaner()
{
}