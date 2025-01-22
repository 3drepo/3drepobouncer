/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo_scene2.h"

#include "repo/core/handler/database/repo_query.h"
#include "repo/core/handler/repo_database_handler_mongo.h"
#include "repo/core/model/bson/repo_bson.h"

using namespace repo::core::model::x;
using namespace repo::core::handler;

RepoScene::RepoScene(
	std::string database, 
	std::string project, 
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler)
	:databaseName(database),
	projectName(project),
	handler(handler)
{
}

size_t RepoScene::getNumNodes(repo::core::model::NodeType type)
{
	return handler->count(databaseName, projectName + "." + REPO_COLLECTION_SCENE,
			repo::core::handler::database::query::Eq(REPO_LABEL_TYPE, std::string(REPO_NODE_TYPE_TRANSFORMATION))
		);
}

std::string RepoScene::getCollectionName()
{
	return projectName + "." + REPO_COLLECTION_SCENE;
}

void RepoScene::removeNode(const repo::core::model::RepoNode& node)
{
	handler->dropDocument(node, databaseName, getCollectionName());
}

void RepoScene::updateNode(const repo::core::model::RepoNode& node)
{

}

RepoScene::Cursor RepoScene::getAllNodesByType(const std::string& type)
{
	//todo: this should not need to convert to mongo db handler
	auto mongo = std::dynamic_pointer_cast<repo::core::handler::MongoDatabaseHandler>(handler);

	return mongo->getAllByCriteria(databaseName, getCollectionName(), database::query::Eq(REPO_NODE_LABEL_TYPE, type));
}

RepoScene::Cursor RepoScene::getSiblings(const repo::core::model::RepoNode& node)
{
	//todo: this should not need to convert to mongo db handler
	auto mongo = std::dynamic_pointer_cast<repo::core::handler::MongoDatabaseHandler>(handler);

	return mongo->getAllByCriteria(databaseName, getCollectionName(), database::query::Eq(REPO_NODE_LABEL_PARENTS, node.getSharedID()));
}