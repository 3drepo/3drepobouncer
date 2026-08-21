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

#include "repo_test_point_utils.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include "repo_test_utils.h"
#include "repo_test_random_generator.h"

using namespace repo::core::model;
using namespace testing;

repo::test::utils::point::point_data::point_data(
	bool name,
	bool sharedId,
	int numParents,
	int numPoints,
	int treeLevels)
{
	if (name) 
	{
		this->name = "Named Point";
	}

	if (sharedId) 
	{
		this->sharedId = getRandUUID();
	}

	for (int i = 0; i < numParents; i++) 
	{
		parents.push_back(getRandUUID());
	}

	treePosition = makeTreePosition(treeLevels);

	repo::lib::RepoVector3D min = repo::lib::RepoVector3D(FLT_MAX, FLT_MAX, FLT_MAX);
	repo::lib::RepoVector3D max = repo::lib::RepoVector3D(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// Generate points for the cloud using the old deterministic randomiser
	for (int i = 0; i < numPoints; i++)
	{
		auto newVert = makeRandomRepoVector();
		points.push_back(newVert);
		min = repo::lib::RepoVector3D::min(min, newVert);
		max = repo::lib::RepoVector3D::max(max, newVert);

		auto newColour = makeRandomRepoColour();
		colourAttributes.push_back(newColour);
	}

	boundingBox.push_back({
		min.x, min.y, min.z
		});

	boundingBox.push_back({
		max.x, max.y, max.z
		});
}

void repo::test::utils::point::comparePointNode(point_data expected, repo::core::model::PointNode actual)
{
	EXPECT_THAT(actual.getUniqueID(), Eq(expected.uniqueId));
	EXPECT_THAT(actual.getSharedID(), Eq(expected.sharedId));
	EXPECT_THAT(actual.getParentIDs(), UnorderedElementsAreArray(expected.parents));
	EXPECT_THAT(actual.getName(), Eq(expected.name));
	EXPECT_THAT(actual.getNumPoints(), Eq(expected.points.size()));
	EXPECT_THAT(actual.getPoints(), ElementsAreArray(expected.points));
	EXPECT_THAT(actual.getColourAttributes(), ElementsAreArray(expected.colourAttributes));
}

std::vector<repo::lib::RepoVector3D> repo::test::utils::point::makePoints(int num)
{
	std::vector<repo::lib::RepoVector3D> points;
	for (int i = 0; i < num; i++) {
		points.push_back(makeRandomRepoVector());
	}
	return points;
}

std::vector<repo::lib::repo_color4d_t> repo::test::utils::point::makeColourAttributes(int num)
{
	std::vector<repo::lib::repo_color4d_t> colourAttributes;
	for (int i = 0; i < num; i++) {
		colourAttributes.push_back(makeRandomRepoColour());
	}
	return colourAttributes;
}

std::vector<uint8_t> repo::test::utils::point::makeTreePosition(int levels)
{
	std::vector<uint8_t> treePosition;

	for (int i = 0; i < levels; i++)
	{
		uint8_t p = static_cast<uint8_t>(floor(((double)rand() / (double)RAND_MAX) * 8));
		treePosition.push_back(p);
	}

	return treePosition;
}

repo::core::model::PointNode repo::test::utils::point::makeDeterministicPointNode(int treeLevels)
{
	restartRand();
	return PointNode(pointNodeTestBSONFactory((point_data(false, false, 0, 100, treeLevels))));
}

/**
* This implementation should be the reference for the node database schema and
* should be effectively independent, but equivalent, to the serialise method
* in PointNode.
*/
RepoBSON repo::test::utils::point::pointNodeTestBSONFactory(point_data data)
{
	RepoBSONBuilder builder;

	builder.append(REPO_NODE_LABEL_ID, data.uniqueId);

	if (!data.sharedId.isDefaultValue())
	{
		builder.append(REPO_NODE_LABEL_SHARED_ID, data.sharedId);
	}

	builder.append(REPO_NODE_LABEL_TYPE, "mesh");

	if (data.parents.size() > 0)
	{
		builder.appendArray(REPO_NODE_LABEL_PARENTS, data.parents);
	}

	if (!data.name.empty())
	{
		builder.append(REPO_NODE_LABEL_NAME, data.name);
	}

	if (data.boundingBox.size() > 0)
	{
		builder.append(REPO_NODE_POINT_LABEL_BOUNDING_BOX, data.boundingBox);
	}

	if (data.points.size() > 0)
	{
		builder.append(REPO_NODE_POINT_LABEL_POINTS_COUNT, (int32_t)(data.points.size()));
		builder.appendLargeArray(REPO_NODE_POINT_LABEL_POINTS, data.points);
	}

	if (data.colourAttributes.size() > 0)
	{
		builder.appendLargeArray(REPO_NODE_POINT_LABEL_COLOURS, data.colourAttributes);
	}

	if (data.treePosition.size() > 0)
	{
		builder.appendByteArray(REPO_NODE_POINT_LABEL_TREE_POSITION, data.treePosition);
	}

	return builder.obj();
}
