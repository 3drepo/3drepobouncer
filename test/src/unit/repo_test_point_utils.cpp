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
using namespace repo::test::utils::point;

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

void repo::test::utils::point::comparePointNode(point_data expected, PointNode actual)
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

PointNode repo::test::utils::point::makeDeterministicPointNode(int treeLevels)
{
	restartRand();
	return PointNode(pointNodeTestBSONFactory((point_data(false, false, 0, 100, treeLevels))));
}

// Helper method for getting binary data from the nodes in getFacesFromDatabase(...)
template <class T>
void deserialiseVector(
	const repo::core::model::RepoBSON& bson,
	const std::vector<uint8_t>& buffer,
	std::vector<T>& vec)
{
	auto start = bson.getLongField(REPO_LABEL_BINARY_START);
	auto size = bson.getLongField(REPO_LABEL_BINARY_SIZE);

	vec.resize(size / sizeof(T));
	memcpy(vec.data(), buffer.data() + (sizeof(uint8_t) * start), size);
}

std::vector<GenericPoint> repo::test::utils::point::getPointsFromDatabase(
	std::string database,
	std::string projectName,
	repo::lib::RepoUUID revId)
{
	std::vector<GenericPoint> genericPoints;

	// Assembly query for db.
	auto handler = getHandler();
	std::string sceneCollection = projectName + "." + REPO_COLLECTION_SCENE;
	repo::core::handler::fileservice::BlobFilesHandler blobHandler(handler->getFileManager(), database, sceneCollection);

	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_POINT)));

	// Query
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, filter);

	// Process the results
	for (auto bson : (*cursor)) {

		std::vector<repo::lib::RepoVector3D> points;
		std::vector<repo::lib::repo_color4d_t> colourAttributes;

		auto binRef = bson.getBinaryReference();
		auto dataRef = repo::core::handler::fileservice::DataRef::deserialise(binRef);
		auto buffer = blobHandler.readToBuffer(dataRef);

		auto blobRefBson = bson.getObjectField(REPO_LABEL_BINARY_REFERENCE);
		auto elementsBson = blobRefBson.getObjectField(REPO_LABEL_BINARY_ELEMENTS);

		if (elementsBson.hasField(REPO_NODE_POINT_LABEL_POINTS)) {
			auto pointBson = elementsBson.getObjectField(REPO_NODE_POINT_LABEL_POINTS);
			deserialiseVector(pointBson, buffer, points);
		}

		if (elementsBson.hasField(REPO_NODE_POINT_LABEL_COLOURS)) {
			auto colBson = elementsBson.getObjectField(REPO_NODE_POINT_LABEL_COLOURS);
			deserialiseVector(colBson, buffer, colourAttributes);
		}
			
		for (int i = 0; i < points.size(); i++)
		{
			auto p = points[i];
			auto c = colourAttributes[i];

			auto point = GenericPoint();
			point.push(p, c);
			genericPoints.push_back(point);
		}
	}

	return genericPoints;
}

std::vector<GenericPoint> repo::test::utils::point::getPointsFromMockImporter(TestPCImport* mockImporter)
{
	auto pointData = mockImporter->getTestData();
	auto offset = mockImporter->getOffset();

	auto points = std::vector<GenericPoint>();

	for (auto& point : pointData) {
		point.position = point.position - offset;
		auto generic = GenericPoint();
		generic.push(point);
		points.push_back(generic);
	}

	return points;
}


bool repo::test::utils::point::comparePointClouds(
	std::string database,
	std::string projectName,
	repo::lib::RepoUUID revId,
	TestPCImport* mockImporter)
{
	// Get test data from the mock importer
	auto testData = getPointsFromMockImporter(mockImporter);

	// Get data from the database
	auto importedData = getPointsFromDatabase(database, projectName, revId);

	// Check for length equality first
	if (testData.size() != importedData.size())
		return false;

	// Points are compared exactly using a hash table for speed
	std::map<long, std::vector<GenericPoint>> actual;

	for (auto& point : importedData)
	{
		actual[point.hash()].push_back(point);
	}

	for (auto& point : testData)
	{
		auto& others = actual[point.hash()];
		for (auto& other : others) {
			if (other.hit)
			{
				continue; // Each actual point may only be matched once
			}
			if (other.equals(point))
			{
				point.hit++;
				other.hit++;
				break;
			}
		}
	}

	// Did we find a match for all faces?

	for (const auto& face : testData)
	{
		if (!face.hit)
		{
			return false;
		}
	}

	return true;
}

void repo::test::utils::point::checkChunkCorrectness(
	std::string database,
	std::string projectName,
	repo::lib::RepoUUID revId,
	repo::lib::RepoBounds expectedBounds)
{
	// For each cell, i.e. point node, check the following:
	// - tree position is consistent with the bounding box?

	// Assembly query for db.
	auto handler = getHandler();
	std::string sceneCollection = projectName + "." + REPO_COLLECTION_SCENE;
	repo::core::handler::fileservice::BlobFilesHandler blobHandler(handler->getFileManager(), database, sceneCollection);

	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, revId));
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_POINT)));

	// Query
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, filter);

	// Process the results
	for (auto bson : (*cursor)) {

		std::vector<repo::lib::RepoVector3D> points;
		std::vector<repo::lib::repo_color4d_t> colourAttributes;
		repo::lib::RepoBounds bounds;
		std::vector<uint8_t> treePosition;

		auto binRef = bson.getBinaryReference();
		auto dataRef = repo::core::handler::fileservice::DataRef::deserialise(binRef);
		auto buffer = blobHandler.readToBuffer(dataRef);

		auto blobRefBson = bson.getObjectField(REPO_LABEL_BINARY_REFERENCE);
		auto elementsBson = blobRefBson.getObjectField(REPO_LABEL_BINARY_ELEMENTS);

		if (elementsBson.hasField(REPO_NODE_POINT_LABEL_POINTS)) {
			auto pointBson = elementsBson.getObjectField(REPO_NODE_POINT_LABEL_POINTS);
			deserialiseVector(pointBson, buffer, points);
		}

		if (elementsBson.hasField(REPO_NODE_POINT_LABEL_COLOURS)) {
			auto colBson = elementsBson.getObjectField(REPO_NODE_POINT_LABEL_COLOURS);
			deserialiseVector(colBson, buffer, colourAttributes);
		}

		if (bson.hasField(REPO_NODE_POINT_LABEL_BOUNDING_BOX)) {
			bounds = bson.getBoundsField(REPO_NODE_POINT_LABEL_BOUNDING_BOX);
		}

		if (bson.hasField(REPO_NODE_POINT_LABEL_TREE_POSITION)) {
			treePosition = bson.getByteArray(REPO_NODE_POINT_LABEL_TREE_POSITION);
		}

		// Check that number of points in this node is equal to the number of attributes
		EXPECT_THAT(points.size(), Eq(colourAttributes.size()));

		// Check that the number of points is under the cap
		EXPECT_THAT(points.size(), Lt(REPO_PC_CHUNKING_MAXPOINTS));

		// For each point, check that it fits into the bounding box of the cell
		for (int i = 0; i < points.size(); i++)
		{
			auto p = points[i];
			EXPECT_TRUE(bounds.contains(p));
		}

		// Check that the bounding box is a cube
		auto boundDimensions = bounds.max() - bounds.min();
		EXPECT_TRUE(boundDimensions.x, FloatNear(boundDimensions.y, 0.01));
		EXPECT_TRUE(boundDimensions.y, FloatNear(boundDimensions.z, 0.01));
		EXPECT_TRUE(boundDimensions.z, FloatNear(boundDimensions.x, 0.01));

		// Check that the size of the bounding box is consistent with the level in the tree position
		float expectedBoundLength = expectedBounds.max().x - expectedBounds.min().x;
		float expectedLength = expectedBoundLength / std::pow(2, treePosition.size());
		EXPECT_TRUE(boundDimensions.x, FloatNear(expectedLengt, 0.01));

		// Ensure that the bounding box is indeed in the location designated by the tree position
		auto boundsMax = expectedBounds.max();
		auto boundsMin = expectedBounds.min();
		auto boundLength = boundsMax - boundsMin;
		for (int i = 0; i < treePosition.size(); i++)
		{
			uint8_t childIndex = treePosition[i];
			boundLength = boundLength / 2;

			switch (childIndex)
			{
			case 0:
				boundsMin = boundsMin + (boundLength * repo::lib::RepoVector3D64(0, 0, 1));
				boundsMax = boundsMax - (boundLength * repo::lib::RepoVector3D64(1, 1, 0));
				break;
			case 1:
				boundsMin = boundsMin + (boundLength * repo::lib::RepoVector3D64(1, 0, 1));
				boundsMax = boundsMax - (boundLength * repo::lib::RepoVector3D64(0, 1, 0));
				break;
			case 2:
				boundsMin = boundsMin + (boundLength * repo::lib::RepoVector3D64(1, 1, 1));
				boundsMax = boundsMax - (boundLength * repo::lib::RepoVector3D64(0, 0, 0));
				break;
			case 3:
				boundsMin = boundsMin + (boundLength * repo::lib::RepoVector3D64(0, 1, 1));
				boundsMax = boundsMax - (boundLength * repo::lib::RepoVector3D64(1, 0, 0));
				break;
			case 4:
				boundsMin = boundsMin + (boundLength * repo::lib::RepoVector3D64(0, 0, 0));
				boundsMax = boundsMax - (boundLength * repo::lib::RepoVector3D64(1, 1, 1));
				break;
			case 5:
				boundsMin = boundsMin + (boundLength * repo::lib::RepoVector3D64(1, 0, 0));
				boundsMax = boundsMax - (boundLength * repo::lib::RepoVector3D64(0, 1, 1));
				break;
			case 6:
				boundsMin = boundsMin + (boundLength * repo::lib::RepoVector3D64(1, 1, 0));
				boundsMax = boundsMax - (boundLength * repo::lib::RepoVector3D64(0, 0, 1));
				break;
			case 7:
				boundsMin = boundsMin + (boundLength * repo::lib::RepoVector3D64(0, 1, 0));
				boundsMax = boundsMax - (boundLength * repo::lib::RepoVector3D64(1, 0, 1));
				break;
			}
		}
		
		EXPECT_THAT(bounds.min(), boundsMin);
		EXPECT_THAT(bounds.max(), boundsMax);
	}
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

// Mock importer to test the functionality embedded in the abstract point cloud importer

repo::test::utils::point::TestPCImport::TestPCImport(const repo::manipulator::modelconvertor::ModelImportConfig& settings) :
	AbstractPointCloudImport(settings)
{
}

repo::test::utils::point::TestPCImport::~TestPCImport()
{
}

void repo::test::utils::point::TestPCImport::createTestData(
	int numPoints,
	repo::lib::RepoBounds bounds)
{
	points.clear();

	// Add min and max of the bounds as points to ensure that the point cloud
	// fills the given bounds exactly.
	points.push_back(PointData(
		bounds.min(),
		makeRandomRepoColour()
	));
	points.push_back(PointData(
		bounds.max(),
		makeRandomRepoColour()
	));

	// Create remaining test data
	for (int i = 0; i < numPoints - 2; i++)
	{
		auto p = makeRandomRepoVector(bounds);
		auto c = makeRandomRepoColour();
		
		points.push_back(PointData(p, c));
	}

	offset = bounds.min();
}

repo::core::model::RepoScene* repo::test::utils::point::TestPCImport::importModel(
	std::string filePath,
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
	uint8_t& errMsg)
{
	// We are just calling the implementation in the abstract point cloud importer
	// since that takes care of the actual loading.
	return AbstractPointCloudImport::importModel(filePath, handler, errMsg);
}

bool repo::test::utils::point::TestPCImport::getNextPoint(repo::core::model::PointData& point)
{
	if (readPos < points.size())
	{
		point = points[readPos];
		readPos++;
		return true;
	}
	else
	{
		return false;
	}
}

void repo::test::utils::point::TestPCImport::resetReader()
{
	readPos = 0;
}

uint8_t repo::test::utils::point::TestPCImport::loadFile(std::string filePath)
{
	// This only returns an error if there is no data
	if (points.size() == 0)
		return REPOERR_MODEL_FILE_READ;

	return 0;
}