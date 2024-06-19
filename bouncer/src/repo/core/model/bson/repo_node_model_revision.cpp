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

using namespace repo::core::model;

ModelRevisionNode::ModelRevisionNode(RepoBSON bson) :
	RevisionNode(bson)
{
}
ModelRevisionNode::ModelRevisionNode() :
	RevisionNode()
{
}

ModelRevisionNode::~ModelRevisionNode()
{
}

ModelRevisionNode ModelRevisionNode::cloneAndUpdateStatus(
	const UploadStatus &status) const
{
	switch (status)
	{
	case UploadStatus::COMPLETE:
		return RepoNode(removeField(REPO_NODE_REVISION_LABEL_INCOMPLETE), bigFiles);
	case UploadStatus::UNKNOWN:
		repoError << "Cannot set the status flag to Unknown state!";
		return *this;
	default:
		RepoBSON bsonChange = BSON(REPO_NODE_REVISION_LABEL_INCOMPLETE << (int)status);
		return cloneAndAddFields(&bsonChange, false);
	}
}

std::string ModelRevisionNode::getMessage() const
{
	return getStringField(REPO_NODE_REVISION_LABEL_MESSAGE);
}

std::string ModelRevisionNode::getTag() const
{
	return getStringField(REPO_NODE_REVISION_LABEL_TAG);
}

std::vector<double> ModelRevisionNode::getCoordOffset() const
{
	std::vector<double> offset;
	if (hasField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT))
	{
		auto offsetObj = getObjectField(REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT);
		if (!offsetObj.isEmpty())
		{
			for (int i = 0; i < 3; ++i)
			{
				offset.push_back(offsetObj.getDoubleField(std::to_string(i)));
			}
		}
		else
		{
			offset.push_back(0);
			offset.push_back(0);
			offset.push_back(0);
		}
	}
	else
	{
		offset.push_back(0);
		offset.push_back(0);
		offset.push_back(0);
	}

	return offset;
}

std::vector<std::string> ModelRevisionNode::getOrgFiles() const
{
	std::vector<std::string> fileList;
	if (hasField(REPO_NODE_REVISION_LABEL_REF_FILE))
	{
		RepoBSON arraybson = getObjectField(REPO_NODE_REVISION_LABEL_REF_FILE);

		std::set<std::string> fields = arraybson.getFieldNames();

		for (const auto &field : fields)
		{
			fileList.push_back(arraybson.getStringField(field));
		}
	}

	return fileList;
}

