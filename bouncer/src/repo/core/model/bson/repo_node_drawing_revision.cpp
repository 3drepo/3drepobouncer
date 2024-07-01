/**
*  Copyright (C) 2024 3D Repo Ltd
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
* A Revision Node - storing information about a specific drawing revision
*/

#include "repo_node_drawing_revision.h"
#include "repo_bson_builder.h"

using namespace repo::core::model;

DrawingRevisionNode::DrawingRevisionNode(RepoBSON bson) :
	RevisionNode(bson)
{
}
DrawingRevisionNode::DrawingRevisionNode() :
	RevisionNode()
{
}

DrawingRevisionNode::~DrawingRevisionNode()
{
}

std::vector<repo::lib::RepoUUID> DrawingRevisionNode::getFiles() const
{
	if (hasField(REPO_NODE_REVISION_LABEL_REF_FILE))
	{
		return getUUIDFieldArray(REPO_NODE_REVISION_LABEL_REF_FILE);
	}
	return {};
}

repo::lib::RepoUUID DrawingRevisionNode::getProject() const
{
	if (hasField(REPO_NODE_DRAWING_REVISION_LABEL_PROJECT))
	{
		return getUUIDField(REPO_NODE_DRAWING_REVISION_LABEL_PROJECT);
	}
	return {};
}

repo::lib::RepoUUID DrawingRevisionNode::getModel() const
{
	if (hasField(REPO_NODE_DRAWING_REVISION_LABEL_MODEL))
	{
		return getUUIDField(REPO_NODE_DRAWING_REVISION_LABEL_MODEL);
	}
	return {};
}

std::string DrawingRevisionNode::getFormat() const
{
	if (hasField(REPO_NODE_LABEL_FORMAT))
	{
		return getStringField(REPO_NODE_LABEL_FORMAT);
	}
	return {};
}

DrawingRevisionNode DrawingRevisionNode::cloneAndAddImage(repo::lib::RepoUUID imageRefNodeId) const
{
	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_LABEL_IMAGE, imageRefNodeId);
	builder.appendElementsUnique(*this);
	return DrawingRevisionNode(builder.obj());
}
