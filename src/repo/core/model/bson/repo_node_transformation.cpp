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
*  Transformation node 
*/

#include "repo_node_transformation.h"

using namespace repo::core::model;

TransformationNode::TransformationNode() :
	RepoNode()
{
}

TransformationNode::TransformationNode(RepoBSON bson) :
	RepoNode(bson)
{

}

TransformationNode::~TransformationNode()
{
}

TransformationNode TransformationNode::createTransformationNode(
	const std::vector<std::vector<float>> &transMatrix,
	const std::string                     &name,
	const std::vector<repoUUID>		  &parents,
	const int                             &apiLevel)
{
	RepoBSONBuilder builder;

	appendDefaults(builder, REPO_NODE_TYPE_TRANSFORMATION, apiLevel, generateUUID(), name, parents);

	//--------------------------------------------------------------------------
	// Store matrix as array of arrays
	uint32_t matrixSize = 4;
	RepoBSONBuilder rows;
	for (uint32_t i = 0; i < transMatrix.size(); ++i)
	{
		RepoBSONBuilder columns;
		for (uint32_t j = 0; j < transMatrix[i].size(); ++j){
			columns << std::to_string(j) << transMatrix[i][j];
		}
		rows.appendArray(std::to_string(i), columns.obj());
	}
	builder.appendArray(REPO_NODE_LABEL_MATRIX, rows.obj());

	return TransformationNode(builder.obj());
}

std::vector<std::vector<float>> TransformationNode::identityMat()
{
	std::vector<std::vector<float>> idMat;
	idMat.push_back({ 1, 0, 0, 0 });
	idMat.push_back({ 0, 1, 0, 0 });
	idMat.push_back({ 0, 0, 1, 0 });
	idMat.push_back({ 0, 0, 0, 1 });
	return idMat;
}

std::vector<float> TransformationNode::getTransMatrix(const bool &rowMajor) const
{
	std::vector<float> transformationMatrix;

	uint32_t rowInd = 0, colInd = 0;
	if (hasField(REPO_NODE_LABEL_MATRIX))
	{

		transformationMatrix.resize(16);
		float *transArr = &transformationMatrix.at(0);
	
		// matrix is stored as array of arrays
		RepoBSON matrixObj =
			getField(REPO_NODE_LABEL_MATRIX).embeddedObject();
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
				transArr[index] = (float)arrayObj.getField(aField).Double();

				++colInd;
			}
			colInd = 0;
			++rowInd;
		}


	}
	return transformationMatrix;
}