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

Lines ClashGenerator::createLines(
	double length,
	double distance,
	double offset
)
{
	auto start = random.vector(offset);

	auto d1 = random.direction() * length;
	auto d2 = random.direction() * length;

	auto d3 = d1.crossProduct(d2);
	d3.normalize();

	repo::lib::RepoLine a(
		start + d1 * random.scalar(),
		start - d1 * random.scalar()
	);

	repo::lib::RepoLine b(
		start + d2 * random.scalar() + d3 * distance,
		start - d2 * random.scalar() + d3 * distance
	);

	return { a, b };
}

// Creates a pair of lines that are separated by the given distance exactly when
// transformed by their respective matrices. The magnitude of the offset of the
// matrix will be on the order of offset.

TransformLines ClashGenerator::createLinesTransformed(
	double length,
	double distance,
	double offset
)
{
	// This method works by creating two randomly transformed lines, then
	// coming up with a translation that moves them together by the required
	// distance, and appending that to the otherwise random transforms.

	repo::lib::RepoLine a(
		random.vector(length * 0.5),
		random.vector(length * 0.5)
	);

	repo::lib::RepoLine b(
		random.vector(length * 0.5),
		random.vector(length * 0.5)
	);

	// We must be very careful with scales; if two vectors are some distance
	// apart under a scaling transform, if those vectors are downcasted, then
	// the rounding error will be amplified by the scale.

	auto ma = random.transform(true, false);
	auto mb = random.transform(true, false);

	auto v1 = ma * a.start;
	auto v2 = ma * a.end;
	auto v3 = mb * b.start;
	auto v4 = mb * b.end;

	// Pick a random point on each line and then derive a translation so that
	// the lines become coincident at those points.

	auto p1 = v1 + (v2 - v1) * random.scalar();
	auto p2 = v3 + (v4 - v3) * random.scalar();

	auto d = p2 - p1;

	auto d1 = d * 0.5;
	auto d2 = -d * 0.5;

	// The meeting point (in world coordinates) - the same for both lines.

	auto mp = p1 + d1;

	auto dir = (v2 + d1 - mp).crossProduct(v4 + d2 - mp);
	dir.normalize();
	dir = dir * distance;

	auto as = random.direction() * (abs(d.norm() - offset));

	ma = repo::lib::RepoMatrix::translate(as + d1 + dir * 0.5) * ma;
	mb = repo::lib::RepoMatrix::translate(as + d2 - dir * 0.5) * mb;

	return { { a, ma }, { b, mb } };
}