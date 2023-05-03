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
#include <algorithm>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

static const std::string FBX_EXTENSION = ".FBX";

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

	ss << cmdGenStash << "\tGenerate Stash for a project. (args: database project [repo|gltf|src|tree] [all|revId])\n";
	ss << cmdGetFile << "\t\tGet original file for the latest revision of the project (args: database project dir)\n";
	ss << cmdImportFile << "\t\tImport file to database. (args: {file database project [dxrotate] [owner] [configfile]} or {-f parameterFile} )\n";
	ss << cmdCreateFed << "\t\tGenerate a federation. (args: fedDetails [owner])\n";
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
	if (cmd == cmdGetFile)
		return 3;
	if (cmd == cmdTestConn)
		return 0;
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

	if (command.command == cmdImportFile)
	{
		try {
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
		try {
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
		try {
			errCode = generateFederation(controller, token, command);
		}
		catch (const std::exception &e)
		{
			repoLogError("Failed to generate federation: " + std::string(e.what()));
			errCode = REPOERR_UNKNOWN_ERR;
		}
	}
	else if (command.command == cmdGetFile)
	{
		try {
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

int32_t generateFederation(
	std::shared_ptr<repo::RepoController> controller,
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
	repo::lib::RepoUUID revId = repo::lib::RepoUUID::createUUID();
	if (command.nArgcs > 1)
		owner = command.args[1];

	repoLog("Federation configuration file: " + fedFile);

	bool  success = false;
	uint8_t errCode = REPOERR_FED_GEN_FAIL;

	boost::property_tree::ptree jsonTree;
	try {
		boost::property_tree::read_json(fedFile, jsonTree);

		const std::string database = jsonTree.get<std::string>("database", "");
		const std::string project = jsonTree.get<std::string>("project", "");
		auto revIdStr = jsonTree.get<std::string>("revId", "");
		if (!revIdStr.empty()) {
			revId = repo::lib::RepoUUID(revIdStr);
		}
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
					errCode = controller->commitScene(token, scene, owner, "", "", revId);
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

	return success ? REPOERR_OK : errCode;
}

bool _generateStash(
	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken      *token,
	const std::string            &type,
	const std::string            &dbName,
	const std::string            &project,
	const bool                   isBranch,
	const std::string            &revID) {
	repoLog("Generating stash of type " + type + " for " + dbName + "." + project + " rev: " + revID + (isBranch ? " (branch ID)" : ""));
	auto scene = controller->fetchScene(token, dbName, project, revID, isBranch, false, true, type == "tree");
	bool  success = false;
	if (scene) {
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

		delete scene;
	}

	return success;
}

int32_t generateStash(
	std::shared_ptr<repo::RepoController> controller,
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

	bool branch = true;
	std::string revId = REPO_HISTORY_MASTER_BRANCH;
	if (command.nArgcs > 3) {
		revId = command.args[3];
		branch = false;
	}

	bool success = true;
	std::string revToLower = revId;
	std::transform(revToLower.begin(), revToLower.end(), revToLower.begin(), ::tolower);
	if (revToLower == "all")
	{
		auto revs = controller->getAllFromCollectionContinuous(token, dbName, project + ".history");
		for (const auto &rev : revs) {
			auto revNode = (const repo::core::model::RevisionNode) rev;
			auto revId = revNode.getUniqueID();
			success &= _generateStash(controller, token, type, dbName, project, false, revId.toString());
		}
	}
	else {
		success = _generateStash(controller, token, type, dbName, project, branch, revId);
	}

	return success ? REPOERR_OK : REPOERR_STASH_GEN_FAIL;
}

int32_t getFileFromProject(
	std::shared_ptr<repo::RepoController> controller,
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

	bool success = controller->saveOriginalFiles(token, dbName, project, dir);

	return success ? REPOERR_OK : REPOERR_GET_FILE_FAILED;
}

repo::manipulator::modelconvertor::ModelUnits determineUnits(const std::string &units) {
	if (units == "m") return repo::manipulator::modelconvertor::ModelUnits::METRES;
	if (units == "cm") return repo::manipulator::modelconvertor::ModelUnits::CENTIMETRES;
	if (units == "mm") return repo::manipulator::modelconvertor::ModelUnits::MILLIMETRES;
	if (units == "dm") return repo::manipulator::modelconvertor::ModelUnits::DECIMETRES;
	if (units == "ft") return repo::manipulator::modelconvertor::ModelUnits::FEET;

	return repo::manipulator::modelconvertor::ModelUnits::UNKNOWN;
}

int32_t importFileAndCommit(
	std::shared_ptr<repo::RepoController> controller,
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

	std::string timeZone;
	std::string fileLoc;
	std::string database;
	std::string project;
	std::string  owner, tag, desc, units;
	repo::lib::RepoUUID revId = repo::lib::RepoUUID::createUUID();
	double simplificationQuality;
	int simplificationMinVertexCount;

	bool success = true;
	bool rotate = false;
	bool importAnimations = true;
	if (usingSettingFiles)
	{
		//if we're using settings file then arg[1] must be file path
		boost::property_tree::ptree jsonTree;
		try {
			boost::property_tree::read_json(command.args[1], jsonTree);

			database = jsonTree.get<std::string>("database", "");
			project = jsonTree.get<std::string>("project", "");

			owner = jsonTree.get<std::string>("owner", "");
			tag = jsonTree.get<std::string>("tag", "");
			desc = jsonTree.get<std::string>("desc", "");
			timeZone = jsonTree.get<std::string>("timezone", "");
			units = jsonTree.get<std::string>("units", "");
			simplificationQuality = jsonTree.get<double>("quality", 0.0);
			simplificationMinVertexCount = jsonTree.get<int>("vertexCount", 0.0);

			rotate = jsonTree.get<bool>("dxrotate", rotate);
			importAnimations = jsonTree.get<bool>("importAnimations", importAnimations);
			fileLoc = jsonTree.get<std::string>("file", "");
			auto revIdStr = jsonTree.get<std::string>("revId", "");
			if (!revIdStr.empty()) {
				revId = repo::lib::RepoUUID(revIdStr);
			}

			if (database.empty() || project.empty() || fileLoc.empty())
			{
				return REPOERR_LOAD_SCENE_FAIL;
			}
		}
		catch (std::exception &e)
		{
			return REPOERR_LOAD_SCENE_FAIL;
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
			}
		}
	}

	boost::filesystem::path filePath(fileLoc);
	std::string fileExt = filePath.extension().string();
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);
	rotate |= fileExt == FBX_EXTENSION;

	auto targetUnits = repo::manipulator::modelconvertor::ModelUnits::UNKNOWN;

	if (!units.empty()) {
		targetUnits = determineUnits(units);
	}

	//FIXME: This is getting complicated, we should consider using boost::program_options and start utilising flags...
	//Something like this: http://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options

	repoLog("File: " + fileLoc + " database: " + database
		+ " project: " + project + " target units: " + (units.empty() ? "none" : units) 
		+ " simplification: " + (simplificationQuality > 0 ? ("(quality " + std::to_string(simplificationQuality)) + ", min vertex count " + std::to_string(simplificationMinVertexCount) + ")" : "off")
		+ " rotate: " + (rotate ? "true" : "false")
		+ " owner :" + owner + " importAnimations: " + (importAnimations ? "true" : "false"));

	repo::manipulator::modelconvertor::ModelImportConfig config(true, rotate, importAnimations, targetUnits, simplificationQuality, simplificationMinVertexCount, timeZone);
	uint8_t err;
	repo::core::model::RepoScene *graph = controller->loadSceneFromFile(fileLoc, err, config);
	if (graph)
	{
		repoLog("Trying to commit this scene to database as " + database + "." + project);
		graph->setDatabaseAndProjectName(database, project);

		err = controller->commitScene(token, graph, owner, tag, desc, revId);

		if (err == REPOERR_OK)
		{
			if (graph->isMissingNodes())
			{
				repoLog("Missing nodes detected!");
				return REPOERR_LOAD_SCENE_MISSING_NODES;
			}
			else if (graph->isMissingTexture())
			{
				repoLog("Missing texture detected!");
				return REPOERR_LOAD_SCENE_MISSING_TEXTURE;
			}
			else
				return err;
		}
	}
	return err ? err : REPOERR_LOAD_SCENE_FAIL;
}