/**
*  Copyright (C) 2015 3D Repo Ltd
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

#include <cstdlib>
#include <gtest/gtest.h>
#include <repo/repo_controller.h>
#include <repo/error_codes.h>
#include "../repo_test_database_info.h"
#include "../repo_test_fileservice_info.h"
#include "../repo_test_utils.h"
#include "../repo_test_scene_utils.h"

using namespace repo;
using namespace testing;

static std::shared_ptr<RepoController> getController()
{
	static std::shared_ptr<RepoController> controller =
		std::make_shared<RepoController>();

	return controller;
}

TEST(RepoControllerTest, CommitScene) {
	auto controller = getController();
	auto token = initController(controller.get());

	// This test covers only legacy behaviour of RepoScene, which is committing a
	// scene when it is fully resident in memory only. The new behaviour is to use
	// the streamed importer (RepoSceneBuilder), for which the revision must be set
	// ahead of time, and be the identical to that provided to commitScene.

	auto scene = testing::makeRandomScene();

	//Try to commit a scene without setting db/project name
	uint8_t errCode;
	EXPECT_EQ(REPOERR_UNKNOWN_ERR, controller->commitScene(token, scene));
	EXPECT_FALSE(scene->isRevisioned());

	//Trying to commit a scene with empty db and project name should also fail
	scene->setDatabaseAndProjectName("", "");
	EXPECT_EQ(REPOERR_UNKNOWN_ERR, controller->commitScene(token, scene));
	EXPECT_FALSE(scene->isRevisioned());

	scene->setDatabaseAndProjectName("balh", "");
	EXPECT_EQ(REPOERR_UNKNOWN_ERR, controller->commitScene(token, scene));
	EXPECT_FALSE(scene->isRevisioned());

	scene->setDatabaseAndProjectName("", "blah");
	EXPECT_EQ(REPOERR_UNKNOWN_ERR, controller->commitScene(token, scene));
	EXPECT_FALSE(scene->isRevisioned());

	//Setting the db name and project name should allow commit successfully
	scene->setDatabaseAndProjectName("commitSceneTest", "commitCube");
	EXPECT_EQ(REPOERR_OK, controller->commitScene(initController(controller.get()), scene));
	EXPECT_TRUE(scene->isRevisioned());
	EXPECT_TRUE(projectExists("commitSceneTest", "commitCube"));
	EXPECT_EQ(scene->getOwner(), "ANONYMOUS USER");

	auto scene2 = makeRandomScene();
	std::string owner = "dog";
	EXPECT_EQ(errCode, 0);
	scene2->setDatabaseAndProjectName("commitSceneTest", "commitCube2");
	EXPECT_EQ(REPOERR_OK, controller->commitScene(initController(controller.get()), scene2, owner));
	EXPECT_TRUE(scene2->isRevisioned());
	EXPECT_TRUE(projectExists("commitSceneTest", "commitCube2"));
	EXPECT_EQ(scene2->getOwner(), owner);

	//null pointer checks
	EXPECT_EQ(REPOERR_UNKNOWN_ERR, controller->commitScene(token, nullptr));
	EXPECT_EQ(REPOERR_UNKNOWN_ERR, controller->commitScene(nullptr, scene));
}

TEST(RepoControllerTest, LoadSceneFromFile) {
	auto controller = getController();
	auto token = initController(controller.get());
	auto defaultG = core::model::RepoScene::GraphType::DEFAULT;

	repo::manipulator::modelconvertor::ModelImportConfig config;
	config.databaseName = "loadSceneFromFileTest";
	config.projectName = "loadSceneFromFileTestContainer";

	//standard import
	uint8_t errCode;
	auto scene = controller->loadSceneFromFile(getDataPath(simpleModel), errCode, config);
	ASSERT_TRUE(scene);
	EXPECT_EQ(errCode, 0);
	ASSERT_TRUE(scene->getRoot(defaultG));
	EXPECT_FALSE(scene->isMissingTexture());
	EXPECT_FALSE(scene->isRevisioned());
	EXPECT_TRUE(dynamic_cast<core::model::TransformationNode*>(scene->getRoot(defaultG))->isIdentity());

	//Import the scene with non existant file
	auto sceneNoFile = controller->loadSceneFromFile("thisFileDoesntExist.obj", errCode, config);
	EXPECT_EQ(errCode, REPOERR_MODEL_FILE_READ);
	EXPECT_FALSE(sceneNoFile);

	//Import the scene with bad Extension
	auto sceneBadExt = controller->loadSceneFromFile(getDataPath(badExtensionFile), errCode, config);
	EXPECT_EQ(errCode, REPOERR_FILE_TYPE_NOT_SUPPORTED);
	EXPECT_FALSE(sceneBadExt);

	//Import the scene with texture but not found
	auto sceneNoTex = controller->loadSceneFromFile(getDataPath(texturedModel), errCode, config);
	EXPECT_EQ(errCode, 0);
	EXPECT_TRUE(sceneNoTex);
	EXPECT_TRUE(sceneNoTex->getRoot(defaultG));
	EXPECT_TRUE(sceneNoTex->isMissingTexture());

	//Import the scene with texture but not found
	auto sceneTex = controller->loadSceneFromFile(getDataPath(texturedModel2), errCode, config);
	EXPECT_EQ(errCode, 0);
	EXPECT_TRUE(sceneTex);
	EXPECT_TRUE(sceneTex->getRoot(defaultG));
	EXPECT_FALSE(sceneTex->isMissingTexture());

	//FIXME: need to test with change of config, but this is probably not trival.
}