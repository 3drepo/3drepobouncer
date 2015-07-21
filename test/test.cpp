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

	configureLogging();

	repo::RepoController *controller = new repo::RepoController();

	std::string errMsg;
	repo::RepoToken* token = controller->authenticateToAdminDatabaseMongo(errMsg, address, port, username, password);


	if (token)
		BOOST_LOG_TRIVIAL(info) << "successfully connected to the database!";
	else
		BOOST_LOG_TRIVIAL(error) << "Failed to authenticate to the database: " << errMsg;

	//testDatabaseRetrieval(dbHandler);

	////insertARepoNode(dbHandler);

	//loadModelFromFileAndCommit(dbHandler);

	//instantiateProject(dbHandler);



	return EXIT_SUCCESS;
}
