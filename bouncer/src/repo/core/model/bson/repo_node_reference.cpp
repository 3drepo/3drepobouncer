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
*  Reference node
*/

#include "repo_node_reference.h"
#include "repo_bson_builder.h"

using namespace repo::core::model;

ReferenceNode::ReferenceNode() :
RepoNode()
{
	isUnique = false;
}

ReferenceNode::ReferenceNode(RepoBSON bson) :
RepoNode(bson)
{
	revisionId = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH);
	deserialise(bson);
}

ReferenceNode::~ReferenceNode()
{
}

void ReferenceNode::deserialise(RepoBSON& bson)
{
	databaseName = bson.getStringField(REPO_NODE_REFERENCE_LABEL_OWNER);
	projectId = bson.getStringField(REPO_NODE_REFERENCE_LABEL_PROJECT);

	if (bson.hasField(REPO_NODE_REFERENCE_LABEL_REVISION_ID))
	{
		revisionId = bson.getUUIDField(REPO_NODE_REFERENCE_LABEL_REVISION_ID);
	}

	if (bson.hasField(REPO_NODE_REFERENCE_LABEL_UNIQUE))
	{
		isUnique = bson.getBoolField(REPO_NODE_REFERENCE_LABEL_UNIQUE);
	}
}


void ReferenceNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	RepoNode::serialise(builder);

	builder.append(REPO_NODE_REFERENCE_LABEL_OWNER, databaseName);
	builder.append(REPO_NODE_REFERENCE_LABEL_PROJECT, projectId);

	if (revisionId != repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH))
	{
		builder.append(REPO_NODE_REFERENCE_LABEL_REVISION_ID, revisionId);
	}

	if (isUnique) 
	{
		builder.append(REPO_NODE_REFERENCE_LABEL_UNIQUE, isUnique);
	}
}

bool ReferenceNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::REFERENCE)
	{
		return false;
	}

	auto& node = dynamic_cast<const ReferenceNode&>(other);

	if (revisionId != node.revisionId)
	{
		return false;
	}

	if (databaseName != node.databaseName) 
	{
		return false;
	}

	if (projectId != node.projectId)
	{
		return false;
	}

	if (isUnique != node.isUnique) 
	{
		return false;
	}

	return true;
}