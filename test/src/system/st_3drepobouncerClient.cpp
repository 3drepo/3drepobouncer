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

/**
* This tests that 3DRepobouncerClient performs as expected
* In terms of returning the right signals back to the caller
*/

#include <cstdlib>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <repo/error_codes.h>
#include <repo/repo_controller.h>
#include "../unit/repo_test_database_info.h"
#include "../unit/repo_test_utils.h"
#include "../unit/repo_test_scene_utils.h"
#include "../unit/repo_test_revision_utils.h"
#include <repo/core/model/bson/repo_bson_builder.h>
#include <repo/core/handler/database/repo_query.h>
#include <unordered_set>
#include <filesystem>

using namespace repo::manipulator::modelconvertor;
using namespace testing;

namespace {
	int runImport(std::string bouncerConfig, std::string importConfig)
	{
		return runProcess(
			getClientExePath() + " "
			+ bouncerConfig
			+ " import -f \""
			+ importConfig + "\""
		);
	}

	/*
	* Run the upload process by passing the client a config file, and check the
	* results. This method assumes the revision node already exists.
	*/
	int testModelUploadFromConfigFile(std::string configFile, bool expectSuccess = true)
	{
		auto returnCode = runImport(getConnConfig(), configFile);

		ModelImportConfig config;
		try {
			config = ModelImportConfig::ParseJsonFile(configFile);
			config.validate();
		}
		catch (repo::lib::RepoInvalidConfigException) {
			if (expectSuccess) {
				ADD_FAILURE() << "Could not parse config file " << configFile;
			}
			return returnCode;
		}

		auto handler = getHandler();

		repo::core::model::ModelRevisionNode revisionNode;
		try {
			auto bson = handler->findOneByCriteria(
				config.getDatabaseName(),
				config.getProjectName() + "." + REPO_COLLECTION_HISTORY,
				repo::core::handler::database::query::Eq(REPO_LABEL_ID, config.getRevisionId())
			);
			revisionNode = repo::core::model::ModelRevisionNode(bson);
		}
		catch (repo::lib::RepoBSONException)
		{
			if (expectSuccess) {
				ADD_FAILURE() << "Could not find expected revision node " << config.getDatabaseName() << " " << config.getProjectName() + "." + REPO_COLLECTION_HISTORY << " " << config.getRevisionId().toString();				return returnCode;
			}
			return returnCode;
		}

		if (expectSuccess)
		{
			EXPECT_THAT(revisionNode.getUploadStatus(), Eq(repo::core::model::ModelRevisionNode::UploadStatus::COMPLETE));

			// check that the scene is populated

			repo::core::model::RepoScene scene(
				config.getDatabaseName(),
				config.getProjectName(),
				config.getRevisionId()
			);
			scene.loadScene(handler.get());

			EXPECT_THAT(scene.hasRoot(), IsTrue());
			EXPECT_THAT(scene.getAllMeshes().size(), Gt(0)) << "Scene has no meshes";
			EXPECT_THAT(scene.getAllTransformations().size(), Gt(0)) << "Scene has no transformations";

			// check that the web resources exist

			auto bundles = handler->findAllByCriteria(
				config.getDatabaseName(),
				config.getProjectName() + "." + REPO_COLLECTION_STASH_BUNDLE,
				repo::core::handler::database::query::Eq(REPO_LABEL_ID, config.getRevisionId())
			);

			EXPECT_THAT(bundles.size(), Eq(1)) << "Incorrect number of bundles found for revision";

			auto assets = bundles[0];

			EXPECT_THAT(assets.getStringArray("assets").size(), Gt(0)) << "No assets found in bundle";
		}
		else
		{
			// If the config file points to an existing revision node, then that node
			// should always be left in a failure state unless the import succeeds.

			EXPECT_THAT(revisionNode.getUploadStatus(), Ne(repo::core::model::ModelRevisionNode::UploadStatus::COMPLETE));
		}

		return returnCode;
	}

	/*
	* Uploads a model file to the database to the container and revision specified
	* in the config, if any. Any fields in the config that are not specified will be
	* filled in with arbitrary values so that the config is valid. This method will
	* create a revision node for the file.
	*/
	int testModelUpload(std::filesystem::path filename, ModelImportConfig& config, bool expectSuccess = true)
	{
		// Ensure the config is valid

		if (config.databaseName.empty()) {
			config.databaseName = "stUpload";
		}
		if (config.projectName.empty()) {
			config.projectName = testing::getRandomContainer();
		}
		if (!config.revisionId) {
			config.revisionId = repo::lib::RepoUUID::createUUID();
		}
		if (config.targetUnits == repo::lib::ModelUnits::UNKNOWN) {
			config.targetUnits = repo::lib::ModelUnits::METRES;
		}

		// Save the config to a temporary file

		auto configPath = getDataPath(repo::lib::RepoUUID::createUUID().toString() + ".json");
		testing::writeImportConfig(config, configPath);

		// make sure there is no existing revision node - this method does not support
		// that, if the test should create its own node, then it should use another
		// method to do the upload

		auto handler = getHandler();

		auto revision = config.getRevisionId();

		EXPECT_THAT(handler->count(
			config.getDatabaseName(),
			config.getProjectName() + "." + REPO_COLLECTION_HISTORY,
			repo::core::handler::database::query::Eq(REPO_LABEL_ID, revision)
		), Eq(0));

		// creates the revision and ref nodes for the specified filename

		testing::makeRevisionNode(handler, config, filename);

		return testModelUploadFromConfigFile(configPath, expectSuccess);
	}

	/*
	* Uploads a model file to the database to a new container, checks if the upload
	* was successful, and returns the exit code. Use this to quickly test the upload
	* of files that should be working. Do not use this to test files that are expected
	* to fail, as this will throw on any failures.
	*/
	int testModelUpload(std::filesystem::path filename, bool expectSuccess = true)
	{
		ModelImportConfig config;
		return testModelUpload(filename, config, expectSuccess);
	}

	int testGenStash(const ModelImportConfig& config, std::string type)
	{
		return runProcess(
			getClientExePath() + " "
			+ getConnConfig() + " "
			+ "genStash" + " "
			+ config.getDatabaseName() + " "
			+ config.getProjectName() + " " 
			+ config.getRevisionId().toString() + " "
			+ type
		);
	}
}

// These next two tests test the behaviour of the client with an invalid
// bouncer config.

TEST(RepoClientTest, UploadTestInvalidDBConn)
{
	ModelImportConfig config;
	config.databaseName = "stUpload";
	config.projectName = "fail";
	config.revisionId = repo::lib::RepoUUID::createUUID();

	testing::makeRevisionNode(getHandler(), config, "cube.obj");

	auto importConfigPath = getDataPath(repo::lib::RepoUUID::createUUID().toString() + ".json");
	testing::writeImportConfig(config, importConfigPath);

	//Test failing to connect to database

	EXPECT_EQ(REPOERR_AUTH_FAILED, runImport(getDataPath("config/invalidAdd.json"), importConfigPath));
	
	EXPECT_FALSE(testing::projectIsPopulated(config));
}

TEST(RepoClientTest, UploadTestBadDBAuth)
{
	ModelImportConfig config;
	config.databaseName = "stUpload";
	config.projectName = "fail";
	config.revisionId = repo::lib::RepoUUID::createUUID();

	testing::makeRevisionNode(getHandler(), config, "cube.obj");

	auto importConfigPath = getDataPath(repo::lib::RepoUUID::createUUID().toString() + ".json");
	testing::writeImportConfig(config, importConfigPath);

	//Test Bad authentication

	EXPECT_EQ(REPOERR_AUTH_FAILED, runImport(getDataPath("config/badAuth.json"), importConfigPath));

	EXPECT_FALSE(testing::projectIsPopulated(config));
}

// These next few consider config files that have a valid schema but
// are not a valid configuration for importing for one reason or
// another.

TEST(RepoClientTest, UploadTestNoFile)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));

	ModelImportConfig config;
	config.databaseName = "stUpload";
	config.projectName = "fail";
	config.revisionId = repo::lib::RepoUUID::createUUID();

	repo::core::model::ModelRevisionNode revision;
	revision.setUniqueID(config.revisionId);
	
	// Insert a revision missing the rFile array

	auto handler = getHandler();
	handler->insertDocument(
		config.databaseName,
		config.projectName + "." + REPO_COLLECTION_HISTORY,
		revision
	);

	auto importConfigPath = getDataPath(repo::lib::RepoUUID::createUUID().toString() + ".json");
	testing::writeImportConfig(config, importConfigPath);

	EXPECT_EQ(REPOERR_MODEL_FILE_READ, runImport(getDataPath("config/config.json"), importConfigPath));

	EXPECT_FALSE(testing::projectIsPopulated(config));
}

TEST(RepoClientTest, UploadTestBadExt)
{
	ModelImportConfig config;
	config.databaseName = "stUpload";
	config.projectName = "fail";
	config.revisionId = repo::lib::RepoUUID::createUUID();

	testing::makeRevisionNode(getHandler(), config, "cube.exe");

	auto importConfigPath = getDataPath(repo::lib::RepoUUID::createUUID().toString() + ".json");
	testing::writeImportConfig(config, importConfigPath);

	//Test Bad authentication

	EXPECT_EQ(REPOERR_FILE_TYPE_NOT_SUPPORTED, runImport(getDataPath("config/config.json"), importConfigPath));

	EXPECT_FALSE(testing::projectIsPopulated(config));
}

TEST(RepoClientTest, UploadTestBadFBXVer)
{
	ModelImportConfig config;
	config.databaseName = "stUpload";
	config.projectName = "fail";
	config.revisionId = repo::lib::RepoUUID::createUUID();

	testing::makeRevisionNode(getHandler(), config, unsupportedFBXVersion);

	auto importConfigPath = getDataPath(repo::lib::RepoUUID::createUUID().toString() + ".json");
	testing::writeImportConfig(config, importConfigPath);

	//Test unsupported fbx

	EXPECT_EQ(REPOERR_UNSUPPORTED_FBX_VERSION, runImport(getDataPath("config/config.json"), importConfigPath));

	EXPECT_FALSE(testing::projectIsPopulated(config));
}

TEST(RepoClientTest, UploadTestBadArgs)
{
	//Invalid arguments (attempting to call using the old style)
	{
		std::string args = getClientExePath() + " " + getDataPath("config/config.json") + " import " + "filename.obj " + "db " + "proj";
		EXPECT_EQ((int)REPOERR_INVALID_ARG, runProcess(args));
	}

	//Invalid arguments (missing config path)
	{
		std::string args = getClientExePath() + " " + getDataPath("config/config.json") + " import " + "-f ";
		EXPECT_EQ((int)REPOERR_INVALID_ARG, runProcess(args));
	}

	//Invalid arguments (config file doesn't exist)
	{
		std::string args = getClientExePath() + " " + getDataPath("config/config.json") + " import " + "-f " + "nonexistentConfig.json";
		EXPECT_EQ((int)REPOERR_INVALID_ARG, runProcess(args));
	}
}

TEST(RepoClientTest, UploadTestSuccess)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload("cube.obj"));
}

TEST(RepoClientTest, UploadTestTexture)
{
	//Test Textured Upload where the texture is missing
	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE, testModelUpload(texturedModel, true));
}

TEST(RepoClientTest, UploadTestBIM)
{
	// This unsupported BIM version should fail to upload

	EXPECT_EQ(REPOERR_UNSUPPORTED_BIM_VERSION, testModelUpload("RepoModelImport/cube_bim1_spoofed.bim", false));

	// Regular BIM files should all succeed

	EXPECT_EQ(REPOERR_OK, testModelUpload("RepoModelImport/cube_bim2_navis_2021_repo_4.6.1.bim"));
	EXPECT_EQ(REPOERR_OK, testModelUpload("RepoModelImport/BrickWalls_bim3.bim"));
	EXPECT_EQ(REPOERR_OK, testModelUpload("RepoModelImport/wall_section_bim4.bim"));

	// These files with texture corruption should still upload OK

	EXPECT_EQ(REPOERR_OK, testModelUpload("RepoModelImport/BrickWalls_bim3_CorruptedTextureField.bim"));
	EXPECT_EQ(REPOERR_OK, testModelUpload("RepoModelImport/BrickWalls_bim3_MissingTexture.bim"));
}

TEST(RepoClientTest, UploadTestIFC)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(ifcModel));
	EXPECT_EQ(REPOERR_OK, testModelUpload(ifc4Model));
}

TEST(RepoClientTest, UploadTestDGN)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(dgnModel));
}

TEST(RepoClientTest, UploadTestDWG)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(dwgModel));

	// Checks that encrypted files fail with the correct code

	EXPECT_EQ(REPOERR_FILE_IS_ENCRYPTED, testModelUpload(dwgPasswordProtected, false));
}

TEST(RepoClientTest, UploadTestDXF)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(dxfModel));
}

TEST(RepoClientTest, UploadTestRVT)
{
	testing::unsetupTextures();

	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE, testModelUpload(rvtModel));

	testing::setupTextures();

	EXPECT_EQ(REPOERR_OK, testModelUpload(rvtModel));

	//Upload RVT file with no valid 3D view (should create a default view)

	EXPECT_EQ(REPOERR_OK, testModelUpload(rvtNo3DViewModel));

	//Upload RVT file with a named view that does not exist
	{
		ModelImportConfig config;
		config.databaseName = "rvtUpload";
		config.projectName = "rvtTest4";
		config.revisionId = repo::lib::RepoUUID::createUUID();
		config.viewName = "Not A View";

		EXPECT_EQ(REPOERR_VIEW_NOT_FOUND, testModelUpload(rvtModel, config, false));
	}

	//Upload RVT file with a named view that is not 3D
	{
		ModelImportConfig config;
		config.databaseName = "rvtUpload";
		config.projectName = "rvtTest5";
		config.revisionId = repo::lib::RepoUUID::createUUID();
		config.viewName = "Site";

		EXPECT_EQ(REPOERR_VIEW_NOT_3D, testModelUpload(rvtModel, config, false));
	}
}

TEST(RepoClientTest, UploadTestRVT2021)
{
	testing::setupTextures();
	EXPECT_EQ(REPOERR_OK, testModelUpload(rvtModel2021));
}

TEST(RepoClientTest, UploadTestRVT2022)
{
	testing::setupTextures();
	EXPECT_EQ(REPOERR_OK, testModelUpload(rvtModel2022));
}

TEST(RepoClientTest, UploadTestNWD2022)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(nwdModel2022));
}

TEST(RepoClientTest, UploadTestRVT2024)
{
	testing::setupTextures();
	EXPECT_EQ(REPOERR_OK, testModelUpload(rvtModel2024));
}

TEST(RepoClientTest, UploadTestNWD2024)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(nwdModel2024));
}

TEST(RepoClientTest, UploadTestRVT2025)
{
	testing::setupTextures();
	EXPECT_EQ(REPOERR_OK, testModelUpload(rvtModel2025));
}

TEST(RepoClientTest, UploadTestNWD2025)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(nwdModel2025));
}

TEST(RepoClientTest, UploadTestRVT2026)
{
	testing::setupTextures();
	EXPECT_EQ(REPOERR_OK, testModelUpload(rvtModel2026));
}

TEST(RepoClientTest, UploadTestNWD2026)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(nwdModel2026));
}

TEST(RepoClientTest, UploadTestNWDProtected)
{
	EXPECT_EQ(REPOERR_FILE_IS_ENCRYPTED, testModelUpload(nwdPasswordProtected, false));
}

TEST(RepoClientTest, UploadTestNWC)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(nwcModel));
}

TEST(RepoClientTest, UploadTestSPM)
{
	//commenting out 6.2 as we don't support this right now and it will crash.
	/*std::string spmUpload = produceUploadArgs(db, "synchroTest", getDataPath(synchroFile));
	EXPECT_EQ((int)REPOERR_OK, runProcess(spmUpload));
	EXPECT_TRUE(projectExists(db, "synchroTest"));*/

	//we don't support 6.1 - this will currently crash
	/*std::string spmUpload2 = produceUploadArgs(db, "synchroTest2", getDataPath(synchroOldVersion));
	EXPECT_EQ((int)REPOERR_UNSUPPORTED_VERSION, runProcess(spmUpload2));
	EXPECT_FALSE(projectExists(db, "synchroTest2"));*/

	//we also support 6.3
	EXPECT_EQ(REPOERR_OK, testModelUpload(synchroVersion6_3));

	EXPECT_EQ(REPOERR_OK, testModelUpload(synchroVersion6_4));

	EXPECT_EQ(REPOERR_OK, testModelUpload(synchroVersion6_5));

	EXPECT_EQ(REPOERR_OK, testModelUpload(synchroVersion6_5_3_7));
}

TEST(RepoClientTest, UploadTestRVTRegressionTests)
{
	// Some of these files have externally referenced or absolute referenced textures
	// that cannot be found on Linux, so we do not consider textures.
	testing::unsetupTextures();
	
	// Regression tests for fixed bugs that previously caused crashes or failed uploads

	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE, testModelUpload(rvtRoofTest));

	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE, testModelUpload(rvtMeta1));

	EXPECT_EQ(REPOERR_OK, testModelUpload(rvtMeta2));

	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE, testModelUpload(rvtHouse));
}

TEST(RepoClientTest, UploadTestMissingFieldsInJSON)
{
	std::vector<std::string> badConfigFiles = {
	//	"empty.json",
	//	"empty2.json",
	//	"importNoDatabase.json",
	//	"importNoDatabase2.json",
		"importNoProject.json",
		"importNoProject2.json",
		"importNoRev.json",
		"importNoRev2.json",
	};

	for(auto& s : badConfigFiles)
	{
		std::string configPath = getDataPath(s);
		EXPECT_EQ(REPOERR_INVALID_CONFIG_FILE, testModelUploadFromConfigFile(getDataPath(s), false)) << "Failed on config file " << s;
	}
}

TEST(RepoClientTest, UploadSplitByFloors)
{
	auto handler = getHandler();

	{
		ModelImportConfig config;
		config.databaseName = "testDB";
		config.projectName = "floors_split";
		config.revisionId = repo::lib::RepoUUID::createUUID();
		config.splitByFloor = true;

		EXPECT_EQ(REPOERR_OK, testModelUpload("floors_and_levels.ifc", config, true));

		std::set<std::string> groups;
		auto assets = handler->getAllFromCollectionTailable("testDB", "floors_split.stash.repobundles", 0, 1, {}, "timestamp");
		EXPECT_EQ(assets.size(), 1);
		for (auto& sm : assets[0].getObjectArray("metadata")) {
			if (sm.hasField("groups")) {
				for (auto& group : sm.getStringArray("groups")) {
					groups.insert(group);	
				}
			}
		}
		EXPECT_THAT(groups, UnorderedElementsAre("Level 0", "Level 1", "Level 2"));
	}

	{
		ModelImportConfig config;
		config.databaseName = "testDB";
		config.projectName = "floors_no_split";
		config.revisionId = repo::lib::RepoUUID::createUUID();
		config.splitByFloor = false;

		EXPECT_EQ(REPOERR_OK, testModelUpload("floors_and_levels.ifc", config, true));

		auto assets = handler->getAllFromCollectionTailable("testDB", "floors_no_split.stash.repobundles", 0, 1, {}, "timestamp");
		EXPECT_EQ(assets.size(), 1);
		for (auto& sm : assets[0].getObjectArray("metadata")) {
			EXPECT_THAT(sm.hasField("groups"), IsFalse());
		}
	}
}

TEST(RepoClientTest, GenStashTest)
{
	using namespace repo::core::handler::database;

	ModelImportConfig config;
	config.databaseName = "genStashTest";
	config.projectName = "cube";
	
	testModelUpload("cube.obj", config);

	auto handler = getHandler();

	// Drop collections from the existing imports so we know any changes are due to this test.

	handler->dropCollection("genStashTest", "cube.stash.repobundles");
	handler->dropCollection("genStashTest", "cube.stash.repobundles.ref");
	handler->dropCollection("genStashTest", "cube.stash.json_mpc.ref");

	std::unordered_set<std::string> collections;
	for (auto& name : handler->getCollections("genStashTest")) {
		collections.insert(name);
	}

	EXPECT_TRUE(collections.find("cube.stash.repobundles") == collections.end());
	EXPECT_TRUE(collections.find("cube.stash.repobundles.ref") == collections.end());
	EXPECT_TRUE(collections.find("cube.stash.json_mpc.ref") == collections.end());

	EXPECT_THAT(testGenStash(config, "repo"), Eq(0));

	collections.clear();
	for (auto& name : handler->getCollections("genStashTest")) {
		collections.insert(name);
	}

	EXPECT_TRUE(collections.find("cube.stash.repobundles") != collections.end());
	EXPECT_TRUE(collections.find("cube.stash.repobundles.ref") != collections.end());
	EXPECT_TRUE(collections.find("cube.stash.json_mpc.ref") != collections.end());

	auto documents = handler->findAllByCriteria("genStashTest", "cube.stash.json_mpc.ref", query::Eq(REPO_NODE_LABEL_ID, config.revisionId.toString() + "/fulltree.json"));
	EXPECT_EQ(documents.size(), 0);

	EXPECT_THAT(testGenStash(config, "tree"), Eq(0));

	documents = handler->findAllByCriteria("genStashTest", "cube.stash.json_mpc.ref", query::Eq(REPO_NODE_LABEL_ID, config.revisionId.toString() + "/fulltree.json"));
	EXPECT_EQ(documents.size(), 1);
}

TEST(RepoClientTest, ProcessDrawing)
{
	// Create a drawing revision to test against. This requires both the drawing
	// itself, and a ref node.

	auto handler = getHandler();

	repo::core::model::RepoBSONBuilder revisionBuilder;

	// Create a mocked-up revision BSON

	auto db = "testDrawing"; // This must match the database in processDrawingConfig
	auto rid = repo::lib::RepoUUID("cad0c3fe-dd1d-4844-ad04-cfb75df26a63"); // This must match the revId in processDrawingConfig
	auto model = repo::lib::RepoUUID::createUUID().toString();
	auto project = repo::lib::RepoUUID::createUUID();
	auto rFile = repo::lib::RepoUUID::createUUID();
	std::vector< repo::lib::RepoUUID> rFiles;
	rFiles.push_back(rFile);
	revisionBuilder.append("_id", rid);
	revisionBuilder.append("project", project);
	revisionBuilder.append("model", model);
	revisionBuilder.append("format", ".dwg");
	revisionBuilder.appendTimeStamp("timestamp");
	revisionBuilder.appendArray("rFile", rFiles);

	// Make sure that the file manager uses the same config as produceProcessDrawingArgs
	auto manager = handler->getFileManager();

	// Get a DWG as a blob
	auto path = getDataPath(dwgDrawing);
	std::ifstream drawingFile{ path, std::ios::binary };
	std::vector<uint8_t> bin(std::istreambuf_iterator<char>{drawingFile}, {});

	repo::core::handler::fileservice::FileManager::Metadata metadata;
	metadata["name"] = std::string("test.dwg");

	manager->uploadFileAndCommit(
		db,
		REPO_COLLECTION_DRAWINGS,
		rFile, // Take care that drawing source ref node Ids are always UUIDs
		bin,
		metadata
	);

	std::string err;
	handler->upsertDocument(
		db,
		REPO_COLLECTION_DRAWINGS,
		revisionBuilder.obj(),
		true
	);

	EXPECT_EQ(err.size(), 0);

	EXPECT_EQ((int)REPOERR_OK, runProcess(produceProcessDrawingArgs(getDataPath(processDrawingConfig))));

	// The revision should now be updated.

	auto revision = handler->findOneByUniqueID(
		db,
		REPO_COLLECTION_DRAWINGS,
		rid
	);

	// The image should be non-null, and point to a file ref

	EXPECT_TRUE(revision.hasField("image"));

	// The other properties should be the same

	EXPECT_EQ(revision.getUUIDField("_id"), rid);
	EXPECT_EQ(revision.getUUIDField("project"), project);
	EXPECT_EQ(revision.getStringField("model"), model);

	// Make sure to use the file manager method because ref nodes are keyed by string not uuid

	auto imageRef = manager->getFileRef(
		db,
		REPO_COLLECTION_DRAWINGS,
		revision.getUUIDField("image")
	);

	// Check that the document is correctly populated

	// Via the RepoRef object
	EXPECT_EQ(imageRef.getType(), repo::core::model::RepoRef::RefType::FS);
	EXPECT_EQ(imageRef.getFileName(), "test.svg"); // The image should have its file extension changed

	auto refNode = handler->findOneByUniqueID(
		db,
		REPO_COLLECTION_DRAWINGS + std::string(".ref"),
		revision.getUUIDField("image")
	);

	EXPECT_EQ(refNode.getStringField("type"), "fs");
	EXPECT_EQ(refNode.getStringField("name"), "test.svg");
	EXPECT_EQ(refNode.getStringField("mimeType"), "image/svg+xml");
	EXPECT_EQ(refNode.getUUIDField("project"), project);
	EXPECT_EQ(refNode.getStringField("model"), model);
	EXPECT_EQ(refNode.getUUIDField("rev_id"), rid);
}

TEST(RepoClientTest, UnicodeFilenames)
{
	EXPECT_EQ(REPOERR_OK, testModelUpload(u8"КР3_очищено.dgn"));
	EXPECT_EQ(REPOERR_OK, testModelUpload(u8"КР3_очищено.dwg"));
	EXPECT_EQ(REPOERR_OK, testModelUpload(u8"КР3_очищено.nwc"));
	EXPECT_EQ(REPOERR_OK, testModelUpload(u8"КР3_очищено.rvt"));
	EXPECT_EQ(REPOERR_OK, testModelUpload(u8"КР3_очищено.bim004.bim"));
	EXPECT_EQ(REPOERR_OK, testModelUpload(u8"КР3_очищено.obj"));
	EXPECT_EQ(REPOERR_OK, testModelUpload(u8"КР3_очищено.ifc"));

	/*
	* Synchro does not support Unicode filenames on Windows. This is a known issue,
	* https://github.com/3drepo/SynchroReader/issues/16/
	* but is low priority because the Linux versions of the plugins do.
	*/
#ifndef WIN32
	EXPECT_EQ(REPOERR_OK, testModelUpload(u8"КР3_очищено.6_4.spm"));
#endif
}

TEST(RepoClientTest, ExistingRevisionArtefactsAreRemoved)
{
	// Perform a couple of imports to pre-populate a container with prior
	// (successful) revisions.

	testing::setupTextures();

	auto database = repo::lib::RepoUUID::createUUID().toString();
	auto container = repo::lib::RepoUUID::createUUID().toString();

	repo::manipulator::modelconvertor::ModelImportConfig config;
	config.databaseName = database;
	config.projectName = container;
	config.targetUnits = repo::lib::ModelUnits::METRES;
	config.splitByFloor = true;

	for (int i = 0; i < 2; i++)
	{
		config.revisionId = repo::lib::RepoUUID::createUUID();
		EXPECT_EQ(REPOERR_OK, testModelUpload("sample.rvt", config, true));
	}

	// Now commit some artefacts for a revision to emulate an import that was
	// aborted mid-progress.

	auto handler = getHandler();
	config.revisionId = repo::lib::RepoUUID::createUUID();

	// Create multiple random graphs over each other and delete one of the root
	// nodes, to encompass the case where multiple failed imports have run, and
	// also when the streaming scene builder is destroyed with nodes still buffered
	// (leaving orphan nodes).

	testing::makeRandomScene(handler, database, container, config.revisionId);
	testing::makeRandomScene(handler, database, container, config.revisionId);
	testing::makeRandomScene(handler, database, container, config.revisionId);

	testing::ContainerUtils utils(handler, database, container, config.revisionId);
	auto rootNodes = utils.getRootNodes();
	utils.deleteNode(rootNodes[0].getUUIDField(REPO_NODE_LABEL_ID));

	// Get all the link names for the orphaned nodes so we can check the files are
	// properly deleted

	auto fileManager = handler->getFileManager();
	std::unordered_set<std::string> orphanLinkNames;
	auto orphanRefNodes = utils.getAllRefNodes();
	for (auto& ref : orphanRefNodes) {
		orphanLinkNames.insert(fileManager->getFilePath(ref));
	}

	for (auto orphanLinkName : orphanLinkNames) {
		EXPECT_TRUE(std::filesystem::exists(orphanLinkName));
	}

	// Now run the import again, which should remove all orphaned artefacts leaving
	// three well-formed revisions.

	EXPECT_EQ(REPOERR_OK, testModelUpload("sample.rvt", config, true));

	auto history = utils.getRevisions();
	EXPECT_EQ(history.size(), 3);

	for (auto& rev : history) {
		EXPECT_EQ(rev.getUploadStatus(), repo::core::model::ModelRevisionNode::UploadStatus::COMPLETE);
		containerHasValidHierarchy(database, container, rev.getUniqueID());
	}

	containerHasNoOrphanRefNodes(database, container);

	for (auto orphanLinkName : orphanLinkNames) {
		EXPECT_FALSE(std::filesystem::exists(orphanLinkName));
	}
}