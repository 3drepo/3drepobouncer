#include <iostream>
#include <list>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "manipulator/repo_project.h"

#include "core/handler/repo_mongo_database_handler.h"
#include "core/model/bson/repo_bson_factory.h"
#include "core/model/bson/repo_node.h"

void configureLogging(){

	boost::log::core::get()->set_filter
	(
		boost::log::trivial::severity >= boost::log::trivial::info //only show info and above
	);
}

void testDatabaseRetrieval(repo::core::handler::AbstractDatabaseHandler *dbHandler){

	BOOST_LOG_TRIVIAL(debug) << "Retrieving list of databases...";
	std::list<std::string> databaseList = dbHandler->getDatabases();

	BOOST_LOG_TRIVIAL(debug) << "# of database: " << databaseList.size();

	std::list<std::string>::const_iterator iterator;
	for (iterator = databaseList.begin(); iterator != databaseList.end(); ++iterator) {
		BOOST_LOG_TRIVIAL(debug) << "\t" << *iterator;
	}

	BOOST_LOG_TRIVIAL(debug) << "Retrieving list of database collections";
	std::map<std::string, std::list<std::string>> mapDBProjects = dbHandler->getDatabasesWithProjects(databaseList, "scene");

	std::map<std::string, std::list<std::string>>::const_iterator mapIterator;
	for (mapIterator = mapDBProjects.begin(); mapIterator != mapDBProjects.end(); ++mapIterator) {
		BOOST_LOG_TRIVIAL(debug) << mapIterator->first << ":";
		std::list<std::string> projectList = mapIterator->second;

		for (iterator = projectList.begin(); iterator != projectList.end(); ++iterator) {
			BOOST_LOG_TRIVIAL(debug) << "\t" << *iterator;
		}
	}
}

void connect(repo::core::handler::AbstractDatabaseHandler *dbHandler, std::string username, std::string password){


	BOOST_LOG_TRIVIAL(debug) << "Connecting to database...";


	BOOST_LOG_TRIVIAL(debug) << (!dbHandler ? "FAIL" : "success!");

	BOOST_LOG_TRIVIAL(debug) << "Authenticating...";
	BOOST_LOG_TRIVIAL(debug) << (!dbHandler->authenticate(username, password) ? "FAIL" : "success!");

}

void insertARepoNode(repo::core::handler::AbstractDatabaseHandler *dbHandler){
	//namespace to insert the node into.
	std::string database = "test";
	std::string collection = "test.scene";

	//builds a repo node and dumps it out to check its value

	repo::core::model::bson::RepoNode node = repo::core::model::bson::RepoBSONFactory::makeRepoNode("<crazy>");

	BOOST_LOG_TRIVIAL(info) << "Repo Node created: ";
	BOOST_LOG_TRIVIAL(info) << "\t" << node.toString();

	BOOST_LOG_TRIVIAL(info) << "Repo Node created inserting it in " << database << "." << collection;
	dbHandler->insertDocument(database, collection, node);


}

void instantiateProject(repo::core::handler::AbstractDatabaseHandler *dbHandler){
	repo::manipulator::RepoProject *project = new repo::manipulator::RepoProject(dbHandler, "map", "arup");
	std::string errMsg;
	if (!project->loadRevision(errMsg)){
		BOOST_LOG_TRIVIAL(info) << "load Revision failed " << errMsg;
	}
	else{
		BOOST_LOG_TRIVIAL(info) << "Revision loaded";
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


	repo::core::handler::AbstractDatabaseHandler *dbHandler = repo::core::handler::MongoDatabaseHandler::getHandler(address, port);

	configureLogging();

	connect(dbHandler, username, password);
	//testDatabaseRetrieval(dbHandler);

	//insertARepoNode(dbHandler);

	instantiateProject(dbHandler);

	return EXIT_SUCCESS;
}
