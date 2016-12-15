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

#include <repo/core/model/bson/repo_bson_factory.h>

#include <sstream>
#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

static const std::string FBX_EXTENSION = ".FBX";

static const std::string cmdCleanProj = "clean"; //clean up a specified project
static const std::string cmdCreateFed = "genFed"; //create a federation
static const std::string cmdGenStash = "genStash";   //test the connection
static const std::string cmdGetFile = "getFile"; //download original file
static const std::string cmdImportFile = "import"; //file import
static const std::string cmdTestConn = "test";   //test the connection
static const std::string cmdVersion = "version";   //get version
static const std::string cmdVersion2 = "-v";   //get version

std::string helpInfo()
{
	std::stringstream ss;

	ss << cmdGenStash << "\tGenerate Stash for a project. (args: database project [repo|gltf|src|tree])\n";
	ss << cmdGetFile << "\t\tGet original file for the latest revision of the project (args: database project dir)\n";
	ss << cmdImportFile << "\t\tImport file to database. (args: {file database project [dxrotate] [owner] [configfile]} or {-f parameterFile} )\n";
	ss << cmdCreateFed << "\t\tGenerate a federation. (args: fedDetails [owner])\n";
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
		return 2;
	if (cmd == cmdGenStash)
		return 3;
	if (cmd == cmdCreateFed)
		return 1;
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
	else if (command.command == cmdCreateFed)
	{
		try{
			errCode = generateFederation(controller, token, command);
		}
		catch (const std::exception &e)
		{
			repoLogError("Failed to generate federation: " + std::string(e.what()));
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

int32_t generateFederation(
	repo::RepoController       *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	)
{
	/*
	* Check the amount of parameters matches
	*/
	if (command.nArgcs < 1)
	{
		repoLogError("Number of arguments mismatch! " + cmdCreateFed
			+ " requires 1 argument: fedDetails");
		return REPOERR_INVALID_ARG;
	}

	std::string fedFile = command.args[0];
	std::string owner;
	if (command.nArgcs > 1)
		owner = command.args[1];

	repoLog("Federation configuration file: " + fedFile);

	bool  success = false;

	boost::property_tree::ptree jsonTree;
	try{
		boost::property_tree::read_json(fedFile, jsonTree);

		const std::string database = jsonTree.get<std::string>("database", "");
		const std::string project = jsonTree.get<std::string>("project", "");
		std::map< repo::core::model::TransformationNode, repo::core::model::ReferenceNode> refMap;

		if (database.empty() || project.empty())
		{
			repoLogError("Failed to generate federation: database name(" + database + ") or project name("
				+ project + ") was not specified!");
		}
		else
		{
			for (const auto &subPro : jsonTree.get_child("subProjects"))
			{
				// ===== Get project info =====
				const std::string spDatabase = subPro.second.get<std::string>("database", database);
				const std::string spProject = subPro.second.get<std::string>("project", "");
				const std::string uuid = subPro.second.get<std::string>("revId", REPO_HISTORY_MASTER_BRANCH);
				const bool isRevID = subPro.second.get<bool>("isRevId", false);
				if (spProject.empty())
				{
					repoLogError("Failed to generate federation: One or more sub projects does not have a project name");
					break;
				}

				// ===== Get Transformation =====
				std::vector<std::vector<float>> matrix;
				int x = 0;
				if (subPro.second.count("transformation"))
				{
					for (const auto &value : subPro.second.get_child("transformation"))
					{
						if (!(matrix.size() % 4))
						{
							matrix.push_back(std::vector<float>());
						}
						matrix.back().push_back(value.second.get_value<float>());
						++x;
					}
				}

				if (x != 16)
				{
					//no matrix/invalid input, assume identity
					if (x)
						repoLogError("Transformation was inserted for " + spDatabase + ":" + spProject
						+ " but it is not a 4x4 matrix(size found: " + std::to_string(x) + "). Using identity...");
					matrix = repo::core::model::TransformationNode::identityMat();
				}

				std::string nodeNames = spDatabase + ":" + spProject;
				auto transNode = repo::core::model::RepoBSONFactory::makeTransformationNode(matrix, nodeNames);
				auto refNode = repo::core::model::RepoBSONFactory::makeReferenceNode(spDatabase, spProject, repo::lib::RepoUUID(uuid), isRevID, nodeNames);
				refMap[transNode] = refNode;
			}

			//Create the reference scene
			if (success = refMap.size())
			{
				auto scene = controller->createFederatedScene(refMap);
				if (success = scene)
				{
					scene->setDatabaseAndProjectName(database, project);
					success = controller->commitScene(token, scene, owner);
				}
			}
			else
			{
				repoLogError("Failed to generate federation: Invalid/no sub-project declared");
			}
		}
	}
	catch (std::exception &e)
	{
		success = false;
		repoLogError("Failed to generate Federation: " + std::string(e.what()));
	}

	return success ? REPOERR_OK : REPOERR_FED_GEN_FAIL;
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
			+ " requires 3 arguments:database project [repo|gltf|src|tree]");
		return REPOERR_INVALID_ARG;
	}

	std::string dbName = command.args[0];
	std::string project = command.args[1];
	std::string type = command.args[2];

	if (!(type == "repo" || type == "gltf" || type == "src" || type == "tree"))
	{
		repoLogError("Unknown stash type: " + type);
		return REPOERR_INVALID_ARG;
	}

	auto scene = controller->fetchScene(token, dbName, project);
	if (!scene)
	{
		return REPOERR_STASH_GEN_FAIL;
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
	else if (type == "tree")
	{
		success = controller->generateAndCommitSelectionTree(token, scene);
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

	bool success  = controller->saveOriginalFiles(token, dbName, project, dir);

	return success ? REPOERR_OK : REPOERR_GET_FILE_FAILED;
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
	bool usingSettingFiles = command.nArgcs == 2 && std::string(command.args[0]) == "-f";
	if (command.nArgcs < 3 && !usingSettingFiles)
	{
		repoLogError("Number of arguments mismatch! " + cmdImportFile
			+ " requires 3 arguments: file database project [dxrotate] [owner] [config file] or 2 arguments: -f <path to json settings>");
		return REPOERR_INVALID_ARG;
	}

	std::string fileLoc;
	std::string database;
	std::string project;
	std::string configFile, owner, tag, desc;

	bool success = true;
	bool rotate = false;
	if (usingSettingFiles)
	{
		//if we're using settles file then arg[1] must be file path
		boost::property_tree::ptree jsonTree;
		try{
			boost::property_tree::read_json(command.args[1], jsonTree);

			database = jsonTree.get<std::string>("database", "");
			project = jsonTree.get<std::string>("project", "");

			if (database.empty() || project.empty())
			{
				success = false;
			}

			owner = jsonTree.get<std::string>("owner", "");
			configFile = jsonTree.get<std::string>("configfile", "");
			tag = jsonTree.get<std::string>("tag", "");
			desc = jsonTree.get<std::string>("desc", "");
			rotate = jsonTree.get<bool>("dxrotate", rotate);
			fileLoc = jsonTree.get<std::string>("file", "");
		}
		catch (std::exception &e)
		{
			success = false;
			repoLogError("Failed to import file: " + std::string(e.what()));
		}
	}
	else
	{
		fileLoc = command.args[0];
		database = command.args[1];
		project = command.args[2];

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
	}

	boost::filesystem::path filePath(fileLoc);
	std::string fileExt = filePath.extension().string();
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);
	rotate |= fileExt == FBX_EXTENSION;

	//FIXME: This is getting complicated, we should consider using boost::program_options and start utilising flags...
	//Something like this: http://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options

	repoLog("File: " + fileLoc + " database: " + database
		+ " project: " + project + " rotate:"
		+ (rotate ? "true" : "false") + " owner :" + owner + " configFile: " + configFile);

	repo::manipulator::modelconvertor::ModelImportConfig config(configFile);

	repo::core::model::RepoScene *graph = controller->loadSceneFromFile(fileLoc, true, rotate, &config);
	if (graph)
	{
		repoLog("Trying to commit this scene to database as " + database + "." + project);
		graph->setDatabaseAndProjectName(database, project);

		if (controller->commitScene(token, graph, owner, tag, desc))
		{
			if (graph->isMissingTexture())
			{
				repoLog("Missing texture detected!");
				return REPOERR_LOAD_SCENE_MISSING_TEXTURE;
			}
			else if (graph->isMissingNodes())
			{
				repoLog("Missing nodes detected!");
				return REPOERR_LOAD_SCENE_MISSING_NODES;
			}
			else
				return REPOERR_OK;
		}
	}

	return REPOERR_LOAD_SCENE_FAIL;
}