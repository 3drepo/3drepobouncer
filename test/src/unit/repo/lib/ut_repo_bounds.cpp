/**
*  Copyright (C) 2016 3D Repo Ltd
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

#include <repo/lib/datastructure/repo_bounds.h>
#include <repo/lib/datastructure/repo_matrix.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include "../../repo_test_utils.h"
#include "../../repo_test_matchers.h"
#include "../../repo_test_mesh_utils.h"

using namespace repo::lib;
using namespace testing;

TEST(RepoBoundsTest, Constructor)
{
	// Should start initialised to opposite bounds, so that Encapsulate works out of the box
	RepoBounds bounds;
	bounds.encapsulate(RepoVector3D64());
	EXPECT_THAT(bounds.size(), Eq(RepoVector3D64()));

	// Swapping min and max elements should still result in an OK bounds

	bounds = RepoBounds(RepoVector3D64(4, 5, 6), RepoVector3D64(1, 2, 3));
	EXPECT_THAT(bounds.min(), RepoVector3D64(1, 2, 3));
	EXPECT_THAT(bounds.max(), RepoVector3D64(4, 5, 6));

	bounds = RepoBounds(RepoVector3D64(1, 2, 3), RepoVector3D64(-4, -5, -6));
	EXPECT_THAT(bounds.min(), RepoVector3D64(-4, -5, -6));
	EXPECT_THAT(bounds.max(), RepoVector3D64(1, 2, 3));

	// Swapping per element

	bounds = RepoBounds(RepoVector3D64(-1, 2, -3), RepoVector3D64(4, -5, 6));
	EXPECT_THAT(bounds.min(), RepoVector3D64(-1, -5, -3));
	EXPECT_THAT(bounds.max(), RepoVector3D64(4, 2, 6));

	// Fully negative box
	bounds = RepoBounds(RepoVector3D64(-1, -2, -3), RepoVector3D64(-4, -5, -6));
	EXPECT_THAT(bounds.min(), RepoVector3D64(-4, -5, -6));
	EXPECT_THAT(bounds.max(), RepoVector3D64(-1, -2, -3));

	// Fully positive box
	bounds = RepoBounds(RepoVector3D64(1, 2, 3), RepoVector3D64(4, 5, 6));
	EXPECT_THAT(bounds.min(), RepoVector3D64(1, 2, 3));
	EXPECT_THAT(bounds.max(), RepoVector3D64(4, 5, 6));
}

TEST(RepoBoundsTest, Encapsulate)
{
	RepoBounds bounds;
	bounds.encapsulate(RepoVector3D64());

	EXPECT_THAT(bounds.min(), Eq(RepoVector3D64()));
	EXPECT_THAT(bounds.max(), Eq(RepoVector3D64()));

	bounds.encapsulate(RepoVector3D64(10, 11, 12));

	EXPECT_THAT(bounds.min(), Eq(RepoVector3D64()));
	EXPECT_THAT(bounds.max(), Eq(RepoVector3D64(10, 11, 12)));

	bounds.encapsulate(RepoVector3D64(-13, -14, -15));

	EXPECT_THAT(bounds.min(), Eq(RepoVector3D64(-13, -14, -15)));
	EXPECT_THAT(bounds.max(), Eq(RepoVector3D64(10, 11, 12)));

	bounds.encapsulate(RepoVector3D64(-13, 1000, -1000));

	EXPECT_THAT(bounds.min(), Eq(RepoVector3D64(-13, -14, -1000)));
	EXPECT_THAT(bounds.max(), Eq(RepoVector3D64(10, 1000, 12)));
}

MATCHER_P(AreContainedBy, bounds, "")
{
	for (const auto& v : arg)
	{
		if (!bounds.contains(v))
		{
			*result_listener << "Vertex " << v << " is not contained by bounds: " << bounds.min() << " - " << bounds.max();
			return false;
		}
	}
	return true;
}

// All bounds computations are done in double precision, as per the type. To avoid
// any effects of downcasting during the tests, we perform all transformations
// in double precision as well.

struct DoubleMesh
{
	DoubleMesh(const repo::core::model::MeshNode& mesh)
		: faces(mesh.getFaces())
	{
		for(auto& v : mesh.getVertices())
		{
			vertices.emplace_back(repo::lib::RepoVector3D64(v.x, v.y, v.z));
		}
	}
	std::vector<repo::lib::RepoVector3D64> vertices;
	std::vector<repo::lib::repo_face_t> faces;

	repo::lib::RepoBounds getBoundingBox() const
	{
		repo::lib::RepoBounds bounds;
		for (const auto& v : vertices)
		{
			bounds.encapsulate(v);
		}
		return bounds;
	}

	void applyTransformation(const repo::lib::RepoMatrix64& matrix)
	{
		for (auto& v : vertices)
		{
			v = matrix * v;
		}
	}

	const std::vector<repo::lib::RepoVector3D64>& getVertices() const
	{
		return vertices;
	}
};

TEST(RepoBoundsTest, TransformBounds)
{
	using namespace repo::lib;

	restartRand(0);
	auto m = DoubleMesh(*repo::test::utils::mesh::createRandomMesh(1000, false, 3, {}, {}));

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(1000, -1, 254));
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	{
		auto transform = RepoMatrix64::rotationX(6) * RepoMatrix64::rotationY(11) * RepoMatrix64::rotationZ(27);
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	// Some optimised AABB transforms are not robust to non-uniform scaling. We
	// cannot assume this, so make sure whatever we use is...

	{
		auto transform = RepoMatrix64::scale(RepoVector3D64(0.93, 12.4, 1));
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(1000, -1, 254)) * RepoMatrix64::rotationX(6) * RepoMatrix64::scale(RepoVector3D64(0.93, 12.4, 1));
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(1000, -1, 254)) * RepoMatrix64::rotationX(6) * RepoMatrix64::scale(RepoVector3D64(-2, 2, 2));
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	// When testing the robustness to large offsets, apply a small rotation
	// to ensure the matrix doesn't solely consist of a translation, which is
	// too easy to subtract without any loss.

	auto smallRotation = RepoMatrix64::rotationX(6.1) * RepoMatrix64::rotationY(11.27) * RepoMatrix64::rotationZ(27.45);

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(99000, -12001, 76000)) * smallRotation;
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(990001, -120012, 760003)) * smallRotation;
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(9900012.1, -1200123.2, 7600034.34)) * smallRotation;
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(99000125.56, -12001236.78, 76000347.91)) * smallRotation;
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(99000125.11, -12001236.12, 76000347.23)) * smallRotation;
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(990001256.13, -120012367.34, 760003478.456)) * smallRotation;
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}

	// The maximum value of a double is 10^308 - that is, these values should be
	// trivial. As our aim is to handle BIM data however that is the range of
	// interest - 10^9 is accurate to the millimeter over 1000 km.

	{
		auto transform = RepoMatrix64::translate(RepoVector3D64(9900012567.56, -1200123678.567, 7600034789.987)) * smallRotation;
		auto copy(m);
		copy.applyTransformation(transform);

		auto transformedBounds = transform * m.getBoundingBox();

		EXPECT_THAT(copy.getVertices(), AreContainedBy(transformedBounds));
	}
}