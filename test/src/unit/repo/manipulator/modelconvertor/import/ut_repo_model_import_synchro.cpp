/**
*  Copyright (C) 2020 3D Repo Ltd
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
#include <repo/manipulator/modelconvertor/import/repo_model_import_synchro.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_manager.h>
#include "../../../../repo_test_database_info.h"
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_scene_utils.h"
#include "../../../../repo_test_common_tests.h"

using namespace repo::manipulator::modelconvertor;
using namespace testing;

repo::core::model::RepoScene* ModelImportManagerImport(std::string db, std::string path)
{
	auto config = ModelImportConfig();
	config.databaseName = "SynchroTestDb";
	config.revisionId = repo::lib::RepoUUID::createUUID();
	config.projectName = db;

	auto handler = getHandler();

	uint8_t err;
	std::string msg;

	ModelImportManager manager;
	auto scene = manager.ImportFromFile(path, config, handler, err);
	scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());

	return scene;
}

TEST(SynchroModelImport, ConstructorTest)
{
	SynchroModelImport(ModelImportConfig());
}

TEST(SynchroModelImport, DeconstructorTest)
{
	auto ptr = new SynchroModelImport(ModelImportConfig());
	delete ptr;
}

TEST(SynchroModelImport, ImportModel)
{
	auto import = SynchroModelImport(ModelImportConfig());
	auto handler = getHandler();
	uint8_t errCode = 0;
	auto scene = import.importModel(getDataPath(synchroVersion6_4), handler, errCode);	
	EXPECT_EQ(0, errCode);
	ASSERT_TRUE(scene);
}

TEST(SynchroModelImport, MetadataParents)
{
	uint8_t errCode;

	{
		SceneUtils scene(ModelImportManagerImport("SynchroMetadataTests", getDataPath("synchro6_4.spm")));
		common::checkMetadataInheritence(scene);
	}

	{
		SceneUtils scene(ModelImportManagerImport("SynchroMetadataTests", getDataPath("synchro6_5.spm")));
		common::checkMetadataInheritence(scene);
	}

}