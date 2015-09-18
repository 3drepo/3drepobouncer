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

static const std::string cmdImportFile = "import";


std::string helpInfo()
{
	std::stringstream ss;

	ss << cmdImportFile << "\t\tImport file to database. (args: file database project [configfile])";

	return ss.str();
}

int32_t knownValid(const std::string &cmd)
{
	if (cmd == cmdImportFile)
		return 3;

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
		return importFileAndCommit(controller, token, command);
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
			+ " requires 3 arguments: file database project [");
		return false;
	}

	std::string fileLoc = command.args[0];
	std::string database = command.args[1];
	std::string project = command.args[2];
	std::string configFile;
	if (command.nArgcs > 3)
	{
		configFile = command.args[3];
	}


	repo::manipulator::modelconvertor::ModelImportConfig config(configFile);






	repo::core::model::RepoScene *graph = controller->loadSceneFromFile(fileLoc, &config);
	if (graph)
	{
		repoLog("Trying to commit this scene to database as " + database + "." + project);

		graph->setDatabaseAndProjectName(database, project);

		controller->commitScene(token, graph);
		//FIXME: should make commitscene return a boolean even though GUI doesn't care...
		return true;
	}

	return false;


}