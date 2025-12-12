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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include <repo/manipulator/modelutility/clashdetection/geometry_tests.h>

#include <repo/manipulator/modelutility/clashdetection/predicates.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_clash_utils.h"
#include "../../../../repo_test_mesh_utils.h"
#include "../../../../repo_test_scene_utils.h"
#include "../../../../repo_test_matchers.h"
#include "../../../../repo_test_mock_clash_scene.h"
#include "../../../../repo_test_random_generator.h"

/*
* The tests in this file evaluate the members of the geometry namespace. This
* is currently a dependency of the clash detection engine. 
* 
* The tests not only evaluate if the geometric tests are correct, but their
* robustness to rounding error, within the supported range of the clash engine.
* This is done probabilistically.
*/

using namespace testing;

TEST(Geometry, LineLineDistanceUnit)
{
	// Tests the accuracy of the line-line distance test across the supported
	// domain.
	// Remark: the engine doesn't yet support true line-line clashes, so this test
	// is really to make sure the geometry library does not degenerate until then.

	CellDistribution space;
	ClashGenerator clashGenerator;

	std::vector<double> distances = { 0, 1, 2 };
	for (auto d : distances) {
		clashGenerator.distance = d;
		for (int i = 0; i < 100000; ++i) {
			auto p = clashGenerator.createLinesTransformed(space.sample());

			repo::lib::RepoLine a(
				p.a.m * p.a.e.start,
				p.a.m * p.a.e.end
			);

			repo::lib::RepoLine b(
				p.b.m * p.b.e.start,
				p.b.m * p.b.e.end
			);

			auto line = geometry::closestPoints(a, b);
			auto m = line.magnitude();
			auto e = abs(m - d);

			// 0.5 on either side of d, for a total permissible error of 1
			EXPECT_THAT(e, Lt(0.5));
		}
	}
}

TEST(Geometry, TrianglesUnitVV)
{
	// Tests that the triangle distance tests will correctly identify triangles
	// which are closest at their vertices.

	// Note that the space sizes are double the expected maximums for a single
	// mesh - as two meshes of those sizes may sit side-by-side.

	CellDistribution space;
	ClashGenerator clashGenerator;

	std::vector<double> distances = { 0, 1, 2 };
	for (auto d : distances) {
		clashGenerator.distance = d;

		for (int i = 0; i < 1000000; ++i) {

			auto p = clashGenerator.createTrianglesVV(space.sample());

			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto line = geometry::closestPoints(a, b);
			auto m = line.magnitude();
			auto e = abs(m - d);

			EXPECT_THAT(e, Le(1.0));

			// For VV, the line should connect two vertices (or very close, providing for
			// rounding error).

			EXPECT_THAT(vectors::Iterator(line), vectors::AreSubsetOf({ a.a, a.b, a.c, b.a, b.b, b.c }, ClashGenerator::suggestTolerance({ a, b })));
		}
	}
}

TEST(Geometry, TrianglesUnitVE)
{
	// Tests that the triangle distance tests correctly identify triangles
	// which are closest at an edge and a vertex.

	CellDistribution space;
	ClashGenerator clashGenerator;

	std::vector<double> distances = { 0, 1, 2 };
	for (auto d : distances) {
		clashGenerator.distance = d;

		for (int i = 0; i < 1000000; ++i) {

			auto p = clashGenerator.createTrianglesVE(space.sample());

			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto line = geometry::closestPoints(a, b);
			auto m = line.magnitude();
			auto e = abs(m - d);

			EXPECT_THAT(e, Le(1.0));

			// For VE, the line should connect a vertex to an edge, so exactly
			// one of the points should be coincident with a vertex.

			// (Note that if the the distance is small enough, both points will
			// match to the vertex, so be robust to that case too.)

			EXPECT_THAT(vectors::Iterator(line), vectors::Intersects({ a.a, a.b, a.c, b.a, b.b, b.c }, { 1, 2 }, ClashGenerator::suggestTolerance({ a, b })));
		}
	}
}

TEST(Geometry, TrianglesUnitEE)
{
	// Tests that the triangle distance tests correctly identify triangles
	// which are closest at an edge and a vertex.

	CellDistribution space;
	ClashGenerator clashGenerator;

	std::vector<double> distances = { 0, 1, 2 };
	for (auto d : distances) {
		clashGenerator.distance = d;

		for (int i = 0; i < 100000; ++i) {

			auto p = clashGenerator.createTrianglesEE(space.sample());

			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto line = geometry::closestPoints(a, b);
			auto m = line.magnitude();
			auto e = abs(m - d);

			EXPECT_THAT(e, Le(1.0));
		}
	}
}

TEST(Geometry, TrianglesUnitVF)
{
	// Tests that the triangle distance tests correctly identify triangles
	// which are closest at an edge and a vertex.

	CellDistribution space;
	ClashGenerator clashGenerator;

	std::vector<double> distances = { 0, 1, 2 };
	for (auto d : distances) {
		clashGenerator.distance = d;

		for (int i = 0; i < 100000; ++i) {

			auto p = clashGenerator.createTrianglesVF(space.sample());

			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto line = geometry::closestPoints(a, b);
			auto m = line.magnitude();
			auto e = abs(m - d);

			EXPECT_THAT(e, Lt(1.0));
		}
	}
}

TEST(Geometry, TrianglesUnitFE)
{
	// Tests that the triangle distance tests correctly identify triangles
	// where the edge of one penetrates the face of another.
	// Note that the distance in this case should always be zero - if a
	// there is no intersection, then the closest points will be on an edge
	// or a vertex, covered by the tests above.

	CellDistribution space;
	ClashGenerator clashGenerator;

	for (int i = 0; i < 100000; ++i) {

		auto p = clashGenerator.createTrianglesFE(space.sample());

		auto [a, b] = ClashGenerator::applyTransforms(p);

		auto line = geometry::closestPoints(a, b);
		auto m = line.magnitude();

		EXPECT_THAT(m, Lt(1.0));
	}
}

TEST(Geometry, BoundsSeparatingAxis)
{
	auto r = repo::lib::RepoBounds({
		repo::lib::RepoVector3D64(-0.5, -0.5, -0.5),
		repo::lib::RepoVector3D64(0.5, 0.5, 0.5)
		});

	// Perfectly overlapping uniform bounds should move by their extents; it doesn't matter
	// in which direction.

	auto a = r;
	auto b = r;
	auto v = geometry::minimumSeparatingAxis(a, b);
	EXPECT_THAT(v.norm(), DoubleEq(1));

	// Perfectly overlapping non-uniform bounds should move by the minimum extent.

	a = repo::lib::RepoMatrix::scale(repo::lib::RepoVector3D64(2, 1, 2)) * r;
	b = repo::lib::RepoMatrix::scale(repo::lib::RepoVector3D64(2, 1, 2)) * r;
	v = geometry::minimumSeparatingAxis(a, b);
	EXPECT_THAT(v.norm(), DoubleEq(1));
	EXPECT_THAT(v.y, DoubleEq(1));

	// Where a bounds encapsulates another with all extents being equal

	a = repo::lib::RepoMatrix::scale(repo::lib::RepoVector3D64(2, 2, 2)) * r;
	b = r;
	v = geometry::minimumSeparatingAxis(a, b);
	EXPECT_THAT(v.norm(), DoubleEq(1.5));

	// Encapsulated bounds, but with an offset there is a clear preferred
	// motion.

	a = repo::lib::RepoMatrix::scale(repo::lib::RepoVector3D64(2, 2, 2)) * r;
	b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(0.1, 0, 0)) * r;
	v = geometry::minimumSeparatingAxis(a, b);
	EXPECT_THAT(v.norm(), DoubleEq(1.4));
	EXPECT_THAT(v.x, DoubleEq(-1.4));

	// The vector resovles the collision when applied to a, so the order of
	// operators matters here.

	v = geometry::minimumSeparatingAxis(b, a);
	EXPECT_THAT(v.norm(), DoubleEq(1.4));
	EXPECT_THAT(v.x, DoubleEq(1.4));

	// Overlapping

	a = r;
	b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(0.9, 0, 0)) * r;
	v = geometry::minimumSeparatingAxis(a, b);
	EXPECT_THAT(v.norm(), DoubleEq(0.1));
	EXPECT_THAT(v.x, DoubleEq(-0.1));

	v = geometry::minimumSeparatingAxis(b, a);
	EXPECT_THAT(v.norm(), DoubleEq(0.1));
	EXPECT_THAT(v.x, DoubleEq(0.1));

	// No overlap - already separated by all axes

	a = r;
	b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(2, 2, 2)) * r;

	v = geometry::minimumSeparatingAxis(a, b);
	EXPECT_THAT(v.norm(), DoubleEq(0));
	v = geometry::minimumSeparatingAxis(b, a);
	EXPECT_THAT(v.norm(), DoubleEq(0));

	a = r;
	b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(-2, -2, -2)) * r;

	v = geometry::minimumSeparatingAxis(a, b);
	EXPECT_THAT(v.norm(), DoubleEq(0));
	v = geometry::minimumSeparatingAxis(b, a);
	EXPECT_THAT(v.norm(), DoubleEq(0));

	// No overlap - separated by one axis

	a = r;
	b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(1.1, 0, 0)) * r;

	v = geometry::minimumSeparatingAxis(a, b);
	EXPECT_THAT(v.norm(), DoubleEq(0));
	v = geometry::minimumSeparatingAxis(b, a);
	EXPECT_THAT(v.norm(), DoubleEq(0));

	a = r;
	b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(-1.1, 0, 0)) * r;

	v = geometry::minimumSeparatingAxis(a, b);
	EXPECT_THAT(v.norm(), DoubleEq(0));
	v = geometry::minimumSeparatingAxis(b, a);
	EXPECT_THAT(v.norm(), DoubleEq(0));
}

TEST(Geometry, Orient3D)
{
	// Tests the orient predicate from the geometry namespace with a range of values

	RepoRandomGenerator random;

	// This tests considers triangles that are both very large and very small
	// relative to the distance of d from from the plane.

	// It is important to consider both cases because an infintesimal distance
	// has the easy geometric interpretation that it is is coplanar, but the
	// opposite is not true for a very small triangle.

	// We vary the size of the triangle from explicitly small to explicitly large,
	// while holding d relatively constant to test both cases.

	for (double power = -14; power < 14; power++) {
		for (size_t i = 0; i < 500000; i++) {

			// The geometric interpretation of the orient predicate is that the sign
			// determines which side of the plane formed by points a, b and c point d
			// lies on.
			// We create a, b and c such that they form a right-handed system, then
			// move d by the normal, to form the test case where d is always on a
			// a known side.

			repo::lib::RepoRange r1{ std::pow(10.0, power), std::pow(10.0, power) };

			auto a = random.vector(r1);
			auto b = random.direction();
			auto n = random.direction();
			auto c = repo::lib::RepoMatrix::rotation(n, random.number({ 1.57, 1.57 })) * b; // Rotation matrices are counterclockwise for positive angles

			b = b * random.number(r1) + a;
			c = c * random.number(r1) + a;

			auto d = n + a;

			//auto r = geometry::orient(a, b, c, d);
			predicates::exactinit();
			auto r = predicates::orient3d((double*)&a, (double*)&b, (double*)&c, (double*)&d);
			EXPECT_THAT(r <= 0, IsTrue());

			r = predicates::orient3d((double*)&c, (double*)&b, (double*)&a, (double*)&d);
			EXPECT_THAT(r >= 0, IsTrue());
		}
	}
}

TEST(Geometry, TriangleTriangleIntersects1)
{
	CellDistribution space;
	ClashGenerator clashGenerator;

	clashGenerator.size1 = { 10, 10 };
	clashGenerator.size2 = { 10, 10 };

	for (int i = 0; i < 500000; ++i) {

		clashGenerator.distance = { -1, -1 };

		{
			auto p = clashGenerator.createTrianglesFF({});
			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto d = geometry::intersects(a, b);

			EXPECT_THAT(d, IsTrue());
		}

		clashGenerator.distance = { 1, 1 };

		{
			auto p = clashGenerator.createTrianglesTransformed({});
			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto d = geometry::intersects(a, b);

			EXPECT_THAT(d, IsFalse());
		}
	}

	clashGenerator.distance = { -1, -1 };

	auto shift = [](repo::lib::RepoTriangle& b) {
		auto t = b.a;
		b.a = b.b;
		b.b = b.c;
		b.c = t;
	};

	// intersects method should be invariant to all permutations of the
	// triangle vertices, including winding order, as well as the order
	// of the triangles themselves.

	for (int i = 0; i < 10000; ++i) {

		auto p = clashGenerator.createTrianglesFF({});
		auto [a, b] = ClashGenerator::applyTransforms(p);

		auto d = geometry::intersects(a, b);

		EXPECT_THAT(d, DoubleNear(geometry::intersects(b, a), 1e-8));

		shift(a);

		EXPECT_THAT(d, DoubleNear(geometry::intersects(a, b), 1e-8));
		EXPECT_THAT(d, DoubleNear(geometry::intersects(b, a), 1e-8));

		shift(b);

		EXPECT_THAT(d, DoubleNear(geometry::intersects(a, b), 1e-8));
		EXPECT_THAT(d, DoubleNear(geometry::intersects(b, a), 1e-8));

		shift(a);

		EXPECT_THAT(d, DoubleNear(geometry::intersects(a, b), 1e-8));
		EXPECT_THAT(d, DoubleNear(geometry::intersects(b, a), 1e-8));

		shift(b);

		EXPECT_THAT(d, DoubleNear(geometry::intersects(a, b), 1e-8));
		EXPECT_THAT(d, DoubleNear(geometry::intersects(b, a), 1e-8));

		std::swap(a.a, a.b);

		EXPECT_THAT(d, DoubleNear(geometry::intersects(a, b), 1e-8));
		EXPECT_THAT(d, DoubleNear(geometry::intersects(b, a), 1e-8));

		std::swap(a.b, a.c);

		EXPECT_THAT(d, DoubleNear(geometry::intersects(a, b), 1e-8));
		EXPECT_THAT(d, DoubleNear(geometry::intersects(b, a), 1e-8));

		std::swap(b.a, b.b);

		EXPECT_THAT(d, DoubleNear(geometry::intersects(a, b), 1e-8));
		EXPECT_THAT(d, DoubleNear(geometry::intersects(b, a), 1e-8));

		std::swap(b.b, b.c);

		EXPECT_THAT(d, DoubleNear(geometry::intersects(a, b), 1e-8));
		EXPECT_THAT(d, DoubleNear(geometry::intersects(b, a), 1e-8));
	}
}

TEST(Geometry, TriangleTriangleIntersects2)
{
	CellDistribution space;
	ClashGenerator clashGenerator;

	// The intersects method for triangles contractually returns an upper bound
	// on the distance.

	// This method is a white-box test that relies on the implementation detail
	// to know what the distances should be for specific configurations, and is
	// mainly used for regression testing of optimisations.

	// If the implementation of intersects changes, this test may need its
	// acceptance criteria updated as well.

	auto& random = clashGenerator.random;
	clashGenerator.downcastVertices = false;

	clashGenerator.distance = { 0.01, 100 };

	clashGenerator.size1 = { 0.0001, 10000 };
	clashGenerator.size2 = { 0.0001, 10000 };

	for (int itr = 0; itr < 1000000; ++itr) 
	{
		{
			// i < k < j < l

			repo::lib::RepoRange r = clashGenerator.distance;

			auto k = random.number(r);
			auto j = random.number(r + k);
			auto l = random.number(r + j);

			auto dp1 = random.number(clashGenerator.distance);
			auto dq1 = random.number(clashGenerator.distance);
			auto dr1 = random.number(clashGenerator.distance);

			auto dp2 = random.number(clashGenerator.distance);
			auto dq2 = random.number(clashGenerator.distance);
			auto dr2 = random.number(clashGenerator.distance);

			auto p = clashGenerator.createTrianglesDevillersGuigue({}, 0, k, j, l, dp1, dp2, dq1, dq2, dr1, dr2);
			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto d = geometry::intersects(a, b);

			auto expected = std::min({ j - k, l - 0, dp1, dp2, std::max(dq1, dr1), std::max(dq2, dr2) });

			EXPECT_THAT(d, DoubleNear(expected, ClashGenerator::suggestTolerance({ a, b })));
		}		
		
		{
			// k < i < j < l

			repo::lib::RepoRange r = clashGenerator.distance;

			auto k = random.number(r);
			auto i = random.number(r + k);
			auto j = random.number(r + i);
			auto l = random.number(r + j);

			auto dp1 = random.number(clashGenerator.distance);
			auto dq1 = random.number(clashGenerator.distance);
			auto dr1 = random.number(clashGenerator.distance);

			auto dp2 = random.number(clashGenerator.distance);
			auto dq2 = random.number(clashGenerator.distance);
			auto dr2 = random.number(clashGenerator.distance);

			auto p = clashGenerator.createTrianglesDevillersGuigue({}, i, k, j, l, dp1, dp2, dq1, dq2, dr1, dr2);
			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto d = geometry::intersects(a, b);

			auto expected = std::min({ j - k, l - i, dp1, dp2, std::max(dq1, dr1), std::max(dq2, dr2) });

			EXPECT_THAT(d, DoubleNear(expected, ClashGenerator::suggestTolerance({ a, b })));
		}

		{
			// i < k < l < j

			repo::lib::RepoRange r = clashGenerator.distance;

			auto i = random.number(r);
			auto k = random.number(r + i);
			auto l = random.number(r + k);
			auto j = random.number(r + l);

			auto dp1 = random.number(clashGenerator.distance);
			auto dq1 = random.number(clashGenerator.distance);
			auto dr1 = random.number(clashGenerator.distance);

			auto dp2 = random.number(clashGenerator.distance);
			auto dq2 = random.number(clashGenerator.distance);
			auto dr2 = random.number(clashGenerator.distance);

			auto p = clashGenerator.createTrianglesDevillersGuigue({}, i, k, j, l, dp1, dp2, dq1, dq2, dr1, dr2);
			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto d = geometry::intersects(a, b);

			auto expected = std::min({ j - k, l - i, dp1, dp2, std::max(dq1, dr1), std::max(dq2, dr2) });

			EXPECT_THAT(d, DoubleNear(expected, ClashGenerator::suggestTolerance({ a, b })));
		}
	}
}

TEST(Geometry, ContactThreshold)
{
	CellDistribution space;
	ClashGenerator clashGenerator;

	clashGenerator.downcastVertices = false;

	for (int itr = 0; itr < 10000000; ++itr) {

		auto r1 = clashGenerator.random.number({ 1, 7 });
		auto r2 = clashGenerator.random.number({ 1, 7 });

		clashGenerator.size1 = { std::pow(10, r1), std::pow(10, r1 + 1) };
		clashGenerator.size2 = { std::pow(10, r2), std::pow(10, r2 + 1) };

		clashGenerator.distance = { 1, 2 };

		auto p = clashGenerator.createTrianglesTransformed({});
		auto [a, b] = ClashGenerator::applyTransforms(p);

		// The purpose of the threshold is to participate in the control flow
		// of algorithms, so we test it in that context: triangles are positioned
		// into what should be the closest we can get to an in-contact state.

		auto th = geometry::contactThreshold(a, b);

		auto line = geometry::closestPoints(a, b);
		a = a + (line.end - line.start);

		// The threshold is used to turn the intersects measure into a binary decision
		// between in-contact and in-collision. As algorithm logic relies on this being
		// robust, it should never fail.

		auto i = geometry::intersects(a, b);
		EXPECT_THAT(i, Le(th));

		// Transforming a by d should produce intersecting triangles within the range.

		auto d = geometry::closestPoints(a, b).magnitude();
		EXPECT_THAT(d, Le(th));

		// The threshold should be suitably small to avoid false positives, though
		// what is positive is rather subjective.
		// We test below that across the largest input range, the threshold is always
		// within 1 mm (assuming units are mm), as this is consistent with our
		// accuracy guarantees.

		EXPECT_THAT(th, Lt(1.0));
	}
}

TEST(Geometry, ClosestPointBoundsBounds)
{
	const repo::lib::RepoBounds _a(
		repo::lib::RepoVector3D64(-1, -1, -1),
		repo::lib::RepoVector3D64(1, 1, 1)
	);

	{
		// Identical bounds

		auto a = _a;
		auto b = a;

		auto line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(0));
		EXPECT_THAT(a.contains(line.start), IsTrue());
	}

	{
		// Overlapping

		auto a = _a;
		auto b = repo::lib::RepoMatrix::scale(2.0f) * a;

		auto line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(0));
		EXPECT_THAT(a.contains(line.start), IsTrue());

		std::swap(a, b);

		line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(0));
		EXPECT_THAT(a.contains(line.start), IsTrue());
	}

	{
		// Partial overlap

		auto a = _a;
		auto b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(1, 1, 1)) * a;

		auto line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(0));
		EXPECT_THAT(a.contains(line.start), IsTrue());

		std::swap(a, b);

		line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(0));
		EXPECT_THAT(a.contains(line.start), IsTrue());
	}

	{
		// Separated (all axes)
		auto a = _a;
		auto b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(3, 3, 3)) * a;

		auto line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(std::sqrt(3.0)));
		EXPECT_THAT(a.contains(line.start), IsTrue());
		EXPECT_THAT(b.contains(line.end), IsTrue());

		std::swap(a, b);

		line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(std::sqrt(3.0)));
		EXPECT_THAT(a.contains(line.start), IsTrue());
		EXPECT_THAT(b.contains(line.end), IsTrue());
	}

	{
		// Separated (one axis)

		auto a = _a;
		auto b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(5, 0, 0)) * a;

		auto line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(3.0));
		EXPECT_THAT(a.contains(line.start), IsTrue());
		EXPECT_THAT(b.contains(line.end), IsTrue());

		std::swap(a, b);

		line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(3.0));
		EXPECT_THAT(a.contains(line.start), IsTrue());
		EXPECT_THAT(b.contains(line.end), IsTrue());
	}

	{
		// Separated (one axis)

		auto a = _a;
		auto b = repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(0, 5, 0)) * a;

		auto line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(3.0));
		EXPECT_THAT(a.contains(line.start), IsTrue());
		EXPECT_THAT(b.contains(line.end), IsTrue());

		std::swap(a, b);

		line = geometry::closestPoints(a, b);
		EXPECT_THAT(line.magnitude(), DoubleEq(3.0));
		EXPECT_THAT(a.contains(line.start), IsTrue());
		EXPECT_THAT(b.contains(line.end), IsTrue());
	}
}

TEST(Geometry, TimeOfContactBounds)
{
	const repo::lib::RepoBounds a(
		repo::lib::RepoVector3D64(-0.5, -0.5, -0.5),
		repo::lib::RepoVector3D64(0.5, 0.5, 0.5)
	);

	auto b = a;

	{
		// Bounds are already in-contact
		auto tau = geometry::timeOfContact(
			a,
			b,
			repo::lib::RepoVector3D64(1,0,0)
		);
		EXPECT_THAT(tau, DoubleEq(0.0));
	}

	{
		// Bounds will not come into contact because v stops short
		auto tau = geometry::timeOfContact(
			repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(-5, 0, 0)) * a,
			b,
			repo::lib::RepoVector3D64(3, 0, 0)
		);
		EXPECT_THAT(tau, Gt(1.0));
	}

	{
		// Bounds will come into contact at the end of d
		auto tau = geometry::timeOfContact(
			repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(-5, 0, 0)) * a,
			b,
			repo::lib::RepoVector3D64(4, 0, 0)
		);
		EXPECT_THAT(tau, DoubleNear(1.0, FLT_EPSILON));
	}

	{
		// Bounds will come into contact halfway along v
		auto tau = geometry::timeOfContact(
			repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(-5, 0, 0)) * a,
			b,
			repo::lib::RepoVector3D64(8, 0, 0)
		);
		EXPECT_THAT(tau, DoubleNear(0.5, FLT_EPSILON));
	}

	{
		// Bounds will not come into contact because they will be missed
		auto tau = geometry::timeOfContact(
			repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(-5, 2, 0)) * a,
			b,
			repo::lib::RepoVector3D64(5, 0, 0)
		);
		EXPECT_THAT(tau, DoubleEq(std::numeric_limits<double>::infinity()));
	}

	{
		// Bounds will not come into contact because b starts behind a
		auto tau = geometry::timeOfContact(
			repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(5, 2, 0)) * a,
			b,
			repo::lib::RepoVector3D64(1, 0, 0)
		);
		EXPECT_THAT(tau, DoubleEq(std::numeric_limits<double>::infinity()));
	}
}

TEST(Geometry, TimeOfContactTriangles)
{
	CellDistribution space;
	ClashGenerator clashGenerator;

	// Tests the triangle-triangle toc using a variety of features

	for (int itr = 0; itr < 1000000; itr++) {

		// Triangles should come into contact at the closest feature, because we
		// are moving by d.

		auto p = clashGenerator.createTrianglesTransformed(space.sample());
		auto [a, b] = ClashGenerator::applyTransforms(p);

		auto L = geometry::closestPoints(a, b);
		auto l = L.end - L.start;
		auto dn = l.norm();
		auto contact = geometry::contactThreshold(a, b); // This should be identical to the internally calculated threshold
		auto th = contact / dn;

		// Triangles will come into contact exactly at the end of d
		auto tau = geometry::timeOfContact(
			a,
			b,
			l
		);
		// We use this condition, rather than comparing tau to a proportion of dn, 
		// because when dn is large we can get relatively large rounding errors in
		// the division of the small threshold.
		EXPECT_THAT(geometry::closestPoints(a + l * tau, b).magnitude(), Lt(contact));

		// Triangles will not come into contact because l is short

		auto d = clashGenerator.random.number({ 0.0,  1 - th - 0.0001 }); // we expect the algorithm to be sensitive enough to distinguish between 0.999 and 1 in all cases
		tau = geometry::timeOfContact(
			a,
			b,
			l * d
		);
		EXPECT_THAT(tau, Gt(1));

		// Triangles will come into contact along d

		tau = geometry::timeOfContact(
			a,
			b,
			l * (1.0/d)
		);
		EXPECT_THAT(tau, DoubleNear(d, th));

		// Time of contact can underestimate by a tiny amount, but should never
		// overestimate to the point of interpenetration.

		auto _a = repo::lib::RepoMatrix::translate(l * (1.0 / d) * tau) * a;
		auto line = geometry::closestPoints(_a, b);
		EXPECT_THAT(line.intersects, IsFalse());

		// Triangles will not come into contact because they will miss eachother

		auto k = l.crossProduct(b.normal()) * 10;

		tau = geometry::timeOfContact(
			a,
			b,
			k
		);
		EXPECT_THAT(tau, Gt(1));
	}
}

TEST(Geometry, ClosedAndManifold)
{ 
	{
		auto m = repo::test::utils::mesh::makeUnitCube();
		EXPECT_THAT(geometry::isClosedAndManifold(m.getFaces()), IsTrue());
	}

	{
		auto m = repo::test::utils::mesh::makeUnitCylinder(100, true);
		EXPECT_THAT(geometry::isClosedAndManifold(m.getFaces()), IsTrue());
	}

	{
		auto m = repo::test::utils::mesh::makeUnitCylinder(100, true);
		auto faces = m.getFaces();
		std::swap(faces[10][0], faces[10][2]);
		m.setFaces(faces);
		EXPECT_THAT(geometry::isClosedAndManifold(m.getFaces()), IsFalse());
	}
}