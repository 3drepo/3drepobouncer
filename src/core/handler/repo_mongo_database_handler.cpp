/**
 * MongoDatabaseHandler.cpp
 *
 *  Created on: 26 Jun 2015
 *      Author: Carmen
 */

//#include <regex>

#include "repo_mongo_database_handler.h"

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

/**
* A Deconstructor
*/
MongoDatabaseHandler::~MongoDatabaseHandler()
{

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
	catch (mongo::DBException& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Database Authentication failed: " << e.what();
		success = false;
	}
	return success;
}

bool MongoDatabaseHandler::caseInsensitiveStringCompare(
	const std::string& s1,
	const std::string& s2)
{
	return strcasecmp(s1.c_str(), s2.c_str()) <= 0;
}


// Returns a list of tables for a given database name.
std::list<std::string> MongoDatabaseHandler::getCollections(
	const std::string &database)
{
	std::list<std::string> collections;
	try
	{
		BOOST_LOG_TRIVIAL(info) << "use " + database + "; show collections;";
		collections = worker->getCollectionNames(database);
	}
	catch (mongo::DBException& e)
	{
		BOOST_LOG_TRIVIAL(error) << e.what();
	}
	return collections;
}

//std::string MongoDatabaseHandler::getCollectionFromNamespace(const std::string &ns)
//{
//
//	std::regex rgx("*\.(\\w+)");
//	std::smatch match;
//
//	std::string collection;
//	if (std::regex_search(ns.begin(), ns.end(), match, rgx))
//		collection =  match[1];
//	else{
//		BOOST_LOG_TRIVIAL(debug) << 
//			"in MongoDatabaseHandler::getCollectionFromNamespace : would not find collection name from namespace :" << ns;
//		collection = ns;
//	}
//
//	return collection;
//}


std::list<std::string> MongoDatabaseHandler::getDatabases(
	const bool &sorted)
{
	std::list<std::string> list;
	try
	{
		BOOST_LOG_TRIVIAL(info) <<"show dbs;";
		list = worker->getDatabaseNames();

		if (sorted)
			list.sort(&MongoDatabaseHandler::caseInsensitiveStringCompare);
	}
	catch (mongo::DBException& e)
	{
		BOOST_LOG_TRIVIAL(error) << e.what();
	}

	return list;
}

//std::string MongoDatabaseHandler::getDatabaseFromNamespace(const std::string &ns)
//{
//
//	std::regex rgx("(\\w+)\.*");
//	std::smatch match;
//
//	std::string collection;
//	if (std::regex_search(ns.begin(), ns.end(), match, rgx))
//		collection = match[1];
//	else{
//		BOOST_LOG_TRIVIAL(debug) <<
//			"in MongoDatabaseHandler::getCollectionFromNamespace : would not find collection name from namespace :" << ns;
//		collection = ns;
//	}
//
//	return collection;
//}

std::map<std::string, std::list<std::string> > MongoDatabaseHandler::getDatabasesWithProjects(
	const std::list<std::string> &databases)
{
	std::map<std::string, std::list<std::string> > mapping;
	try
	{
		for (std::list<std::string>::const_iterator it = databases.begin();
			it != databases.end(); ++it)
		{
			std::string database = *it;
			std::list<std::string> projects = getProjects(database);
			mapping.insert(std::make_pair(database, projects));
		}
	}
	catch (mongo::DBException& e)
	{
		BOOST_LOG_TRIVIAL(error) << e.what();
	}
	return mapping;
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
			delete handler;
			handler = NULL;
		}
	}


	return handler;
}

//FIXME: this changed? do i just return collections? collections = project?
std::list<std::string> MongoDatabaseHandler::getProjects(const std::string &database)
{
	// TODO: remove db.info from the list (anything that is not a project basically)
	std::list<std::string> collections = getCollections(database);
	std::list<std::string> projects;
	//for (std::list<std::string>::iterator it = collections.begin(); it != collections.end(); ++it)
	//	projects.push_back(getDatabaseFromNamespace(getCollectionFromNamespace(*it)));
	//projects.sort();
	//projects.unique();
	return collections;
}

bool MongoDatabaseHandler::intialiseWorkers(){
	//FIXME: this should be a collection of workers
	bool success = true;
	std::string csErrorMsg;

	std::string errmsg;
	worker = dbAddress.connect(errmsg);
	if (!worker) {
		BOOST_LOG_TRIVIAL(error) << "couldn't connect: " << errmsg << std::endl;
		success = false;
	}


	return success;
}
