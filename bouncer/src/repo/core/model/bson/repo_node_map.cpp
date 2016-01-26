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

std::vector<float> MapNode::getTransformationMatrix(const bool &rowMajor) const
{
	std::vector<float> transformationMatrix;
	uint32_t rowInd = 0, colInd = 0;
	if (hasField(REPO_NODE_MAP_LABEL_TRANS))
	{

		transformationMatrix.resize(16);
		float *transArr = &transformationMatrix.at(0);

		// matrix is stored as array of arrays
		RepoBSON matrixObj =
			getField(REPO_NODE_MAP_LABEL_TRANS).embeddedObject();

		std::set<std::string> mFields;
		matrixObj.getFieldNames(mFields);
		for (auto &field : mFields)
		{
			RepoBSON arrayObj = matrixObj.getField(field).embeddedObject();
			std::set<std::string> aFields;
			arrayObj.getFieldNames(aFields);
			for (auto &aField : aFields)
			{

				//figure out the index depending on if it's row or col major
				uint32_t index;
				if (rowMajor)
				{
					index = colInd * 4 + rowInd;
				}
				else
				{
					index = rowInd * 4 + colInd;
				}

				auto f = arrayObj.getField(aField);
				if (f.type() == ElementType::DOUBLE)
					transArr[index] = (float)f.Double();
				else if (f.type() == ElementType::INT)
					transArr[index] = (float)f.Int();
				else
				{
					repoError << "Unexpected type within transformation matrix!";
				}

				++colInd;
			}
			colInd = 0;
			++rowInd;
		}


	}
	else
	{
		//No trans matrix, return identity
		transformationMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	}
	return transformationMatrix;
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