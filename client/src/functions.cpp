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

#include <repo/lib/repo_units.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <repo/manipulator/modelutility/repo_clash_detection_config.h>
#include <repo/manipulator/modelutility/repo_web_buffer_config.h>

#include <sstream>
#include <fstream>
#include <algorithm>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

using namespace repo::lib;

static const std::string FBX_EXTENSION = ".FBX";

static const std::string cmdGenStash = "genStash";   //test the connection
static const std::string cmdClash = "clash";   //perform a clash detection operation
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
	ss << cmdClash << "\t\tPerform a clash detection operation. (args: configFile [resultsFile])\n";
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
	if (cmd == cmdTestConn)
		return 0;
	if (cmd == cmdVersion || cmd == cmdVersion2)
		return 0;
	if (cmd == cmdClash)
		return 1;
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
	else if (command.command == cmdClash)
	{
		try {
			errCode = performClashDetection(controller, token, command);
		}
		catch (const repo::lib::RepoException& e) {
			throw; // RepoExceptions have additional context as well as an embedded return code, so let these bubble up
		}
		catch (const std::exception& e)
		{
			repoLogError("Failed to perform clash detection: " + std::string(e.what()));
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

bool _generateStash(
	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken* token,
	const std::string& type,
	const std::string& dbName,
	const std::string& project,
	const bool                   isBranch,
	const std::string& revID) {
	repoLog("Generating stash of type " + type + " for " + dbName + "." + project + " rev: " + revID + (isBranch ? " (branch ID)" : ""));
	auto scene = controller->fetchScene(token, dbName, project, revID, isBranch, type == "tree");
	bool  success = false;
	if (scene) {
		if (type == "repo")
		{
			repo::manipulator::modelutility::WebBufferConfig config;
			config.splitByFloor = true;
			controller->updateRevisionStatus(scene, repo::core::model::ModelRevisionNode::UploadStatus::GEN_WEB_STASH);
			success = controller->generateAndCommitRepoBundlesBuffer(token, scene, config);
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
	if (command.nArgcs < 4)
	{
		repoLogError("Number of arguments mismatch! " + cmdGenStash
			+ " requires 3 arguments: database project revId [repo|tree]");
		return REPOERR_INVALID_ARG;
	}

	std::string dbName = command.args[0];
	std::string project = command.args[1];
	std::string revId = command.args[2];
	std::string type = command.args[3];

	if (type != "repo" && type != "tree") {
		repoLogError("Invalid stash type: " + type + ". Must be 'repo' or 'tree'.");
		return REPOERR_INVALID_ARG;
	}

	if (repo::lib::RepoUUID(revId).isDefaultValue()) {
		repoLogError("Invalid revision ID: " + revId + ". Must be a valid UUID.");
		return REPOERR_INVALID_ARG;
	}

	auto success = _generateStash(controller, token, type, dbName, project, false, revId);

	return success ? REPOERR_OK : REPOERR_STASH_GEN_FAIL;
}

int32_t performClashDetection(
	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken* token,
	const repo_op_t& command)
{
	if (command.nArgcs < 1) {
		repoError << "Clash detection requires a config file path";
		return REPOERR_INVALID_ARG;
	}
	repo::manipulator::modelutility::ClashDetectionConfig clashConfig;
	repo::manipulator::modelutility::ClashDetectionConfig::ParseJsonFile(command.args[0], clashConfig);
	if (command.nArgcs > 1) {
		clashConfig.resultsFile = command.args[1];
	}
	clashConfig.validate(); // Will throw if the config is invalid.
	controller->performClashDetection(token, clashConfig);
	return REPOERR_OK;
}

int32_t importFileAndCommit(
	std::shared_ptr<repo::RepoController> controller,
	const repo::RepoController::RepoToken* token,
	const repo_op_t& command
)
{
	using namespace repo::manipulator::modelconvertor;

	/*
	* Check the amount of parameters matches
	*/
	bool usingSettingFiles = command.nArgcs == 2 && std::string(command.args[0]) == "-f";
	if (!usingSettingFiles)
	{
		repoError << "Number of arguments mismatch! Bouncer imports can now only be invoked with a config file specified via -f.";
		return REPOERR_INVALID_ARG;
	}

	if (!std::filesystem::exists(command.args[1]))
	{
		repoError << "Config file does not exist: " << command.args[1];
		return REPOERR_INVALID_ARG;
	};

	auto config = ModelImportConfig::ParseJsonFile(command.args[1]);
	config.validate(); // Will throw if the config is invalid.

	return controller->processModelRevision(config);
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

		// Support both v4 and v5 config terminology for backwards compatibility

		database = jsonTree.get<std::string>("teamspace", {});

		if (database.empty()) {
			database = jsonTree.get<std::string>("database", "");
		}

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