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

#include "functions.h"

#include <sstream>

static const std::string cmdCleanProj = "clean"; //clean up a specified project
static const std::string cmdGenStash = "genStash";   //test the connection
static const std::string cmdGetFile = "getFile"; //download original file
static const std::string cmdImportFile = "import"; //file import
static const std::string cmdTestConn = "test";   //test the connection
static const std::string cmdVersion = "version";   //get version
static const std::string cmdVersion2 = "-v";   //get version

std::string helpInfo()
{
	std::stringstream ss;

	ss << cmdGenStash << "\tGenerate Stash for a project. (args: database project [repo|gltf|src])\n";
	ss << cmdGetFile << "\t\tGet original file for the latest revision of the project (args: database project dir)\n";
	ss << cmdImportFile << "\t\tImport file to database. (args: file database project [dxrotate] [owner] [configfile])\n";
	ss << cmdCleanProj << "\t\tClean up a specified project removing/repairing corrupted revisions. (args: database project)\n";
	ss << cmdTestConn << "\t\tTest the client and database connection is working. (args: none)\n";
	ss << cmdVersion << "[-v]\tPrints the version of Repo Bouncer Client/Library\n";

	return ss.str();
}

bool isSpecialCommand(const std::string &cmd)
{
	return cmd == cmdVersion || cmd == cmdVersion2;
}

int32_t knownValid(const std::string &cmd)
{
	if (cmd == cmdImportFile)
		return 3;
	if (cmd == cmdGenStash)
		return 3;
	if (cmd == cmdCleanProj)
		return 2;
	if (cmd == cmdGetFile)
		return 3;
	if (cmd == cmdTestConn)
		return 0;
	if (cmd == cmdVersion || cmd == cmdVersion2)
		return 0;
	return -1;
}

int32_t performOperation(

	repo::RepoController *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	)
{
	int32_t errCode = REPOERR_UNKNOWN_CMD;

	if (command.command == cmdImportFile)
	{
		try{
			errCode = importFileAndCommit(controller, token, command);
		}
		catch (const std::exception &e)
		{
			repoLogError("Failed to import and commit file: " + std::string(e.what()));
			errCode = REPOERR_UNKNOWN_ERR;
		}
	}
	else if (command.command == cmdGenStash)
	{
		try{
			errCode = generateStash(controller, token, command);
		}
		catch (const std::exception &e)
		{
			repoLogError("Failed to generate optimised stash: " + std::string(e.what()));
			errCode = REPOERR_UNKNOWN_ERR;
		}
	}
	else if (command.command == cmdCleanProj)
	{
		try{
			errCode = cleanUpProject(controller, token, command);
		}
		catch (const std::exception &e)
		{
			repoLogError("Failed to generate optimised stash: " + std::string(e.what()));
			errCode = REPOERR_UNKNOWN_ERR;
		}
	}
	else if (command.command == cmdGetFile)
	{
		try{
			errCode = getFileFromProject(controller, token, command);
		}
		catch (const std::exception &e)
		{
			repoLogError("Failed to retrieve file from project: " + std::string(e.what()));
			errCode = REPOERR_UNKNOWN_ERR;
		}
	}
	else if (command.command == cmdTestConn)
	{
		//This is just to test if the client is working and if the connection is working
		//if we got a token from the controller we can assume that it worked.
		return token ? REPOERR_OK : REPOERR_AUTH_FAILED;
	}
	else if (command.command == cmdVersion || command.command == cmdVersion2)
	{
		std::cout << "3D Repo Bouncer Client v" + controller->getVersion() << std::endl;
		errCode = REPOERR_OK;
	}
	else
		repoLogError("Unrecognised command: " + command.command + ". Type --help for info");

	return errCode;
}

/*
* ======================== Command functions ===================
*/

int32_t cleanUpProject(
	repo::RepoController       *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	)
{
	/*
	* Check the amount of parameters matches
	*/
	if (command.nArgcs < 2)
	{
		repoLogError("Number of arguments mismatch! " + cmdCleanProj
			+ " requires 2 arguments:database project");
		return REPOERR_INVALID_ARG;
	}

	std::string dbName = command.args[0];
	std::string project = command.args[1];

	controller->cleanUp(token, dbName, project);

	return REPOERR_OK;
}

int32_t generateStash(
	repo::RepoController       *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	)
{
	/*
	* Check the amount of parameters matches
	*/
	if (command.nArgcs < 3)
	{
		repoLogError("Number of arguments mismatch! " + cmdGenStash
			+ " requires 3 arguments:database project [repo|gltf|src]");
		return REPOERR_INVALID_ARG;
	}

	std::string dbName = command.args[0];
	std::string project = command.args[1];
	std::string type = command.args[2];

	if (!(type == "repo" || type == "gltf" || type == "src"))
	{
		repoLogError("Unknown stash type: " + type);
		return REPOERR_INVALID_ARG;
	}

	auto scene = controller->fetchScene(token, dbName, project);
	if (!scene)
	{
		return REPOERR_LOAD_SCENE_FAIL;
	}

	bool  success = false;

	if (type == "repo")
	{
		success = controller->generateAndCommitStashGraph(token, scene);
	}
	else if (type == "gltf")
	{
		success = controller->generateAndCommitGLTFBuffer(token, scene);
	}
	else if (type == "src")
	{
		success = controller->generateAndCommitSRCBuffer(token, scene);
	}

	return success ? REPOERR_OK : REPOERR_STASH_GEN_FAIL;
}

int32_t getFileFromProject(
	repo::RepoController       *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	)
{
	/*
	* Check the amount of parameters matches
	*/
	if (command.nArgcs < 3)
	{
		repoLogError("Number of arguments mismatch! " + cmdGenStash
			+ " requires 3 arguments:database project dir");
		return REPOERR_INVALID_ARG;
	}

	std::string dbName = command.args[0];
	std::string project = command.args[1];
	std::string dir = command.args[2];

	controller->saveOriginalFiles(token, dbName, project, dir);

	return REPOERR_OK;
}

int32_t importFileAndCommit(
	repo::RepoController *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	)
{
	/*
	* Check the amount of parameters matches
	*/
	if (command.nArgcs < 3)
	{
		repoLogError("Number of arguments mismatch! " + cmdImportFile
			+ " requires 3 arguments: file database project [dxrotate] [owner] [config file]");
		return REPOERR_INVALID_ARG;
	}

	std::string fileLoc = command.args[0];
	std::string database = command.args[1];
	std::string project = command.args[2];
	std::string configFile;
	std::string owner;
	bool rotate = false;

	//FIXME: This is getting complicated, we should consider using boost::program_options and start utilising flags...
	//Something like this: http://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options
	if (command.nArgcs > 3)
	{
		//If 3rd argument is "dxrotate", we need to rotate the X axis
		//Otherwise the user is trying to name the owner, rotate is false.
		std::string arg3 = command.args[3];
		if (arg3 == "dxrotate")
		{
			rotate = true;
		}
		else
		{
			owner = command.args[3];
		}
	}
	if (command.nArgcs > 4)
	{
		//If the last argument is rotate, this is owner
		//otherwise this is configFile (confusing, I know.)
		if (rotate)
		{
			owner = command.args[4];
			if (command.nArgcs > 5)
			{
				configFile = command.args[5];
			}
		}
		else
		{
			configFile = command.args[4];
		}
	}

	repoLogDebug("File: " + fileLoc + " database: " + database
		+ " project: " + project + " rotate:"
		+ (rotate ? "true" : "false") + " owner :" + owner + " configFile: " + configFile);

	repo::manipulator::modelconvertor::ModelImportConfig config(configFile);

	repo::core::model::RepoScene *graph = controller->loadSceneFromFile(fileLoc, &config);
	if (graph)
	{
		repoLog("Trying to commit this scene to database as " + database + "." + project);
		graph->setDatabaseAndProjectName(database, project);
		if (rotate)
			graph->reorientateDirectXModel();
		if (owner.empty())
			controller->commitScene(token, graph);
		else
			controller->commitScene(token, graph, owner);
		//FIXME: should make commitscene return a boolean even though GUI doesn't care...
		return REPOERR_OK;
	}

	return REPOERR_LOAD_SCENE_FAIL;
}