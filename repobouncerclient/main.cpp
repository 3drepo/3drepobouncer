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

#include <iostream>
#include <repo/repo_controller.h>

void loadModelFromFileAndCommit(repo::RepoController *controller, const repo::RepoToken *token)
{

	std::string fileName;

	//fileName = "C:\\Users\\Carmen\\Desktop\\models\\A556-CAP-7000-S06-IE-S-1001.ifc"; - no worky at the moment
	//fileName = "C:\\Users\\Carmen\\Desktop\\models\\chair\\Bo Concept Imola.obj";
	//fileName = "C:\\Users\\Carmen\\Desktop\\models\\Duplex_A_20110907.ifc";
	fileName = "C:\\Users\\Carmen\\Desktop\\models\\103EW-A-BASEMENT.fbx";

	repo::manipulator::modelconvertor::ModelImportConfig config;

	config.setPreTransformVertices(true);
	config.setRemoveRedundantMaterials(true);

	repo::core::model::RepoScene *graph = controller->loadSceneFromFile(fileName, &config);
	if (graph)
	{
		BOOST_LOG_TRIVIAL(info) << "model loaded successfully! Attempting to port to Repo World...";

		BOOST_LOG_TRIVIAL(info) << "RepoScene generated. Printing graph statistics...";
		std::stringstream		stringMaker;
		graph->printStatistics(stringMaker);
		std::cout << stringMaker.str();

		std::string databaseName = "test";
		std::string projectName = "stashTest";
		BOOST_LOG_TRIVIAL(info) << "Trying to commit this scene to database as " << databaseName << "." << projectName;
		
		graph->setDatabaseAndProjectName(databaseName, projectName);

		controller->commitScene(token, graph);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to load model from file : " << fileName;
	}


}

int main(int argc, char* argv[]){

	//TODO: configuration needs to be done properly, but hey, i'm just a quick test!
	if (argc != 5){
		std::cout << "Usage: " << std::endl;
		std::cout << "\t " << argv[0] << " address port username password" << std::endl;
		return EXIT_FAILURE;
	}

	std::string address = argv[1];
	int port = atoi(argv[2]);
	std::string username = argv[3];
	std::string password = argv[4];


	repo::RepoController *controller = new repo::RepoController();

	std::string errMsg;
	repo::RepoToken* token = controller->authenticateToAdminDatabaseMongo(errMsg, address, port, username, password);
	if (token)
		std::cout << "successfully connected to the database!" << std::endl;
	else
		std::cerr << "Failed to authenticate to the database: " << errMsg << std::endl;

	loadModelFromFileAndCommit(controller, token);



	return EXIT_SUCCESS;
}
