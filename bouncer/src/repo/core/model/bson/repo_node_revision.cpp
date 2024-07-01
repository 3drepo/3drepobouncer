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

RevisionNode::UploadStatus RevisionNode::getUploadStatus() const
{
	UploadStatus status = UploadStatus::COMPLETE;
	if (hasField(REPO_NODE_REVISION_LABEL_INCOMPLETE))
	{
		status = (UploadStatus)getIntField(REPO_NODE_REVISION_LABEL_INCOMPLETE);
	}

	return status;
}

std::string RevisionNode::getAuthor() const
{
	return getStringField(REPO_NODE_REVISION_LABEL_AUTHOR);
}

int64_t RevisionNode::getTimestampInt64() const
{
	return getTimeStampField(REPO_NODE_REVISION_LABEL_TIMESTAMP);
}