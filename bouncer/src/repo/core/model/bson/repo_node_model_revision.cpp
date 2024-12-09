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

#include "repo_node_model_revision.h"
#include "repo_bson_builder.h"

using namespace repo::core::model;

ModelRevisionNode::ModelRevisionNode(RepoBSON bson) :
	RevisionNode(bson)
{
	offset = { 0, 0, 0 };
	deserialise(bson);
}

ModelRevisionNode::ModelRevisionNode() :
	RevisionNode()
{
	status = UploadStatus::COMPLETE;
	offset = { 0, 0, 0 };
}

ModelRevisionNode::~ModelRevisionNode()
{
}

void ModelRevisionNode::deserialise(RepoBSON& bson)
{
	status = UploadStatus::COMPLETE;
	if (bson.hasField(REPO_NODE_REVISION_LABEL_INCOMPLETE))
	{
		status = (UploadStatus)bson.getIntField(REPO_NODE_REVISION_LABEL_INCOMPLETE);
	}
	if (bson.hasField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT))
	{
		offset = bson.getDoubleVectorField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT);
	}
	if (bson.hasField(REPO_NODE_REVISION_LABEL_REF_FILE))
	{
		files = bson.getFileList(REPO_NODE_REVISION_LABEL_REF_FILE);
	}
	if (bson.hasField(REPO_NODE_REVISION_LABEL_MESSAGE))
	{
		message = bson.getStringField(REPO_NODE_REVISION_LABEL_MESSAGE);
	}
	if (bson.hasField(REPO_NODE_REVISION_LABEL_TAG))
	{
		tag = bson.getStringField(REPO_NODE_REVISION_LABEL_TAG);
	}
}

void ModelRevisionNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	RevisionNode::serialise(builder);
	builder.append(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT, offset); // This is never empty because it initialises to 0,0,0.
	if (files.size())
	{
		builder.appendArray(REPO_NODE_REVISION_LABEL_REF_FILE, files);
	}
	if (!message.empty())
	{
		builder.append(REPO_NODE_REVISION_LABEL_MESSAGE, message);
	}
	if (!tag.empty())
	{
		builder.append(REPO_NODE_REVISION_LABEL_TAG, tag);
	}
	if (status != UploadStatus::COMPLETE)
	{
		builder.append(REPO_NODE_REVISION_LABEL_INCOMPLETE, (uint32_t)status);
	}
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId); // By convention the ModelRevisionNode always has a SharedId member, even if zero
}

bool ModelRevisionNode::sEqual(const RepoNode& other) const
{
	auto otherRevision = dynamic_cast<const ModelRevisionNode*>(&other);

	bool success = false;
	if (otherRevision != nullptr)
	{
		success = offset == otherRevision->offset;
		success &= message == otherRevision->message;
		success &= tag == otherRevision->tag;
		success &= files == otherRevision->files;
	}

	return success;
}
