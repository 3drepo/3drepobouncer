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
#include <repo/manipulator/modelconvertor/import/repo_model_import_oda.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_manager.h>
#include <repo/lib/repo_log.h>
#include <repo/error_codes.h>
#include "../../../../../repo_test_utils.h"
#include "../../../../../repo_test_database_info.h"
#include "../../../../../repo_test_scene_utils.h"

using namespace repo::manipulator::modelconvertor;
using namespace repo::core::model;
using namespace testing;

#define TESTDB "ODAModelImportTest"

#pragma optimize("", off)

namespace ODAModelImportUtils
{
	repo::core::model::RepoScene* ModelImportManagerImport(std::string collection, std::string filename)
	{
		ModelImportConfig config(
			true,
			ModelUnits::MILLIMETRES,
			"",
			0,
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			collection);

		auto handler = getHandler();

		uint8_t err;
		std::string msg;

		ModelImportManager manager;
		auto scene = manager.ImportFromFile(filename, config, handler, err);
		scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());
		scene->loadScene(handler.get(), msg);

		return scene;
	}
}

TEST(ODAModelImport, Sample2025NWDTree)
{
	auto collection = "Sample2025NWD";
	auto scene = ODAModelImportUtils::ModelImportManagerImport(collection, getDataPath("sample2025.nwd"));

	// For NWDs, Layers/Levels & Collections always end up as branch nodes. Composite
	// Objects and Groups are leaf nodes if they contain only Geometric Items and
	// Instances as children.

	// See this page for the meaning of the icons in Navis: https://help.autodesk.com/view/NAV/2024/ENU/?guid=GUID-BC657B3A-5104-45B7-93A9-C6F4A10ED0D4

	// This snippet tests whether geometry is grouped successfully

	SceneHelper helper(scene);

	auto nodes = helper.findNodesByMetadata("Element::Id", repo::lib::RepoVariant(int64_t(309347)));
	EXPECT_THAT(nodes.size(), Eq(1));
	EXPECT_THAT(nodes[0].isLeaf(), IsTrue());
	EXPECT_THAT(nodes[0].hasGeometry(), IsTrue());

	nodes = helper.findTransformationNodesByName("Wall-Ext_102Bwk-75Ins-100LBlk-12P");
	EXPECT_THAT(nodes.size(), Eq(1));

	auto children = nodes[0].getChildren();
	EXPECT_THAT(children.size(), Eq(6));

	for (auto n : children) {
		EXPECT_THAT(n.isLeaf(), IsTrue());
		EXPECT_THAT(n.hasGeometry(), IsTrue());
		EXPECT_THAT(n.name(), Eq("Basic Wall"));
	}

	// These two nodes correspond to hidden items. In Navis, hidden elements should
	// be imported unaffected.

	EXPECT_THAT(helper.findNodesByMetadata("Element::Id", repo::lib::RepoVariant(int64_t(694))).size(), Gt(0));
	EXPECT_THAT(helper.findNodesByMetadata("Element::Id", repo::lib::RepoVariant(int64_t(311))).size(), Gt(0));
}

