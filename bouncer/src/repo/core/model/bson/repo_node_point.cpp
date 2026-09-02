/**
*  Copyright (C) 2026 3D Repo Ltd
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

#include "repo_node_point.h"
#include "repo_bson_builder.h"


using namespace repo::core::model;

PointNode::PointNode() :
	RepoNode()
{
}

PointNode::PointNode(RepoBSON bson) :
	RepoNode(bson)
{
	deserialise(bson);
}

PointNode::~PointNode()
{
}

void PointNode::deserialise(RepoBSON& bson)
{
	if (bson.hasField(REPO_NODE_POINT_LABEL_TREE_POSITION))
		treePosition = bson.getByteArray(REPO_NODE_POINT_LABEL_TREE_POSITION);

	if (bson.hasField(REPO_NODE_POINT_LABEL_BOUNDING_BOX))
		boundingBox = bson.getBoundsField(REPO_NODE_POINT_LABEL_BOUNDING_BOX);

	if (bson.hasBinField(REPO_NODE_POINT_LABEL_POINTS))
	{
		bson.getBinaryFieldAsVector(REPO_NODE_POINT_LABEL_POINTS, points);
	}

	if (bson.hasBinField(REPO_NODE_POINT_LABEL_COLOURS))
	{
		bson.getBinaryFieldAsVector(REPO_NODE_POINT_LABEL_COLOURS, colourAttributes);
	}
}

static void appendBounds(RepoBSONBuilder& builder, const repo::lib::RepoBounds& boundingBox)
{
	 builder.append(REPO_NODE_POINT_LABEL_BOUNDING_BOX, boundingBox);
}

static void appendPoints(RepoBSONBuilder& builder, const std::vector<repo::lib::RepoVector3D>& points)
{
	if (points.size() > 0)
	{
		builder.append(REPO_NODE_POINT_LABEL_POINTS_COUNT, (int32_t)(points.size()));
		builder.appendLargeArray(REPO_NODE_POINT_LABEL_POINTS, points);
	}
}

static void appendColourAttributes(RepoBSONBuilder& builder, const std::vector<repo::lib::repo_color4d_t>& colourAttributes)
{
	if (colourAttributes.size() > 0)
	{
		builder.appendLargeArray(REPO_NODE_POINT_LABEL_COLOURS, colourAttributes);
	}
}

static void appendTreePosition(RepoBSONBuilder& builder, const std::vector<uint8_t>& treePosition)
{
	if (treePosition.size() > 0)
	{
		builder.appendByteArray(REPO_NODE_POINT_LABEL_TREE_POSITION, treePosition);
	}
}

void PointNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	RepoNode::serialise(builder);
	appendBounds(builder, boundingBox);
	appendTreePosition(builder, treePosition);
	
	if (points.size() != colourAttributes.size())
	{
		throw repo::lib::RepoException("Attribute length mismatch on point node serialisation.");
	}
	appendPoints(builder, points);
	appendColourAttributes(builder, colourAttributes);
}

// TODO FT: Revisit whether we really need this
PointNode PointNode::cloneAndApplyTransformation(
	const repo::lib::RepoMatrix& matrix) const
{
	PointNode copy = *this;	// Copy the node
	copy.applyTransformation(matrix);
	return copy;
}

// TODO FT: Revisit whether we really need this
void PointNode::applyTransformation(
	const repo::lib::RepoMatrix& matrix)
{
	if (!matrix.isIdentity())
	{
		boundingBox = repo::lib::RepoBounds();

		for (auto& v : points) {
			v = matrix * v;
			boundingBox.encapsulate(v);
		}
	}
}

void PointNode::addPoint(const PointData& point)
{
	points.push_back(point.position);
	colourAttributes.push_back(point.colour);
}

bool PointNode::sEqual(const RepoNode& other) const
{
	auto otherPoint = dynamic_cast<const PointNode*>(&other);

	bool success = false;

	if (otherPoint != nullptr)
	{
		success = true;
		success &= boundingBox == otherPoint->boundingBox;
		success &= treePosition == otherPoint->treePosition;
		success &= points == otherPoint->points;
		success &= colourAttributes == otherPoint->colourAttributes;
	}

	return success;
}