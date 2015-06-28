/*
 * MongoDatabaseHandler.cpp
 *
 *  Created on: 26 Jun 2015
 *      Author: Carmen
 */

#include "MongoDatabaseHandler.h"

using namespace repo::core::handler;


//------------------------------------------------------------------------------
const std::string repo::core::handler::MongoDatabaseHandler::ID = "_id";
const std::string repo::core::handler::MongoDatabaseHandler::UUID = "uuid";
const std::string repo::core::handler::MongoDatabaseHandler::ADMIN_DATABASE = "admin";
const std::string repo::core::handler::MongoDatabaseHandler::SYSTEM_ROLES_COLLECTION = "system.roles";
const std::list<std::string> repo::core::handler::MongoDatabaseHandler::ANY_DATABASE_ROLES =
{ "dbAdmin", "dbOwner", "read", "readWrite", "userAdmin" };
const std::list<std::string> repo::core::handler::MongoDatabaseHandler::ADMIN_ONLY_DATABASE_ROLES =
{ "backup", "clusterAdmin", "clusterManager", "clusterMonitor", "dbAdminAnyDatabase",
"hostManager", "readAnyDatabase", "readWriteAnyDatabase", "restore", "root",
"userAdminAnyDatabase" };

const std::string repo::core::handler::MongoDatabaseHandler::AUTH_MECH = "MONGODB-CR";
//------------------------------------------------------------------------------

MongoDatabaseHandler* MongoDatabaseHandler::handler = NULL;

MongoDatabaseHandler::MongoDatabaseHandler(
	const mongo::ConnectionString &dbAddress)
{
	mongo::client::initialize();

	this->dbAddress = dbAddress;
}


bool MongoDatabaseHandler::authenticate(
	const std::string &username,
	const std::string &plainTextPassword)
{
	return authenticate(ADMIN_DATABASE, username, plainTextPassword, false);
}

bool repo::core::handler::MongoDatabaseHandler::authenticate(
	const std::string &database,
	const std::string &username,
	const std::string &password,
	bool isPasswordDigested)
{
	bool success = true;
	std::string passwordDigest = isPasswordDigested ? password : worker->createPasswordDigest(username, password);
	try
	{
		//FIXME: it's aborting  if i use the auth(db, username, pw...) function call. I don't get why.
		worker->auth(BSON("user" << username <<
			"db" << database<<
			"pwd" << passwordDigest <<
			"digestPassword" << false <<
			"mechanism" << AUTH_MECH));
	}
	catch (std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Database Authentication failed: " << e.what();
		success = false;
	}
	return success;
}

MongoDatabaseHandler* MongoDatabaseHandler::getHandler(
	const std::string &host, 
	const int         &port)
{

	std::ostringstream connectionString;

	mongo::HostAndPort hostAndPort = mongo::HostAndPort(host, port >= 0 ? port : -1);

	std::string errmsg;
	mongo::ConnectionString mongoConnectionString = mongo::ConnectionString(hostAndPort);


	if(!handler){
		//initialise the mongo client
		handler = new MongoDatabaseHandler(mongoConnectionString);
		if (!handler->intialiseWorkers()){
			handler = NULL;
		}
	}


	return handler;
}

bool MongoDatabaseHandler::intialiseWorkers(){
	//FIXME: this should be a collection of workers
	bool success = true;
	std::string csErrorMsg;

	//try{
	//	std::string errmsg;
	//	dbAddress.connect(errmsg);
	//	//worker = new mongo::DBClientConnection(dbAddress.connect(errmsg));
	//	BOOST_LOG_TRIVIAL(info) << "Error msg : " << errmsg;
	//}
	//catch( const mongo::DBException &e ) {
	//		BOOST_LOG_TRIVIAL(error) << "Failed to connect to Mongo database: " << e.what();
	//		success = false;
	//}

	std::string errmsg;
	worker = dbAddress.connect(errmsg);
	if (!worker) {
		std::cout << "couldn't connect: " << errmsg << std::endl;
		return EXIT_FAILURE;
	}


	return success;
}
