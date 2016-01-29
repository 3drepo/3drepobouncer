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
*  Map node
*/

#include "repo_node_map.h"

using namespace repo::core::model;

MapNode::MapNode() :
RepoNode()
{
}

MapNode::MapNode(RepoBSON bson) :
RepoNode(bson)
{

}

MapNode::~MapNode()
{
}

repo_vector_t MapNode::getCentre() const
{
	repo_vector_t centre;
	RepoBSON centreArr = getObjectField(REPO_NODE_MAP_LABEL_TRANS);
	if (!centreArr.isEmpty() && centreArr.couldBeArray())
	{
		size_t nVec = centreArr.nFields();
		int32_t nFields = centreArr.nFields();

			if (nFields >= 3)
			{
				centre.x = centreArr.getField("0").Double();
				centre.y = centreArr.getField("1").Double();
				centre.z = centreArr.getField("2").Double();

			}
			else
			{
				repoError << "Insufficient amount of elements within centre point! #fields: " << nFields;
			}

	}	
	else
	{
		repoError << "Failed to fetch centre point from Map Node!";
	}

	return centre;
}

float MapNode::getLat() const
{
	float lat = 0;

	if (hasField(REPO_NODE_MAP_LABEL_LAT))
	{
		lat = getField(REPO_NODE_MAP_LABEL_LAT).Double();
	}

	return lat;
}

float MapNode::getLong() const
{
	float longit = 0;

	if (hasField(REPO_NODE_MAP_LABEL_LONG))
	{
		longit = getField(REPO_NODE_MAP_LABEL_LONG).Double();
	}

	return longit;
}

float MapNode::getTileSize() const
{
	float tileSize = 1;

	if (hasField(REPO_NODE_MAP_LABEL_TILESIZE))
	{
		tileSize = getField(REPO_NODE_MAP_LABEL_TILESIZE).Double();
	}

	return tileSize;
}

uint32_t MapNode::getWidth() const
{
	uint32_t width = 1;

	if (hasField(REPO_NODE_MAP_LABEL_WIDTH))
	{
		width = getField(REPO_NODE_MAP_LABEL_WIDTH).Int();
	}


	return width;
}

float MapNode::getYRot() const
{
	float yrot = 0;

	if (hasField(REPO_NODE_MAP_LABEL_YROT))
	{
		yrot = getField(REPO_NODE_MAP_LABEL_YROT).Double();
	}

	return yrot;
}

uint32_t MapNode::getZoom() const
{
	uint32_t zoom = 0;

	if (hasField(REPO_NODE_MAP_LABEL_ZOOM))
	{
		zoom = getField(REPO_NODE_MAP_LABEL_ZOOM).Int();
	}

	return zoom;
}

bool MapNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::MAP || other.getParentIDs().size() != getParentIDs().size())
	{
		return false;
	}

	//TODO:
	repoError << "Semantic comparison of map nodes are currently not supported!";
	return false;
}