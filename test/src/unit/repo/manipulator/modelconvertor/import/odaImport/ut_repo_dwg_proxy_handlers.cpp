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
#include "repo/manipulator/modelconvertor/import/odaHelper/civil3d_proxy_handler.h"
#include "repo/manipulator/modelconvertor/import/odaHelper/dwg_proxy_inspector.h"
#include "repo/lib/datastructure/repo_variant_utils.h"

using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace repo::lib;
using namespace testing;

// Civil3DProxyHandler::matchesClassName is what routes a DWG proxy entity's
// original class name to Civil3D handling in DwgProxyInspector::classifyApplication.
// It is pure string logic with no ODA-entity dependency, so it can be tested
// directly without constructing a real proxy entity.

TEST(Civil3DProxyHandlerTest, MatchesClassNameTrueCases)
{
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName("AeccDbSurfaceTin"), IsTrue());
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName("AeccDbAlignment"), IsTrue());
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName("SomeCivilThing"), IsTrue());
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName("AeccAndCivilBoth"), IsTrue());

	// The match is a substring search, not a prefix check.
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName("XAeccY"), IsTrue());
}

TEST(Civil3DProxyHandlerTest, MatchesClassNameFalseCases)
{
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName("AcDbLine"), IsFalse());
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName(""), IsFalse());

	// A near-miss that doesn't complete either substring.
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName("Aec"), IsFalse());

	// The match is case-sensitive by design.
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName("aeccdbsurfacetin"), IsFalse());
	EXPECT_THAT(Civil3DProxyHandler::matchesClassName("civil"), IsFalse());
}

TEST(Civil3DProxyHandlerTest, IsTinSurfaceClassTrueCases)
{
	Civil3DProxyHandler handler;

	EXPECT_THAT(handler.isTinSurfaceClass("AeccDbSurfaceTin"), IsTrue());
	EXPECT_THAT(handler.isTinSurfaceClass("AeccDbTinSurface"), IsTrue());

	// Substring matches, for future/variant class names.
	EXPECT_THAT(handler.isTinSurfaceClass("AeccDbGridSurfaceTin2025"), IsTrue());
	EXPECT_THAT(handler.isTinSurfaceClass("SomeTinSurfaceVariant"), IsTrue());
}

TEST(Civil3DProxyHandlerTest, IsTinSurfaceClassFalseCases)
{
	Civil3DProxyHandler handler;

	// A Civil3D surface that isn't a TIN, and an unrelated Civil3D class.
	EXPECT_THAT(handler.isTinSurfaceClass("AeccDbSurface"), IsFalse());
	EXPECT_THAT(handler.isTinSurfaceClass("AeccDbAlignment"), IsFalse());
	EXPECT_THAT(handler.isTinSurfaceClass(""), IsFalse());

	// Case-sensitive by design.
	EXPECT_THAT(handler.isTinSurfaceClass("aeccdbsurfacetin"), IsFalse());
}

TEST(Civil3DProxyHandlerTest, GetDisplayNameKnownClasses)
{
	Civil3DProxyHandler handler;
	std::string outName;

	EXPECT_THAT(handler.getDisplayName("AeccDbSurfaceTin", outName), IsTrue());
	EXPECT_THAT(outName, Eq("TIN Surface"));

	EXPECT_THAT(handler.getDisplayName("AeccDbAlignment", outName), IsTrue());
	EXPECT_THAT(outName, Eq("Alignment"));

	EXPECT_THAT(handler.getDisplayName("AeccDbPipe", outName), IsTrue());
	EXPECT_THAT(outName, Eq("Pipe"));
}

TEST(Civil3DProxyHandlerTest, GetDisplayNameUnknownClassLeavesOutNameUntouched)
{
	Civil3DProxyHandler handler;
	std::string outName = "Sentinel";

	EXPECT_THAT(handler.getDisplayName("NotARealCivil3DClass", outName), IsFalse());
	EXPECT_THAT(outName, Eq("Sentinel"));

	EXPECT_THAT(handler.getDisplayName("", outName), IsFalse());
	EXPECT_THAT(outName, Eq("Sentinel"));
}

// DwgProxyInspector::setMetadataIfMissing is shared by DwgProxyInspector itself
// and by app handlers (e.g. Civil3DProxyHandler::TinCapture) to contribute a
// computed property without overwriting one already set by a higher-priority
// source. It is pure map logic with no ODA dependency.

TEST(DwgProxyInspectorTest, SetMetadataIfMissingAddsNewKey)
{
	std::unordered_map<std::string, RepoVariant> metadata;

	DwgProxyInspector::setMetadataIfMissing(metadata, "Data::Minimum Elevation", RepoVariant(1.5));
	ASSERT_THAT(metadata.count("Data::Minimum Elevation"), Eq(1u));
	EXPECT_THAT(boost::get<double>(metadata["Data::Minimum Elevation"]), Eq(1.5));

	// A second, different key doesn't disturb the first.
	DwgProxyInspector::setMetadataIfMissing(metadata, "Data::Maximum Elevation", RepoVariant(9.5));
	ASSERT_THAT(metadata.size(), Eq(2));
	EXPECT_THAT(boost::get<double>(metadata["Data::Minimum Elevation"]), Eq(1.5));
	EXPECT_THAT(boost::get<double>(metadata["Data::Maximum Elevation"]), Eq(9.5));
}

TEST(DwgProxyInspectorTest, SetMetadataIfMissingDoesNotOverwriteExisting)
{
	std::unordered_map<std::string, RepoVariant> metadata;
	metadata["Information::Style"] = std::string("Original Value");

	DwgProxyInspector::setMetadataIfMissing(metadata, "Information::Style", RepoVariant(std::string("Contours and Triangles")));

	ASSERT_THAT(metadata.size(), Eq(1));
	EXPECT_THAT(boost::get<std::string>(metadata["Information::Style"]), Eq("Original Value"));
}

TEST(DwgProxyInspectorTest, SetMetadataIfMissingHandlesDifferentValueTypes)
{
	std::unordered_map<std::string, RepoVariant> metadata;

	DwgProxyInspector::setMetadataIfMissing(metadata, "String Value", RepoVariant(std::string("hello")));
	DwgProxyInspector::setMetadataIfMissing(metadata, "Double Value", RepoVariant(3.14));
	DwgProxyInspector::setMetadataIfMissing(metadata, "Int64 Value", RepoVariant((int64_t)42));

	EXPECT_THAT(boost::get<std::string>(metadata["String Value"]), Eq("hello"));
	EXPECT_THAT(boost::get<double>(metadata["Double Value"]), Eq(3.14));
	EXPECT_THAT(boost::get<int64_t>(metadata["Int64 Value"]), Eq((int64_t)42));
}
