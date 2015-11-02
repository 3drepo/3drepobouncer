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

using namespace repo::core::model;

RepoNode::RepoNode(RepoBSON bson,
	const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping) : RepoBSON(bson, binMapping){
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

	if (binMapping.size() == 0)
		bigFiles = bson.getFilesMapping();

}

RepoNode::~RepoNode()
{
}

RepoNode RepoNode::cloneAndAddParent(
	const repoUUID &parentID) const
{
	RepoBSONBuilder builder;
	RepoBSONBuilder arrayBuilder;


	std::vector<repoUUID> currentParents = getParentIDs();
	currentParents.push_back(parentID);
	builder.appendArray(REPO_NODE_LABEL_PARENTS, currentParents);	

	builder.appendElementsUnique(*this);

	return RepoNode(builder.obj(), bigFiles);
}

RepoNode RepoNode::cloneAndAddParent(
	const std::vector<repoUUID> &parentIDs) const
{
	RepoBSONBuilder builder;
	RepoBSONBuilder arrayBuilder;


	std::vector<repoUUID> currentParents = getParentIDs();
	currentParents.insert(currentParents.end(), parentIDs.begin(), parentIDs.end());

	builder.appendArray(REPO_NODE_LABEL_PARENTS, currentParents);

	builder.appendElementsUnique(*this);

	return RepoNode(builder.obj(), bigFiles);
}

RepoNode RepoNode::cloneAndAddFields(
	const RepoBSON *changes,
	const bool     &newUniqueID) const
{
	RepoBSONBuilder builder;
	if (newUniqueID)
	{
		builder.append(REPO_NODE_LABEL_ID, generateUUID());
	}
	else
	{
		builder.append(REPO_NODE_LABEL_ID, getUniqueID());
	}

	builder.append(REPO_NODE_LABEL_SHARED_ID, getSharedID());

	if (hasField(REPO_NODE_LABEL_PARENTS))
		builder.appendArray(REPO_NODE_LABEL_PARENTS, getField(REPO_NODE_LABEL_PARENTS).embeddedObject());

	builder.appendElementsUnique(*changes);

	builder.appendElementsUnique(*this);

	return RepoNode(builder.obj(), bigFiles);
}

RepoNode RepoNode::cloneAndAddMergedNodes(
	const std::vector<repoUUID> &mergeMap) const
{
	RepoBSONBuilder builder;
	RepoBSONBuilder arrayBuilder;
	builder.appendArray(REPO_LABEL_MERGED_NODES, mergeMap);

	builder.appendElementsUnique(*this);

	return RepoNode(builder.obj(), bigFiles);
}

std::vector<repoUUID> RepoNode::getParentIDs() const
{
	return getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);
}


NodeType RepoNode::getTypeAsEnum() const
{ 
	std::string type = getType(); 

	NodeType enumType = NodeType::UNKNOWN;

	if (REPO_NODE_TYPE_CAMERA == type)
		enumType = NodeType::CAMERA;
	else if (REPO_NODE_TYPE_MAP == type)
		enumType = NodeType::MAP;
	else if (REPO_NODE_TYPE_MATERIAL == type)
		enumType = NodeType::MATERIAL;
	else if (REPO_NODE_TYPE_MESH == type)
		enumType = NodeType::MESH;
	else if (REPO_NODE_TYPE_METADATA == type)
		enumType = NodeType::METADATA;
	else if (REPO_NODE_TYPE_REFERENCE == type)
		enumType = NodeType::REFERENCE;
	else if (REPO_NODE_TYPE_REVISION == type)
		enumType = NodeType::REVISION;
	else if (REPO_NODE_TYPE_TEXTURE == type)
		enumType = NodeType::TEXTURE;
	else if (REPO_NODE_TYPE_TRANSFORMATION == type)
		enumType = NodeType::TRANSFORMATION;

	return enumType;

}