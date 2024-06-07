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
#include <functional>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

static const std::string FBX_EXTENSION = ".FBX";

static const std::string cmdGenStash = "genStashAll";   //Generate stashes for all databases
static const std::string cmdGenStashSpecific = "genStash";   //Generate stashes for a specific database
static const std::string cmdTestConn = "test";   //test the connection
static const std::string cmdTestImport = "testImport";   //get version
static const std::string cmdVersion = "version";   //get version
static const std::string cmdVersion2 = "-v";   //get version

std::string helpInfo()
{
	std::stringstream ss;
	ss << cmdTestImport << "\tRun file import for the given file (no saving to database, no optimisation). (args: <file to import>)\n";
	ss << cmdGenStash << "\tGenerate Stash for all databases. (args: [repo|gltf|src|tree])\n";
	ss << cmdGenStashSpecific << "\tGenerate Stash for specific list of databases. (args: <file with list of databases> [repo|gltf|src|tree])\n";
	ss << cmdTestConn << "\t\tTest the client and database connection is working. (args: none)\n";
	ss << cmdVersion << "[-v]\tPrints the version of Repo Bouncer Client/Library\n";

	return ss.str();
}

bool isSpecialCommand(const std::string &cmd)
{
	return cmd == cmdVersion || cmd == cmdVersion2;
}

bool noConnectionRequired(const std::string &cmd) {
	return cmd == cmdTestImport;
}

int32_t knownValid(const std::string &cmd)
{
	if (cmd == cmdGenStashSpecific)
		return 2;
	if (cmd == cmdGenStash)
		return 1;
	if (cmd == cmdTestConn)
		return 0;
	if (cmd == cmdTestImport)
		return 1;
	if (cmd == cmdVersion || cmd == cmdVersion2)
		return 0;
	return -1;
}

int32_t performOperation(

	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
)
{
	int32_t errCode = REPOERR_UNKNOWN_CMD;
	if (command.command == cmdGenStash || command.command == cmdGenStashSpecific)
	{
		try {
			errCode = generateStash(controller, token, command);
		}
		catch (const std::exception &e)
		{
			repoLogError("Failed to generate optimised stash: " + std::string(e.what()));
			errCode = REPOERR_UNKNOWN_ERR;
		}
	}
	else if (command.command == cmdTestConn)
	{
		//This is just to test if the client is working and if the connection is working
		//if we got a token from the controller we can assume that it worked.
		errCode = token ? REPOERR_OK : REPOERR_AUTH_FAILED;
	}
	else if (command.command == cmdTestImport)
	{
		try {
			errCode = runImport(controller, command);
		}
		catch (const std::exception &e)
		{
			repoLogError("Failed to import file: " + std::string(e.what()));
			errCode = REPOERR_UNKNOWN_ERR;
		}
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
* ======================== Command helper functions ===================
*/

static bool genRepoStash(std::shared_ptr<repo::RepoController> controller, const repo::RepoController::RepoToken *token, repo::core::model::RepoScene *scene)
{
	return controller->generateAndCommitRepoBundlesBuffer(token, scene);
}

static bool genSrcStash(std::shared_ptr<repo::RepoController> controller, const repo::RepoController::RepoToken *token, repo::core::model::RepoScene *scene)
{
	return controller->generateAndCommitSRCBuffer(token, scene);
}

static bool genGLTFStash(std::shared_ptr<repo::RepoController> controller, const repo::RepoController::RepoToken *token, repo::core::model::RepoScene *scene)
{
	return controller->generateAndCommitGLTFBuffer(token, scene);
}

static bool genSelectionTree(std::shared_ptr<repo::RepoController> controller, const repo::RepoController::RepoToken *token, repo::core::model::RepoScene *scene)
{
	return controller->generateAndCommitSelectionTree(token, scene);
}

static std::list<std::string> readListFromFile(const std::string &fileLoc)
{
	std::ifstream inputS;
	std::list<std::string> databaseList;

	inputS.open(fileLoc);

	if (inputS.is_open())
	{
		while (!inputS.eof())
		{
			std::string db;
			inputS >> db;
			databaseList.push_back(db);
		}
		inputS.close();
	}
	else
	{
		repoLogError("Failed to open file for list of database: " + fileLoc);
	}

	return databaseList;
}
/*
* ======================== Command functions ===================
*/

int32_t generateStash(
	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
)
{
	std::list<std::string> databaseList;
	std::string type;
	/*
	* Check the amount of parameters matches
	*/
	if (command.command == cmdGenStash)
	{
		if (command.nArgcs < 1)
		{
			repoLogError("Number of arguments mismatch! " + cmdGenStash
				+ " requires 1 arguments: [repo|gltf|src|tree]");
			return REPOERR_INVALID_ARG;
		}
		else
		{
			type = command.args[0];
			databaseList = controller->getDatabases(token);
		}
	}
	else
	{
		if (command.nArgcs < 2)
		{
			repoLogError("Number of arguments mismatch! " + cmdGenStashSpecific
				+ " requires 2 arguments: <file with list of database> [repo|gltf|src|tree]");
			return REPOERR_INVALID_ARG;
		}
		else
		{
			type = command.args[1];
			databaseList = readListFromFile(command.args[0]);
		}
	}

	if (!(type == "repo" || type == "gltf" || type == "src" || type == "tree"))
	{
		repoLogError("Unknown stash type: " + type);
		return REPOERR_INVALID_ARG;
	}

	std::function<bool(std::shared_ptr<repo::RepoController> , const repo::RepoController::RepoToken*, repo::core::model::RepoScene*)> genFn;
	bool stashOnly = false;

	if (type == "repo")
	{
		genFn = genRepoStash;
		stashOnly = false;
	}
	else if (type == "gltf")
	{
		genFn = genGLTFStash;
		stashOnly = true;
	}
	else if (type == "src")
	{
		genFn = genSrcStash;
		stashOnly = true;
	}
	else if (type == "tree")
	{
		genFn = genSelectionTree;
		stashOnly = false;
	}

	int nDBs = databaseList.size();

	int dbCount = 0;
	int tolCount = 0;
	int failCount = 0;
	for (const auto database : databaseList)
	{
		auto dbEntry = controller->getDatabasesWithProjects(token, { database });
		repoLog("Processing database " + database + "(" + std::to_string(++dbCount) + "/" + std::to_string(nDBs) + ")...");
		if (!dbEntry.size())
		{
			repoLog(database + " have no projects, skipping...");
			continue;
		}

		auto projects = dbEntry.begin()->second;
		repoLog("#projects in this database: " + std::to_string(projects.size()));

		int projCount = 0;
		int nProjects = projects.size();

		for (const auto &project : projects)
		{
			repoLog("\tProcessing " + database + "." + project + "(" + std::to_string(++projCount) + "/" + std::to_string(nProjects) + ")...");
			auto revs = controller->getAllFromCollectionContinuous(token, database, project + ".history");
			for (const auto &rev : revs)
			{
				repo::core::model::RevisionNode node(rev);
				if (node.getUploadStatus() == repo::core::model::RevisionNode::UploadStatus::COMPLETE)
				{
					try {
						repo::core::model::RepoScene *scene = controller->fetchScene(token, database, project, node.getUniqueID().toString(), false, stashOnly, true);
						if (scene->getAllReferences(repo::core::model::RepoScene::GraphType::DEFAULT).size()) break; //This project is a federation
						if (scene)
						{
							auto functionRes = genFn(controller, token, scene);
							repoLog("\t\t" + node.getUniqueID().toString() + " : " + (functionRes ? "OK" : "FAILED"));
							if (!functionRes) ++failCount;

							delete scene;
						}
					}
					catch (const std::exception &e)
					{
						repoLog("\t\t" + node.getUniqueID().toString() + " : FAILED (" + e.what() + ")");
						++failCount;
					}

					++tolCount;
				}
			}
		}
	}

	repoLog("Stash generation completed. " + std::to_string(tolCount) + " processed, " + std::to_string(failCount) + " failed");

	return failCount == 0 ? REPOERR_OK : REPOERR_STASH_GEN_FAIL;
}

int32_t runImport(
	std::shared_ptr<repo::RepoController> controller,
	const repo_op_t            &command
)
{
	if (command.nArgcs < 1)
	{
		repoLogError("Number of arguments mismatch! " + cmdGenStash
			+ " requires 1 arguments: <path to file>");
		return REPOERR_INVALID_ARG;
	}

	auto file = command.args[0];
	uint8_t err;
	controller->loadSceneFromFile(file, err);

	return err;
}