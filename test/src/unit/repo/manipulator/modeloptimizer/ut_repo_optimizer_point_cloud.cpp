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

#include <repo/manipulator/modeloptimizer/repo_optimizer_point_cloud.h>
#include <repo/lib/point_cloud/repo_point_cloud_utils.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include <unordered_set>
#include "../test/src/unit/repo_test_matchers.h"
#include "../test/src/unit/repo_test_utils.h"
#include "../test/src/unit/repo_test_point_utils.h"
#include <repo/core/model/bson/repo_bson_factory.h>

using namespace repo::manipulator::modeloptimizer;
using namespace testing;
using namespace repo::test::utils::point;
using namespace repo::lib::pointcloud;

#define DBPOINTCLOUDOPTIMIZERTEST "pointcloudOptimiserTest"

TEST(PointCloudOptimizerTest, OctreeCreation)
{
	srand(time(NULL));

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto randomBounds = repo::lib::RepoBounds(randV1, randV2);

	// Create octree
	auto tree = Octree(splits, randomBounds, {1});

	// Check octree root position
	auto rootPos = tree.getRootPosition();
	EXPECT_THAT(rootPos.size(), Eq(1));
	EXPECT_THAT(rootPos[0], Eq(1));

	// Traverse tree to check bounds and get number of nodes
	int noNodesActual = 0;
	std::stack<OctreeNode*> nodes;
	nodes.push(tree.getRoot());
	while (nodes.size() > 0)
	{
		auto node = nodes.top();
		nodes.pop();

		// Count the node
		noNodesActual++;

		// Check the state of the children
		int valid = 0;
		for (int i = 0; i < 8; i++)
		{
			if (node->children[i] != nullptr)
				valid++;
		}
		if (valid == 0)
		{
			// All nullptr, this is a leaf, we just continue
			continue;
		}
		else if (valid < 8)
		{
			// Mixed nullptr and valid. This is an instant fail.
			FAIL() << "Encountered nodes with missing children.";
		}

		// Check the bounds of the node against the bounds it is supposed to have
		// according to its tree position
		auto nodeBoundsAct = node->nodeBounds;
		auto nodePosition = node->nodeTreePosition;
		auto nodeBoundsExp = PointCloudUtils::getBoundsFromTreePosition(nodePosition, randomBounds);
		EXPECT_THAT(nodeBoundsAct, Eq(nodeBoundsExp));

		// Check the bounds of the node against the bounds of the children.
		// Together, they should make up the parent bounds
		auto childBoundsAccumulated = repo::lib::RepoBounds();
		for (int i = 0; i < 8; i++)
		{
			auto childBounds = node->children[i]->nodeBounds;
			childBoundsAccumulated.encapsulate(childBounds);
		}
		EXPECT_THAT(nodeBoundsAct, Eq(childBoundsAccumulated));

		// Add the children to the stack for further processing
		for (int i = 0; i < 8; i++)
		{
			nodes.push(node->children[i].get());
		}
	}

	// Compare number of nodes
	int noNodesExpected = (std::pow(8, splits+1) - 1) / 7; // (N^L -1) / (N - 1) with N being the number of children and L the number of levels (split + 1)
	EXPECT_THAT(noNodesActual, Eq(noNodesExpected));
}



TEST(PointCloudOptimizerTest, OctreeProjection)
{
	srand(time(NULL));

	int noPoints = 1000;

	// Random number of splits (between 3 and 8)
	int splits = (rand() % 5) + 3;

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto randomBounds = repo::lib::RepoBounds(randV1, randV2);

	// Create octree
	auto tree = Octree(splits, randomBounds, {});

	// Get tree position for a random leaf
	auto randomTreePos = makeTreePosition(splits);

	// Get the bounds for that leaf
	auto leafBounds = PointCloudUtils::getBoundsFromTreePosition(randomTreePos, randomBounds);

	// We are actually shrinking the bounds for the generation a little bit (1%).
	// The projection is actually not always inclusive of the lower bound
	// (for more detail see description in repo_point_cloud_utils.h), which in
	// production is not an issue, but gets in the way of testing.
	auto shrink = (leafBounds.max() - leafBounds.min()) * 0.01;
	auto leafMin = leafBounds.min() + shrink;
	auto leafMax = leafBounds.max() - shrink;
	leafBounds = repo::lib::RepoBounds(leafMin, leafMax);

	// Generate a number of points that are fitting in that leaf
	auto points = makePoints(noPoints - 2, leafBounds);

	// Project the points into the tree.
	// and also add them to a set for easier comparison later.
	std::unordered_set<repo::core::model::PointData, repo::core::model::PointDataHasher> pointSet;
	for (const auto& point : points)
	{
		tree.project(point);
		pointSet.insert(point);
	}

	// Traverse the tree
	// There should only be points in that single leaf
	// No other node should have points
	// The leaf also needs to contain the correct number of points
	// The leaf needs to contain the right points
	int noNodesActual = 0;
	std::stack<OctreeNode*> nodes;
	nodes.push(tree.getRoot());
	while (nodes.size() > 0)
	{
		auto node = nodes.top();
		nodes.pop();

		// Check tree position agains the randomPos
		auto nodePosition = node->nodeTreePosition;
		if (nodePosition != randomTreePos)
		{
			// If this is not the leaf node we projected into, we check
			// whether it holds points. It should not.
			if (node->points != nullptr && node->points->size() != 0)
			{
				FAIL() << "Points found in node outside the expected leaf. << Size: " << node->points->size();
			}
		}
		else {
			// If this is our expected leaf, we check the points
			EXPECT_TRUE(node->points != nullptr);
			EXPECT_THAT(node->points->size(), Eq(points.size()));

			auto nodeBounds = node->nodeBounds;
			for (auto& point : *(node->points))
			{
				// Check if it belongs into the bounds of this node
				EXPECT_TRUE(nodeBounds.contains(point.position));

				// Check it agains the set from earlier and strike it out if found
				if (pointSet.contains(point))
					pointSet.erase(point);
				else
					FAIL() << "Point found in leaf without equivalent in expected data.";
			}
		}
		
		// Add the children to the stack for further processing
		// (if they are not nulltr because this is a leaf that is
		for (int i = 0; i < 8; i++)
		{
			if(node->children[i] != nullptr)
				nodes.push(node->children[i].get());
		}
	}
}

TEST(PointCloudOptimizerTest, OctreeSubsamplingSingleLevel)
{
	// This test is testing the overall subsampling behaviour for a single level.
	// Each child will be equipped with enough points to (if evenly spread) fill all
	// its cells and then subsampled once.
	// The test checks for possible duplications, missing points, or failures in
	// exporting the point data of the children before freeing their memory.

	int splits = 1;

	// The points of each child are projected into a cube of 64x64x64 cells
	// during subsampling and one is picked from each cell randomly. To ensure
	// that this test does not simply drain each child, we need to have at least
	// as many points.
	int noPointsPerChild = 64 * 64 * 64 + 1;

	// Generate (cubic) bounding box
	double boundsLength = 10;
	auto boundsMin = repo::lib::RepoVector3D64(-5, -5, -5);
	auto dims = repo::lib::RepoVector3D64(boundsLength, boundsLength, boundsLength);
	auto boundsMax = boundsMin + dims;
	auto cloudBounds = repo::lib::RepoBounds(boundsMin, boundsMax);

	// Create octree
	auto tree = Octree(splits, cloudBounds, {123});

	// Make mock exporter
	auto mockExporter = std::make_unique<TestPCExport>(
		nullptr,
		"",
		"",
		repo::lib::RepoUUID(),
		std::vector<double>(3, 0),
		cloudBounds);

	// For each of the children, create points
	auto pointSet = std::unordered_set<repo::core::model::PointData, repo::core::model::PointDataHasher>();
	for (int i = 0; i < 8; i++)
	{
		auto childBounds = PointCloudUtils::getChildBoundsByIndex(cloudBounds, i);
		// We are actually shrinking the bounds for the generation a little bit (1%).
		// The projection is actually not always inclusive of the lower bound
		// (for more detail see description in repo_point_cloud_utils.h), which in
		// production is not an issue, but gets in the way of testing.
		auto shrink = (childBounds.max() - childBounds.min()) * 0.01;
		auto leafMin = childBounds.min() + shrink;
		auto leafMax = childBounds.max() - shrink;
		childBounds = repo::lib::RepoBounds(leafMin, leafMax);

		auto childPoints = makePoints(noPointsPerChild, childBounds);
		for (auto& point : childPoints)
		{
			// Insert point into point set
			pointSet.insert(point);

			// Then project it into the tree
			tree.project(point);
		}
	}

	// Check number of points for the nodes.
	// The root should have zero at this point.
	// Each of the children should have noPointsPerChild
	auto root = tree.getRoot();
	EXPECT_THAT(root->points->size(), Eq(0));
	for (int i = 0; i < 8; i++)
	{
		auto child = root->children[i].get();
		EXPECT_THAT(child->points->size(), Eq(noPointsPerChild));
	}

	// Ask the tree to subsample
	tree.subsampleTree(mockExporter.get());

	// All children should be exported at this time.
	EXPECT_THAT(mockExporter->getExportedNodeCount(), Eq(8));

	// Pointers to the children should also have been reset
	for (int i = 0; i < 8; i++)
	{
		EXPECT_THAT(root->children[i], Eq(nullptr));
	}

	// Get exported data from the mock exporter
	auto exportedNodes = mockExporter->getExportedNodes();

	// Check that the root position was always appended to the position of the exported node
	for (auto& exportedNode : exportedNodes)
	{
		EXPECT_THAT(exportedNode.treePosition.size(), Gt(1));
		EXPECT_THAT(exportedNode.treePosition[0], Eq(123));
	}

	// Check number of points
	// Sum needs to be equivalent to the original number of points
	auto sum = mockExporter->getExportedPointCount();
	sum += root->points->size();

	int expectedPointNo = noPointsPerChild * 8;
	EXPECT_THAT(sum, Eq(expectedPointNo));

	// Now go over all points (root and exported children) and check them against our original set.
	// If there is a duplicate point, we will try to find it twice in the set and strike out the second
	// time.
	// If there is a point missing, there will be points left in the set later.

	// Root first
	for (auto& point : *(root->points))
	{
		if (!pointSet.contains(point))
		{
			FAIL() << "Encountered point not present in original set anymore. Duplicate?";
			return;
		}

		// If we have seen the point, we can erase it.
		pointSet.erase(point);
	}

	// Now the children
	for (auto& exportedNode : exportedNodes)
	{
		for (auto& point : exportedNode.pointData)
		{
			if (!pointSet.contains(point))
			{
				FAIL() << "Encountered point not present in original set anymore. Duplicate?";
				return;
			}

			// If we have seen the point, we can erase it.
			pointSet.erase(point);
		}
	}

	// The set should be empty now if we indeed saw all points
	EXPECT_THAT(pointSet.size(), Eq(0));	
}

TEST(PointCloudOptimizerTest, OctreeSubsamplingSingleCell)
{
	// This test is checking that the subsampling itself works correctly.
	// Once the points of the child are split across the 64x64x64 cube,
	// only a single point per cell should be elevated to the node above.
	// This test is placing 10 nodes into a single cell for each child
	// so after the subsampling, 9 should remain in the child and one should
	// have made it to the parent.

	int splits = 1;

	// We will be just projecting into a single cell per child, so we won't need that many points.
	int noPointsPerChild = 10;

	// Generate (cubic) bounding box
	double boundsLength = 10;
	auto boundsMin = repo::lib::RepoVector3D64(-5, -5, -5);
	auto dims = repo::lib::RepoVector3D64(boundsLength, boundsLength, boundsLength);
	auto boundsMax = boundsMin + dims;
	auto cloudBounds = repo::lib::RepoBounds(boundsMin, boundsMax);

	// Create octree
	auto tree = Octree(splits, cloudBounds, { 123 });

	// Make mock exporter
	auto mockExporter = std::make_unique<TestPCExport>(
		nullptr,
		"",
		"",
		repo::lib::RepoUUID(),
		std::vector<double>(3, 0),
		cloudBounds);

	// For each of the children, create points
	auto pointSet = std::unordered_set<repo::core::model::PointData, repo::core::model::PointDataHasher>();
	for (int i = 0; i < 8; i++)
	{
		// Try to calculate a single cell somewhere in the middle of the leaf

		auto childBounds = PointCloudUtils::getChildBoundsByIndex(cloudBounds, i);
		
		double cellLength = (childBounds.max() - childBounds.min()).x / 64;
		auto cellDims = repo::lib::RepoVector3D64(cellLength, cellLength, cellLength);
		
		// Make a midpoint in a cell
		auto cellMid = childBounds.min() + (cellDims * 31.5);
		
		// We take a quarter of a cell length as the "radius"
		auto shrink = cellDims * 0.25;

		// Calculate min and max
		auto cellMin = cellMid - shrink;
		auto cellMax = cellMid + shrink;

		// Calculate cell bounds
		auto cellBounds = repo::lib::RepoBounds(cellMin, cellMax);

		auto childPoints = makePoints(noPointsPerChild, cellBounds);
		for (auto& point : childPoints)
		{
			// Insert point into point set
			pointSet.insert(point);

			// Then project it into the tree
			tree.project(point);
		}
	}

	// Check number of points for the nodes.
	// The root should have zero at this point.
	// Each of the children should have noPointsPerChild
	auto root = tree.getRoot();
	EXPECT_THAT(root->points->size(), Eq(0));
	for (int i = 0; i < 8; i++)
	{
		auto child = root->children[i].get();
		EXPECT_THAT(child->points->size(), Eq(noPointsPerChild));
	}

	// Ask the tree to subsample
	tree.subsampleTree(mockExporter.get());


	// All children should be exported at this time.
	EXPECT_THAT(mockExporter->getExportedNodeCount(), Eq(8));

	// Pointers to the children should also have been reset
	for (int i = 0; i < 8; i++)
	{
		EXPECT_THAT(root->children[i], Eq(nullptr));
	}

	// Get exported data from the mock exporter
	auto exportedNodes = mockExporter->getExportedNodes();

	// Check that the root position was always appended to the position of the exported node
	for (auto& exportedNode : exportedNodes)
	{
		EXPECT_THAT(exportedNode.treePosition.size(), Gt(1));
		EXPECT_THAT(exportedNode.treePosition[0], Eq(123));
	}

	// Check that there are no empty nodes exported
	EXPECT_THAT(mockExporter->getEmptyExportedNodesCount(), Eq(0));

	// Check number of points
	// The sum of the children should be (noPointsPerChild * 8) - 8
	// since only one point should have been extracted
	auto sum = mockExporter->getExportedPointCount();
	EXPECT_THAT(sum, Eq((noPointsPerChild * 8) - 8));
		
	// Root should have 8 points
	EXPECT_THAT(root->points->size(), Eq(8));

	// Add the number of parent points to the sum
	sum += root->points->size();

	// Sum needs to be equivalent to the original number of points
	int expectedPointNo = noPointsPerChild * 8;
	EXPECT_THAT(sum, Eq(expectedPointNo));

	// Now go over all points (root and exported children) and check them against our original set.
	// If there is a duplicate point, we will try to find it twice in the set and strike out the second
	// time.
	// If there is a point missing, there will be points left in the set later.

	// Root first
	for (auto& point : *(root->points))
	{
		if (!pointSet.contains(point))
		{
			FAIL() << "Encountered point not present in original set anymore. Duplicate?";
			return;
		}

		// If we have seen the point, we can erase it.
		pointSet.erase(point);
	}

	// Now the children
	for (auto& exportedNode : exportedNodes)
	{
		for (auto& point : exportedNode.pointData)
		{
			if (!pointSet.contains(point))
			{
				FAIL() << "Encountered point not present in original set anymore. Duplicate?";
				return;
			}

			// If we have seen the point, we can erase it.
			pointSet.erase(point);
		}
	}

	// The set should be empty now if we indeed saw all points
	EXPECT_THAT(pointSet.size(), Eq(0));
}

TEST(PointCloudOptimizerTest, OctreeSubsamplingMultiLevel)
{
	// This test is testing the overall subsampling behaviour for a multiple level.
	// Each leaf will be equipped with enough points to (if evenly spread) fill all
	// its cells and then subsampled three times.
	// The test checks for missing points, failures in exporting the point data, and
	// that the memory is freed correctly of the children before freeing their memory.

	int splits = 3;

	// The points of each child are projected into a cube of 64x64x64 cells
	// during subsampling and one is picked from each cell randomly. To ensure
	// that this test does not simply drain each child, we need to have at least
	// as many points.
	int noPointsPerChild = 64 * 64 * 64 + 1;

	// Generate (cubic) bounding box
	double boundsLength = 10;
	auto boundsMin = repo::lib::RepoVector3D64(-5, -5, -5);
	auto dims = repo::lib::RepoVector3D64(boundsLength, boundsLength, boundsLength);
	auto boundsMax = boundsMin + dims;
	auto cloudBounds = repo::lib::RepoBounds(boundsMin, boundsMax);

	// Create octree
	auto tree = Octree(splits, cloudBounds, { 123 });

	// Make mock exporter
	auto mockExporter = std::make_unique<TestPCExport>(
		nullptr,
		"",
		"",
		repo::lib::RepoUUID(),
		std::vector<double>(3, 0),
		cloudBounds);

	// For each of the leafs, create points
	for (int i0 = 0; i0 < 8; i0++)
	{
		for (int i1 = 0; i1 < 8; i1++)
		{
			for (int i2 = 0; i2 < 8; i2++)
			{
				auto leafPos = std::vector<uint8_t>();
				leafPos.push_back(i0);
				leafPos.push_back(i1);
				leafPos.push_back(i2);

				auto leafBounds = PointCloudUtils::getBoundsFromTreePosition(leafPos, cloudBounds);
				// We are actually shrinking the bounds for the generation a little bit (1%).
				// The projection is actually not always inclusive of the lower bound
				// (for more detail see description in repo_point_cloud_utils.h), which in
				// production is not an issue, but gets in the way of testing.
				auto shrink = (leafBounds.max() - leafBounds.min()) * 0.01;
				auto leafMin = leafBounds.min() + shrink;
				auto leafMax = leafBounds.max() - shrink;
				leafBounds = repo::lib::RepoBounds(leafMin, leafMax);

				auto childPoints = makePoints(noPointsPerChild, leafBounds);
				for (auto& point : childPoints)
				{
					// Project into the tree
					tree.project(point);
				}
			}
		}
	}
	
	auto root = tree.getRoot();

	// Traverse tree before the supersampling to check the
	// number of points in the nodes.
	// Each of the children should have noPointsPerChild.
	// All nodes above should have zero.
	std::stack<OctreeNode*> stack;
	stack.push(root);
	while (stack.size() > 0)
	{
		auto node = stack.top();
		stack.pop();

		bool isLeaf = true;
		for (int i = 0; i < 8; i++)
		{
			if (node->children[i] != nullptr)
				isLeaf = false;
		}

		if (isLeaf)
		{
			EXPECT_THAT(node->points->size(), Eq(noPointsPerChild));
		}
		else
		{
			EXPECT_THAT(node->points->size(), Eq(0));
			for (int i = 0; i < 8; i++)
			{
				if (node->children[i] != nullptr)
					stack.push(node->children[i].get());
			}
		}
	}

	// Ask the tree to subsample
	tree.subsampleTree(mockExporter.get());

	// All nodes except root should be exported at this time.
	// (N^L -1) / (N - 1) with N being the number of children and L the number of levels (split + 1)
	int noNodesExpected = (std::pow(8, splits + 1) - 1) / 7;
	noNodesExpected -= 1;
	EXPECT_THAT(mockExporter->getExportedNodeCount(), Eq(noNodesExpected));

	// Pointers to the children should also have been reset
	for (int i = 0; i < 8; i++)
	{
		EXPECT_THAT(root->children[i], Eq(nullptr));
	}

	// Get exported data from the mock exporter
	auto exportedNodes = mockExporter->getExportedNodes();

	// Check that the root position was always appended to the position of the exported node
	for (auto& exportedNode : exportedNodes)
	{
		EXPECT_THAT(exportedNode.treePosition.size(), Gt(1));
		EXPECT_THAT(exportedNode.treePosition[0], Eq(123));
	}

	// Check that there are no empty nodes exported
	EXPECT_THAT(mockExporter->getEmptyExportedNodesCount(), Eq(0));

	// Check number of points
	// Sum needs to be equivalent to the original number of points
	auto sum = mockExporter->getExportedPointCount();
	sum += root->points->size();

	int expectedPointNo = noPointsPerChild * (std::pow(8, splits));
	EXPECT_THAT(sum, Eq(expectedPointNo));
}

TEST(PointCloudOptimizerTest, OctreeSubsamplingPostTreeMerge)
{
	// This test is testing the overall subsampling behaviour for a multiple level tree
	// that had one node replaced with one that was already subsampled. This could happen
	// when multiple trees are merged together and would lead to an "imbalanced" tree where
	// different paths have different depths.
	// The test verifies that we are not having segfaults or missing points when this tree shape
	// is encountered

	int splits = 3;

	// The points of each child are projected into a cube of 64x64x64 cells
	// during subsampling and one is picked from each cell randomly. To ensure
	// that this test does not simply drain each child, we need to have at least
	// as many points.
	int noPointsPerChild = 64 * 64 * 64 + 1;

	// Generate (cubic) bounding box
	double boundsLength = 10;
	auto boundsMin = repo::lib::RepoVector3D64(-5, -5, -5);
	auto dims = repo::lib::RepoVector3D64(boundsLength, boundsLength, boundsLength);
	auto boundsMax = boundsMin + dims;
	auto cloudBounds = repo::lib::RepoBounds(boundsMin, boundsMax);

	// Create octree
	auto tree = Octree(splits, cloudBounds, { 123 });

	// Make mock exporter
	auto mockExporter = std::make_unique<TestPCExport>(
		nullptr,
		"",
		"",
		repo::lib::RepoUUID(),
		std::vector<double>(3, 0),
		cloudBounds);

	// For each of the leafs, create points
	// but we are skipping the path that starts with
	// the child with the index 7
	for (int i0 = 0; i0 < 7; i0++)
	{
		for (int i1 = 0; i1 < 8; i1++)
		{
			for (int i2 = 0; i2 < 8; i2++)
			{
				auto leafPos = std::vector<uint8_t>();
				leafPos.push_back(i0);
				leafPos.push_back(i1);
				leafPos.push_back(i2);

				auto leafBounds = PointCloudUtils::getBoundsFromTreePosition(leafPos, cloudBounds);
				// We are actually shrinking the bounds for the generation a little bit (1%).
				// The projection is actually not always inclusive of the lower bound
				// (for more detail see description in repo_point_cloud_utils.h), which in
				// production is not an issue, but gets in the way of testing.
				auto shrink = (leafBounds.max() - leafBounds.min()) * 0.01;
				auto leafMin = leafBounds.min() + shrink;
				auto leafMax = leafBounds.max() - shrink;
				leafBounds = repo::lib::RepoBounds(leafMin, leafMax);

				auto childPoints = makePoints(noPointsPerChild, leafBounds);
				for (auto& point : childPoints)
				{
					// Project into the tree
					tree.project(point);
				}
			}
		}
	}

	auto root = tree.getRoot();

	// Now we replace the path of child 8 (index 7) of the root with one node that we constructed ourselves.
	// This simulates merging a tree that was subsampled already on that path
	auto targetPos = std::vector<uint8_t>(1, 7);
	auto node = std::make_unique<OctreeNode>();
	node->nodeTreePosition = targetPos;
	node->nodeBounds = PointCloudUtils::getBoundsFromTreePosition(targetPos, cloudBounds);
	node->points = std::make_unique < std::vector<repo::core::model::PointData>>();
	
	// Populate that node with points too
	auto nodeBounds = PointCloudUtils::getBoundsFromTreePosition(targetPos, cloudBounds);
	auto shrink = (nodeBounds.max() - nodeBounds.min()) * 0.01;
	auto leafMin = nodeBounds.min() + shrink;
	auto leafMax = nodeBounds.max() - shrink;
	nodeBounds = repo::lib::RepoBounds(leafMin, leafMax);

	auto childPoints = makePoints(noPointsPerChild, nodeBounds);
	for (auto& point : childPoints)
	{
		// Add to node
		node->points->push_back(point);
	}

	// Add node to tree
	tree.addNode(std::move(node), targetPos);

	// Ask the tree to subsample
	tree.subsampleTree(mockExporter.get());


	// All nodes except root should be exported at this time.
	// (N^L -1) / (N - 1) with N being the number of children and L the number of levels (split + 1)
	int noNodesExpected = (std::pow(8, splits + 1) - 1) / 7;
	noNodesExpected -= (std::pow(8, splits) - 1) / 7; // substract the one branch that is already subsampled
	EXPECT_THAT(mockExporter->getExportedNodeCount(), Eq(noNodesExpected));

	// Pointers to the children should also have been reset
	for (int i = 0; i < 8; i++)
	{
		EXPECT_THAT(root->children[i], Eq(nullptr));
	}

	// Get exported data from the mock exporter
	auto exportedNodes = mockExporter->getExportedNodes();

	// Check that the root position was always appended to the position of the exported node
	for (auto& exportedNode : exportedNodes)
	{
		EXPECT_THAT(exportedNode.treePosition.size(), Gt(1));
		EXPECT_THAT(exportedNode.treePosition[0], Eq(123));
	}

	// Check that there are no empty nodes exported
	EXPECT_THAT(mockExporter->getEmptyExportedNodesCount(), Eq(0));

	// Check number of points
	// Sum needs to be equivalent to the original number of points
	auto sum = mockExporter->getExportedPointCount();
	sum += root->points->size();

	int expectedPointNo = noPointsPerChild * ((std::pow(8, splits)) - (std::pow(8, splits - 1))) + noPointsPerChild;
	EXPECT_THAT(sum, Eq(expectedPointNo));
}


TEST(PointCloudOptimizerTest, OctreeAddNode)
{
	// Generate (cubic) bounding box
	double boundsLength = 10;
	auto boundsMin = repo::lib::RepoVector3D64(-5, -5, -5);
	auto dims = repo::lib::RepoVector3D64(boundsLength, boundsLength, boundsLength);
	auto boundsMax = boundsMin + dims;
	auto cloudBounds = repo::lib::RepoBounds(boundsMin, boundsMax);

	// Insert node at root level
	{
		auto tree = Octree(3, cloudBounds, {});
		
		auto rootPos = std::vector<uint8_t>();
		auto point = repo::core::model::PointData(cloudBounds.center(), repo::lib::repo_color4d_t());

		// Make node to add
		auto node = std::make_unique<OctreeNode>();
		node->nodeTreePosition = rootPos;
		node->nodeBounds = cloudBounds;
		node->points = std::make_unique < std::vector<repo::core::model::PointData>>();
		node->points->push_back(point);


		// Check root before adding the node
		auto root = tree.getRoot();
		EXPECT_THAT(root->points->size(), Eq(0));
		for (int i = 0; i < 8; i++)
			EXPECT_THAT(root->children[i], Ne(nullptr));

		// Add the node
		tree.addNode(std::move(node), rootPos);

		// Check root after.
		// Should be the node just added now.
		// Can be recognised by the one point we added
		// and it not having any children anymore.
		root = tree.getRoot();
		EXPECT_THAT(root->points->size(), Eq(1));
		for (int i = 0; i < 8; i++)
			EXPECT_THAT(root->children[i], Eq(nullptr));
	}

	// Insert node at first level
	{
		auto tree = Octree(3, cloudBounds, {});

		auto targetPos = std::vector<uint8_t>();
		targetPos.push_back(1);
		auto point = repo::core::model::PointData(cloudBounds.center(), repo::lib::repo_color4d_t());

		// Make node to add
		auto node = std::make_unique<OctreeNode>();
		node->nodeTreePosition = targetPos;
		node->nodeBounds = PointCloudUtils::getBoundsFromTreePosition(targetPos, cloudBounds);
		node->points = std::make_unique < std::vector<repo::core::model::PointData>>();
		node->points->push_back(point);

		// Check node before adding the node
		auto treeNode = tree.getNode(targetPos);
		EXPECT_THAT(treeNode->points->size(), Eq(0));
		for (int i = 0; i < 8; i++)
			EXPECT_THAT(treeNode->children[i], Ne(nullptr));

		// Add the node
		tree.addNode(std::move(node), targetPos);

		// Check tree node after.
		// Should be the node just added now.
		// Can be recognised by the one point we added
		// and it not having any children anymore.
		treeNode = tree.getNode(targetPos);
		EXPECT_THAT(treeNode->points->size(), Eq(1));
		for (int i = 0; i < 8; i++)
			EXPECT_THAT(treeNode->children[i], Eq(nullptr));
	}

	// Insert node at leaf level
	{
		auto tree = Octree(3, cloudBounds, {});

		auto targetPos = std::vector<uint8_t>();
		targetPos.push_back(1);
		targetPos.push_back(2);
		targetPos.push_back(3);
		auto point = repo::core::model::PointData(cloudBounds.center(), repo::lib::repo_color4d_t());

		// Make node to add
		auto node = std::make_unique<OctreeNode>();
		node->nodeTreePosition = targetPos;
		node->nodeBounds = PointCloudUtils::getBoundsFromTreePosition(targetPos, cloudBounds);
		node->points = std::make_unique < std::vector<repo::core::model::PointData>>();
		node->points->push_back(point);

		// Check node before adding the node
		auto treeNode = tree.getNode(targetPos);
		EXPECT_THAT(treeNode->points->size(), Eq(0));
		for (int i = 0; i < 8; i++)
			EXPECT_THAT(treeNode->children[i], Eq(nullptr));

		// Add the node
		tree.addNode(std::move(node), targetPos);

		// Check tree node after.
		// Should be the node just added now.
		// Can be recognised by the one point we added.
		treeNode = tree.getNode(targetPos);
		EXPECT_THAT(treeNode->points->size(), Eq(1));
		for (int i = 0; i < 8; i++)
			EXPECT_THAT(treeNode->children[i], Eq(nullptr));
	}

	// insert node at level beyond leaves
	{
		auto tree = Octree(3, cloudBounds, {});

		auto targetPos = std::vector<uint8_t>();
		targetPos.push_back(1);
		targetPos.push_back(2);
		targetPos.push_back(3);
		targetPos.push_back(4);
		auto point = repo::core::model::PointData(cloudBounds.center(), repo::lib::repo_color4d_t());

		// Make node to add
		auto node = std::make_unique<OctreeNode>();
		node->nodeTreePosition = targetPos;
		node->nodeBounds = PointCloudUtils::getBoundsFromTreePosition(targetPos, cloudBounds);
		node->points = std::make_unique < std::vector<repo::core::model::PointData>>();
		node->points->push_back(point);


		// Add the node
		EXPECT_THROW({
			tree.addNode(std::move(node), targetPos);
			},
			repo::lib::RepoException);
	}
}

TEST(PointCloudOptimizerTest, ProcessSingleChunkPointCloud)
{
	srand(time(NULL));

	auto handler = getHandler();
	std::string database = DBPOINTCLOUDOPTIMIZERTEST;
	std::string projectName = "TestSingleChunkPointCloud";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto cloudBounds = repo::lib::RepoBounds(randV1, randV2);

	// Generate single chunk based on these bounds
	sceneBuilder.addNode(createRandomPointChunk(
		REPO_PC_CHUNKING_MAXPOINTS,
		cloudBounds, 
		{ rootNodeId },
		{}));

	sceneBuilder.finalise();

	// Make mock exporter
	auto mockExporter = std::make_unique<TestPCExport>(
		nullptr,
		"",
		"",
		repo::lib::RepoUUID(),
		std::vector<double>(3, 0),
		cloudBounds);

	PointCloudOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId,
		cloudBounds
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	// (N^L -1) / (N - 1) with N being the number of children and L the number of levels (split + 1)
	// This is the maximum number of nodes we expect to see, since we don't export empty nodes.
	int noNodesExpectedUpperBound = (std::pow(8, REPO_PC_OPTIMISATION_SPLITS + 1) - 1) / 7;
	EXPECT_THAT(mockExporter->getExportedNodeCount(), Lt(noNodesExpectedUpperBound));

	// Lastly check number of points and that there are no empty nodes
	EXPECT_THAT(mockExporter->getExportedPointCount(), Eq(REPO_PC_CHUNKING_MAXPOINTS));
	EXPECT_THAT(mockExporter->getEmptyExportedNodesCount(), Eq(0));
}

TEST(PointCloudOptimizerTest, ProcessMultiChunkPointCloud)
{
	srand(time(NULL));

	auto handler = getHandler();
	std::string database = DBPOINTCLOUDOPTIMIZERTEST;
	std::string projectName = "TestSingleChunkPointCloud";
	auto revId = repo::lib::RepoUUID::createUUID();

	auto sceneBuilder = repo::manipulator::modelutility::RepoSceneBuilder(handler, database, projectName, revId);

	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder.addNode(rootNode);
	auto rootNodeId = rootNode.getSharedID();

	// Generate random (cubic) bounding box
	double randLength = (double)rand();
	auto randV1 = makeRandomRepoVector();
	auto dims = repo::lib::RepoVector3D64(randLength, randLength, randLength);
	auto randV2 = randV1 + dims;
	auto cloudBounds = repo::lib::RepoBounds(randV1, randV2);

	// Split this bounding box into eight chunks and generate points for each
	for (uint8_t i = 0; i < 8; i++)
	{
		auto min = cloudBounds.min();
		auto max = cloudBounds.max();
		auto halfDim = (max - min) * 0.5;

		switch (i)
		{
			case 0:
				min = min + halfDim * repo::lib::RepoVector3D64(0, 0, 1);
				max = max - halfDim * repo::lib::RepoVector3D64(1, 1, 0);
				break;
			case 1:
				min = min + halfDim * repo::lib::RepoVector3D64(1, 0, 1);
				max = max - halfDim * repo::lib::RepoVector3D64(0, 1, 0);
				break;
			case 2:
				min = min + halfDim * repo::lib::RepoVector3D64(1, 1, 1);
				max = max - halfDim * repo::lib::RepoVector3D64(0, 0, 0);
				break;
			case 3:
				min = min + halfDim * repo::lib::RepoVector3D64(0, 1, 1);
				max = max - halfDim * repo::lib::RepoVector3D64(1, 0, 0);
				break;
			case 4:
				min = min + halfDim * repo::lib::RepoVector3D64(0, 0, 0);
				max = max - halfDim * repo::lib::RepoVector3D64(1, 1, 1);
				break;
			case 5:
				min = min + halfDim * repo::lib::RepoVector3D64(1, 0, 0);
				max = max - halfDim * repo::lib::RepoVector3D64(0, 1, 1);
				break;
			case 6:
				min = min + halfDim * repo::lib::RepoVector3D64(1, 1, 0);
				max = max - halfDim * repo::lib::RepoVector3D64(0, 0, 1);
				break;
			case 7:
				min = min + halfDim * repo::lib::RepoVector3D64(0, 1, 0);
				max = max - halfDim * repo::lib::RepoVector3D64(1, 0, 1);
				break;
		}

		auto chunkBounds = repo::lib::RepoBounds(min, max);

		auto chunkPos = std::vector<uint8_t>(1, i);

		sceneBuilder.addNode(createRandomPointChunk(
			REPO_PC_CHUNKING_MAXPOINTS,
			chunkBounds,
			{ rootNodeId },
			chunkPos));
	}

	sceneBuilder.finalise();

	// Make mock exporter
	auto mockExporter = std::make_unique<TestPCExport>(
		nullptr,
		"",
		"",
		repo::lib::RepoUUID(),
		std::vector<double>(3, 0),
		cloudBounds);

	PointCloudOptimizer opt(handler.get(), mockExporter.get());

	opt.processScene(
		database,
		projectName,
		revId,
		cloudBounds
	);

	EXPECT_TRUE(mockExporter->isFinalised());

	// (N^L -1) / (N - 1) with N being the number of children and L the number of levels (split + 1)
	// This is the maximum number of nodes we expect to see, since we don't export empty nodes.
	int noNodesExpectedUpperBound = (std::pow(8, REPO_PC_OPTIMISATION_SPLITS + 1) - 1) / 7;
	EXPECT_THAT(mockExporter->getExportedNodeCount(), Lt(noNodesExpectedUpperBound));

	// Lastly check number of points and that there are no empty nodes
	EXPECT_THAT(mockExporter->getExportedPointCount(), Eq(8 * REPO_PC_CHUNKING_MAXPOINTS));
	EXPECT_THAT(mockExporter->getEmptyExportedNodesCount(), Eq(0));
}