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

/**
*  Mongo database handler
*/

#include <regex>

#include "repo_database_handler_mongo.h"

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


mongo::BSONObj MongoDatabaseHandler::fieldsToReturn(
	const std::list<std::string>& fields,
	bool excludeIdField)
{
	mongo::BSONObjBuilder fieldsToReturn;
	std::list<std::string>::const_iterator it;
	for (it = fields.begin(); it != fields.end(); ++it)
	{
		fieldsToReturn << *it << 1;
		excludeIdField = excludeIdField && ID != *it;
	}
	if (excludeIdField)
		fieldsToReturn << ID << 0;

	return fieldsToReturn.obj();
}

std::vector<repo::core::model::bson::RepoBSON> MongoDatabaseHandler::findAllByUniqueIDs(
	const std::string& database,
	const std::string& collection,
	const repo::core::model::bson::RepoBSON& uuids){

	std::vector<repo::core::model::bson::RepoBSON> data;

	mongo::BSONArray array = mongo::BSONArray(uuids);

	int fieldsCount = array.nFields();
	if (fieldsCount > 0)
	{
		try{
			unsigned long long retrieved = 0;
			std::auto_ptr<mongo::DBClientCursor> cursor;
			do
			{

				mongo::BSONObjBuilder query;
				query << ID << BSON("$in" << array);


				cursor = worker->query(
					database + "." + collection,
					query.obj(),
					0,
					retrieved);

				for (; cursor.get() && cursor->more(); ++retrieved)
				{
					data.push_back(repo::core::model::bson::RepoBSON(cursor->nextSafe().copy()));
				}
			} while (cursor.get() && cursor->more());

			if (fieldsCount != retrieved){
				BOOST_LOG_TRIVIAL(error) << "Number of documents("<< retrieved<<") retreived by findAllByUniqueIDs did not match the number of unique IDs(" <<  fieldsCount <<")!";
			}
		}
		catch (mongo::DBException& e)
		{
			BOOST_LOG_TRIVIAL(error) << e.what();
		}
	}



	return data;
}

repo::core::model::bson::RepoBSON MongoDatabaseHandler::findOneBySharedID(
	const std::string& database,
	const std::string& collection,
	const repo_uuid& uuid,
	const std::string& sortField)
{

	repo::core::model::bson::RepoBSON bson;
	
	try
	{
		repo::core::model::bson::RepoBSONBuilder queryBuilder;
		queryBuilder.append("shared_id", uuid);
		//----------------------------------------------------------------------
		mongo::BSONObj bsonMongo = worker->findOne(
			getNamespace(database, collection),
			mongo::Query(queryBuilder.obj()).sort(sortField, -1));

		bson = repo::core::model::bson::RepoBSON(bsonMongo);
	}
	catch (mongo::DBException& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Error querying the database: "<< std::string(e.what());
	}
	return bson;
}

mongo::BSONObj MongoDatabaseHandler::findOneByUniqueID(
	const std::string& database,
	const std::string& collection,
	const repo_uuid& uuid){

	repo::core::model::bson::RepoBSON bson;
	try
	{
		repo::core::model::bson::RepoBSONBuilder queryBuilder;
		queryBuilder.append(ID, uuid);

		mongo::BSONObj bsonMongo = worker->findOne(getNamespace(database, collection),
			mongo::Query(queryBuilder.obj()));

		bson = repo::core::model::bson::RepoBSON(bsonMongo);
	}
	catch (mongo::DBException& e)
	{
		BOOST_LOG_TRIVIAL(error) << e.what();
	}
	return bson;
}

// Returns a list of tables for a given database name.
std::list<std::string> MongoDatabaseHandler::getCollections(
	const std::string &database)
{
	std::list<std::string> collections;
	try
	{
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

std::string MongoDatabaseHandler::getProjectFromCollection(const std::string &ns, const std::string &projectExt)
{
	size_t ind = ns.find("." + projectExt);
	std::string project;
	//just to make sure string is empty.
	project.clear();
	if (ind != std::string::npos){
		project = ns.substr(0, ind);
	}
	return project;
}

std::map<std::string, std::list<std::string> > MongoDatabaseHandler::getDatabasesWithProjects(
	const std::list<std::string> &databases, const std::string &projectExt)
{
	std::map<std::string, std::list<std::string> > mapping;
	try
	{
		for (std::list<std::string>::const_iterator it = databases.begin();
			it != databases.end(); ++it)
		{
			std::string database = *it;
			std::list<std::string> projects = getProjects(database, projectExt);
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

std::string MongoDatabaseHandler::getNamespace(
	const std::string &database, 
	const std::string &collection)
{	
	return database + "." + collection;
}

std::list<std::string> MongoDatabaseHandler::getProjects(const std::string &database, const std::string &projectExt)
{
	// TODO: remove db.info from the list (anything that is not a project basically)
	std::list<std::string> collections = getCollections(database);
	std::list<std::string> projects;
	for (std::list<std::string>::iterator it = collections.begin(); it != collections.end(); ++it){
		std::string project = getProjectFromCollection(*it, projectExt);
		if (!project.empty())
			projects.push_back(project);
	}
	projects.sort();
	projects.unique();
	return projects;
}

void MongoDatabaseHandler::insertDocument(
	const std::string &database,
	const std::string &collection,
	const repo::core::model::bson::RepoBSON &obj)
{
	worker->insert(getNamespace(database, collection), obj);
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
