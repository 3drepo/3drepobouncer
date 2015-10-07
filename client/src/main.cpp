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

static const uint32_t minArgs = 6;  //exe address port username password command

void printHelp()
{
	std::cout << "Usage: repobouncerclient <address> <port> <username> <password> <command> [<args>]" << std::endl;
	std::cout << std::endl;
	std::cout << "address\t\tAddress of database instance" << std::endl;
	std::cout << "port\t\tPort of database instance" << std::endl;
	std::cout << "username\tUsername to connect to database" << std::endl;
	std::cout << "password\tPassword of user" << std::endl;
	std::cout << std::endl;
	std::cout << "Supported Commands:" << std::endl;
	std::cout << helpInfo() << std::endl;
}

int main(int argc, char* argv[]){

	
	if (argc < minArgs){
		printHelp();
		return EXIT_FAILURE;
	}

	std::string address = argv[1];
	int port = atoi(argv[2]);
	std::string username = argv[3];
	std::string password = argv[4];

	repo_op_t op;
	op.command = argv[5];
	if (argc > minArgs)
		op.args = &argv[minArgs];
	op.nArgcs = argc - minArgs;

	//Check before connecting to the database
	int32_t cmdnArgs = knownValid(op.command);
	if (cmdnArgs <= op.nArgcs)
	{

		repo::RepoController *controller = new repo::RepoController();

		std::string errMsg;
		repo::RepoToken* token = controller->authenticateToAdminDatabaseMongo(errMsg, address, port, username, password);
		if (token)
		{
			repoLog("successfully connected to the database!");
			bool success = performOperation(controller, token, op);

			delete controller;
			delete token;
			return !success;
		}
		else{
			repoLogError("Failed to authenticate to the database: " + errMsg);
			return EXIT_FAILURE;
		}
			

		
	}
	else
	{
		std::cout << "Not enough arguments for command: " << op.command << std::endl;
		printHelp();
		return EXIT_FAILURE;
	}


	std::cout << "Unknown command: " << op.command << std::endl;
	printHelp();
	return EXIT_FAILURE;
	
}
