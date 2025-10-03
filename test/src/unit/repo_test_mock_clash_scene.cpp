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

#include "repo_test_mock_clash_scene.h"

#pragma optimize("", off)

using namespace testing;
using namespace repo::core::model;
using namespace repo::manipulator::modelutility;
using namespace repo::lib;

ClashDetectionConfigHelper::ClashDetectionConfigHelper() {
	this->containers.push_back(std::make_unique<repo::lib::Container>());
	this->containers[0]->teamspace = "clash";
	this->containers[0]->container = "test_container";
	this->containers[0]->revision = repo::lib::RepoUUID::createUUID();
}

void ClashDetectionConfigHelper::addCompositeObjects(
	const repo::lib::RepoUUID& uniqueIdA, 
	const repo::lib::RepoUUID& uniqueIdB
) {
	this->setA.push_back(
		repo::manipulator::modelutility::CompositeObject(
			uniqueIdA,
			{ repo::manipulator::modelutility::MeshReference(containers[0].get(), uniqueIdA) }
		)
	);
	this->setB.push_back(
		repo::manipulator::modelutility::CompositeObject(
			uniqueIdB,
			{ repo::manipulator::modelutility::MeshReference(containers[0].get(), uniqueIdB) }
		)
	);
}

const repo::lib::RepoUUID& testing::ClashDetectionConfigHelper::getRevision()
{
	return this->containers[0]->revision;
}

MockClashScene::MockClashScene(const RepoUUID& revision) {
	auto n = RepoBSONFactory::makeTransformationNode({}, "rootNode");
	root = n.getSharedID();
	bsons.push_back(n.getBSON());

	auto r = RepoBSONFactory::makeRevisionNode(
		{},
		{},
		revision,
		{},
		{},
		{ 0.0, 0.0, 0.0 }
	);
	bsons.push_back(r.getBSON());
}

const MeshNode MockClashScene::add(RepoLine line, const RepoUUID& parentSharedId) {
	auto m1 = RepoBSONFactory::makeMeshNode(
		{ line.start, line.start, line.end },
		{ {0, 1, 2} },
		{},
		repo::lib::RepoBounds({ line.start, line.end }),
		{},
		{},
		{ parentSharedId }
	);
	bsons.push_back(m1.getBSON());
	return m1;
}

const TransformationNode MockClashScene::add(const RepoMatrix& matrix) {
	auto t1 = RepoBSONFactory::makeTransformationNode(
		matrix,
		{},
		{ root }
	);
	bsons.push_back(t1.getBSON());
	return t1;
}

UUIDPair MockClashScene::add(TransformLines lines, ClashDetectionConfigHelper& config)
{
	// Adding lines will create degenerate triangles - this is not only sufficient
	// for the tests, but the clash pipeline should explicitly handle these cases
	// as they may be encountered in real data.

	auto t1 = add(lines.first.second);
	auto m1 = add(lines.first.first, t1.getSharedID());

	auto t2 = add(lines.second.second);
	auto m2 = add(lines.second.first, t2.getSharedID());

	config.addCompositeObjects(m1.getUniqueID(), m2.getUniqueID());

	return { m1.getUniqueID(), m2.getUniqueID() };
}

void ClashGenerator::downcast(repo::lib::RepoLine& line) {
	line.start = repo::lib::RepoVector3D(line.start);
	line.end = repo::lib::RepoVector3D(line.end);
}

void ClashGenerator::downcast(repo::lib::RepoTriangle& triangle) {
	triangle.a = repo::lib::RepoVector3D(triangle.a);
	triangle.b = repo::lib::RepoVector3D(triangle.b);
	triangle.c = repo::lib::RepoVector3D(triangle.c);
}

void ClashGenerator::downcast(TransformTriangles& problem)
{
	downcast(problem.first.first);
	downcast(problem.second.first);
}

void ClashGenerator::downcast(TransformLines& problem)
{
	downcast(problem.first.first);
	downcast(problem.second.first);
}

void ClashGenerator::moveB(TransformTriangles& problem, const repo::lib::RepoRange& range)
{
	// As this method will modify the vertices before the downcast, we may only apply
	// transforms that do not move the vertex beyond the supported range (currently 8e6).

	auto largestDimension = std::max({
		std::abs(problem.second.first.a.x),
		std::abs(problem.second.first.a.y),
		std::abs(problem.second.first.a.z),
		std::abs(problem.second.first.b.x),
		std::abs(problem.second.first.b.y),
		std::abs(problem.second.first.b.z),
		std::abs(problem.second.first.c.x),
		std::abs(problem.second.first.c.y),
		std::abs(problem.second.first.c.z)
		});
	auto remainingRange = std::max(0.0, range.max() - largestDimension);

	problem.second.second = random.transform(true, { 0, remainingRange }, {});
	problem.second.first = problem.second.second.invert() * problem.second.first;
}

void ClashGenerator::moveProblem(TransformTriangles& problem, const repo::lib::RepoRange& range)
{
	problem.first.second = random.transform(true, range, {});
	problem.second.second = problem.first.second * problem.second.second;
}

void ClashGenerator::moveToBounds(TransformTriangles& problem, const repo::lib::RepoBounds& bounds)
{
	auto& a = problem.first.first;
	auto& ma = problem.first.second;
	auto& b = problem.second.first;
	auto& mb = problem.second.second;
	auto pb = repo::lib::RepoBounds({ ma * a.a, ma * a.b, ma * a.c, mb * b.a, mb * b.b, mb * b.c });
	auto offset = bounds.center() - pb.center();
	ma = repo::lib::RepoMatrix::translate(offset) * ma;
	mb = repo::lib::RepoMatrix::translate(offset) * mb;
}

void ClashGenerator::moveToBounds(TransformLines& problem, const repo::lib::RepoBounds& bounds)
{
	auto& a = problem.first.first;
	auto& ma = problem.first.second;
	auto& b = problem.second.first;
	auto& mb = problem.second.second;
	auto pb = repo::lib::RepoBounds({
		ma * a.start,
		ma * a.end,
		mb * b.start,
		mb * b.end
	});
	auto offset = bounds.center() - pb.center();
	ma = repo::lib::RepoMatrix::translate(offset) * ma;
	mb = repo::lib::RepoMatrix::translate(offset) * mb;
}

TrianglePair ClashGenerator::applyTransforms(TransformTriangles& problem)
{
	return {
		problem.first.second * problem.first.first,
		problem.second.second * problem.second.first
	};
}

TransformLines ClashGenerator::createLinesTransformed(
	const repo::lib::RepoBounds& bounds
)
{
	// This method works by creating two randomly transformed lines, then
	// coming up with a translation that moves them together by the required
	// distance, and appending that to the otherwise random transforms.

	repo::lib::RepoLine a(random.vector(size1 * 0.5), random.vector(size1 * 0.5));
	repo::lib::RepoLine b(random.vector(size2 * 0.5), random.vector(size2 * 0.5));

	// We must be very careful with scales; if two vectors are some distance
	// apart under a scaling transform, if those vectors are downcasted, then
	// the rounding error will be amplified by the scale.

	auto ma = random.transform(true, {}, {});
	auto mb = random.transform(true, {}, {});

	auto v1 = ma * a.start;
	auto v2 = ma * a.end;
	auto v3 = mb * b.start;
	auto v4 = mb * b.end;

	// Pick a random point on each line and then derive a translation so that
	// the lines become coincident at those points.

	auto p1 = v1 + (v2 - v1) * random.scalar();
	auto p2 = v3 + (v4 - v3) * random.scalar();

	auto d = p2 - p1;

	// The movement for lines 1 and 2 required for p1 and p2 to meet.

	auto d1 = d * 0.5;
	auto d2 = -d * 0.5;

	// The meeting point (in world coordinates) - the same for both lines.

	auto mp = p1 + d1;

	// The offset that is applied to push the lines *away* again after applying
	// d1/d2, so that they are exactly 'distance' apart.

	auto distance = random.number(this->distance);

	auto dir = (v2 + d1 - mp).crossProduct(v4 + d2 - mp);
	dir.normalize();
	dir = dir * distance;

	// Get the bounds of the transformed problem, and move them so they are centered
	// within the given bounds.

	ma = repo::lib::RepoMatrix::translate(d1 + dir * 0.5) * ma;
	mb = repo::lib::RepoMatrix::translate(d2 - dir * 0.5) * mb;

	TransformLines problem({ a, ma }, { b, mb });

	moveToBounds(problem, bounds);

	if (downcastVertices) {
		downcast(problem);
	}

	return problem;
}

// The triangle generation methods follow a similar pattern. Triangles are
// created in a known configuration using the ZY plane as a separator. Transforms
// that do not effect the distance of the primitives are then applied to introduce
// additional variation. Finally the whole problem as a unit is offset from the
// origin.

TransformTriangles testing::ClashGenerator::createTrianglesVV(const repo::lib::RepoBounds& bounds)
{
	// Create two triangles which meet at the origin at vertex zero.

	auto d = random.number(distance);

	// One triangle should have its other vertices along -x (by at least d), and
	// the other along +x.

	auto margin = d + d * std::min(size1.min(), size2.min());

	repo::lib::RepoTriangle a(
		repo::lib::RepoVector3D64(0, 0, 0),
		random.vector({ -margin, -size1.max() }, { -size1.max(), size1.max() }, { -size1.max(), size1.max() }),
		random.vector({ -margin, -size1.max() }, { -size1.max(), size1.max() }, { -size1.max(), size1.max() })
	);

	repo::lib::RepoTriangle b(
		repo::lib::RepoVector3D64(0, 0, 0),
		random.vector({ margin, size2.max() }, { -size2.max(), size2.max() }, { -size2.max(), size2.max() }),
		random.vector({ margin, size2.max() }, { -size2.max(), size2.max() }, { -size2.max(), size2.max() })
	);

	// Separate the triangles by d

	b += repo::lib::RepoVector3D64(d, 0, 0);

	TransformTriangles problem({ a, RepoMatrix() }, {b, RepoMatrix() });

	moveB(problem, size2);
	moveProblem(problem, size2);
	moveToBounds(problem, bounds);
	if (downcastVertices) {
		downcast(problem);
	}

	return problem;
}

TransformTriangles testing::ClashGenerator::createTrianglesVE(const repo::lib::RepoBounds& bounds)
{
	// Two triangles, one with an edge coplanar in ZY and overlapping the origin.
	// The other, with one vertex at the origin. All other vertices separated from
	// the XY plane by a safe margin around 'distance'.

	auto d = random.number(distance);

	auto margin = d + 0.1;

	auto edgeDirection = random.direction();
	edgeDirection.x = 0;
	edgeDirection.normalize();

	repo::lib::RepoTriangle a(
		edgeDirection * random.number({ margin, size1.max() }),
		edgeDirection * random.number({ -margin, -size1.max() }),
		random.vector({ -margin, -size1.max() }, size1, size1)
	);

	repo::lib::RepoTriangle b(
		repo::lib::RepoVector3D64(0, 0, 0),
		random.vector({ margin, size2.max() }, size2, size2),
		random.vector({ margin, size2.max() }, size2, size2)
	);

	// Separate the triangles by d

	b += repo::lib::RepoVector3D64(d, 0, 0);

	auto mb = random.transform(true, size2, {});
	b = mb.invert() * b;

	auto ma = random.transform(true, size2, {});
	mb = ma * mb;

	TransformTriangles problem({ a, ma }, { b, mb });

	moveToBounds(problem, bounds);

	if (downcastVertices) {
		downcast(problem);
	}

	return problem;
}

TransformTriangles testing::ClashGenerator::createTrianglesEE(const repo::lib::RepoBounds& bounds)
{
	throw std::exception("Not implemented yet");
}

TransformTriangles testing::ClashGenerator::createTrianglesFE(const repo::lib::RepoBounds& bounds)
{
	throw std::exception("Not implemented yet");
}

TransformTriangles testing::ClashGenerator::createTrianglesFV(const repo::lib::RepoBounds& bounds)
{
	throw std::exception("Not implemented yet");
}


CellDistribution::CellDistribution(size_t cellSize, size_t spaceSize)
	:cellSize(cellSize)
{
	cellsPerAxis = (spaceSize / cellSize) * 2;
	totalCells = cellsPerAxis * cellsPerAxis * cellsPerAxis;
	start = repo::lib::RepoVector3D64(-(double)spaceSize, -(double)spaceSize, -(double)spaceSize);
}

repo::lib::RepoBounds CellDistribution::getBounds(size_t cell) const
{
	auto z = cell / (cellsPerAxis * cellsPerAxis);
	auto y = (cell / cellsPerAxis) % cellsPerAxis;
	auto x = cell % cellsPerAxis;
	return repo::lib::RepoBounds(
		repo::lib::RepoVector3D64(
			x * cellSize,
			y * cellSize,
			z * cellSize
		) + start,
		repo::lib::RepoVector3D64(
			(x + 1) * cellSize,
			(y + 1) * cellSize,
			(z + 1) * cellSize
		) + start
	);
}

repo::lib::RepoBounds CellDistribution::sample()
{
	while(true) {
		auto cell = random.range(0, totalCells);
		if (used.find(cell) == used.end()) {
			used.insert(cell);
			return getBounds(cell);
		}
	}
}