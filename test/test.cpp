#include <iostream>
#include <list>
#include <sstream>

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <repo/repo_controller.h>


void configureLogging(){

	boost::log::core::get()->set_filter
	(
		boost::log::trivial::severity >= boost::log::trivial::info //only show info and above
	);
}

void testDatabaseRetrieval(repo::RepoController *controller, repo::RepoToken *token)
{
	BOOST_LOG_TRIVIAL(info) << "Fetching list of databases...";
	std::list<std::string> databases = controller->getDatabases(token);
	BOOST_LOG_TRIVIAL(info) << "Found " << databases.size() << " databases.";
	std::string dbList;

	std::list<std::string>::iterator it;
	for (it = databases.begin(); it != databases.end(); ++it)
	{
		if (it != databases.begin())
			dbList += ",";
		dbList += *it;
	}

	BOOST_LOG_TRIVIAL(info) << "Database List: {" << dbList << "}";


	if (databases.size() > 0)
	{
		std::string colList;

		std::string dbName = *databases.begin();
		BOOST_LOG_TRIVIAL(info) << "Fetching list of collections in " << dbName << " ...";
		std::list<std::string> collections = controller->getCollections(token, dbName);
		BOOST_LOG_TRIVIAL(info) << "Found " << collections.size() << " collections.";
		
		for (it = collections.begin(); it != collections.end(); ++it)
		{
			if (it != collections.begin())
				colList += ",";
			colList += *it;
		}
		BOOST_LOG_TRIVIAL(info) << "Collection List: {" << colList << "}";

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

	/*configureLogging();

	std::vector<repo::lib::RepoAbstractListener*> listeners;
	listeners.push_back(new TestListener());*/

	repo::RepoController *controller = new repo::RepoController();


	//BOOST_LOG_TRIVIAL(info) << "Subscribed to the logger!";

	std::string errMsg;
	repo::RepoToken* token = controller->authenticateToAdminDatabaseMongo(errMsg, address, port, username, password);
	
	
	if (token)
		std::cout << "successfully connected to the database!" << std::endl;
	else
		std::cerr << "Failed to authenticate to the database: " << errMsg << std::endl;

	//testDatabaseRetrieval(controller, token);
	errMsg.clear();

	//testDeletion(controller, token);

	//<< " bsons. first bson has a type of " << bsons.at(0).getStringField(REPO_NODE_LABEL_TYPE);


	////insertARepoNode(dbHandler);

	//loadModelFromFileAndCommit(dbHandler);

	//instantiateProject(dbHandler);
	repo::manipulator::graph::RepoScene *scene = controller->fetchScene(token, "test", "chair");
	controller->saveSceneToFile("C:/Users/Carmen/Desktop/chairtest.obj", scene);


	return EXIT_SUCCESS;
}

