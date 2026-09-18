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

#include "repo_point_cloud_utils.h"
#include "repo/lib/repo_exception.h"
#include <algorithm>

using namespace repo::lib::pointcloud;

std::vector<uint8_t> PointCloudUtils::getTreePositionFromCellCoordinates(
	int x,
	int y,
	int z,
	int splits)
{
	auto treePosition = std::vector<uint8_t>();
	int cellsPerSideSplit = std::pow(2, splits) / 2;
	int midpointX = cellsPerSideSplit;
	int midpointY = cellsPerSideSplit;
	int midpointZ = cellsPerSideSplit;
	for (int i = 0; i < splits; i++)
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
		
		cellsPerSideSplit = cellsPerSideSplit / 2;

		bool xLower = x < midpointX;
		bool yLower = y < midpointY;
		bool zLower = z < midpointZ;

		if (xLower && yLower && !zLower)
		{
			treePosition.push_back(0);
			midpointX -= cellsPerSideSplit;
			midpointY -= cellsPerSideSplit;
			midpointZ += cellsPerSideSplit;
		}
		else if (!xLower && yLower && !zLower)
		{
			treePosition.push_back(1);
			midpointX += cellsPerSideSplit;
			midpointY -= cellsPerSideSplit;
			midpointZ += cellsPerSideSplit;

		}
		else if (!xLower && !yLower && !zLower)
		{
			treePosition.push_back(2);
			midpointX += cellsPerSideSplit;
			midpointY += cellsPerSideSplit;
			midpointZ += cellsPerSideSplit;

		}
		else if (xLower && !yLower && !zLower)
		{
			treePosition.push_back(3);
			midpointX -= cellsPerSideSplit;
			midpointY += cellsPerSideSplit;
			midpointZ += cellsPerSideSplit;

		}
		else if (xLower && yLower && zLower)
		{
			treePosition.push_back(4);
			midpointX -= cellsPerSideSplit;
			midpointY -= cellsPerSideSplit;
			midpointZ -= cellsPerSideSplit;

		}
		else if (!xLower && yLower && zLower)
		{
			treePosition.push_back(5);
			midpointX += cellsPerSideSplit;
			midpointY -= cellsPerSideSplit;
			midpointZ -= cellsPerSideSplit;
		}
		else if (!xLower && !yLower && zLower)
		{
			treePosition.push_back(6);
			midpointX += cellsPerSideSplit;
			midpointY += cellsPerSideSplit;
			midpointZ -= cellsPerSideSplit;
		}
		else if (xLower && !yLower && zLower)
		{
			treePosition.push_back(7);
			midpointX -= cellsPerSideSplit;
			midpointY += cellsPerSideSplit;
			midpointZ -= cellsPerSideSplit;
		}
	}

	return treePosition;
}

int PointCloudUtils::getIndexFromCellCoordinates(
	int x,
	int y,
	int z,
	int splits)
{
	int cellsPerSide = std::pow(2, splits);
	return (x * (cellsPerSide * cellsPerSide)) + (y * cellsPerSide) + z;
}

int PointCloudUtils::getIndexFromTreePosition(
	std::vector<uint8_t>& treePosition,
	int splits)
{
	int x = 0;
	int y = 0;
	int z = 0;

	int currentSteps = std::pow(2, splits);

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

	return getIndexFromCellCoordinates(x, y, z, splits);
}

repo::lib::RepoBounds PointCloudUtils::getBoundsFromTreePosition(
	std::vector<uint8_t>& treePosition,
	repo::lib::RepoBounds bounds)
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

int PointCloudUtils::project3DPositionToCellIndexSingleAxis(
	double value,
	double min,
	double max,
	int cellsPerSide)
{
	// Filter out the edges at the top and bottom of the bounds
	if (value <= min)
		return 0;
	if (value >= max)
		return cellsPerSide - 1;

	const double span = max - min;
	double scaled = ((value - min) / span) * cellsPerSide;

	// We always move the value down very very slightly.
	// This way, should a point land on a cell boundary
	// where it mathematically would fit in either, it
	// will always be parked in the lower one.
	scaled = std::nextafter(scaled, -std::numeric_limits<double>::infinity());

	int index = (int)std::floor(scaled);
	return std::clamp(index, 0, cellsPerSide - 1);
}

void PointCloudUtils::project3DPositionToCellCoordinates(
	repo::lib::RepoVector3D64 position,
	repo::lib::RepoBounds bounds,
	int splits,
	int& x,
	int& y,
	int& z)
{
	int cellsPerSide = std::pow(2, splits);

	auto min = bounds.min();
	auto max = bounds.max();

	// Calculate X-Index of cell
	x = project3DPositionToCellIndexSingleAxis(
		position.x,
		min.x,
		max.x,
		cellsPerSide);

	// Calculate Y-Index of cell
	y = project3DPositionToCellIndexSingleAxis(
		position.y,
		min.y,
		max.y,
		cellsPerSide);

	// Calculate Z-Index of cell
	z = project3DPositionToCellIndexSingleAxis(
		position.z,
		min.z,
		max.z,
		cellsPerSide);

	if (x > cellsPerSide || y > cellsPerSide || z > cellsPerSide)
		throw repo::lib::RepoException("Point Cloud Projection: Point outside of the grid detected.");
}

int PointCloudUtils::project3DPositionToCellIndex(
	repo::lib::RepoVector3D64 position,
	repo::lib::RepoBounds bounds,
	int splits)
{
	int x, y, z;
	project3DPositionToCellCoordinates(
		position,
		bounds,
		splits,
		x, y, z);

	// Calculate index in the counting array
	int index = repo::lib::pointcloud::PointCloudUtils::getIndexFromCellCoordinates(
		x, y, z,
		splits);

	return index;
}

std::vector<uint8_t> PointCloudUtils::project3DPositionToTreePosition(
	repo::lib::RepoVector3D64 position,
	repo::lib::RepoBounds bounds,
	int splits)
{
	int x, y, z;
	project3DPositionToCellCoordinates(
		position,
		bounds,
		splits,
		x, y, z);

	return getTreePositionFromCellCoordinates(
		x, y, z,
		splits);
}

repo::lib::RepoBounds PointCloudUtils::getChildBoundsByIndex(
	repo::lib::RepoBounds bounds,
	int childIndex)
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

	auto min = bounds.min();
	auto max = bounds.max();
	
	auto newDims = (max - min) / 2.0;
	RepoVector3D64 newMin;
	RepoVector3D64 newMax;

	switch (childIndex)
	{
	case 0:
		newMin = min + (newDims * repo::lib::RepoVector3D64(0, 0, 1));
		newMax = max - (newDims * repo::lib::RepoVector3D64(1, 1, 0));
		break;
	case 1:
		newMin = min + (newDims * repo::lib::RepoVector3D64(1, 0, 1));
		newMax = max - (newDims * repo::lib::RepoVector3D64(0, 1, 0));
		break;
	case 2:
		newMin = min + (newDims * repo::lib::RepoVector3D64(1, 1, 1));
		newMax = max - (newDims * repo::lib::RepoVector3D64(0, 0, 0));
		break;
	case 3:
		newMin = min + (newDims * repo::lib::RepoVector3D64(0, 1, 1));
		newMax = max - (newDims * repo::lib::RepoVector3D64(1, 0, 0));
		break;
	case 4:
		newMin = min + (newDims * repo::lib::RepoVector3D64(0, 0, 0));
		newMax = max - (newDims * repo::lib::RepoVector3D64(1, 1, 1));
		break;
	case 5:
		newMin = min + (newDims * repo::lib::RepoVector3D64(1, 0, 0));
		newMax = max - (newDims * repo::lib::RepoVector3D64(0, 1, 1));
		break;
	case 6:
		newMin = min + (newDims * repo::lib::RepoVector3D64(1, 1, 0));
		newMax = max - (newDims * repo::lib::RepoVector3D64(0, 0, 1));
		break;
	case 7:
		newMin = min + (newDims * repo::lib::RepoVector3D64(0, 1, 0));
		newMax = max - (newDims * repo::lib::RepoVector3D64(1, 0, 1));
		break;
	default:
		throw repo::lib::RepoException("Point Cloud getChildBoundsByIndex: invalid child index.");
	}

	return repo::lib::RepoBounds(newMin, newMax);
}

std::vector<uint8_t> repo::lib::pointcloud::PointCloudUtils::combineTreePositions(
	std::vector<uint8_t> a,
	std::vector<uint8_t> b)
{
	a.insert(a.end(), b.begin(), b.end());
	return a;
}
