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

using namespace repo::core::model::bson;

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

TransformationNode* TransformationNode::createTransformationNode(
	const std::vector<std::vector<float>> &transMatrix,
	const std::string                     &name,
	const std::vector<repo_uuid>		  &parents,
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
			columns << boost::lexical_cast<std::string>(j) << transMatrix[i][j];
		}
		rows.appendArray(boost::lexical_cast<std::string>(i), columns.obj());
	}
	builder.appendArray(REPO_NODE_LABEL_MATRIX, rows.obj());

	return new TransformationNode(builder.obj());
}
