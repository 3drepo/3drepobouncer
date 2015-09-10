#include <iostream>
#include <list>
#include <sstream>

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <repo/repo_controller.h>

/**
* This is really essentially a sandbox environment to perform a quick test while coding.
*/

void configureLogging(){

	boost::log::core::get()->set_filter
	(
		boost::log::trivial::severity >= boost::log::trivial::info //only show info and above
	);
}

void testDatabaseRetrieval(repo::RepoController *controller, const repo::RepoToken *token)
{
	repoInfo << "Fetching list of databases...";
	std::list<std::string> databases = controller->getDatabases(token);
	repoInfo << "Found " << databases.size() << " databases.";
	std::string dbList;

	std::list<std::string>::iterator it;
	for (it = databases.begin(); it != databases.end(); ++it)
	{
		if (it != databases.begin())
			dbList += ",";
		dbList += *it;
	}

	repoInfo << "Database List: {" << dbList << "}";


	if (databases.size() > 0)
	{
		std::string colList;

		std::string dbName = *databases.begin();
		repoInfo << "Fetching list of collections in " << dbName << " ...";
		std::list<std::string> collections = controller->getCollections(token, dbName);
		repoInfo << "Found " << collections.size() << " collections.";
		
		for (it = collections.begin(); it != collections.end(); ++it)
		{
			if (it != collections.begin())
				colList += ",";
			colList += *it;
		}
		repoInfo << "Collection List: {" << colList << "}";

	}
}

class TestListener : public repo::lib::RepoAbstractListener
{
public:
	TestListener() {}
	~TestListener(){}

	void messageGenerated(const std::string &message)
	{
		std::cout << "[LISTENER] " << message;
	}
};

void testDeletion(repo::RepoController *controller, repo::RepoToken *token)
{
	std::cout << "Trying to delete collection : deleteME";
	std::string errMsg;
	if (controller->removeCollection(token, "deleteME", "deleteME", errMsg))
	{
		std::cout << "Collection Deleted!" << std::endl;
	}
	else
	{
		std::cerr << "Failed to delete collection : " << errMsg;
	}

	std::cout << "Trying to delete database : deleteME";

	if (controller->removeDatabase(token, "deleteME", errMsg))
	{
		std::cout << "Database Deleted!" << std::endl;
	}
	else
	{
		std::cerr << "Failed to delete database : " << errMsg;
	}
}

void loadModelFromFileAndCommit(repo::RepoController *controller, const repo::RepoToken *token)
{
	
	std::string fileName;

	//fileName = "C:\\Users\\Carmen\\Desktop\\models\\A556-CAP-7000-S06-IE-S-1001.ifc"; - no worky at the moment
	//fileName = "C:\\Users\\Carmen\\Desktop\\models\\chair\\Bo Concept Imola.obj";
	fileName = "C:\\Users\\Carmen\\Desktop\\models\\Duplex_A_20110907.ifc";

	repo::manipulator::modelconvertor::ModelImportConfig config;

	config.setPreTransformVertices(true);
	config.setRemoveRedundantMaterials(true);

	repo::core::model::RepoScene *graph = controller->loadSceneFromFile(fileName, &config);
	if (graph)
	{
		BOOST_LOG_TRIVIAL(info) << "model loaded successfully! Attempting to port to Repo World...";

		BOOST_LOG_TRIVIAL(info) << "RepoScene generated. Printing graph statistics...";
		//std::stringstream		stringMaker;
		//graph->printStatistics(stringMaker);
		//std::cout << stringMaker.str();

		//std::string databaseName = "test";
		//std::string projectName = "stashTest";
		//BOOST_LOG_TRIVIAL(info) << "Trying to commit this scene to database as " << databaseName << "." << projectName;
		//
		//graph->setDatabaseAndProjectName(databaseName, projectName);

		//controller->commitScene(token, graph);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to load model from file : " << fileName  ;
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

	//testDatabaseRetrieval(controller, token);
	//errMsg.clear();

	////testDeletion(controller, token);

	////<< " bsons. first bson has a type of " << bsons.at(0).getStringField(REPO_NODE_LABEL_TYPE);


	//////insertARepoNode(dbHandler);

	//loadModelFromFileAndCommit(controller, token);

	////instantiateProject(dbHandler);
	//repo::core::model::RepoScene *scene = controller->fetchScene(token, "test", "stashTest");
	//std::stringstream		stringMaker;
	//scene->printStatistics(stringMaker);
	//std::cout << stringMaker.str();
	//controller->saveSceneToFile("C:/Users/Carmen/Desktop/camTest.dae", scene);


	return EXIT_SUCCESS;
}

