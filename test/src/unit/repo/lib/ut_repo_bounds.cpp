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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include "../../repo_test_utils.h"
#include "../../repo_test_matchers.h"

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