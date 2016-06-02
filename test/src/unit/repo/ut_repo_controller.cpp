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
#include "../repo_test_database_info.h"
#include "../repo_test_utils.h"

using namespace repo;

static std::shared_ptr<RepoController> getController()
{
	static std::shared_ptr<RepoController> controller =
		std::make_shared<RepoController>();

	return controller;
}

static RepoController::RepoToken* getToken()
{
	auto controller = getController();
	std::string errMsg;
	return controller->authenticateToAdminDatabaseMongo(errMsg, REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);
}

TEST(RepoControllerTest, CommitScene){
	auto controller = getController();
	auto token = getToken();
	//Try to commit a scene without setting db/project name
	auto scene = controller->loadSceneFromFile(getDataPath(simpleModel));
	EXPECT_FALSE(controller->commitScene(token, scene));
	EXPECT_FALSE(scene->isRevisioned());

	//Trying to commit a scene with empty db and project name should also fail
	scene->setDatabaseAndProjectName("", "");
	EXPECT_FALSE(controller->commitScene(token, scene));
	EXPECT_FALSE(scene->isRevisioned());

	scene->setDatabaseAndProjectName("balh", "");
	EXPECT_FALSE(controller->commitScene(token, scene));
	EXPECT_FALSE(scene->isRevisioned());

	scene->setDatabaseAndProjectName("", "blah");
	EXPECT_FALSE(controller->commitScene(token, scene));
	EXPECT_FALSE(scene->isRevisioned());

	//Setting the db name and project name should allow commit successfully
	scene->setDatabaseAndProjectName("commitSceneTest", "commitCube");
	EXPECT_TRUE(controller->commitScene(getToken(), scene));
	EXPECT_TRUE(scene->isRevisioned());
	EXPECT_TRUE(projectExists("commitSceneTest", "commitCube"));
	EXPECT_EQ(scene->getOwner(), REPO_GTEST_DBUSER);

	auto scene2 = controller->loadSceneFromFile(getDataPath(simpleModel));
	std::string owner = "dog";
	scene2->setDatabaseAndProjectName("commitSceneTest", "commitCube2");
	EXPECT_TRUE(controller->commitScene(getToken(), scene2, owner));
	EXPECT_TRUE(scene2->isRevisioned());
	EXPECT_TRUE(projectExists("commitSceneTest", "commitCube2"));
	EXPECT_EQ(scene2->getOwner(), owner);

	//null pointer checks
	EXPECT_FALSE(controller->commitScene(token, nullptr));
	EXPECT_FALSE(controller->commitScene(nullptr, scene));
}

TEST(RepoControllerTest, LoadSceneFromFile){
	auto controller = getController();
	auto defaultG = core::model::RepoScene::GraphType::DEFAULT;
	auto optG = core::model::RepoScene::GraphType::OPTIMIZED;

	//standard import
	auto scene = controller->loadSceneFromFile(getDataPath(simpleModel));
	ASSERT_TRUE(scene);
	ASSERT_TRUE(scene->getRoot(defaultG));
	ASSERT_TRUE(scene->getRoot(optG));
	EXPECT_FALSE(scene->isMissingTexture());
	EXPECT_FALSE(scene->isRevisioned());
	EXPECT_TRUE(dynamic_cast<core::model::TransformationNode*>(scene->getRoot(defaultG))->isIdentity());

	//Import the scene with no transformation reduction
	auto sceneNoReduction = controller->loadSceneFromFile(getDataPath(simpleModel), false);
	EXPECT_TRUE(sceneNoReduction);
	EXPECT_TRUE(sceneNoReduction->getRoot(defaultG));
	EXPECT_TRUE(sceneNoReduction->getRoot(optG));
	EXPECT_FALSE(sceneNoReduction->isMissingTexture());
	//There should be more transformations in the non-reduced scene than the standard scene
	EXPECT_TRUE(sceneNoReduction->getAllTransformations(defaultG).size()
			> scene->getAllTransformations(defaultG).size());

	//Import the scene with root trans rotated
	auto sceneRotated = controller->loadSceneFromFile(getDataPath(simpleModel), true, true);
	EXPECT_TRUE(sceneRotated);
	ASSERT_TRUE(sceneRotated->getRoot(defaultG));
	EXPECT_TRUE(sceneRotated->getRoot(optG));
	EXPECT_FALSE(sceneRotated->isMissingTexture());
	//The root transformation should not be an identity
	core::model::TransformationNode *rootTrans = dynamic_cast<core::model::TransformationNode*>(sceneRotated->getRoot(defaultG));
	EXPECT_FALSE(rootTrans->isIdentity());

	//Import the scene with non existant file
	auto sceneNoFile = controller->loadSceneFromFile("thisFileDoesntExist.obj");
	EXPECT_FALSE(sceneNoFile);

	//Import the scene with bad Extension
	auto sceneBadExt = controller->loadSceneFromFile(getDataPath(badExtensionFile));
	EXPECT_FALSE(sceneBadExt);

	//Import the scene with texture but not found
	auto sceneNoTex = controller->loadSceneFromFile(getDataPath(texturedModel));
	EXPECT_TRUE(sceneNoTex);
	EXPECT_TRUE(sceneNoTex->getRoot(defaultG));
	EXPECT_TRUE(sceneNoTex->getRoot(optG));
	EXPECT_TRUE(sceneNoTex->isMissingTexture());

	//Import the scene with texture but not found
	auto sceneTex = controller->loadSceneFromFile(getDataPath(texturedModel2));
	EXPECT_TRUE(sceneTex);
	EXPECT_TRUE(sceneTex->getRoot(defaultG));
	EXPECT_TRUE(sceneTex->getRoot(optG));
	EXPECT_FALSE(sceneTex->isMissingTexture());

	//FIXME: need to test with change of config, but this is probably not trival.
}