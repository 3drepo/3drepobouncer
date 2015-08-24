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
* A Revision Node - storing information about a specific revision
*/


#include "repo_node_revision.h"

using namespace repo::core::model;


RevisionNode::RevisionNode(RepoBSON bson) :
	RepoNode(bson)
{

}
RevisionNode::RevisionNode() :
	RepoNode()
{
}


RevisionNode::~RevisionNode()
{
}

RevisionNode RevisionNode::createRevisionNode(
	const std::string			 &user,
	const repoUUID              &branch,
	const std::vector<repoUUID> &currentNodes,
	const std::vector<repoUUID> &added,
	const std::vector<repoUUID> &removed,
	const std::vector<repoUUID> &modified,
	const std::vector<repoUUID> &parent,
	const std::string            &message,
	const std::string            &tag,
	const int                    &apiLevel
	)
{
	RepoBSONBuilder builder;

	//--------------------------------------------------------------------------
	// Compulsory fields such as _id, type, api as well as path
	appendDefaults(builder, REPO_NODE_TYPE_REVISION, apiLevel, branch, "", parent);

	//--------------------------------------------------------------------------
	// Author
	if (!user.empty())
		builder << REPO_NODE_REVISION_LABEL_AUTHOR << user;

	//--------------------------------------------------------------------------
	// Message
	if (!message.empty())
		builder << REPO_NODE_REVISION_LABEL_MESSAGE << message;

	//--------------------------------------------------------------------------
	// Tag
	if (!tag.empty())
		builder << REPO_NODE_REVISION_LABEL_TAG << tag;

	//--------------------------------------------------------------------------
	// Timestamp
	builder.appendTimeStamp(REPO_NODE_REVISION_LABEL_TIMESTAMP);

	//--------------------------------------------------------------------------
	
	// Current Unique IDs
	if (currentNodes.size() > 0)
		builder.appendArray(REPO_NODE_REVISION_LABEL_CURRENT_UNIQUE_IDS, builder.createArrayBSON(currentNodes));

	//--------------------------------------------------------------------------
	// Added Shared IDs
		
	if (added.size() > 0)
		builder.appendArray(REPO_NODE_REVISION_LABEL_ADDED_SHARED_IDS, builder.createArrayBSON(added));

	//--------------------------------------------------------------------------
	// Deleted Shared IDs		
	if (removed.size() > 0)
		builder.appendArray(REPO_NODE_REVISION_LABEL_DELETED_SHARED_IDS, builder.createArrayBSON(removed));

	//--------------------------------------------------------------------------
	// Modified Shared IDs
	if (modified.size() > 0)
		builder.appendArray(REPO_NODE_REVISION_LABEL_MODIFIED_SHARED_IDS, builder.createArrayBSON(modified));

	//--------------------------------------------------------------------------
	return RevisionNode(builder.obj());

}



std::string RevisionNode::getAuthor() const
{
	return getStringField(REPO_NODE_REVISION_LABEL_AUTHOR);
}

std::string RevisionNode::getMessage() const
{
	return getStringField(REPO_NODE_REVISION_LABEL_MESSAGE);
}

std::vector<repoUUID> RevisionNode::getCurrentIDs() const
{
	return getUUIDFieldArray(REPO_NODE_REVISION_LABEL_CURRENT_UNIQUE_IDS);
}

std::vector<repoUUID> RevisionNode::getAddedIDs() const
{
 	return getUUIDFieldArray(REPO_NODE_REVISION_LABEL_ADDED_SHARED_IDS);
}

std::vector<repoUUID> RevisionNode::getModifiedIDs() const
{
	return getUUIDFieldArray(REPO_NODE_REVISION_LABEL_MODIFIED_SHARED_IDS);
}

std::vector<repoUUID> RevisionNode::getDeletedIDs() const
{
	return getUUIDFieldArray(REPO_NODE_REVISION_LABEL_DELETED_SHARED_IDS);
}


int64_t RevisionNode::getTimestampInt64() const
{
	return getTimeStampField(REPO_NODE_REVISION_LABEL_TIMESTAMP);
}