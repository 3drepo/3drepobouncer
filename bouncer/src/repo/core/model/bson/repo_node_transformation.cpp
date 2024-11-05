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
#include "repo_bson_builder.h"

using namespace repo::core::model;

TransformationNode::TransformationNode() :
	RepoNode()
{
	// (The default matrix constructor initialises itself to identity)
}

TransformationNode::TransformationNode(RepoBSON bson) :
	RepoNode(bson)
{
	deserialise(bson);
}

TransformationNode::~TransformationNode()
{
}

void TransformationNode::deserialise(RepoBSON& bson)
{
	if (bson.hasField(REPO_NODE_LABEL_MATRIX)) {
		matrix = bson.getMatrixField(REPO_NODE_LABEL_MATRIX);
	}
}

void TransformationNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	RepoNode::serialise(builder);
	builder.append(REPO_NODE_LABEL_MATRIX, matrix);
}

void TransformationNode::applyTransformation(const repo::lib::RepoMatrix& t)
{
	matrix = matrix * t;
}

void TransformationNode::setTransformation(const repo::lib::RepoMatrix& t)
{
	matrix = t;
}

bool TransformationNode::isIdentity(const float &eps) const
{
	return  getTransMatrix().isIdentity(eps);
}

bool TransformationNode::sEqual(const RepoNode &other) const
{
	if (other.getTypeAsEnum() != NodeType::TRANSFORMATION)
	{
		return false;
	}

	auto node = dynamic_cast<const TransformationNode&>(other);
	return matrix == node.matrix;
}