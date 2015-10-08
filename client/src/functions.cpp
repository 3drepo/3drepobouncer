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

static const std::string cmdImportFile = "import"; //file import
static const std::string cmdTestConn   = "test";   //test the connection


std::string helpInfo()
{
	std::stringstream ss;

	ss << cmdImportFile << "\t\tImport file to database. (args: file database project [dxrotate] [owner] [configfile])\n";
	ss << cmdTestConn << "\t\tTest the client and database connection is working. (args: none)\n";

	return ss.str();
}

int32_t knownValid(const std::string &cmd)
{
	if (cmd == cmdImportFile)
		return 3;
	if (cmd == cmdTestConn)
		return 0;
	return -1;
}

bool performOperation(
	repo::RepoController *controller,
	const repo::RepoToken      *token,
	const repo_op_t            &command
	)
{
	if (command.command == cmdImportFile)
	{
		bool success = false;
		
		try{

			success = importFileAndCommit(controller, token, command);
		}
		catch (const std::exception &e)
		{
			repoLogError("Failed to import and commit file: " + std::string(e.what()));
		}

		return success;
		
	}
	else if (command.command == cmdTestConn)
	{
		//This is just to test if the client is working and if the connection is working
		//if we got a token from the controller we can assume that it worked.
		return token;
	}


	repoLogError("Unrecognised command: " + command.command + ". Type --help for info");
	return false;
}

/*
* ======================== Command functions ===================
*/


bool importFileAndCommit(
	repo::RepoController *controller,
	const repo::RepoToken      *token,
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
		return false;
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
		return true;
	}

	return false;


}