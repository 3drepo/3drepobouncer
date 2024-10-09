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
#include "../../../lib/repo_exception.h"

using namespace repo::core::model;

RepoNode::RepoNode(RepoBSON bson,
	const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping) {
	uniqueId = repo::lib::RepoUUID::createUUID();
	sharedId = repo::lib::RepoUUID::defaultValue;
	revId = repo::lib::RepoUUID::defaultValue;
	bigFiles = binMapping;
	deserialise(bson);
}

RepoNode::RepoNode()
{
	uniqueId = repo::lib::RepoUUID::createUUID();
	sharedId = repo::lib::RepoUUID::defaultValue;
	revId = repo::lib::RepoUUID::defaultValue;
}

RepoBSON RepoNode::getBSON() const {
	RepoBSONBuilder builder;
	serialise(builder);
	return RepoBSON(builder.obj(), builder.mapping());
}

void RepoNode::deserialise(RepoBSON& bson) {
	uniqueId = bson.getUUIDField(REPO_NODE_LABEL_ID);
	name = bson.getStringField(REPO_NODE_LABEL_NAME);
	if (bson.hasField(REPO_NODE_LABEL_SHARED_ID))
	{
		sharedId = bson.getUUIDField(REPO_NODE_LABEL_SHARED_ID);
	}
	if (bson.hasField(REPO_NODE_REVISION_ID))
	{
		revId = bson.getUUIDField(REPO_NODE_REVISION_ID);
	}
	auto parents = bson.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);
	parentIds = std::set<repo::lib::RepoUUID>(parents.begin(), parents.end());
}

void RepoNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	if (!name.empty()) {
		builder.append(REPO_NODE_LABEL_NAME, name);
	}
	if (!getType().empty())
	{
		builder.append(REPO_NODE_LABEL_TYPE, getType());
	}
	if (!sharedId.isDefaultValue()) {
		builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId);
	}
	builder.append(REPO_NODE_LABEL_ID, uniqueId);
	if (parentIds.size()) {
		builder.appendArray(REPO_NODE_LABEL_PARENTS, getParentIDs());
	}
	if (!revId.isDefaultValue()) {
		builder.append(REPO_NODE_REVISION_ID, revId);
	}
	for (auto& pair : bigFiles) {
		builder.appendLargeArray(pair.first, pair.second.second);
	}
}

RepoNode::~RepoNode()
{
}

RepoNode RepoNode::cloneAndAddParent(
	const repo::lib::RepoUUID &parentID,
	const bool     &newUniqueID,
	const bool     &newSharedID,
	const bool     &overwrite) const
{
	RepoBSONBuilder builder;

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

	builder.appendElementsUnique(getBSON());

	return RepoNode(builder.obj(), bigFiles);
}

RepoNode RepoNode::cloneAndAddParent(
	const std::vector<repo::lib::RepoUUID> &parentIDs) const
{
	if (!parentIDs.size()) return RepoNode(getBSON(), bigFiles);

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

	builder.appendElementsUnique(getBSON());

	return RepoNode(builder.obj(), bigFiles);
}

NodeType RepoNode::getTypeAsEnum() const // todo: get rid of this in favour of RTTI
{
	std::string type = getType();

	NodeType enumType = NodeType::UNKNOWN;

	if (REPO_NODE_TYPE_MATERIAL == type)
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

bool RepoNode::sEqual(const RepoNode& other) const
{
	throw lib::RepoException("sEqual called on two RepoNode base class instances. This comparison is meaningless and likely indicates a logic error.");
}