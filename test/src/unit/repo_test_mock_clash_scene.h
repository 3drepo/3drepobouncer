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

#pragma once

#include <repo/manipulator/modelutility/repo_clash_detection_config.h>
#include <repo/lib/datastructure/repo_container.h>
#include <repo/lib/datastructure/repo_uuid.h>
#include <repo/lib/datastructure/repo_matrix.h>
#include <repo/lib/datastructure/repo_line.h>
#include <repo/lib/datastructure/repo_triangle.h>
#include <repo/lib/datastructure/repo_range.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <vector>
#include <set>

#include "repo_test_random_generator.h"

namespace testing {

	using TransformLine = std::pair<repo::lib::RepoLine, repo::lib::RepoMatrix>;
	using TransformTriangle = std::pair<repo::lib::RepoTriangle, repo::lib::RepoMatrix>;
	using TransformLines = std::pair<TransformLine, TransformLine>;
	using TransformTriangles = std::pair<TransformTriangle, TransformTriangle>;
	using UUIDPair = std::pair<repo::lib::RepoUUID, repo::lib::RepoUUID>;
	using TrianglePair = std::pair<repo::lib::RepoTriangle, repo::lib::RepoTriangle>;

	struct ClashDetectionConfigHelper : public repo::manipulator::modelutility::ClashDetectionConfig
	{
		ClashDetectionConfigHelper();

		void addCompositeObjects(
			const repo::lib::RepoUUID& uniqueIdA, 
			const repo::lib::RepoUUID& uniqueIdB
		);

		/*
		* Gets the revision for Container 0, which is the container created by the
		* constructor, and to which composite objects are added by the helper methods.
		*/
		const repo::lib::RepoUUID& getRevision();
	};

	/*
	* Helper class that partitions space into separate cells in which clash problems
	* can be created so that they don't overlap.
	*/

	struct CellDistribution
	{
		CellDistribution(size_t cellSize, size_t spaceSize);

		// Gets a cell from the distribution, using uniform sampling. Each cell may
		// only be returned once for a given distribution.

		repo::lib::RepoBounds sample();

		repo::lib::RepoBounds getBounds(size_t cell) const;

	private:
		std::set<size_t> used;
		RepoRandomGenerator random;
		size_t cellSize;
		size_t cellsPerAxis;
		size_t totalCells;
		repo::lib::RepoVector3D64 start;
	};

	struct ClashGenerator
	{
		RepoRandomGenerator random;

		using Range = repo::lib::RepoRange;

		// The scenes are generated using the inbuilt rng object, using a subset of the
		// following ranges. Which ranges are used depends on the scene being generated.

		Range size1 = { 0.001, 8e6 };
		Range size2 = { 0.001, 8e6 };

		Range distance = { 0, 100 };

		// When true, any "vertices" such as the vectors of lines and triangles will be
		// downcast to single precision, even if the eventual return type is double
		// precision.

		bool downcastVertices = true;

		// Creates a pair of lines that are separated by the given distance exactly 
		// when transformed by their respective matrices. The problem will be centered
		// within the given bounds (it may not be constrained by the bounds, if size1 
		// or size2 exceed them). This method uses size1 and size2 for the line
		// lengths, and distance.

		TransformLines createLinesTransformed(
			const repo::lib::RepoBounds& bounds
		);

		// These next methods create pairs of triangles that are separated by the given
		// distance exactly when transformed by their respective matrices.

		// Triangles are closest at two vertices

		TransformTriangles createTrianglesVV(
			const repo::lib::RepoBounds& bounds
		);

		// Triangles are closest between a vertex from one triangle, and an edge
		// from the other.

		TransformTriangles createTrianglesVE(
			const repo::lib::RepoBounds& bounds
		);

		// Triangles are closest at two edges, where the closest points are *not*
		// conincident with vertices of either edge.

		TransformTriangles createTrianglesEE(
			const repo::lib::RepoBounds& bounds
		);

		// Triangles are closest at the face of one triangle and a vertex of the other
		// (the point on the face is farther from the face's own vertices or edges than
		// the vertex on the other triangle). This test includes the case where two
		// vertices from one triangle have the same distance (i.e. the edge itself has
		// a constant distance to the face).

		TransformTriangles createTrianglesVF(
			const repo::lib::RepoBounds& bounds
		);

		// One edge from one triangle intersects the face of the other triangle. 
		// (Imagine, the triangle borders are like links in a chain.) The true distance
		// in this case is always zero. The point of intersection on the face is
		// farther from the face's own vertices or edges than any distance between the
		// pairs of vertices or edges forming the two triangles.

		TransformTriangles createTrianglesFE(
			const repo::lib::RepoBounds& bounds
		);

		void moveToBounds(TransformLines& problem, const repo::lib::RepoBounds& bounds);

		void moveToBounds(TransformTriangles& problem, const repo::lib::RepoBounds& bounds);

		void moveB(TransformTriangles& problem, const repo::lib::RepoRange& range);

		void moveProblem(TransformTriangles& problem, const repo::lib::RepoRange& range);

		// Circular shift the vertices by a random amount, so that if a procedural triangle
		// is created such that vertex a is always, for example, the closest feature, then
		// other vertices may be that feature.
		void shiftTriangles(repo::lib::RepoTriangle& problem);

		void downcast(TransformTriangles& problem);

		void downcast(TransformLines& line);

		void downcast(repo::lib::RepoTriangle& triangle);

		void downcast(repo::lib::RepoLine& line);

		static TrianglePair applyTransforms(TransformTriangles& problem);

		// Returns a value that can be used as a tolerance when checking if a vector
		// is close to a particular element of the triangle, based on the size of 
		// the provided triangles.

		static double suggestTolerance(std::initializer_list<repo::lib::RepoTriangle> triangles);
	};

	struct MockClashScene
	{
		std::vector<repo::core::model::RepoBSON> bsons;
		repo::lib::RepoUUID root;

		MockClashScene(const repo::lib::RepoUUID& revision);

		const repo::core::model::MeshNode add(repo::lib::RepoLine line, const repo::lib::RepoUUID& parentSharedId);

		const repo::core::model::TransformationNode add(const repo::lib::RepoMatrix& matrix);

		UUIDPair add(TransformLines lines, ClashDetectionConfigHelper& config);
	};
}