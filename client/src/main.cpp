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

#include <repo/lib/repo_listener_stdout.h>
#include <repo/lib/repo_exception.h>
#include "functions.h"

static const uint32_t minArgs = 3;  //exe configFile command

void printHelp()
{
	std::cout << "Usage: 3drepobouncerClient path_to_config [<args>]" << std::endl;
	std::cout << std::endl;
	std::cout << "path_to_config\t\tPath to configuration json" << std::endl;
	std::cout << std::endl;
	std::cout << "Supported Commands:" << std::endl;
	std::cout << helpInfo() << std::endl;
	std::cout << std::endl;
	std::cout << "Environmental Variables:" << std::endl;
	std::cout << "REPO_DEBUG\tEnable debug logging" << std::endl;
	std::cout << "REPO_LOG_DIR\tSpecify the log directory (default is ./log)" << std::endl;
	std::cout << "REPO_VERBOSE\tEnable verbose logging" << std::endl;
}

std::shared_ptr<repo::RepoController>  instantiateController()
{
	repo::lib::LogToStdout *stdOutListener = new repo::lib::LogToStdout();
	std::vector<repo::lib::RepoAbstractListener*> listeners = { stdOutListener };
	std::shared_ptr<repo::RepoController> controller;

	try {
		controller =std::make_shared<repo::RepoController>(listeners);
	}
	catch (const repo::lib::RepoValidityExpiredException) {
		std::cerr << "License expired. Please contact support@3drepo.org should you wish to continue using the software." << std::endl;
		std::cerr << repo::RepoController::getLicenseInfo() << std::endl;
		exit(REPOERR_AUTH_FAILED);
	}

	char* debug = getenv("REPO_DEBUG");
	char* verbose = getenv("REPO_VERBOSE");
	char* logDir = getenv("REPO_LOG_DIR");

	if (verbose)
	{
		controller->setLoggingLevel(repo::lib::RepoLog::RepoLogLevel::TRACE);
	}
	else if (debug)
	{
		controller->setLoggingLevel(repo::lib::RepoLog::RepoLogLevel::DEBUG);
	}
	else
	{
		controller->setLoggingLevel(repo::lib::RepoLog::RepoLogLevel::INFO);
	}

	std::string logPath;
	if (logDir)
		logPath = std::string(logDir);
	else
		logPath = "./log/";

	controller->logToFile(logPath);
	repoLog("3D Repo Bouncer Version: " + controller->getVersion());
	return controller;
}

void logCommand(int argc, char* argv[])
{
	for (int i = minArgs - 1; i < argc; ++i)
	{
		if (i == (minArgs - 1))
		{
			repoLog("Operation: " + std::string(argv[i]));
		}
		else
		{
			repoLog("Arg " + std::to_string(i - (minArgs - 1)) + ": " + std::string(argv[i]));
		}
	}
}

int main(int argc, char* argv[]) {
	auto controller = instantiateController();
	if (argc < minArgs) {
		if (argc == 2 && isSpecialCommand(argv[1]))
		{
			repo_op_t op;
			op.command = argv[1];
			op.nArgcs = 0;

			int32_t errcode = performOperation(controller, nullptr, op);

			return errcode;
		}
		printHelp();
		if (argv[1] != "-h")
			repoLogError("Invalid number of arguments");
		return REPOERR_INVALID_ARG;
	}

	int idx = 0;
	std::string configPath = argv[++idx];
	logCommand(argc, argv);
	repo_op_t op;
	op.command = argv[++idx];
	if (argc > minArgs)
		op.args = &argv[minArgs];
	op.nArgcs = argc - minArgs;

	//Check before connecting to the database
	int32_t cmdnArgs = knownValid(op.command);
	if (cmdnArgs <= op.nArgcs)
	{
		std::string errMsg;
		try {
			auto config = repo::lib::RepoConfig::fromFile(configPath);
			repo::RepoController::RepoToken* token = controller->init(errMsg, config);
			if (token)
			{
				repoLog("successfully connected to the database!");
				int32_t errcode = performOperation(controller, token, op);

				controller->destroyToken(token);
				repoLog("Process completed, returning with error code: " + std::to_string(errcode));
				return errcode;
			}
			else {
				repoLogError("Failed to authenticate to the database: " + errMsg);
				return REPOERR_AUTH_FAILED;
			}
		}
		catch (const repo::lib::RepoException &e) {
			repoLogError("Failed to read configuration from file: " + configPath + " : " + e.what());
			return REPOERR_INVALID_CONFIG_FILE;
		}
	}
	else
	{
		repoLogError("Not enough arguments for command: " + op.command);
		printHelp();
		return REPOERR_INVALID_ARG;
	}

	repoLogError("Unknown command: " + op.command);
	printHelp();
	return REPOERR_UNKNOWN_CMD;
}