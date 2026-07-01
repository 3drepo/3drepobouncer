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


#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include <repo/manipulator/modelutility/clashdetection/bvh_operators.h>
#include <repo/manipulator/modelutility/clashdetection/geometry_tests.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_clash_utils.h"
#include "../../../../repo_test_mesh_utils.h"
#include "../../../../repo_test_scene_utils.h"
#include "../../../../repo_test_matchers.h"
#include "../../../../repo_test_mock_clash_scene.h"
#include "../../../../repo_test_random_generator.h"

using namespace testing;

using Bvh = bvh::Bvh<double>;

namespace {
	Bvh::Node createBounds(const repo::lib::RepoTriangle& t) {
		auto bbox = bvh::BoundingBox<double>::empty();
		bbox.extend(bvh::Vector3<double>(t.a.x, t.a.y, t.a.z));
		bbox.extend(bvh::Vector3<double>(t.b.x, t.b.y, t.b.z));
		bbox.extend(bvh::Vector3<double>(t.c.x, t.c.y, t.c.z));
		return Bvh::Node{
			bbox.min[0], bbox.max[0],
			bbox.min[1], bbox.max[1],
			bbox.min[2], bbox.max[2],
			0, 0
		};
	}
}

TEST(Bvh, Contacts)
{
	CellDistribution space;
	ClashGenerator clashGenerator;

	clashGenerator.downcastVertices = false;

	auto hyperplanes = std::vector<repo::lib::RepoVector3D64>{
		repo::lib::RepoVector3D64(1, 0, 0),
		repo::lib::RepoVector3D64(0, 1, 0),
		repo::lib::RepoVector3D64(0, 0, 1)
	};

	for (int itr = 0; itr < 100000; ++itr) {

		auto r1 = clashGenerator.random.number({1, 7});
		auto r2 = clashGenerator.random.number({1, 7});

		clashGenerator.size1 = {std::pow(10, r1), std::pow(10, r1 + 1)};
		clashGenerator.size2 = {std::pow(10, r2), std::pow(10, r2 + 1)};

		clashGenerator.distance = {1, 2};

		{
			clashGenerator.hyperplane = {};

			auto p = clashGenerator.createTrianglesTransformed({});
			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto d = geometry::closestPoints(a, b);
			auto th = geometry::contactThreshold(a, b);

			// This operation moves the triangles to within the contact threshold.
			// The bounds of the triangles in this state must always be considered
			// in-contact.

			a = a + (d.end - d.start);

			EXPECT_THAT(bvh::predicates::contacts(createBounds(a), createBounds(b)), IsTrue());
		}

		{
			clashGenerator.distance = {0, 1};

			// The hyperplane parameter forces the primitives to be split along a specific
			// direction. If this is axis aligned, the bounds should approach the contact
			// threshold as the triangles do.

			clashGenerator.hyperplane = hyperplanes[clashGenerator.random.number(0, 2)];
			
			// Currently only the VV case uses the hyperplane paramter.

			auto p = clashGenerator.createTrianglesVV(space.sample());
			auto [a, b] = ClashGenerator::applyTransforms(p);

			auto d = geometry::closestPoints(a, b);
			auto th = geometry::contactThreshold(a, b);

			auto m = d.magnitude();

			if (m <= th) {
				EXPECT_THAT(bvh::predicates::contacts(createBounds(a), createBounds(b)), IsTrue());
			}
			
			if (m > th * 2) {
				EXPECT_THAT(bvh::predicates::contacts(createBounds(a), createBounds(b)), IsFalse());
			}

			a = a + (d.end - d.start);

			EXPECT_THAT(bvh::predicates::contacts(createBounds(a), createBounds(b)), IsTrue());
		}
	}

	for (int itr = 0; itr < 100000; ++itr) {

		Bvh::Node a;
		Bvh::Node b;
		{
			auto p = clashGenerator.createTrianglesTransformed(space.sample());
			auto [_a, _b] = ClashGenerator::applyTransforms(p);
			a = createBounds(_a);
		}
		{
			auto p = clashGenerator.createTrianglesTransformed(space.sample());
			auto [_a, _b] = ClashGenerator::applyTransforms(p);
			b = createBounds(_a);
		}

		// As we are using two different sample calls above, there is no way that
		// these bounds should be able to overlap.

		EXPECT_THAT(bvh::predicates::contacts(a, b), IsFalse());

		// Exact overlaps should be considered as partial overlaps (i.e. contacts).

		EXPECT_THAT(bvh::predicates::contacts(a, a), IsTrue());
	}
}