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

#include "repo_model_import_point_cloud_abstract.h"
#include "repo/core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::modelconvertor;

AbstractPointCloudImport::AbstractPointCloudImport(const ModelImportConfig& settings) :
	AbstractModelImport(settings)
{
}

AbstractPointCloudImport::~AbstractPointCloudImport()
{
}

int AbstractPointCloudImport::getIndexFromCellCoordinates(
	int xIndex,
	int yIndex,
	int zIndex)
{
	return (xIndex * (steps * steps)) + (yIndex * steps) + zIndex;
}

int AbstractPointCloudImport::projectPositionIntoCell(repo::lib::RepoVector3D64 pos)
{
	auto min = bounds.min();

	// Calculate X-Index of cell
	int xIndex = floor((pos.x - min.x) / stepLength);
	if (xIndex == steps)
		xIndex--;

	// Calculate Y-Index of cell
	int yIndex = floor((pos.y - min.y) / stepLength);
	if (yIndex == steps)
		yIndex--;

	// Calculate Z-Index of cell
	int zIndex = floor((pos.z - min.z) / stepLength);
	if (zIndex == steps)
		zIndex--;

	if (xIndex > steps || yIndex > steps || zIndex > steps)
		throw repo::lib::RepoException("Point Cloud Import: Point outside of the grid detected.");

	// Calculate index in the counting array
	int index = getIndexFromCellCoordinates(
		xIndex,
		yIndex,
		zIndex);

	return index;

}

int AbstractPointCloudImport::getIndexFromTreePosition(
	std::vector<uint8_t>& treePosition)
{
	if (treePosition.size() != REPO_PC_CHUNKING_DEPTH)
	{
		throw repo::lib::RepoException("Partial tree positions cannot be used to retrieve an index.");
	}

	int x = 0;
	int y = 0;
	int z = 0;

	int currentSteps = steps;

	for (int i = 0; i < treePosition.size(); i++)
	{
		// Split is numbered as such:
		// ------------------------------ x
		// |							|
		// |	0/4		|		1/5		|
		// |							|
		// |----------------------------|
		// |							|
		// |	3/7		|		2/6		|
		// |							|
		// ------------------------------
		// y
		// 
		// 0 to 3 are the top level, 4 to 7 are the bottom level

		uint8_t childIndex = treePosition[i];

		// Calculate the lengths of the children for this split
		currentSteps = currentSteps / 2;

		switch (childIndex)
		{
		case 0:
			// X remains the same
			// Y Remains the same
			// Z is changed
			z = z + currentSteps;
			break;
		case 1:
			// X is changed
			// Y remains the same
			// Z is changed
			x = x + currentSteps;
			z = z + currentSteps;
			break;
		case 2:
			// X is changed
			// Y is changed
			// Z is changed
			x = x + currentSteps;
			y = y + currentSteps;
			z = z + currentSteps;
			break;
		case 3:
			// X remains the same
			// Y is changed
			// Z is changed
			y = y + currentSteps;
			z = z + currentSteps;
			break;
		case 4:
			// X remains the same
			// Y remains the same
			// Z remains the same
			break;
		case 5:
			// X is changed
			// Y remains the same
			// Z remains the same
			x = x + currentSteps;
			break;
		case 6:
			// X is changed
			// Y is changed
			// Z remains the same
			x = x + currentSteps;
			y = y + currentSteps;
			break;
		case 7:
			// X remains the same
			// Y is changed
			// Z remains the same
			y = y + currentSteps;
			break;
		}
	}

	return getIndexFromCellCoordinates(x, y, z);
}

// TODO FT: This will probably be moved into a utility class at some point
repo::lib::RepoBounds AbstractPointCloudImport::getBoundsFromTreePosition(std::vector<uint8_t>& treePosition)
{
	auto min = bounds.min();
	auto dimensions = bounds.max() - bounds.min();

	for (int i = 0; i < treePosition.size(); i++)
	{
		// Split is numbered as such:
		// ------------------------------ x
		// |							|
		// |	0/4		|		1/5		|
		// |							|
		// |----------------------------|
		// |							|
		// |	3/7		|		2/6		|
		// |							|
		// ------------------------------
		// y
		// 
		// 0 to 3 are the top level, 4 to 7 are the bottom level

		uint8_t childIndex = treePosition[i];

		// Calculate the dimensions of the children for this split
		dimensions = dimensions / 2.0;

		switch (childIndex)
		{
		case 0:
			// X remains the same
			// Y Remains the same
			// Z is changed
			min.z = min.z + dimensions.z;
			break;
		case 1:
			// X is changed
			// Y remains the same
			// Z is changed
			min.x = min.x + dimensions.x;
			min.z = min.z + dimensions.z;
			break;
		case 2:
			// X is changed
			// Y is changed
			// Z is changed
			min.x = min.x + dimensions.x;
			min.y = min.y + dimensions.y;
			min.z = min.z + dimensions.z;
			break;
		case 3:
			// X remains the same
			// Y is changed
			// Z is changed
			min.y = min.y + dimensions.y;
			min.z = min.z + dimensions.z;
			break;
		case 4:
			// X remains the same
			// Y remains the same
			// Z remains the same
			break;
		case 5:
			// X is changed
			// Y remains the same
			// Z remains the same
			min.x = min.x + dimensions.x;
			break;
		case 6:
			// X is changed
			// Y is changed
			// Z remains the same
			min.x = min.x + dimensions.x;
			min.y = min.y + dimensions.y;
			break;
		case 7:
			// X remains the same
			// Y is changed
			// Z remains the same
			min.y = min.y + dimensions.y;
			break;
		}
	}

	auto max = min + dimensions;

	return repo::lib::RepoBounds(min, max);
}

void AbstractPointCloudImport::createNode(
	repo::lib::RepoBounds bounds,
	std::vector<uint8_t>& treePosition,
	int numPoints)
{
	auto node = std::make_unique<repo::core::model::PointNode>(
		repo::core::model::RepoBSONFactory::makePointNode(
			bounds,
			treePosition,
			"",
			{ rootNodeId }));

	// Add the finished node to the collection and get its new index
	nodes.push_back(std::move(node));
	int nodeIndex = nodes.size() - 1;

	// Set index for this node in the lookup table for all the leaves of this child
	// and update their counters with the merged count
	updateLeaves(treePosition, nodeIndex, numPoints);
}

void AbstractPointCloudImport::mergeCells(
	std::vector<uint8_t> treePosition,
	int& numPoints,
	bool& cellsMergedBelow)
{
	// Check if we have reached the bottom level
	if (treePosition.size() == REPO_PC_CHUNKING_DEPTH)
	{
		// If we are at the bottom, we just get the count and return that
		int cellIndex = getIndexFromTreePosition(treePosition);
		numPoints = counters[cellIndex];
		cellsMergedBelow = false;
		return;
	}

	// If we are not at the bottom, we will need to ask the children
	int sum = 0;
	bool anyChildContainsMerged = false;
	int childCounts[8];
	bool childrenContainMerged[8];

	for (int i = 0; i < 8; i++)
	{
		auto childTreePosition = treePosition;
		childTreePosition.push_back(i);

		mergeCells(
			childTreePosition,
			childCounts[i],
			childrenContainMerged[i]);

		sum += childCounts[i];
		anyChildContainsMerged = anyChildContainsMerged || childrenContainMerged[i];
	}

	// If the sum is under the cap, none of the children has merged cells, and we are not at
	// the top yet, we just move this up the chain
	if (sum < REPO_PC_CHUNKING_MAXPOINTS && !anyChildContainsMerged && treePosition.size() > 0)
	{
		numPoints = sum;
		cellsMergedBelow = anyChildContainsMerged;
		return;
	}

	// If the sum is over the cap, or one of the children has merged cells
	// then the children cannot be merged into this node.
	// We now make nodes for each of the children that has points and is not merged yet.
	if (sum > REPO_PC_CHUNKING_MAXPOINTS || anyChildContainsMerged)
	{
		for (int i = 0; i < 8; i++)
		{
			// Check if the number of points of is either 0 or if they are
			// already merged.
			// Children with 0 don't need a PointNode.
			// Children reporting a merge below canot be merged and are also
			// to be be ignored.
			if (childCounts[i] == 0 || childrenContainMerged[i])
				continue;

			// Create a node for this child.
			auto childTreePosition = treePosition;
			childTreePosition.push_back(i);
			auto nodeBounds = getBoundsFromTreePosition(childTreePosition);

			createNode(
				nodeBounds,
				childTreePosition,
				childCounts[i]
			);
		}

		// Return cellsMergedBelow set to true to signal that there can't be any more merging above this.
		numPoints = 0;
		cellsMergedBelow = true;
		return;
	}

	// If we have arrived at the top without exceeding the cap and have no merged nodes below, all is just
	// merged in this one node
	createNode(
		bounds,
		treePosition,
		sum
	);
}

void AbstractPointCloudImport::updateLeaves(
	std::vector<uint8_t>& treePosition,
	int nodeIndex,
	int pointCount)
{
	if (treePosition.size() == REPO_PC_CHUNKING_DEPTH)
	{
		// If we hit the bottom, we store the index of the node associated with this cell.
		// We also need to update the counter to the value of the merged node
		int cellIndex = getIndexFromTreePosition(treePosition);	

		nodeIndices[cellIndex] = nodeIndex;
		counters[cellIndex] = pointCount;
	}
	else
	{
		// If we are not at the bottom yet, we pass it on to all children
		for (int i = 0; i < 8; i++)
		{
			auto childTreePosition = treePosition;
			childTreePosition.push_back(i);
			updateLeaves(childTreePosition, nodeIndex, pointCount);
		}
	}
}

void AbstractPointCloudImport::mergeCells()
{
	std::vector<uint8_t> treePosition;
	int numPoints;
	bool cellsMergedBelow;
	mergeCells(treePosition, numPoints, cellsMergedBelow);
}

void AbstractPointCloudImport::createRootNode(repo::manipulator::modelutility::RepoSceneBuilder* builder)
{
	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode(
		{},
		"rootNode",
		{}
	);
	builder->addNode(rootNode);
	rootNodeId = rootNode.getSharedID();
}

repo::core::model::RepoScene* AbstractPointCloudImport::importModel(
	std::string filePath,
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
	uint8_t& errMsg)
{
	// Load file
	errMsg = loadFile(filePath);

	if (errMsg != 0)
		return nullptr;

	// First pass to determine the bounding box
	repoInfo << "POINT CLOUD IMPORT CHUNKING PASS 1 OF 3: DETERMINING BOUNDS";
	bounds = repo::lib::RepoBounds();
	bool dataAvailable = true;
	while (dataAvailable)
	{
		repo::core::model::PointData pointData;
		dataAvailable = getNextPoint(pointData);
		if (dataAvailable)
		{
			bounds.encapsulate(pointData.position);
		}
	}

	// Preparation for the counting step
	
	// Calculate the number of steps for the grid
	steps = pow(2, REPO_PC_CHUNKING_DEPTH);

	// Determine the longest side
	double lengthX = std::abs(bounds.min().x - bounds.max().x);
	double lengthY = std::abs(bounds.min().y - bounds.max().y);
	double lengthZ = std::abs(bounds.min().z - bounds.max().z);
	double longest = std::max(lengthX, std::max(lengthY, lengthZ));

	// Calculate step length for the grid
	stepLength = longest / steps;

	// Recalculate the bounding box based on these values
	// This can significantly enlarge the bounding box if
	// the model is not cubic, but is needed for the octree.
	auto gridDimensions = repo::lib::RepoVector3D64(longest, longest, longest);
	auto newMax = bounds.min() + gridDimensions;
	bounds = repo::lib::RepoBounds(bounds.min(), newMax);

	// Create offset to shift bounding box minimum into the world centre
	repo::lib::RepoVector3D64 worldOffset = bounds.min();
	auto shiftedMin = bounds.min() - worldOffset;
	auto shiftedMax = bounds.max() - worldOffset;
	bounds = repo::lib::RepoBounds(shiftedMin, shiftedMax);

	// Create array of counters for each cell
	const long noCells = pow(8, REPO_PC_CHUNKING_DEPTH);
	counters.resize(noCells);

	// Reset and run second pass
	// In this pass, we count the points per cell
	repoInfo << "POINT CLOUD IMPORT CHUNKING PASS 2 OF 3: COUNTING STEP";
	resetReader();
	dataAvailable = true;
	while (dataAvailable)
	{
		repo::core::model::PointData pointData;

		dataAvailable = getNextPoint(pointData);

		if (dataAvailable)
		{
			// Apply shift to position
			pointData.position = pointData.position - worldOffset;

			// Get position of point
			auto pos = pointData.position;

			// Project point to get cell index
			int cellIndex = projectPositionIntoCell(pos);

			// Increase the counter of that cell by one
			counters[cellIndex]++;
		}
	}

	// Merging smaller cells until they hold a certian amount of points
	// Limit set to 10 mil for the start
	

	auto sceneBuilder = std::make_unique<repo::manipulator::modelutility::RepoSceneBuilder>(
		handler,
		settings.getDatabaseName(),
		settings.getProjectName(),
		settings.getRevisionId()
	);
	sceneBuilder->createIndexes();
	repoInfo << "POST INDEX CREATION";
	createRootNode(sceneBuilder.get());

	// First, initialise structures to keep track of nodes.
	nodes = std::vector<std::unique_ptr<repo::core::model::PointNode>>();
	nodeIndices.resize(noCells, -1);

	// Now merge the cells and create nodes
	mergeCells();

	// Reset and run third pass to distribute points into their respective nodes
	repoInfo << "POINT CLOUD IMPORT CHUNKING PASS 3 OF 3: DISTRIBUTION STEP";
	resetReader();
	dataAvailable = true;
	while (dataAvailable)
	{
		repo::core::model::PointData pointData;

		dataAvailable = getNextPoint(pointData);

		if (dataAvailable)
		{
			// Apply shift to position
			pointData.position = pointData.position - worldOffset;

			// Get position of point
			auto pos = pointData.position;

			// Project point to get cell index
			int cellIndex = projectPositionIntoCell(pos);

			// Get pointer to node belonging to that cell.
			auto nodeIndex = nodeIndices[cellIndex];
			auto& node = nodes[nodeIndex];

			// Check if the node has already been considered completed and committed to the db
			if (node == nullptr)
			{
				throw repo::lib::RepoException("Point Cloud Importer: Encountered completed node with a new point. This should not happen.");
			}

			// Add the point to the node
			node->addPoint(pointData);

			// Check if the point now has all the points it should have
			if (counters[cellIndex] == node->getNumPoints())
			{
				// If this was the last, move the node to the builder so it can be offloaded
				// This should reset the pointer to nullptr
				sceneBuilder->addNode(std::move(node));
			}
		}
	}

	// Sanity check, all pointers of PointNodes should be nullptr now.
	int nodesLeft = 0;
	for (auto& node : nodes)
	{
		if (node != nullptr)
			nodesLeft++;
	}
	if (nodesLeft > 0)
		throw repo::lib::RepoException("Point Cloud Importer: Encountered nodes that were not exported");

	sceneBuilder->finalise();

	// Construct scene object
	repo::core::model::RepoScene* scene;
	scene = new repo::core::model::RepoScene(
		settings.getDatabaseName(),
		settings.getProjectName()
	);

	// scene->setDataType(PointCloud) // TODO FT: To implement

	scene->setWorldOffset(worldOffset);
	
	scene->setRevision(settings.getRevisionId());
	scene->setOriginalFiles({ filePath });
	scene->loadRootNode(handler.get());
	
	return scene;
}
