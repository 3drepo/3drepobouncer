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
#include "repo_bson_builder.h"

using namespace repo::core::model;

RevisionNode::RevisionNode(RepoBSON bson) :
	RepoNode(bson)
{
	deserialise(bson);
}

RevisionNode::RevisionNode() :
	RepoNode()
{
	status = UploadStatus::COMPLETE;
	timestamp = 0;
}

RevisionNode::~RevisionNode()
{
}

void RevisionNode::deserialise(RepoBSON& bson)
{
	status = UploadStatus::COMPLETE;
	if (bson.hasField(REPO_NODE_REVISION_LABEL_INCOMPLETE))
	{
		status = (UploadStatus)bson.getIntField(REPO_NODE_REVISION_LABEL_INCOMPLETE);
	}
	if (bson.hasField(REPO_NODE_REVISION_LABEL_AUTHOR))
	{
		author = bson.getStringField(REPO_NODE_REVISION_LABEL_AUTHOR);
	}
	timestamp = bson.getTimeStampField(REPO_NODE_REVISION_LABEL_TIMESTAMP);
}


void RevisionNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	RepoNode::serialise(builder);
	if (status != UploadStatus::COMPLETE)
	{
		builder.append(REPO_NODE_REVISION_LABEL_INCOMPLETE, (uint32_t)status);
	}
	if (!author.empty()) {
		builder.append(REPO_NODE_REVISION_LABEL_AUTHOR, author);
	}

	builder.appendTime(REPO_NODE_REVISION_LABEL_TIMESTAMP, (int64_t)timestamp);
}