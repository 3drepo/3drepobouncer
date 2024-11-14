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
#include <repo/core/model/bson/repo_bson_builder.h>
#include <unordered_set>

using namespace testing;

static std::string getSuccessFilePath()
{
	return getDataPath(simpleModel);
}

static std::string produceCleanArgs(
	const std::string& database,
	const std::string& project,
	const std::string& dbAdd = REPO_GTEST_DBADDRESS,
	const int& port = REPO_GTEST_DBPORT,
	const std::string& username = REPO_GTEST_DBUSER,
	const std::string& password = REPO_GTEST_DBPW
)
{
	return  getClientExePath() + " "
		+ getConnConfig()
		+ " clean "
		+ database + " "
		+ project;
}

static std::string produceGenStashArgs(
	const std::string& database,
	const std::string& project,
	const std::string& type
)
{
	return  getClientExePath() + " "
		+ getConnConfig()
		+ " genStash "
		+ database + " "
		+ project + " "
		+ type;
}

static std::string produceGetFileArgs(
	const std::string& file,
	const std::string& database,
	const std::string& project
)
{
	return  getClientExePath() + " "
		+ getConnConfig()
		+ " getFile "
		+ database + " "
		+ project + " \""
		+ file + "\"";
}

static std::string produceCreateFedArgs(
	const std::string& file,
	const std::string& owner = std::string()
)
{
	return  getClientExePath() + " "
		+ getConnConfig()
		+ " genFed \""
		+ file + "\" "
		+ owner;
}

static std::string produceUploadFileArgs(
	const std::string& filePath
) {
	return  getClientExePath() + " "
		+ getConnConfig()
		+ " import -f \""
		+ filePath + "\"";
}

static std::string produceProcessDrawingArgs(
	const std::string& filePath
)
{
	return  getClientExePath() + " "
		+ getConnConfig()
		+ " processDrawing \""
		+ filePath + "\"";
}

static std::string produceUploadArgs(
	const std::string& database,
	const std::string& project,
	const std::string& filePath,
	const std::string& configPath = getConnConfig())
{
	return  getClientExePath()
		+ " " + configPath
		+ " import \""
		+ filePath + "\" "
		+ database + " " + project;
}

static int runProcess(
	const std::string& cmd)
{
	int status = system(cmd.c_str());
#ifndef _WIN32
	//Linux, use WIFEXITED(status) to get the real exit code
	return WEXITSTATUS(status);
#else
	return status;
#endif
}

static int testUpload(
	std::string mongoDbName,
	std::string projectName,
	std::string fileName
)
{
	std::string uploadCmd = produceUploadArgs(
		mongoDbName,
		projectName,
		getDataPath(fileName));

	int errCode = runProcess(uploadCmd);
	;
	repoInfo << "Error code from bouncer client: " << errCode
		<< ", " << (int)errCode;
	return errCode;
};

TEST(RepoClientTest, UploadTestInvalidDBConn)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));

	//Test failing to connect to database
	std::string db = "stUpload";
	std::string failToConnect = produceUploadArgs(db, "failConn", getSuccessFilePath(), getDataPath("config/invalidAdd.json"));
	EXPECT_EQ((int)REPOERR_AUTH_FAILED, runProcess(failToConnect));
	EXPECT_FALSE(projectExists(db, "failConn"));
}

TEST(RepoClientTest, UploadTestBadDBAuth)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Test Bad authentication
	std::string failToAuth = produceUploadArgs(db, "failAuth", getSuccessFilePath(), getDataPath("config/badAuth.json"));
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
	fflush(stdout);
	std::cout << " Executing: " << badFilePath << std::endl;
	fflush(stdout);
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
	std::string lackArg = getClientExePath() + " " + getConnConfig() + " import " + getSuccessFilePath();
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
	std::cout << " Running Missing nodes... " << misUpload << std::endl;
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_MISSING_NODES, runProcess(misUpload));
	EXPECT_TRUE(projectExists(db, "missing"));
}

TEST(RepoClientTest, UploadTestBIM)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));

	std::string mongoDbName = "stUpload";

	std::string spoofedBim1PrjName = "spoofedBIM1Test";
	EXPECT_EQ(REPOERR_UNSUPPORTED_BIM_VERSION,
		testUpload(mongoDbName, spoofedBim1PrjName, "RepoModelImport/cube_bim1_spoofed.bim"));
	EXPECT_FALSE(projectExists(mongoDbName, spoofedBim1PrjName));

	std::string okBim2PrjName = "okBIM2Test";
	EXPECT_EQ(REPOERR_OK,
		testUpload(mongoDbName, okBim2PrjName, "RepoModelImport/cube_bim2_navis_2021_repo_4.6.1.bim"));
	EXPECT_TRUE(projectExists(mongoDbName, okBim2PrjName));

	std::string okBim3PrjName = "okBIM3Test";
	EXPECT_EQ(REPOERR_OK,
		testUpload(mongoDbName, okBim3PrjName, "RepoModelImport/BrickWalls_bim3.bim"));
	EXPECT_TRUE(projectExists(mongoDbName, okBim3PrjName));

	std::string okBim4PrjName = "okBIM4Test";
	EXPECT_EQ(REPOERR_OK,
		testUpload(mongoDbName, okBim4PrjName, "RepoModelImport/wall_section_bim4.bim"));
	EXPECT_TRUE(projectExists(mongoDbName, okBim4PrjName));

	std::string corrTxtrBim3PrjName = "corruptedTextureBIM3Test";
	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_TEXTURE,
		testUpload(mongoDbName, corrTxtrBim3PrjName, "RepoModelImport/BrickWalls_bim3_CorruptedTextureField.bim"));
	EXPECT_TRUE(projectExists(mongoDbName, corrTxtrBim3PrjName));

	std::string corrMatBim3PrjName = "corruptedMaterialBIM3Test";
	EXPECT_EQ(REPOERR_LOAD_SCENE_MISSING_NODES,
		testUpload(mongoDbName, corrMatBim3PrjName, "RepoModelImport/BrickWalls_bim3_MissingTexture.bim"));
	EXPECT_TRUE(projectExists(mongoDbName, corrMatBim3PrjName));
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

	//Upload IFCFile
	std::string ifcUploadReg = produceUploadArgs(db, "ifcTestRegression", getDataPath(ifcModel_InfiniteLoop));
	EXPECT_EQ((int)REPOERR_OK, runProcess(ifcUploadReg));
	EXPECT_TRUE(projectExists(db, "ifcTestRegression"));

	std::string ifc4Upload = produceUploadArgs(db, "ifc4Test", getDataPath(ifc4Model));
	EXPECT_EQ((int)REPOERR_OK, runProcess(ifc4Upload));
	EXPECT_TRUE(projectExists(db, "ifc4Test"));
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

TEST(RepoClientTest, UploadTestDWG)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Upload DWG file
	std::string dwgUpload = produceUploadArgs(db, "dwgTest", getDataPath(dwgModel));
	EXPECT_EQ((int)REPOERR_OK, runProcess(dwgUpload));
	EXPECT_TRUE(projectExists(db, "dwgTest"));

	// This snippet tests the behaviour of Block References and nested Block
	// References in the tree.
	std::string dwgUploadNestedBlocks = produceUploadArgs(db, "dwgTestNestedBlocks", getDataPath(dwgNestedBlocks));
	EXPECT_EQ((int)REPOERR_OK, runProcess(dwgUploadNestedBlocks));
	EXPECT_TRUE(projectHasMetaNodesWithPaths(db, "dwgTestNestedBlocks", "Entity Handle::Value", "[423]", { "rootNode->0->Block Text->Block Text" }));
	EXPECT_TRUE(projectHasMetaNodesWithPaths(db, "dwgTestNestedBlocks", "Entity Handle::Value", "[50D]", { "rootNode->0->My Block->My Block", "rootNode->Layer1->My Block->My Block" }));
	EXPECT_TRUE(projectHasMetaNodesWithPaths(db, "dwgTestNestedBlocks", "Entity Handle::Value", "[4FA]", { "rootNode->0->My Outer Block->My Outer Block", "rootNode->Layer1->My Outer Block->My Outer Block", "rootNode->Layer3->My Outer Block->My Outer Block" }));
	EXPECT_FALSE(projectHasGeometryWithMetadata(db, "dwgTestNestedBlocks", "Entity Handle::Value", "[534]")); // Even though this handle exists, it should be compressed in the tree.

	// This snippet checks if encrypted files are handled correctly.
	std::string dwgUploadProtected = produceUploadArgs(db, "dwgTestProtected", getDataPath(dwgPasswordProtected));
	EXPECT_EQ(REPOERR_FILE_IS_ENCRYPTED, runProcess(dwgUploadProtected));
}

TEST(RepoClientTest, UploadTestDXF)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Upload XF file
	std::string dxfUpload = produceUploadArgs(db, "dxfTest", getDataPath(dxfModel));
	EXPECT_EQ((int)REPOERR_OK, runProcess(dxfUpload));
	EXPECT_TRUE(projectExists(db, "dxfTest"));
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

	//Linux putenv takes in a char* instead of const char* - need a copy of the const char*
	char* texturePathEnv = new char[texturePath.size() + 1];
	strncpy(texturePathEnv, texturePath.c_str(), texturePath.size() + 1);

	putenv(texturePathEnv);
	std::string rvtUpload2 = produceUploadArgs(db, "rvtTest2", getDataPath(rvtModel));
	EXPECT_EQ((int)REPOERR_OK, runProcess(rvtUpload2));
	EXPECT_TRUE(projectExists(db, "rvtTest2"));

	//Upload RVT file with no valid 3D view
	std::string rvtUpload3 = produceUploadArgs(db, "rvtTest3", getDataPath(rvtNo3DViewModel));
	EXPECT_EQ((int)REPOERR_VALID_3D_VIEW_NOT_FOUND, runProcess(rvtUpload3));
	EXPECT_FALSE(projectExists(db, "rvtTest3"));
}

TEST(RepoClientTest, UploadTestRVT2021)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Upload RVT file
	std::string rvtUpload = produceUploadArgs(db, "rvtTest2021", getDataPath(rvtModel2021));
	EXPECT_EQ((int)REPOERR_OK, runProcess(rvtUpload));
	EXPECT_TRUE(projectExists(db, "rvtTest2021"));
}

TEST(RepoClientTest, UploadTestRVT2022)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Upload RVT file
	std::string rvtUpload = produceUploadArgs(db, "rvtTest2022", getDataPath(rvtModel2022));
	EXPECT_EQ((int)REPOERR_OK, runProcess(rvtUpload));
	EXPECT_TRUE(projectExists(db, "rvtTest2022"));
}

TEST(RepoClientTest, UploadTestNWD2022)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Upload NWD file
	std::string nwdUpload = produceUploadArgs(db, "nwdTest2022", getDataPath(nwdModel2022));
	EXPECT_EQ((int)REPOERR_OK, runProcess(nwdUpload));
	EXPECT_TRUE(projectExists(db, "nwdTest2022"));
}

TEST(RepoClientTest, UploadTestRVT2024)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Upload RVT file
	std::string rvtUpload = produceUploadArgs(db, "rvtTest2024", getDataPath(rvtModel2024));
	EXPECT_EQ((int)REPOERR_OK, runProcess(rvtUpload));
	EXPECT_TRUE(projectExists(db, "rvtTest2024"));
}

TEST(RepoClientTest, UploadTestNWD2024)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Upload NWD file
	std::string nwdUpload = produceUploadArgs(db, "nwdTest2024", getDataPath(nwdModel2024));
	EXPECT_EQ((int)REPOERR_OK, runProcess(nwdUpload));
	EXPECT_TRUE(projectExists(db, "nwdTest2024"));
}

TEST(RepoClientTest, UploadTestNWDProtected)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//Upload password-protected NWD
	std::string nwdUpload = produceUploadArgs(db, "nwdPasswordProtected", getDataPath(nwdPasswordProtected));
	EXPECT_EQ((int)REPOERR_FILE_IS_ENCRYPTED, runProcess(nwdUpload));
	EXPECT_FALSE(projectExists(db, "nwdPasswordProtected"));
}

TEST(RepoClientTest, UploadTestNWC)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";
	std::string project = "nwcTest";

	std::string nwcUpload = produceUploadArgs(db, project, getDataPath(nwcModel));
	EXPECT_EQ((int)REPOERR_OK, runProcess(nwcUpload));
	EXPECT_TRUE(projectExists(db, project));
}

TEST(RepoClientTest, UploadTestSPM)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	//commenting out 6.2 as we don't support this right now and it will crash.
	/*std::string spmUpload = produceUploadArgs(db, "synchroTest", getDataPath(synchroFile));
	EXPECT_EQ((int)REPOERR_OK, runProcess(spmUpload));
	EXPECT_TRUE(projectExists(db, "synchroTest"));*/

	//we don't support 6.1 - this will currently crash
	/*std::string spmUpload2 = produceUploadArgs(db, "synchroTest2", getDataPath(synchroOldVersion));
	EXPECT_EQ((int)REPOERR_UNSUPPORTED_VERSION, runProcess(spmUpload2));
	EXPECT_FALSE(projectExists(db, "synchroTest2"));*/

	//we also support 6.3
	std::string spmUpload3 = produceUploadArgs(db, "synchroTest3", getDataPath(synchroVersion6_3));
	EXPECT_EQ((int)REPOERR_OK, runProcess(spmUpload3));
	EXPECT_TRUE(projectExists(db, "synchroTest3"));

	std::string spmUpload4 = produceUploadArgs(db, "synchroTest4", getDataPath(synchroVersion6_4));
	EXPECT_EQ((int)REPOERR_OK, runProcess(spmUpload4));
	EXPECT_TRUE(projectExists(db, "synchroTest4"));

	std::string spmUpload5 = produceUploadArgs(db, "synchroTest5", getDataPath(synchroVersion6_5));
	EXPECT_EQ((int)REPOERR_OK, runProcess(spmUpload5));
	EXPECT_TRUE(projectExists(db, "synchroTest5"));

	std::string spmUpload6 = produceUploadArgs(db, "synchroTest6", getDataPath(synchroVersion6_5_3_7));
	EXPECT_EQ((int)REPOERR_OK, runProcess(spmUpload6));
	EXPECT_TRUE(projectExists(db, "synchroTest6"));
}

TEST(RepoClientTest, UploadTestRVTRegressionTests)
{
	ASSERT_TRUE(system(nullptr));
	std::string db = "stUpload";

	// Regression tests for fixed bugs
	std::string rvtUpload4 = produceUploadArgs(db, "rvtTest4", getDataPath(rvtRoofTest));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_MISSING_TEXTURE, runProcess(rvtUpload4));
	EXPECT_TRUE(projectExists(db, "rvtTest4"));

	std::string rvtUpload5 = produceUploadArgs(db, "rvtTest5", getDataPath(rvtMeta1));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_MISSING_TEXTURE, runProcess(rvtUpload5));
	EXPECT_TRUE(projectExists(db, "rvtTest5"));

	std::string rvtUpload6 = produceUploadArgs(db, "rvtTest6", getDataPath(rvtMeta2));
	EXPECT_EQ((int)REPOERR_OK, runProcess(rvtUpload6));
	EXPECT_TRUE(projectExists(db, "rvtTest6"));

	std::string rvtUpload7 = produceUploadArgs(db, "rvtTest7", getDataPath(rvtHouse));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_MISSING_TEXTURE, runProcess(rvtUpload7));
	EXPECT_TRUE(projectExists(db, "rvtTest7"));

	// Check if the metadata applied to geometric elements connected to MEP
	// systems is correct
	std::string rvtUpload8 = produceUploadArgs(db, "rvtTest8", getDataPath(rvtMeta3));
	EXPECT_EQ((int)REPOERR_OK, runProcess(rvtUpload8));
	EXPECT_TRUE(projectExists(db, "rvtTest8"));
	// In rvtMeta3, some of these elements belong to systems, and others do not
	EXPECT_TRUE(projectHasGeometryWithMetadata(db, "rvtTest8", "Element ID", "702167"));
	EXPECT_TRUE(projectHasGeometryWithMetadata(db, "rvtTest8", "Element ID", "702041"));
	EXPECT_TRUE(projectHasGeometryWithMetadata(db, "rvtTest8", "Element ID", "706118"));
	EXPECT_TRUE(projectHasGeometryWithMetadata(db, "rvtTest8", "Element ID", "706347"));
	EXPECT_TRUE(projectHasGeometryWithMetadata(db, "rvtTest8", "Element ID", "703971"));
	EXPECT_TRUE(projectHasGeometryWithMetadata(db, "rvtTest8", "Element ID", "704116"));
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
	EXPECT_EQ((int)REPOERR_MODEL_FILE_READ, runProcess(produceUploadFileArgs(getDataPath(importbadDir))));
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
	EXPECT_TRUE(projectSettingsCheck("testDB", importNoOwnerPro, "ANONYMOUS USER", "thisTag", "MyUpload"));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceUploadFileArgs(getDataPath(importNoOwner2))));
	EXPECT_TRUE(projectExists("testDB", importNoOwnerPro2));
	EXPECT_TRUE(projectSettingsCheck("testDB", importNoOwnerPro2, "ANONYMOUS USER", "thisTag", "MyUpload"));
}

TEST(RepoClientTest, CreateFedTest)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));

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

TEST(RepoClientTest, GenStashTest)
{
	repo::RepoController* controller = new repo::RepoController();
	std::string errMsg;
	repo::RepoController::RepoToken* token = initController(controller);
	repo::lib::RepoUUID stashRoot;

	EXPECT_EQ((int)REPOERR_OK, runProcess(produceUploadArgs("genStashTest", "cube", getDataPath(simpleModel))));
	EXPECT_TRUE(projectExists("genStashTest", "cube"));

	auto handler = getHandler();

	// Drop collections from the existing imports so we know any changes are due to this test.

	std::string error;
	handler->dropCollection("genStashTest", "cube.stash.repobundles", error);
	handler->dropCollection("genStashTest", "cube.stash.repobundles.ref", error);
	handler->dropCollection("genStashTest", "cube.stash.json_mpc.ref", error);

	EXPECT_EQ((int)REPOERR_OK, runProcess(produceGenStashArgs("genStashTest", "cube", "repo")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceGenStashArgs("genStashTest", "cube", "src")));
	EXPECT_EQ((int)REPOERR_OK, runProcess(produceGenStashArgs("genStashTest", "cube", "tree")));

	std::unordered_set<std::string> collections;
	for (auto& name : handler->getCollections("genStashTest"))
	{
		collections.insert(name);
	}

	EXPECT_TRUE(collections.find("cube.stash.src.ref") != collections.end());
	EXPECT_TRUE(collections.find("cube.stash.repobundles") != collections.end());
	EXPECT_TRUE(collections.find("cube.stash.repobundles.ref") != collections.end());
	EXPECT_TRUE(collections.find("cube.stash.json_mpc.ref") != collections.end());

	// Now check for the tree

	repo::core::model::RepoBSONBuilder revisionCriteria;
	revisionCriteria.append("type", "revision");

	auto revisions = handler->findAllByCriteria("genStashTest", "cube.history", revisionCriteria.obj());

	EXPECT_EQ(revisions.size(), 1);

	auto revisionId = revisions[0].getUUIDField("_id").toString();
	repo::core::model::RepoBSONBuilder builder;
	builder.append("_id", revisionId + "/fulltree.json");
	auto documents = handler->findAllByCriteria("genStashTest", "cube.stash.json_mpc.ref", builder.obj());
	EXPECT_EQ(documents.size(), 1);

	delete controller;
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
		true,
		err
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

	// And via the node directly
	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_LABEL_ID, revision.getUUIDField("image"));
	auto criteria = builder.obj();

	auto refNode = handler->findOneByCriteria(
		db,
		REPO_COLLECTION_DRAWINGS + std::string(".ref"),
		criteria
	);

	EXPECT_EQ(refNode.getStringField("type"), "fs");
	EXPECT_EQ(refNode.getStringField("name"), "test.svg");
	EXPECT_EQ(refNode.getStringField("mimeType"), "image/svg+xml");
	EXPECT_EQ(refNode.getUUIDField("project"), project);
	EXPECT_EQ(refNode.getStringField("model"), model);
	EXPECT_EQ(refNode.getUUIDField("rev_id"), rid);
}