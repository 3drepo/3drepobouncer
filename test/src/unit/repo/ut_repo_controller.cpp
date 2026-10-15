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
#include "../repo_test_revision_utils.h"
#include "../repo_test_scene_utils.h"

using namespace repo;
using namespace testing;

static std::shared_ptr<RepoController> getController()
{
	static std::shared_ptr<RepoController> controller =
		std::make_shared<RepoController>();

	return controller;
}

TEST(RepoControllerTest, ProcessModelRevision) {
	auto controller = getController();
	auto token = initController(controller.get());
	auto defaultG = core::model::RepoScene::GraphType::DEFAULT;
	auto handler = getHandler();
	auto teamspace = "RepoControllerTest";

	// Config with that points to a non-existent revision

	{
		repo::manipulator::modelconvertor::ModelImportConfig config;
		config.databaseName = teamspace;
		config.projectName = testing::getRandomContainer();
		config.revisionId = repo::lib::RepoUUID::createUUID();

		// No revision node has been created for this Id, so resolving the
		// revision should fail.

		try {
			controller->processModelRevision(config);
			FAIL() << "processModelRevision should have thrown for a non-existent revision";
		}
		catch (const repo::lib::RepoException& e) {
			EXPECT_EQ(e.repoCode(), REPOERR_REVISION_NOT_FOUND);
		}

		EXPECT_FALSE(testing::sceneHasArtefacts(config));
	}

	// Config that points to an incomplete revision (one that has no rFile
	// entries at all, as would happen if the upload was interrupted before
	// the ref could be attached)

	{
		repo::manipulator::modelconvertor::ModelImportConfig config;
		config.databaseName = teamspace;
		config.projectName = testing::getRandomContainer();
		config.revisionId = repo::lib::RepoUUID::createUUID();

		repo::core::model::ModelRevisionNode revision;
		revision.setUniqueID(config.revisionId);

		handler->insertDocument(
			config.databaseName,
			config.projectName + "." + REPO_COLLECTION_HISTORY,
			revision
		);

		try {
			controller->processModelRevision(config);
			FAIL() << "processModelRevision should have thrown for a revision with no rFile entries";
		}
		catch (const repo::lib::RepoException& e) {
			EXPECT_EQ(e.repoCode(), REPOERR_FILE_REF_MISSING);
		}

		EXPECT_FALSE(testing::sceneHasArtefacts(config));
	}

	// Config that points to a revision missing a ref node (the revision
	// references a file Id, but no corresponding document exists in the
	// .ref collection)

	{
		repo::manipulator::modelconvertor::ModelImportConfig config;
		config.databaseName = teamspace;
		config.projectName = testing::getRandomContainer();
		config.revisionId = repo::lib::RepoUUID::createUUID();

		repo::core::model::ModelRevisionNode revision;
		revision.setUniqueID(config.revisionId);
		revision.setFiles({repo::lib::RepoUUID::createUUID().toString()});

		handler->insertDocument(
			config.databaseName,
			config.projectName + "." + REPO_COLLECTION_HISTORY,
			revision
		);

		try {
			controller->processModelRevision(config);
			FAIL() << "processModelRevision should have thrown for a revision with a missing ref node";
		}
		catch (const repo::lib::RepoException& e) {
			EXPECT_EQ(e.repoCode(), REPOERR_FILE_REF_MISSING);
		}

		EXPECT_FALSE(testing::sceneHasArtefacts(config));
	}

	// Config that points to a revision with a ref node that can't be resolved
	// to an actual file (the file is marked as an external file outside of
	// bouncer's control; here the link does not point to anything that
	// exists on disk).

	{
		repo::manipulator::modelconvertor::ModelImportConfig config;
		config.databaseName = teamspace;
		config.projectName = testing::getRandomContainer();
		config.revisionId = repo::lib::RepoUUID::createUUID();
		config.targetUnits = repo::lib::ModelUnits::METRES;

		testing::makeRevisionNode(handler, config, getDataPath("nonExistentFile"));

		try {
			controller->processModelRevision(config);
			FAIL() << "processModelRevision should have thrown for a revision with a ref node that can't be resolved to an actual file";
		}
		catch (const repo::lib::RepoException& e) {
			EXPECT_EQ(e.repoCode(), REPOERR_MODEL_FILE_READ);
		}

		EXPECT_FALSE(testing::sceneHasArtefacts(config));
	}

	// Config that points to a model with textures (without textures available)

	{
		repo::manipulator::modelconvertor::ModelImportConfig config;
		config.databaseName = teamspace;
		config.projectName = testing::getRandomContainer();
		config.revisionId = repo::lib::RepoUUID::createUUID();
		config.targetUnits = repo::lib::ModelUnits::METRES;

		testing::makeRevisionNode(handler, config, getDataPath(texturedModel));

		EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE, controller->processModelRevision(config));

		EXPECT_TRUE(testing::projectIsPopulated(config));
	}

	// Config that points to a model with textures (with textures available)

	{
		repo::manipulator::modelconvertor::ModelImportConfig config;
		config.databaseName = teamspace;
		config.projectName = testing::getRandomContainer();
		config.revisionId = repo::lib::RepoUUID::createUUID();
		config.targetUnits = repo::lib::ModelUnits::METRES;

		testing::makeRevisionNode(handler, config, getDataPath(texturedModel2));

		EXPECT_EQ(REPOERR_OK, controller->processModelRevision(config));

		EXPECT_TRUE(testing::projectIsPopulated(config));
	}

	// Successful import

	{
		repo::manipulator::modelconvertor::ModelImportConfig config;
		config.databaseName = teamspace;
		config.projectName = testing::getRandomContainer();
		config.revisionId = repo::lib::RepoUUID::createUUID();
		config.targetUnits = repo::lib::ModelUnits::METRES;

		testing::makeRevisionNode(handler, config, getDataPath(simpleModel));

		EXPECT_EQ(REPOERR_OK, controller->processModelRevision(config));

		EXPECT_TRUE(testing::projectIsPopulated(config));
	}
}