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

#include "repo_optimizer_point_cloud.h"
#include "repo/lib/point_cloud/repo_point_cloud_utils.h"
#include <queue>
#include <repo/manipulator/modelconvertor/import/repo_model_import_point_cloud_abstract.h>

using namespace repo::lib;
using namespace repo::manipulator::modeloptimizer;
using namespace repo::core::handler::database::query;

PointCloudOptimizer::PointCloudOptimizer(
	repo::core::handler::AbstractDatabaseHandler* handler,
	repo::manipulator::modelconvertor::AbstractPointCloudExport* exporter)
	:
	handler(handler),
	exporter(exporter)
{
}

void PointCloudOptimizer::processScene(
	std::string database,
	std::string collection,
	repo::lib::RepoUUID revId,
	repo::lib::RepoBounds cloudBounds)
{
	// Get all PointNodes in their lightweight streaming structure and queue them

	// Create filter
	RepoQueryBuilder filter;
	filter.append(Eq(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_POINT));
	filter.append(Eq(REPO_NODE_REVISION_ID, revId));

	// Create Projection
	RepoProjectionBuilder projection;
	projection.excludeField(REPO_NODE_LABEL_ID);
	projection.includeField(REPO_NODE_LABEL_SHARED_ID);
	projection.includeField(REPO_NODE_POINT_LABEL_BOUNDING_BOX);
	projection.includeField(REPO_NODE_POINT_LABEL_POINTS_COUNT);
	projection.includeField(REPO_NODE_LABEL_PARENTS);
	projection.includeField(REPO_NODE_POINT_LABEL_TREE_POSITION);

	// Get cursor
	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	auto cursor = handler->findCursorByCriteria(database, sceneCollection, filter, projection);

	// Iterate cursor and pack into lighweight point node structure and push them into queue
	auto chunkQueue = std::queue<std::unique_ptr<repo::core::model::StreamingPointNode>>();
	for (auto bson : (*cursor)) {
		auto node = std::make_unique<repo::core::model::StreamingPointNode>(bson);
		chunkQueue.push(std::move(node));
	}

	if (chunkQueue.size() == 0)
		throw repo::lib::RepoException("No chunks retrieved for this point cloud. Import aborted.");

	// Before processing, setup the final octree structure
	auto octree = Octree(REPO_PC_CHUNKING_DEPTH, cloudBounds, {});

	// Mutexes
	std::mutex queueMutex{};
	std::mutex treeMutex{};

	auto chunkProcessingThread = [&]
	{
		while (true)
		{
			// Lock queue, check for chunks, and retrieve one if available
			std::unique_ptr<repo::core::model::StreamingPointNode> chunk;
			{
				std::scoped_lock queueLock{ queueMutex };

				// If we are out of chunks, unlock and terminate
				if (chunkQueue.empty())
				{
					break;
				}

				// Else, we get a chunk
				chunk = std::move(chunkQueue.front());
				chunkQueue.pop();
			}

			// Process chunk
			auto processedNode = processChunk(database, collection, chunk.get());

			// After processing, lock overall tree and insert the processed node
			{
				std::scoped_lock treeLock{ treeMutex };
				auto nodePos = chunk->getTreePosition();
				octree.addNode(std::move(processedNode), nodePos);
			}
		}
	};

	// Create and launch the threads
	// TODO FT: From config somewhere?
	// Also a flag for further optimisation. These chunks are quite hefty, we might
	// not want to run more than four threads or so at the same time.
	int numThreads = std::thread::hardware_concurrency();
	std::vector<std::jthread> threads;
	for (int i = 0; i < numThreads; i++)
		threads.emplace_back(std::jthread(chunkProcessingThread));

	// Wait for all threads to complete
	for (auto& t : threads)
		t.join();

	// After processing all, subsample the tree again
	octree.subsampleTree(exporter);

	// Then export the root
	exporter->addTreeNode((octree.getRoot()->points.get()), std::vector<uint8_t>());

	// Finalise export
	exporter->finalise();
}

std::unique_ptr<OctreeNode> PointCloudOptimizer::processChunk(
	std::string database,
	std::string collection,
	repo::core::model::StreamingPointNode* chunk)
{
	auto sceneCollection = collection + "." + REPO_COLLECTION_SCENE;
	repo::core::handler::fileservice::BlobFilesHandler blobHandler(handler->getFileManager(), database, sceneCollection);

	// Load data

	// Create Filter
	auto filter = Eq(REPO_NODE_LABEL_SHARED_ID, chunk->getSharedId());

	// Create Projection
	RepoProjectionBuilder projection;
	projection.includeField(REPO_NODE_POINT_LABEL_POINTS_COUNT);
	projection.includeField(REPO_LABEL_BINARY_REFERENCE);

	auto node = handler->findOneByCriteria(database, sceneCollection, filter,projection);

	// Load geometry for this node.
	// Placed In its own scope so that buffer can be discarded as soon as it is processed
	{
		auto binRef = node.getBinaryReference();
		auto dataRef = repo::core::handler::fileservice::DataRef::deserialise(binRef);
		auto buffer = blobHandler.readToBuffer(dataRef);

		chunk->loadPointOptimisationData(node, buffer);
	}

	// Create octree with five splits to achieve the 32x32x32 cube
	auto bounds = chunk->getBoundingBox();
	auto octree = Octree(REPO_PC_OPTIMISATION_SPLITS, bounds, chunk->getTreePosition());

	// Project all points into the octree
	auto& points = chunk->getLoadedPoints();
	for (auto& point : points)
	{
		octree.project(point);
	}

	// unload the geometry data from the node since a copy is now in the tree
	chunk->unloadPointOptimisationData();

	// Have the tree perform the subsampling and export
	octree.subsampleTree(exporter);

	return octree.moveRoot();
}

OctreeNode::OctreeNode(int splits, std::vector<uint8_t> treePosition, repo::lib::RepoBounds bounds)
{
	// Update the tree position
	nodeTreePosition = treePosition;

	// Set node bounds
	nodeBounds = bounds;

	// Initialise point storage
	points = std::make_unique < std::vector<repo::core::model::PointData>>();

	// If we have not reached the bottom, create children.
	if (splits > treePosition.size())
	{
		for (int i = 0; i < 8; i++)
		{
			auto childPosition = treePosition;
			childPosition.push_back(i);

			auto childBounds = pointcloud::PointCloudUtils::getChildBoundsByIndex(nodeBounds, i);

			children[i] = std::make_unique<OctreeNode>(splits, childPosition, childBounds);
		}
	}
}

void OctreeNode::insert(std::vector<uint8_t> treePosition, const repo::core::model::PointData& point)
{
	if (nodeTreePosition == treePosition)
	{
		points->push_back(point);
	}
	else
	{
		int next = treePosition[nodeTreePosition.size()];
		children[next]->insert(treePosition, point);
	}
}

void OctreeNode::addNode(std::vector<uint8_t> treePosition, std::unique_ptr<OctreeNode> node)
{
	if (nodeTreePosition.size() == (treePosition.size() - 1))
	{
		// Replace the child with this node.
		int next = treePosition[nodeTreePosition.size()];
		children[next].reset();
		children[next] = std::move(node);
	}
	else {
		int next = treePosition[nodeTreePosition.size()];
		children[next]->addNode(treePosition, std::move(node));
	}
}

OctreeNode* OctreeNode::getNode(std::vector<uint8_t> treePosition)
{
	if (nodeTreePosition.size() == (treePosition.size() - 1))
	{
		int next = treePosition[nodeTreePosition.size()];
		return children[next].get();
	}
	else {
		int next = treePosition[nodeTreePosition.size()];
		return children[next]->getNode(treePosition);
	}
}

Octree::Octree(int splits, repo::lib::RepoBounds bounds, std::vector<uint8_t> rootPos)
{
	root = std::make_unique<OctreeNode>(splits, std::vector<uint8_t>(), bounds);
	treeBounds = bounds;
	treeSplits = splits;
	rootPosition = rootPos;

	// Calculate cell length
	noCells = std::pow(2, splits);
	cellLength = (bounds.max() - bounds.min()).x / noCells;
}

void Octree::project(const repo::core::model::PointData& point)
{
	// Now, convert the XYZ position into a tree position given the number of splits
	auto treePosition = pointcloud::PointCloudUtils::project3DPositionToTreePosition(
		point.position,
		treeBounds,
		treeSplits);

	// Now insert the point into the tree
	root->insert(treePosition, point);
}

void Octree::subsample(
	repo::manipulator::modelconvertor::AbstractPointCloudExport* exporter,
	OctreeNode* node)
{
	// Check if the node is a leaf

	if (node->isLeaf())
	{
		// If we have reached the child level, we just return.
		return;
	}

	// If we are no leaf, we ask each of our children to subsample
	for (int i = 0; i < 8; i++)
	{
		subsample(exporter, node->children[i].get());
	}

	// Then we subsample ourselves
	auto sample = std::make_unique<std::vector<repo::core::model::PointData>>();
	int sampleCellsCount = pow(8, REPO_PC_SUBSAMPLE_SPLITS);
	for (int i = 0; i < 8; i++)
	{
		auto child = node->children[i].get();

		// If there is no child attached to this, continue
		if (child == nullptr)
			continue;

		// Create structure to hold the indices of our sorted points
		auto cells = std::vector<std::vector<int>>();
		cells.resize(sampleCellsCount);

		// Now go through points
		auto points = child->points.get();
		int numPoints = points->size();
		auto childBounds = child->nodeBounds;
		for (int e = 0; e < numPoints; e++)
		{
			auto point = (*points)[e];
			
			// Project the point to a cell in a 64x64x64 cube spanning the bounds of the child
			int index = pointcloud::PointCloudUtils::project3DPositionToCellIndex(
				point.position,
				childBounds,
				REPO_PC_SUBSAMPLE_SPLITS);

			cells[index].push_back(e);
		}

		// Now go over cells and extract one point by random.
		// The remaining points go into a different collection
		auto reducedPoints = std::make_unique<std::vector<repo::core::model::PointData>>();
		for (int e = 0; e < sampleCellsCount; e++)
		{
			auto& cell = cells[e];
			if (cell.size() > 0)
			{
				// Pick random index
				int randIndex = rand() % cell.size();
				
				// Go over points in cell and either copy to the sample collection or the reduced collection
				for (int j = 0; j < cell.size(); j++)
				{
					int index = cell[j];
					auto p = (*points)[index];
					if (j == randIndex)
					{
						sample->push_back(p);
					}
					else
					{
						reducedPoints->push_back(p);
					}
				}
			}
		}
		
		// After the points are separated, delete the points the child is currently holding
		// and export the reduced points, if there are any left.
		child->points.reset();
		
		auto exportPos = pointcloud::PointCloudUtils::combineTreePositions(rootPosition, child->nodeTreePosition);
		if(reducedPoints->size() > 0)
			exporter->addTreeNode(
				reducedPoints.get(), exportPos);
	}

	// After extracting sample points from all children, assign them to this node
	node->points = std::move(sample);

	// Now that the children are all exported, release the child nodes
	for (int i = 0; i < 8; i++)
	{
		node->children[i].reset();
	}
}

void Octree::subsampleTree(repo::manipulator::modelconvertor::AbstractPointCloudExport* exporter)
{
	// Post order depth first tree traversal

	subsample(exporter, root.get());
}

void Octree::addNode(std::unique_ptr<OctreeNode> node, std::vector<uint8_t> nodePosition)
{
	if (nodePosition.size() > treeSplits)
		throw repo::lib::RepoException("Attempted to insert node into tree with too few levels.");

	// Update the node with the tree position it will have in the new tree.
	// This is important since the incoming node will be coming from a different tree.
	node->nodeTreePosition = nodePosition;

	if (nodePosition.size() == 0)
		root = std::move(node);
	else
		return root->addNode(nodePosition, std::move(node));
}

OctreeNode* Octree::getNode(std::vector<uint8_t> treePosition)
{
	if (treePosition.size() == 0)
		return getRoot();
	else
		return root->getNode(treePosition);
}
