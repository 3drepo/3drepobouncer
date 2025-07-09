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

#include <repo/manipulator/modelconvertor/import/repo_model_import_manager.h>
#include <repo/manipulator/modelutility/repo_scene_manager.h>
#include <repo/manipulator/modelutility/repo_maker_selection_tree.h>
#include <repo/manipulator/modelutility/rapidjson/rapidjson.h>
#include <repo/manipulator/modelutility/rapidjson/document.h>

#include "../../../repo_test_utils.h"
#include "../../../repo_test_mesh_utils.h"
#include "../../../repo_test_database_info.h"
#include "../../../repo_test_fileservice_info.h"

using namespace repo::core::model;
using namespace testing;
using namespace repo::manipulator::modelconvertor;
using namespace repo::manipulator::modelutility;
using namespace rapidjson;

#define TESTDB "SelectionTreeTest"

repo::core::model::RepoScene* ModelImportManagerImport(std::string filename, const ModelImportConfig& config)
{
	auto handler = getHandler();

	uint8_t err;
	std::string msg;

	ModelImportManager manager;
	auto scene = manager.ImportFromFile(filename, config, handler, err);
	scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());
	scene->loadScene(handler.get(), msg);

	SceneManager manager2;	
	manager2.generateAndCommitSelectionTree(scene, handler.get());

	return scene;
}

// Parsing the selection tree is something only supported in unit tests.
// The tree is otherwise never meant to be read back outside the backend.

#pragma optimize("", off)

void readNode(const rapidjson::GenericObject<true, rapidjson::Value>& src)
{

}

TEST(RepoSelectionTreeTest, RvtHouse)
{
	auto collection = "dwgModel";

	ModelImportConfig config(
		repo::lib::RepoUUID::createUUID(),
		TESTDB,
		collection
	);
	config.targetUnits = ModelUnits::MILLIMETRES;

	auto scene = ModelImportManagerImport(getDataPath(dwgModel), config);

	// Get the selection tree from the database

	auto handler = getHandler();

	auto fullTree = handler->getFileManager()->getFile(
		TESTDB,
		collection + std::string(".stash.json_mpc"),
		scene->getRevisionID().toString() + std::string("/fulltree.json")
	);

    Document doc;
    doc.Parse(reinterpret_cast<const char*>(fullTree.data()), fullTree.size());
    ASSERT_FALSE(doc.HasParseError());
    ASSERT_TRUE(doc.IsObject());

	auto o = doc["hello"].GetObject();

	SelectionTree tree;

	SelectionTree::Container container;

	container.account = TESTDB;
	container.project = collection;


	
}