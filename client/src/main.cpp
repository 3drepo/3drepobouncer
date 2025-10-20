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

#include <repo_log.h>
#include <repo/lib/repo_exception.h>
#include "functions.h"
#include <locale>

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
#ifdef REPO_LICENSE_CHECK
	std::cout << "REPO_LICENSE\tLicense key for running bouncer" << std::endl;
#endif
	std::cout << "REPO_DEBUG\tEnable debug logging" << std::endl;
	std::cout << "REPO_LOG_DIR\tSpecify the log directory (default is ./log)" << std::endl;
	std::cout << "REPO_VERBOSE\tEnable verbose logging" << std::endl;
}

std::shared_ptr<repo::RepoController> instantiateController()
{
	std::vector<repo::lib::RepoAbstractListener*> listeners; // RepoLog currently will default to console
	std::shared_ptr<repo::RepoController> controller;

	try
	{
		controller = std::make_shared<repo::RepoController>(listeners);
	}
	catch (const repo::lib::RepoInvalidLicenseException e) {
		exit(ERRCODE_REPO_LICENCE_INVALID);
	}

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
	
	// The following line sets the locale to the system's one, instead of the C
	// minimal default. This ensures string and formatting functions work as
	// expected with extended character sets.
	setlocale(LC_ALL, "");

	std::shared_ptr<repo::RepoController> controller = instantiateController();
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
		catch (const repo::lib::RepoException &e)
		{
			repoError << e.printFull();
			return e.repoCode();
		}
		catch (const std::exception& e) // We expect all exceptions to be nested inside a RepoException, so this is only really a last ditch fallback
		{
			repoError << e.what();
			return REPOERR_UNKNOWN_ERR;
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