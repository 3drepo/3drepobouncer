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

// The ODA NWD importer also uses rapidjson, so make sure to import our
// copy into a different namespace to avoid interference between two
// possibly different versions.

#define RAPIDJSON_NAMESPACE repo::rapidjson
#define RAPIDJSON_NAMESPACE_BEGIN namespace repo { namespace rapidjson {
#define RAPIDJSON_NAMESPACE_END } }

#include <repo/manipulator/modelutility/rapidjson/rapidjson.h>
#include <repo/manipulator/modelutility/rapidjson/document.h>

#include <repo/manipulator/modelutility/repo_clash_detection_engine.h>
#include <repo/manipulator/modelutility/clashdetection/sparse_scene_graph.h>

#include "../../../repo_test_utils.h"
#include "../../../repo_test_mesh_utils.h"
#include "../../../repo_test_database_info.h"
#include "../../../repo_test_fileservice_info.h"
#include "../../../repo_test_scene_utils.h"

using namespace repo::core::model;
using namespace testing;
using namespace repo::manipulator::modelconvertor;
using namespace repo::manipulator::modelutility;
using namespace repo;

#define TESTDB "ClashDetection"


TEST(SparseSceneGraph, Simple)
{
	// Test a simple graph against a known import.

	auto handler = getHandler();

	lib::Container container;
	container.teamspace = "sebjf";
	container.container = "50817fcf-9e3a-4290-965c-d5c4cae736cc";
	container.revision = repo::lib::RepoUUID("7537cd67-d30e-4b89-a79f-1c400116aa79");

	// Cube was imported units of m from an OBJ

	sparse::SceneGraph sceneGraph;
	sceneGraph.populate(
		handler,
		&container,
		{
			repo::lib::RepoUUID("e63c49f5-0725-48a0-a237-1d2610e177cb")
		}
	);
}