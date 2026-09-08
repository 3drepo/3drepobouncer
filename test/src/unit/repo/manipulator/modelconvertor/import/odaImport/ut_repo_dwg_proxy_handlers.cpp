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
#include <OdaCommon.h>
#include "repo/manipulator/modelconvertor/import/odaHelper/data_processor_dwg.h"
#include "repo/manipulator/modelconvertor/import/odaHelper/dwg_proxy_utils.h"
#include "repo/lib/datastructure/repo_variant_utils.h"

using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace repo::lib;
using namespace testing;

// ProxyInfo::isCivil3DSurfaceClass is pure string logic over originalClass
// (no ODA-entity dependency), so it can be tested directly by constructing a
// ProxyInfo with the class name under test.

TEST(ProxyInfoTest, IsCivil3DSurfaceClassTrueCases)
{
	ProxyInfo info;

	info.originalClass = "AeccDbSurfaceTin";
	EXPECT_THAT(info.isCivil3DSurfaceClass(), IsTrue());

	info.originalClass = "AeccDbTinSurface";
	EXPECT_THAT(info.isCivil3DSurfaceClass(), IsTrue());

	// Substring matches, for future/variant class names.
	info.originalClass = "AeccDbGridSurfaceTin2025";
	EXPECT_THAT(info.isCivil3DSurfaceClass(), IsTrue());

	info.originalClass = "SomeTinSurfaceVariant";
	EXPECT_THAT(info.isCivil3DSurfaceClass(), IsTrue());
}

TEST(ProxyInfoTest, IsCivil3DSurfaceClassFalseCases)
{
	ProxyInfo info;

	// A Civil3D surface that isn't a TIN, and an unrelated Civil3D class.
	info.originalClass = "AeccDbSurface";
	EXPECT_THAT(info.isCivil3DSurfaceClass(), IsFalse());

	info.originalClass = "AeccDbAlignment";
	EXPECT_THAT(info.isCivil3DSurfaceClass(), IsFalse());

	info.originalClass = "";
	EXPECT_THAT(info.isCivil3DSurfaceClass(), IsFalse());

	// Case-sensitive by design.
	info.originalClass = "aeccdbsurfacetin";
	EXPECT_THAT(info.isCivil3DSurfaceClass(), IsFalse());
}

// ProxyInfo::hasEdge/resetEdges are pure position-based dedup logic (no
// ODA-entity dependency), so - like isCivil3DSurfaceClass - they can be
// tested directly without importing a fixture.

TEST(ProxyInfoTest, HasEdgeDedupIsOrderIndependent)
{
	ProxyInfo info;
	RepoVector3D64 a(0, 0, 0), b(1, 0, 0), c(0, 1, 0);

	// First time this edge is seen, in either direction, it should be recorded
	// and hasEdge should report it as new (false).
	EXPECT_THAT(info.hasEdge(a, b), IsFalse());

	// Same edge, same order - already seen.
	EXPECT_THAT(info.hasEdge(a, b), IsTrue());

	// Same edge, reversed order - still the same undirected edge.
	EXPECT_THAT(info.hasEdge(b, a), IsTrue());

	// A genuinely different edge sharing an endpoint is still new.
	EXPECT_THAT(info.hasEdge(b, c), IsFalse());
	EXPECT_THAT(info.hasEdge(c, b), IsTrue());
}

TEST(ProxyInfoTest, ResetEdgesClearsDedupState)
{
	ProxyInfo info;
	RepoVector3D64 a(0, 0, 0), b(1, 0, 0);

	EXPECT_THAT(info.hasEdge(a, b), IsFalse());
	EXPECT_THAT(info.hasEdge(a, b), IsTrue());

	info.resetEdges();

	// After resetting, the same edge is treated as new again.
	EXPECT_THAT(info.hasEdge(a, b), IsFalse());
}
