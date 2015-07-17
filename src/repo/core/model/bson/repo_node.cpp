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
* Repo Node - A node representation of RepoBSON
*/

#include "repo_node.h"

using namespace repo::core::model::bson;

RepoNode::RepoNode(RepoBSON bson) : RepoBSON(bson){
	//--------------------------------------------------------------------------
	// Type
	if (bson.hasField(REPO_NODE_LABEL_TYPE))
		type = bson.getField(REPO_NODE_LABEL_TYPE).String();
	else
		type = REPO_NODE_TYPE_UNKNOWN; // failsafe

	//--------------------------------------------------------------------------
	// ID
	uniqueID = bson.getUUIDField(REPO_NODE_LABEL_ID);

	//--------------------------------------------------------------------------
	// Shared ID
	if (bson.hasField(REPO_NODE_LABEL_SHARED_ID))
		sharedID = bson.getUUIDField(REPO_NODE_LABEL_SHARED_ID);

}

RepoNode::~RepoNode()
{
}

RepoNode* RepoNode::createRepoNode(
	const std::string &type,
	const unsigned int api,
	const repoUUID &sharedId,
	const std::string &name,
	const std::vector<repoUUID> &parents)
{
	RepoBSONBuilder builder;
	
	appendDefaults(builder, type, api, sharedId, name, parents);

	return new RepoNode(builder.obj());
}

RepoNode RepoNode::cloneAndAddParent(repoUUID parentID)
{
	RepoBSONBuilder builder;
	RepoBSONBuilder arrayBuilder;


	std::vector<repoUUID> currentParents = getParentIDs();
	currentParents.push_back(parentID);
	
	builder.appendArray(REPO_NODE_LABEL_PARENTS, arrayBuilder.createArrayBSON(currentParents));

	builder.appendElementsUnique(*this);

	return RepoNode(builder.obj());
}

std::vector<repoUUID> RepoNode::getParentIDs()
{
	return getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);
}