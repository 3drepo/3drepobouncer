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
#include <error_codes.h>
#include <repo/repo_controller.h>
#include "../unit/repo_test_database_info.h"

static std::string getSuccessFilePath()
{
	return getDataPath(simpleModel);
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
	return  getClientExePath() + " " + dbAdd + " "
		+ std::to_string(port) + " "
		+ username + " "
		+ password
		+ " import "
		+ filePath + " "
		+ database + " " + project;
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

bool projectExists(
	const std::string &db,
	const std::string &project)
{
	bool res = false;
	repo::RepoController *controller = new repo::RepoController();
	std::string errMsg;
	repo::RepoController::RepoToken *token =
		controller->authenticateToAdminDatabaseMongo(errMsg, REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);
	if (token)
	{
		std::list<std::string> dbList;
		dbList.push_back(db);
		auto dbMap = controller->getDatabasesWithProjects(token, dbList);
		auto dbMapIt = dbMap.find(db);
		if (dbMapIt != dbMap.end())
		{
			std::list<std::string> projects = dbMapIt->second;
			res = std::find(projects.begin(), projects.end(), project) != projects.end();
		}
	}
	controller->disconnectFromDatabase(token);
	delete controller;
	return res;
}

TEST(RepoClientTest, UploadTest)
{
	//this ensures we can run processes
	ASSERT_TRUE(system(nullptr));

	//Test failing to connect to database
	std::string db = "stUpload";
	std::string failToConnect = produceUploadArgs("invalidAdd", 12345, db, "failConn", getSuccessFilePath());
	EXPECT_EQ((int)REPOERR_AUTH_FAILED, runProcess(failToConnect));
	EXPECT_FALSE(projectExists(db, "failConn"));

	//Test Bad authentication
	std::string failToAuth = produceUploadArgs("blah", "blah", db, "failAuth", getSuccessFilePath());
	EXPECT_EQ((int)REPOERR_AUTH_FAILED, runProcess(failToAuth));
	EXPECT_FALSE(projectExists(db, "failAuth"));

	//Test Bad FilePath
	std::string badFilePath = produceUploadArgs(db, "failPath", "nonExistentFile.obj");
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(badFilePath));
	EXPECT_FALSE(projectExists(db, "failPath"));

	//Test Bad extension
	std::string badExt = produceUploadArgs(db, "failExt", getDataPath(badExtensionFile));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(badExt));
	EXPECT_FALSE(projectExists(db, "failExt"));

	//Insufficient arguments
	std::string lackArg = getClientExePath() + " " + REPO_GTEST_DBADDRESS + " " + std::to_string(REPO_GTEST_DBPORT) + " "
		+ REPO_GTEST_DBUSER + " " + REPO_GTEST_DBPW + " import " + getSuccessFilePath();
	EXPECT_EQ((int)REPOERR_INVALID_ARG, runProcess(lackArg));

	//Test Good Upload
	std::string goodUpload = produceUploadArgs(db, "cube", getSuccessFilePath());
	EXPECT_EQ((int)REPOERR_OK, runProcess(goodUpload));
	EXPECT_TRUE(projectExists(db, "cube"));

	//Test Textured Upload
	std::string texUpload = produceUploadArgs(db, "textured", getDataPath(texturedModel));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_MISSING_TEXTURE, runProcess(texUpload));
	EXPECT_TRUE(projectExists(db, "textured"));
}