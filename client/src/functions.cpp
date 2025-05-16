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

#include <repo/core/model/bson/repo_bson.h>
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
static const std::string cmdImportFile = "import"; //file import
static const std::string cmdProcessDrawing = "processDrawing"; //drawing import from revision node
static const std::string cmdTestConn = "test";   //test the connection
static const std::string cmdVersion = "version";   //get version
static const std::string cmdVersion2 = "-v";   //get version

std::string helpInfo()
{
	std::stringstream ss;

	ss << cmdGenStash << "\tGenerate Stash for a project. (args: database project [repo|src|tree] [all|revId])\n";
	ss << cmdImportFile << "\t\tImport file to database. (args: {file database project [dxrotate] [owner] [configfile]} or {-f parameterFile} )\n";
	ss << cmdProcessDrawing << "\t\tProcess drawing revision node into an image. (args: parameterFile)\n";
	ss << cmdCreateFed << "\t\tGenerate a federation. (args: fedDetails [owner])\n";
	ss << cmdTestConn << "\t\tTest the client and database connection is working. (args: none)\n";
	ss << cmdVersion << "[-v]\tPrints the version of Repo Bouncer Client/Library\n";

	return ss.str();
}

bool isSpecialCommand(const std::string& cmd)
{
	return cmd == cmdVersion || cmd == cmdVersion2;
}

int32_t knownValid(const std::string& cmd)
{
	if (cmd == cmdImportFile)
		return 2;
	if (cmd == cmdProcessDrawing)
		return 1;
	if (cmd == cmdGenStash)
		return 3;
	if (cmd == cmdCreateFed)
		return 1;
	if (cmd == cmdTestConn)
		return 0;
	if (cmd == cmdVersion || cmd == cmdVersion2)
		return 0;
	return -1;
}

int32_t performOperation(

	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken* token,
	const repo_op_t& command
)
{
	int32_t errCode = REPOERR_UNKNOWN_CMD;

	if (command.command == cmdImportFile)
	{
		try {
			errCode = importFileAndCommit(controller, token, command);
		}
		catch (const repo::lib::RepoException& e) {
			throw; // RepoExceptions have additional context as well as an embedded return code, so let these bubble up
		}
		catch (const std::exception& e)
		{
			repoLogError("Failed to import and commit file: " + std::string(e.what()));
			errCode = REPOERR_UNKNOWN_ERR;
		}
	}
	else if (command.command == cmdProcessDrawing)
	{
		try {
			errCode = processDrawing(controller, token, command);
		}
		catch (const std::exception& e)
		{
			repoLogError("Failed to process drawing: " + std::string(e.what()));
			errCode = REPOERR_UNKNOWN_ERR;
		}
	}
	else if (command.command == cmdGenStash)
	{
		try {
			errCode = generateStash(controller, token, command);
		}
		catch (const std::exception& e)
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
		catch (const std::exception& e)
		{
			repoLogError("Failed to generate federation: " + std::string(e.what()));
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
	const repo::RepoController::RepoToken* token,
	const repo_op_t& command
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
		std::map< repo::core::model::ReferenceNode, std::string> refMap;

		if (database.empty() || project.empty())
		{
			repoLogError("Failed to generate federation: database name(" + database + ") or project name("
				+ project + ") was not specified!");
		}
		else
		{
			for (const auto& subPro : jsonTree.get_child("subProjects"))
			{
				// ===== Get project info =====
				const std::string spDatabase = subPro.second.get<std::string>("database", database);
				const std::string spProject = subPro.second.get<std::string>("project", "");
				const std::string group = subPro.second.get<std::string>("group", "");
				const std::string uuid = subPro.second.get<std::string>("revId", REPO_HISTORY_MASTER_BRANCH);
				const bool isRevID = subPro.second.get<bool>("isRevId", false);
				if (spProject.empty())
				{
					repoLogError("Failed to generate federation: One or more sub projects does not have a project name");
					break;
				}

				auto refNode = repo::core::model::RepoBSONFactory::makeReferenceNode(spDatabase, spProject, repo::lib::RepoUUID(uuid), isRevID);
				refMap[refNode] = group;
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
	catch (std::exception& e)
	{
		success = false;
		repoLogError("Failed to generate Federation: " + std::string(e.what()));
	}

	return success ? REPOERR_OK : errCode;
}

bool _generateStash(
	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken* token,
	const std::string& type,
	const std::string& dbName,
	const std::string& project,
	const bool                   isBranch,
	const std::string& revID) {
	repoLog("Generating stash of type " + type + " for " + dbName + "." + project + " rev: " + revID + (isBranch ? " (branch ID)" : ""));
	auto scene = controller->fetchScene(token, dbName, project, revID, isBranch, false, type == "tree");
	bool  success = false;
	if (scene) {
		if (type == "repo")
		{
			controller->updateRevisionStatus(scene, repo::core::model::ModelRevisionNode::UploadStatus::GEN_WEB_STASH);
			success = controller->generateAndCommitRepoBundlesBuffer(token, scene);
		}
		else if (type == "tree")
		{
			controller->updateRevisionStatus(scene, repo::core::model::ModelRevisionNode::UploadStatus::GEN_SEL_TREE);
			success = controller->generateAndCommitSelectionTree(token, scene);
		}

		if (success)
		{
			controller->updateRevisionStatus(scene, repo::core::model::ModelRevisionNode::UploadStatus::COMPLETE);
		}

		delete scene;
	}

	return success;
}

int32_t generateStash(
	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken* token,
	const repo_op_t& command
)
{
	/*
	* Check the amount of parameters matches
	*/
	if (command.nArgcs < 3)
	{
		repoLogError("Number of arguments mismatch! " + cmdGenStash
			+ " requires 3 arguments:database project [repo|tree]");
		return REPOERR_INVALID_ARG;
	}

	std::string dbName = command.args[0];
	std::string project = command.args[1];
	std::string type = command.args[2];

	if (!(type == "repo" || type == "tree"))
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
		for (const auto& rev : revs) {
			auto revId = repo::core::model::RepoNode(rev).getUniqueID();
			success &= _generateStash(controller, token, type, dbName, project, false, revId.toString());
		}
	}
	else {
		success = _generateStash(controller, token, type, dbName, project, branch, revId);
	}

	return success ? REPOERR_OK : REPOERR_STASH_GEN_FAIL;
}

repo::manipulator::modelconvertor::ModelUnits determineUnits(const std::string& units) {
	if (units == "m") return repo::manipulator::modelconvertor::ModelUnits::METRES;
	if (units == "cm") return repo::manipulator::modelconvertor::ModelUnits::CENTIMETRES;
	if (units == "mm") return repo::manipulator::modelconvertor::ModelUnits::MILLIMETRES;
	if (units == "dm") return repo::manipulator::modelconvertor::ModelUnits::DECIMETRES;
	if (units == "ft") return repo::manipulator::modelconvertor::ModelUnits::FEET;

	return repo::manipulator::modelconvertor::ModelUnits::UNKNOWN;
}

int32_t importFileAndCommit(
	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken* token,
	const repo_op_t& command
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

	repo::manipulator::modelconvertor::ModelImportConfig config;

	config.revisionId = repo::lib::RepoUUID::createUUID(); // If the caller does not specify one

	std::string fileLoc;
	std::string  owner, tag, desc;

	bool success = true;
	bool rotate = false;
	if (usingSettingFiles)
	{
		//if we're using settings file then arg[1] must be file path
		boost::property_tree::ptree jsonTree;
		try {
			boost::property_tree::read_json(command.args[1], jsonTree);

			fileLoc = jsonTree.get<std::string>("file", "");
			owner = jsonTree.get<std::string>("owner", "");
			tag = jsonTree.get<std::string>("tag", "");
			desc = jsonTree.get<std::string>("desc", "");

			config.databaseName = jsonTree.get<std::string>("database", "");
			config.projectName = jsonTree.get<std::string>("project", "");
			config.timeZone = jsonTree.get<std::string>("timezone", config.timeZone);
			config.targetUnits = determineUnits(jsonTree.get<std::string>("units", ""));
			config.lod = jsonTree.get<int>("lod", config.lod);
			config.importAnimations = jsonTree.get<bool>("importAnimations", config.importAnimations);
			config.viewName = jsonTree.get<std::string>("view", config.viewName);
			auto revIdStr = jsonTree.get<std::string>("revId", "");
			if (!revIdStr.empty()) {
				config.revisionId = repo::lib::RepoUUID(revIdStr);
			}
			config.numThreads = jsonTree.get<int>("numThreads", config.numThreads);

			if (config.databaseName.empty() || config.projectName.empty() || fileLoc.empty())
			{
				return REPOERR_LOAD_SCENE_FAIL;
			}
		}
		catch (std::exception& e)
		{
			return REPOERR_LOAD_SCENE_FAIL;
		}
	}
	else
	{
		fileLoc = command.args[0];
		config.databaseName = command.args[1];
		config.projectName = command.args[2];

		if (command.nArgcs > 3)
		{
			owner = command.args[3];
		}
	}

	boost::filesystem::path filePath(fileLoc);
	std::string fileExt = filePath.extension().string();
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);

	//FIXME: This is getting complicated, we should consider using boost::program_options and start utilising flags...
	//Something like this: http://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options

	repoLog("File: " + fileLoc + " owner: " + owner + " " + config.prettyPrint());

	uint8_t err;
	repo::core::model::RepoScene* graph = controller->loadSceneFromFile(fileLoc, err, config);
	if (graph)
	{
		repoLog("Trying to commit this scene to database as " + config.getDatabaseName() + "." + config.getProjectName());

		err = controller->commitScene(token, graph, owner, tag, desc, config.revisionId);

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

int32_t processDrawing(
	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken* token,
	const repo_op_t& command
)
{
	/*
	* Check the amount of parameters matches
	*/
	if (command.nArgcs < 1)
	{
		repoLogError("Number of arguments mismatch! " + cmdProcessDrawing + " requires 1 arguments: <path to json settings>");
		return REPOERR_INVALID_ARG;
	}

	std::string database;
	std::string svgPath;
	repo::lib::RepoUUID revision;

	boost::property_tree::ptree jsonTree;
	try {
		boost::property_tree::read_json(command.args[0], jsonTree);

		database = jsonTree.get<std::string>("database", "");
		auto revisionStr = jsonTree.get<std::string>("revId", "");

		if (database.empty() || revisionStr.empty())
		{
			return REPOERR_LOAD_SCENE_FAIL;
		}

		revision = repo::lib::RepoUUID(revisionStr);
	}
	catch (std::exception& e)
	{
		return REPOERR_LOAD_SCENE_FAIL;
	}

	if (command.nArgcs > 1) {
		svgPath = command.args[1];
	}
	repoLog("Drawing Revision: " + database + ", " + revision.toString() + (svgPath.empty() ? "" : (", processed image file in " + svgPath)));

	uint8_t err;

	controller->processDrawingRevision(token, database, revision, err, svgPath);
	return err;
}