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

RepoNode::RepoNode(const RepoBSON& bson) {
	uniqueId = repo::lib::RepoUUID::defaultValue;
	sharedId = repo::lib::RepoUUID::defaultValue;
	revId = repo::lib::RepoUUID::defaultValue;
	deserialise(bson);
}

RepoNode::RepoNode()
{
	uniqueId = repo::lib::RepoUUID::createUUID();
	sharedId = repo::lib::RepoUUID::defaultValue;
	revId = repo::lib::RepoUUID::defaultValue;
}

RepoNode::operator RepoBSON() const
{
	return getBSON();
}

RepoBSON RepoNode::getBSON() const {
	RepoBSONBuilder builder;
	serialise(builder);
	return RepoBSON(builder.obj(), builder.mapping());
}

void RepoNode::deserialise(const RepoBSON& bson) {
	if (bson.hasField(REPO_NODE_LABEL_ID))
	{
		uniqueId = bson.getUUIDField(REPO_NODE_LABEL_ID);
	}
	if (bson.hasField(REPO_NODE_LABEL_NAME))
	{
		name = bson.getStringField(REPO_NODE_LABEL_NAME);
	}
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
}

RepoNode::~RepoNode()
{
}

// In the future we should consider replacing this with RTTI

NodeType RepoNode::getTypeAsEnum() const
{
	return parseType(getType());
}

NodeType RepoNode::parseType(const std::string& nodeType)
{
	if (REPO_NODE_TYPE_TRANSFORMATION == nodeType) {
		return repo::core::model::NodeType::TRANSFORMATION;
	}
	else if (REPO_NODE_TYPE_MESH == nodeType) {
		return repo::core::model::NodeType::MESH;
	}
	else if (REPO_NODE_TYPE_MATERIAL == nodeType) {
		return repo::core::model::NodeType::MATERIAL;
	}
	else if (REPO_NODE_TYPE_TEXTURE == nodeType) {
		return repo::core::model::NodeType::TEXTURE;
	}
	else if (REPO_NODE_TYPE_REFERENCE == nodeType) {
		return repo::core::model::NodeType::REFERENCE;
	}
	else if (REPO_NODE_TYPE_METADATA == nodeType) {
		return repo::core::model::NodeType::METADATA;
	}
	else if (REPO_NODE_TYPE_REVISION == nodeType) {
		return repo::core::model::NodeType::REVISION;
	}
	else {
		return repo::core::model::NodeType::UNKNOWN;
	}
}

bool RepoNode::sEqual(const RepoNode& other) const
{
	throw lib::RepoException("sEqual called on two RepoNode base class instances. This comparison is meaningless and likely indicates a logic error.");
}