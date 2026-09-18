/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo_node_streaming_point.h"

repo::core::model::StreamingPointNode::PointOptimisationData::PointOptimisationData(const repo::core::model::RepoBSON& bson, const std::vector<uint8_t>& buffer)
{
	this->uniqueId = bson.getUUIDField(REPO_NODE_LABEL_ID);
	deserialise(bson, buffer);
}

void repo::core::model::StreamingPointNode::PointOptimisationData::deserialise(const repo::core::model::RepoBSON& bson, const std::vector<uint8_t>& buffer)
{
	auto blobRefBson = bson.getObjectField(REPO_LABEL_BINARY_REFERENCE);
	auto elementsBson = blobRefBson.getObjectField(REPO_LABEL_BINARY_ELEMENTS);

	std::vector<repo::lib::RepoVector3D> positions;
	std::vector<repo::lib::repo_color4d_t> colourAttributes;

	if (elementsBson.hasField(REPO_NODE_POINT_LABEL_POINTS)) {
		auto positionBson = elementsBson.getObjectField(REPO_NODE_POINT_LABEL_POINTS);
		deserialiseVector(positionBson, buffer, positions);
	}

	if (elementsBson.hasField(REPO_NODE_POINT_LABEL_COLOURS)) {
		auto colourBson = elementsBson.getObjectField(REPO_NODE_POINT_LABEL_COLOURS);
		deserialiseVector(colourBson, buffer, colourAttributes);
	}

	if (positions.size() != colourAttributes.size())
		throw repo::lib::RepoException("Attribute size mismatch when loading streaming point node.");

	for (int i = 0; i < positions.size(); i++)
	{
		auto pos = positions[i];
		auto col = colourAttributes[i];
		points.push_back(repo::core::model::PointData(pos, col));
	}
}

repo::core::model::StreamingPointNode::StreamingPointNode(const repo::core::model::RepoBSON& bson)
{
	if (bson.hasField(REPO_NODE_LABEL_SHARED_ID)) {
		sharedId = bson.getUUIDField(REPO_NODE_LABEL_SHARED_ID);
	}
	if (bson.hasField(REPO_NODE_POINT_LABEL_POINTS_COUNT)) {
		numPoints = bson.getIntField(REPO_NODE_POINT_LABEL_POINTS_COUNT);
	}
	if (bson.hasField(REPO_NODE_LABEL_PARENTS)) {
		auto parents = bson.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);
		parent = parents[0];
	}
	if (bson.hasField(REPO_NODE_POINT_LABEL_BOUNDING_BOX)) {
		bounds = bson.getBoundsField(REPO_NODE_POINT_LABEL_BOUNDING_BOX);
	}
	if (bson.hasField(REPO_NODE_POINT_LABEL_TREE_POSITION)) {
		treePosition = bson.getByteArray(REPO_NODE_POINT_LABEL_TREE_POSITION);
	}
}

void repo::core::model::StreamingPointNode::loadPointOptimisationData(const repo::core::model::RepoBSON& bson, const std::vector<uint8_t>& buffer)
{
	if (pointOptimisationDataLoaded())
	{
		repoWarning << "StreamingPointNode instructed to load geometry data, but geometry data is already loaded.";
		unloadPointOptimisationData();
	}

	pOpData = std::make_unique<PointOptimisationData>(bson, buffer);
}

void repo::core::model::StreamingPointNode::assertPointOptimisationDataLoaded() {
	if (!pointOptimisationDataLoaded()) {
		throw repo::lib::RepoException("Tried to access supermesh geometry of StreamingPointNode without loading geometry first.");
	}
}

void repo::core::model::StreamingPointNode::transformBounds(const repo::lib::RepoMatrix& transform)
{
	auto newMinBound = transform * bounds.min();
	auto newMaxBound = transform * bounds.max();
	bounds = repo::lib::RepoBounds(newMinBound, newMaxBound);
}

const repo::lib::RepoUUID repo::core::model::StreamingPointNode::getUniqueId() {
	assertPointOptimisationDataLoaded();
	return pOpData->getUniqueId();
}

const std::uint32_t repo::core::model::StreamingPointNode::getNumLoadedPoints() {
	assertPointOptimisationDataLoaded();
	return pOpData->getNumPoints();
}

const std::vector<repo::core::model::PointData>& repo::core::model::StreamingPointNode::getLoadedPoints() {
	assertPointOptimisationDataLoaded();
	return pOpData->getPoints();
}
