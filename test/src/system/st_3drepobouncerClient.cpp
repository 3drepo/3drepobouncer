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
#include <boost/filesystem.hpp>
#include <error_codes.h>
#include "../unit/repo_test_database_info.h"

const static std::string clientExe = "3drepobouncerClient";
const static std::string simpleModel = "cube.obj";
const static std::string badExtensionFile = "cube.exe";
const static std::string texturedModel = "texturedPlane.dae";

static std::string getClientExePath()
{
	char* path = getenv("REPO_CLIENT_PATH");
	std::string returnPath = clientExe;
	if (path)
	{
		boost::filesystem::path fileDir(path);
		auto fullPath = fileDir / boost::filesystem::path(clientExe);
		returnPath = fullPath.string();
	}
	return returnPath;
}

static std::string getDataPath(
	const std::string &file)
{
	char* path = getenv("REPO_MODEL_PATH");
	std::string returnPath = simpleModel;
	if (path)
	{
		boost::filesystem::path fileDir(path);
		auto fullPath = fileDir / boost::filesystem::path(file);
		returnPath = fullPath.string();
	}
	return returnPath;
}

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
	//Linux, use WIFEXITED(status) as for some reason it wasn't returning the right code
	return WEXITSTATUS(status);

#else
	return status;
#endif
}

TEST(RepoClientTest, UploadTest)
{
	ASSERT_TRUE(system(nullptr));

	//Test failing to connect to database
	std::string failToConnect = produceUploadArgs("invalidAdd", 12345, "stUpload", "failEg", getSuccessFilePath());
	EXPECT_EQ((int)REPOERR_AUTH_FAILED, runProcess(failToConnect));

	//Test Bad authentication
	std::string failToAuth = produceUploadArgs("blah", "blah", "stUpload", "failEg", getSuccessFilePath());
	EXPECT_EQ((int)REPOERR_AUTH_FAILED, runProcess(failToAuth));

	//Test Bad FilePath
	std::string badFilePath = produceUploadArgs("stUpload", "failEg", "nonExistentFile.obj");
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(badFilePath));

	//Test Bad FilePath
	std::string badExt = produceUploadArgs("stUpload", "failEg", getDataPath(badExtensionFile));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_FAIL, runProcess(badExt));

	//Insufficient arguments
	std::string lackArg = getClientExePath() + " " + REPO_GTEST_DBADDRESS + " " + std::to_string(REPO_GTEST_DBPORT) + " "
		+ REPO_GTEST_DBUSER + " " + REPO_GTEST_DBPW + " import " + getSuccessFilePath();
	EXPECT_EQ((int)REPOERR_INVALID_ARG, runProcess(lackArg));

	//Test Good Upload
	std::string goodUpload = produceUploadArgs("stUpload", "cube", getSuccessFilePath());
	EXPECT_EQ((int)REPOERR_OK, runProcess(goodUpload));

	//Test Textured Upload
	std::string texUpload = produceUploadArgs("stUpload", "textured", getDataPath(texturedModel));
	EXPECT_EQ((int)REPOERR_LOAD_SCENE_MISSING_TEXTURE, runProcess(texUpload));
}