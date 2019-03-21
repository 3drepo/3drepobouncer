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
#include <repo/error_codes.h>
#include <repo/repo_controller.h>
#include "../unit/repo_test_database_info.h"
#include "../unit/repo_test_utils.h"

static std::string getSuccessFilePath()
{
	return getDataPath(simpleModel);
}

static std::string produceCleanArgs(
	const std::string &database,
	const std::string &project,
	const std::string &dbAdd = REPO_GTEST_DBADDRESS,
	const int         &port = REPO_GTEST_DBPORT,
	const std::string &username = REPO_GTEST_DBUSER,
	const std::string &password = REPO_GTEST_DBPW
	)
{
	return  getClientExePath() + " " + dbAdd + " "
		+ std::to_string(port) + " "
		+ username + " "
		+ password + " "
		+ REPO_GTEST_S3_BUCKET + " "
		+ REPO_GTEST_S3_REGION
		+ " clean "
		+ database + " "
		+ project;
}

static std::string produceGenStashArgs(
	const std::string &database,
	const std::string &project,
	const std::string &type,
	const std::string &dbAdd = REPO_GTEST_DBADDRESS,
	const int         &port = REPO_GTEST_DBPORT,
	const std::string &username = REPO_GTEST_DBUSER,
	const std::string &password = REPO_GTEST_DBPW
	)
{
	return  getClientExePath() + " " + dbAdd + " "
		+ std::to_string(port) + " "
		+ username + " "
		+ password + " "
		+ REPO_GTEST_S3_BUCKET + " "
		+ REPO_GTEST_S3_REGION
		+ " genStash "
		+ database + " "
		+ project + " "
		+ type;
}

static std::string produceGetFileArgs(
	const std::string &file,
	const std::string &database,
	const std::string &project,
	const std::string &dbAdd = REPO_GTEST_DBADDRESS,
	const int         &port = REPO_GTEST_DBPORT,
	const std::string &username = REPO_GTEST_DBUSER,
	const std::string &password = REPO_GTEST_DBPW
	)
{
	return  getClientExePath() + " " + dbAdd + " "
		+ std::to_string(port) + " "
		+ username + " "
		+ password + " "
		+ REPO_GTEST_S3_BUCKET + " "
		+ REPO_GTEST_S3_REGION
		+ " getFile "
		+ database + " "
		+ project + " \""
		+ file + "\"";
}

static std::string produceCreateFedArgs(
	const std::string &file,
	const std::string &owner = std::string(),
	const std::string &dbAdd = REPO_GTEST_DBADDRESS,
	const int         &port = REPO_GTEST_DBPORT,
	const std::string &username = REPO_GTEST_DBUSER,
	const std::string &password = REPO_GTEST_DBPW
	)
{
	return  getClientExePath() + " " + dbAdd + " "
		+ std::to_string(port) + " "
		+ username + " "
		+ password + " "
		+ REPO_GTEST_S3_BUCKET + " "
		+ REPO_GTEST_S3_REGION
		+ " genFed \""
		+ file + "\" "
		+ owner;
}

static std::string produceUploadFileArgs(
	const std::string &filePath,
	const std::string &dbAdd = REPO_GTEST_DBADDRESS,
	const int         &port = REPO_GTEST_DBPORT,
	const std::string &username = REPO_GTEST_DBUSER,
	const std::string &password = REPO_GTEST_DBPW)
{
	return  getClientExePath() + " " + dbAdd + " "
		+ std::to_string(port) + " "
		+ username + " "
		+ password + " "
		+ REPO_GTEST_S3_BUCKET + " "
		+ REPO_GTEST_S3_REGION
		+ " import -f \""
		+ filePath + "\"";
}

static std::string produceUploadArgs(
	const std::string &dbAdd,
	const int         &port,
	const std::string &username,
	const std::string &password,
	const std::string &database,
	const std::string &project,
	const std::string &bucketName,
	const std::string &bucketRegion,
	const std::string &filePath)
{
	return  getClientExePath() + " " + dbAdd + " "
		+ std::to_string(port) + " "
		+ username + " "
		+ password + " "
		+ bucketName + " "
		+ bucketRegion
		+ " import \""
		+ filePath + "\" "
		+ database + " " + project;
}

static std::string produceUploadArgs(
	const std::string &dbAdd,
	const int         &port,
	const std::string &username,
	const std::string &password,
	const std::string &database,
	const std::string &project,
	const std::string &filePath)
{
	return produceUploadArgs(dbAdd, port,
		username, password,
		database, project,
		REPO_GTEST_S3_BUCKET, REPO_GTEST_S3_REGION,
		filePath);
}

static std::string produceUploadArgs(
	const std::string &dbAdd,
	const int         &port,
	const std::string &database,
	const std::string &project,
	const std::string &filePath)
{
	return produceUploadArgs(dbAdd, port,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW,
		database, project, filePath);
}

static std::string produceUploadArgs(
	const std::string &username,
	const std::string &password,
	const std::string &database,
	const std::string &project,
	const std::string &filePath)
{
	return produceUploadArgs(REPO_GTEST_DBADDRESS,
		REPO_GTEST_DBPORT,
		username, password,
		database, project, filePath);
}

static std::string produceUploadArgs(
	const std::string &database,
	const std::string &project,
	const std::string &filePath)
{
	return produceUploadArgs(REPO_GTEST_DBADDRESS,
		REPO_GTEST_DBPORT,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW,
		database, project, filePath);
}

static int runProcess(
	const std::string &cmd)
{
	int status = system(cmd.c_str());
#ifndef _WIN32
	//Linux, use WIFEXITED(status) to get the real exit code
	return WEXITSTATUS(status);
#else
	return status;
#endif
}

TEST(RepoClientTest, UploadTestInvalidDBConn)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));

	//Test failing to connect to database
	std::string db = "stUpload";
	std::string failToConnect = produceUploadArgs("invalidAdd", 12345, db, "failConn", getSuccessFilePath());
	EXPECT_EQ((int)REPOERR_AUTH_FAILED, runProcess(failToConnect));
	EXPECT_FALSE(projectExists(db, "failConn"));
}
TEST(RepoClientTest, UploadTestBadDBAuth)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Test Bad authentication
	std::string failToAuth = produceUploadArgs("blah", "blah", db, "failAuth", getSuccessFilePath());
	EXPECT_EQ((int)REPOERR_AUTH_FAILED, runProcess(failToAuth));
	EXPECT_FALSE(projectExists(db, "failAuth"));

}

TEST(RepoClientTest, UploadTestNoFile)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Test Bad FilePath
	std::string badFilePath = produceUploadArgs(db, "failPath", "nonExistentFile.obj");
	EXPECT_EQ((int)REPOERR_MODEL_FILE_READ, runProcess(badFilePath));
	EXPECT_FALSE(projectExists(db, "failPath"));
}

TEST(RepoClientTest, UploadTestBadExt)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Test Bad extension
	std::string badExt = produceUploadArgs(db, "failExt", getDataPath(badExtensionFile));
	EXPECT_EQ((int)REPOERR_FILE_TYPE_NOT_SUPPORTED, runProcess(badExt));
	EXPECT_FALSE(projectExists(db, "failExt"));
}

TEST(RepoClientTest, UploadTestBadFBXVer)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Unsupported FBX version
	std::string unsupportedVersion = produceUploadArgs(db, "failExt", getDataPath(unsupportedFBXVersion));
	EXPECT_EQ((int)REPOERR_UNSUPPORTED_FBX_VERSION, runProcess(unsupportedVersion));
	EXPECT_FALSE(projectExists(db, "failExt"));
}

TEST(RepoClientTest, UploadTestBadArgs)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));

	//Insufficient arguments
	std::string lackArg = getClientExePath() + " " + REPO_GTEST_DBADDRESS + " " + std::to_string(REPO_GTEST_DBPORT) + " "
		+ REPO_GTEST_DBUSER + " " + REPO_GTEST_DBPW + " import " + getSuccessFilePath();
	EXPECT_EQ((int)REPOERR_INVALID_ARG, runProcess(lackArg));
}

TEST(RepoClientTest, UploadTestSuccess)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Test Good Upload
	std::string goodUpload = produceUploadArgs(db, "cube", getSuccessFilePath());
	EXPECT_EQ((int)REPOERR_OK, runProcess(goodUpload));
	EXPECT_TRUE(projectExists(db, "cube"));


	EXPECT_EQ((int)REPOERR_OK, runProcess(produceUploadFileArgs(getDataPath(importSuccess))));
	EXPECT_TRUE(projectExists("testDB", importSuccessPro));
	EXPECT_TRUE(projectSettingsCheck("testDB", importSuccessPro, "owner", "", ""));

	EXPECT_EQ((int)REPOERR_OK, runProcess(produceUploadFileArgs(getDataPath(importSuccess2))));
	EXPECT_TRUE(projectExists("testDB", importSuccessPro2));
	EXPECT_TRUE(projectSettingsCheck("testDB", importSuccessPro2, "owner", "taggg", "desccc"));

}

TEST(RepoClientTest, UploadTestTexture)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";


	//Test Textured Upload
	std::string texUpload = produceUploadArgs(db, "textured", getDataPath(texturedModel));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_MISSING_TEXTURE, runProcess(texUpload));
	EXPECT_TRUE(projectExists(db, "textured"));
}

TEST(RepoClientTest, UploadTestMissingNodes)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Test missing nodes Upload
	std::string misUpload = produceUploadArgs(db, "missing", getDataPath(missingNodesModel));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_MISSING_NODES, runProcess(misUpload));
	EXPECT_TRUE(projectExists(db, "missing"));

}

TEST(RepoClientTest, UploadTestIFC)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";


	//Upload IFCFile
	std::string ifcUpload = produceUploadArgs(db, "ifcTest", getDataPath(ifcModel));
	EXPECT_EQ((int)REPOERR_OK, runProcess(ifcUpload));
	EXPECT_TRUE(projectExists(db, "ifcTest"));
}

TEST(RepoClientTest, UploadTestDGN)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";


	//Upload DGN file
	std::string dgnUpload = produceUploadArgs(db, "dgnTest", getDataPath(dgnModel));
	EXPECT_EQ((int)REPOERR_OK, runProcess(dgnUpload));
	EXPECT_TRUE(projectExists(db, "dgnTest"));
}

TEST(RepoClientTest, UploadTestRVT)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";


	//Upload RVT file
	std::string rvtUpload = produceUploadArgs(db, "rvtTest", getDataPath(rvtModel));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_MISSING_TEXTURE, runProcess(rvtUpload));
	EXPECT_TRUE(projectExists(db, "rvtTest"));

	//Upload RVT file with texture directory set
	std::string texturePath = "REPO_RVT_TEXTURES=" + getDataPath("textures");
	putenv(texturePath.c_str());
	std::string rvtUpload2 = produceUploadArgs(db, "rvtTest2", getDataPath(rvtModel));
	EXPECT_EQ((int)REPOERR_OK, runProcess(rvtUpload2));
	EXPECT_TRUE(projectExists(db, "rvtTest2"));

	//Upload RVT file with no valid 3D view
	std::string rvtUpload3 = produceUploadArgs(db, "rvtTest3", getDataPath(rvtNo3DViewModel));
	EXPECT_EQ((int)REPOERR_VALID_3D_VIEW_NOT_FOUND, runProcess(rvtUpload3));
	EXPECT_FALSE(projectExists(db, "rvtTest3"));

}

TEST(RepoClientTest, UploadTestMissingFieldsInJSON)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//JSON AS argument
	//Empty JSON
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(produceUploadFileArgs(getDataPath(emptyFile))));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(produceUploadFileArgs(getDataPath(importNoFile))));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(produceUploadFileArgs(getDataPath(emptyJSONFile))));
	EXPECT_EQ((int)REPOERR_UNKNOWN_ERR, runProcess(produceUploadFileArgs(getDataPath(importbadDir))));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(produceUploadFileArgs(getDataPath(importbadDir2))));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(produceUploadFileArgs(getDataPath(importNoDatabase))));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(produceUploadFileArgs(getDataPath(importNoDatabase2))));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(produceUploadFileArgs(getDataPath(importNoProject))));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(produceUploadFileArgs(importNoProject2)));

}

TEST(RepoClientTest, UploadTestOwner)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));

	EXPECT_EQ((int)REPOERR_OK, runProcess(produceUploadFileArgs(getDataPath(importNoOwner))));
	EXPECT_TRUE(projectExists("testDB", importNoOwnerPro));
	EXPECT_TRUE(projectSettingsCheck("testDB", importNoOwnerPro, REPO_GTEST_DBUSER, "thisTag", "MyUpload"));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceUploadFileArgs(getDataPath(importNoOwner2))));
	EXPECT_TRUE(projectExists("testDB", importNoOwnerPro2));
	EXPECT_TRUE(projectSettingsCheck("testDB", importNoOwnerPro2, REPO_GTEST_DBUSER, "thisTag", "MyUpload"));
}

	


TEST(RepoClientTest, CreateFedTest)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));

	//Test failing to connect to database
	std::string db = "stFed";
	std::string failToConnect = produceCreateFedArgs("whatever", "", "invalidAdd", 12345);
	EXPECT_EQ((int)REPOERR_AUTH_FAILED, runProcess(failToConnect));

	//Test Bad authentication
	std::string failToAuth = produceCreateFedArgs("whatever", "", REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT, "badUser", "invalidPasswrd2");
	EXPECT_EQ((int)REPOERR_AUTH_FAILED, runProcess(failToAuth));

	//Test Bad FilePath
	std::string badFilePath = produceCreateFedArgs("nonExistentFile.json");
	EXPECT_EQ((int)REPOERR_FED_GEN_FAIL, runProcess(badFilePath));

	//Test Completely empty file
	std::string emptyFilePath = produceCreateFedArgs(getDataPath(emptyFile));
	EXPECT_EQ((int)REPOERR_FED_GEN_FAIL, runProcess(emptyFilePath));

	//Test json file with {}
	std::string empty2FilePath = produceCreateFedArgs(getDataPath(emptyJSONFile));
	EXPECT_EQ((int)REPOERR_FED_GEN_FAIL, runProcess(empty2FilePath));

	//Test json file with no sub projects
	std::string noSPFilePath = produceCreateFedArgs(getDataPath(noSubProjectJSONFile));
	EXPECT_EQ((int)REPOERR_FED_GEN_FAIL, runProcess(noSPFilePath));
	EXPECT_FALSE(projectExists(genFedDB, genFedNoSubProName));

	//Test json file with empty string as database name
	std::string noDBFilePath = produceCreateFedArgs(getDataPath(noDbNameJSONFile));
	EXPECT_EQ((int)REPOERR_FED_GEN_FAIL, runProcess(noDBFilePath));

	//Test json file with empty string as project name
	std::string noProFilePath = produceCreateFedArgs(getDataPath(noProNameJSONFile));
	EXPECT_EQ((int)REPOERR_FED_GEN_FAIL, runProcess(noProFilePath));

	//Test badly formatted JSON file
	std::string invalidJSONFilePath = produceCreateFedArgs(getDataPath(invalidJSONFile));
	EXPECT_EQ((int)REPOERR_FED_GEN_FAIL, runProcess(invalidJSONFilePath));

	//Test success
	std::string goodFilePath = produceCreateFedArgs(getDataPath(validGenFedJSONFile));
	EXPECT_EQ((int)REPOERR_OK, runProcess(goodFilePath));
	EXPECT_TRUE(projectExists(genFedDB, genFedSuccessName));
}

TEST(RepoClientTest, GetFileTest)
{
	EXPECT_EQ((int)REPOERR_GET_FILE_FAILED, runProcess(produceGetFileArgs(".", "nonExistent1", "nonExistent2")));
	EXPECT_EQ((int)REPOERR_GET_FILE_FAILED, runProcess(produceGetFileArgs(".", REPO_GTEST_DBNAME1, "nonExistent2")));

	EXPECT_EQ((int)REPOERR_OK, runProcess(produceGetFileArgs(".", "sampleDataRW", "cube")));
	EXPECT_TRUE(fileExists(getFileFileName));
	EXPECT_TRUE(filesCompare(getFileFileName, getDataPath("cube.obj")));
}

TEST(RepoClientTest, GenStashTest)
{
	repo::RepoController *controller = new repo::RepoController();
	std::string errMsg;
	repo::RepoController::RepoToken *token =
		controller->init(errMsg, REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW, REPO_GTEST_S3_BUCKET, REPO_GTEST_S3_REGION);
	repo::lib::RepoUUID stashRoot;
	if (token)
	{
		auto scene = controller->fetchScene(token, "sampleDataRW", "cube");
		if (scene)
		{
			stashRoot = scene->getRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED)->getUniqueID();
			delete scene;
		}
	}

	EXPECT_EQ((int)REPOERR_OK, runProcess(produceGenStashArgs("sampleDataRW", "cube", "src")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceGenStashArgs("sampleDataRW", "cube", "tree")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceGenStashArgs("sampleDataRW", "cube", "gltf")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceGenStashArgs("sampleDataRW", "cube", "repo")));

	if (token)
	{
		auto scene = controller->fetchScene(token, "sampleDataRW", "cube");
		if (scene)
		{
			EXPECT_NE(scene->getRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED)->getUniqueID(), stashRoot);

			delete scene;
		}
	}

	controller->disconnectFromDatabase(token);
	EXPECT_EQ((int)REPOERR_STASH_GEN_FAIL, runProcess(produceGenStashArgs("blash", "blah", "tree")));
	EXPECT_EQ((int)REPOERR_STASH_GEN_FAIL, runProcess(produceGenStashArgs("blash", "blah", "src")));
	EXPECT_EQ((int)REPOERR_STASH_GEN_FAIL, runProcess(produceGenStashArgs("blash", "blah", "gltf")));

	delete controller;
}

TEST(RepoClientTest, CleanTest)
{
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceCleanArgs("sampleDataRW", "cleanTest1")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceCleanArgs("sampleDataRW", "cleanTest2")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceCleanArgs("sampleDataRW", "cleanTest3")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceCleanArgs("sampleDataRW", "cleanTest4")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceCleanArgs("sampleDataRW", "cleanTest5")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceCleanArgs("sampleDataRW", "nonExistentbadfsd")));

	EXPECT_FALSE(projectHasValidRevision("sampleDataRW", "cleanTest1"));
	EXPECT_FALSE(projectHasValidRevision("sampleDataRW", "cleanTest2")); // FIXME: change dump?
	EXPECT_FALSE(projectHasValidRevision("sampleDataRW", "cleanTest3")); // FIXME: change dump?
	EXPECT_FALSE(projectHasValidRevision("sampleDataRW", "cleanTest4")); // FIXME: change dump?
	EXPECT_TRUE(projectHasValidRevision("sampleDataRW", "cleanTest5"));
}
