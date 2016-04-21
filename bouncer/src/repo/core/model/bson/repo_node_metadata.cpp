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
*  Metadata node
*/

#include "repo_node_metadata.h"

using namespace repo::core::model;

MetadataNode::MetadataNode() :
RepoNode()
{
}

MetadataNode::MetadataNode(RepoBSON bson) :
RepoNode(bson)
{
}

MetadataNode::~MetadataNode()
{
}

bool MetadataNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::METADATA || other.getParentIDs().size() != getParentIDs().size())
	{
		return false;
	}

	std::set<std::string> fieldNames, otherFieldNames;

	bool match = false;
	if (match = (getFieldNames(fieldNames) == other.getFieldNames(otherFieldNames)))
	{
		for (const std::string &fieldName : fieldNames)
		{
			auto it = otherFieldNames.find(fieldName);
			if (match = (it != otherFieldNames.end()))
			{
				RepoBSONElement element = getField(fieldName);
				RepoBSONElement otherElement = other.getField(fieldName);

				if (match = (element.type() == otherElement.type()))
				{
					//it'll get complicated to check the actual value, so just
					//rely on element comparison..?
					match = element == otherElement;
				}
			}
			if (!match) break;
		}
	}
	return match;
}