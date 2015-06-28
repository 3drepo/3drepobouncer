#include <iostream>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "core/handler/MongoDatabaseHandler.h"


void configureLogging(){

	//boost::log::core::get()->set_filter
	//(
	//	//boost::log::trivial::severity >= boost::log::trivial::info //only show info and above
	//);
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


	BOOST_LOG_TRIVIAL(info) << "Connecting to database...";

	repo::core::handler::AbstractDatabaseHandler *dbHandler = repo::core::handler::MongoDatabaseHandler::getHandler(address, port);

	BOOST_LOG_TRIVIAL(info) << (!dbHandler ? "FAIL" : "success!");

	BOOST_LOG_TRIVIAL(info) << "Authenticating...";
	BOOST_LOG_TRIVIAL(info) << (!dbHandler->authenticate(username, password) ? "FAIL" : "success!");


	return EXIT_SUCCESS;
}
