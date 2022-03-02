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
#include "repo_bson_builder.h"

using namespace repo::core::model;

RepoNode::RepoNode(RepoBSON bson,
	const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping) : RepoBSON(bson, binMapping) {
	if (binMapping.size() == 0)
		bigFiles = bson.getFilesMapping();
}

RepoNode::~RepoNode()
{
}

RepoNode RepoNode::cloneAndChangeIdentity() const {
	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_ID, repo::lib::RepoUUID::createUUID());

	builder.append(REPO_NODE_LABEL_SHARED_ID, repo::lib::RepoUUID::createUUID());

	builder.appendElementsUnique(*this);

	return RepoNode(builder.obj(), bigFiles);
}

RepoNode RepoNode::cloneAndAddParent(
	const repo::lib::RepoUUID &parentID,
	const bool     &newUniqueID,
	const bool     &newSharedID,
	const bool     &overwrite) const
{
	RepoBSONBuilder builder;
	RepoBSONBuilder arrayBuilder;

	std::vector<repo::lib::RepoUUID> currentParents;
	if (!overwrite)
	{
		currentParents = getParentIDs();
	}

	if (std::find(currentParents.begin(), currentParents.end(), parentID) == currentParents.end())
		currentParents.push_back(parentID);
	builder.appendArray(REPO_NODE_LABEL_PARENTS, currentParents);

	if (newUniqueID)
		builder.append(REPO_NODE_LABEL_ID, repo::lib::RepoUUID::createUUID());

	if (newSharedID)
		builder.append(REPO_NODE_LABEL_SHARED_ID, repo::lib::RepoUUID::createUUID());

	builder.appendElementsUnique(*this);

	return RepoNode(builder.obj(), bigFiles);
}

RepoNode RepoNode::cloneAndAddParent(
	const std::vector<repo::lib::RepoUUID> &parentIDs) const
{
	if (!parentIDs.size()) return RepoNode(*this, bigFiles);

	RepoBSONBuilder builder;
	RepoBSONBuilder arrayBuilder;
	std::set<std::string> currentParents;
	for (const auto &parent : getParentIDs()) {
		currentParents.insert(parent.toString());
	}

	for (const auto &parent : parentIDs) {
		currentParents.insert(parent.toString());
	}

	std::vector<repo::lib::RepoUUID> newParents;
	for (const auto &parent : currentParents) {
		newParents.push_back(repo::lib::RepoUUID(parent));
	}

	builder.appendArray(REPO_NODE_LABEL_PARENTS, newParents);

	builder.appendElementsUnique(*this);

	return RepoNode(builder.obj(), bigFiles);
}

RepoNode RepoNode::cloneAndRemoveParent(
	const repo::lib::RepoUUID &parentID,
	const bool     &newUniqueID) const
{
	RepoBSONBuilder builder;
	RepoBSONBuilder arrayBuilder;

	std::vector<repo::lib::RepoUUID> currentParents = getParentIDs();
	auto parentIdx = std::find(currentParents.begin(), currentParents.end(), parentID);
	if (parentIdx != currentParents.end())
	{
		currentParents.erase(parentIdx);
		if (newUniqueID)
		{
			builder.append(REPO_NODE_LABEL_ID, repo::lib::RepoUUID::createUUID());
		}
	}
	else
	{
		repoWarning << "Trying to remove a parent that isn't really a parent!";
	}
	if (currentParents.size() > 0)
	{
		builder.appendArray(REPO_NODE_LABEL_PARENTS, currentParents);
		builder.appendElementsUnique(*this);
	}
	else
	{
		builder.appendElementsUnique(removeField(REPO_NODE_LABEL_PARENTS));
	}

	return RepoNode(builder.obj(), bigFiles);
}

RepoNode RepoNode::cloneAndAddRevId(
	const repo::lib::RepoUUID &revId) const
{
	RepoBSONBuilder builder;

	builder.append(REPO_NODE_REVISION_ID, revId);

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
		builder.append(REPO_NODE_LABEL_ID, repo::lib::RepoUUID::createUUID());
	}

	builder.appendElementsUnique(*changes);

	builder.appendElementsUnique(*this);

	return RepoNode(builder.obj(), bigFiles);
}

std::vector<repo::lib::RepoUUID> RepoNode::getParentIDs() const
{
	return getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);
}

NodeType RepoNode::getTypeAsEnum() const
{
	std::string type = getType();

	NodeType enumType = NodeType::UNKNOWN;

	if (REPO_NODE_TYPE_CAMERA == type)
		enumType = NodeType::CAMERA;
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