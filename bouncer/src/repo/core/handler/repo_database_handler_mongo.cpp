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
#include <unordered_map>

#include "repo_database_handler_mongo.h"
#include "fileservice/repo_file_manager.h"
#include "fileservice/repo_blob_files_creator.h"
#include "../../lib/repo_log.h"

using namespace repo::core::handler;

static uint64_t MAX_MONGO_BSON_SIZE = 16777216L;
static uint64_t MAX_PARALLEL_BSON = 10000;
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

//------------------------------------------------------------------------------

MongoDatabaseHandler* MongoDatabaseHandler::handler = NULL;

MongoDatabaseHandler::MongoDatabaseHandler(
	const mongo::ConnectionString &dbAddress,
	const uint32_t                &maxConnections,
	const std::string             &dbName,
	const std::string             &username,
	const std::string             &password,
	const bool                    &pwDigested) :
	AbstractDatabaseHandler(MAX_MONGO_BSON_SIZE)
{
	mongo::client::initialize();
	workerPool = new connectionPool::MongoConnectionPool(maxConnections, dbAddress, createAuthBSON(dbName, username, password, pwDigested));
}

MongoDatabaseHandler::MongoDatabaseHandler(
	const mongo::ConnectionString &dbAddress,
	const uint32_t                &maxConnections,
	const std::string             &dbName,
	const repo::core::model::RepoBSON  *cred) :
	AbstractDatabaseHandler(MAX_MONGO_BSON_SIZE)
{
	mongo::client::initialize();
	workerPool = new connectionPool::MongoConnectionPool(maxConnections, dbAddress, (mongo::BSONObj*)cred);
}

/**
* A Deconstructor
*/
MongoDatabaseHandler::~MongoDatabaseHandler()
{
	if (workerPool)
		delete workerPool;
	mongo::client::shutdown();
}

bool MongoDatabaseHandler::caseInsensitiveStringCompare(
	const std::string& s1,
	const std::string& s2)
{
	return strcasecmp(s1.c_str(), s2.c_str()) <= 0;
}

uint64_t MongoDatabaseHandler::countItemsInCollection(
	const std::string &database,
	const std::string &collection,
	std::string &errMsg)
{
	uint64_t numItems = 0;
	mongo::DBClientBase *worker;
	if (database.empty() || collection.empty())
	{
		errMsg = "Failed to count num. items in collection: database name or collection name was not specified";
	}
	try {
		worker = workerPool->getWorker();
		if (worker)
			numItems = worker->count(database + "." + collection);
		else
			errMsg = "Failed to count number of items in collection: cannot obtain a database worker from the pool";
	}
	catch (mongo::DBException& e)
	{
		errMsg = "Failed to count num. items within "
			+ database + "." + collection + ":" + e.what();
		repoError << errMsg;
	}

	workerPool->returnWorker(worker);

	return numItems;
}

mongo::BSONObj* MongoDatabaseHandler::createAuthBSON(
	const std::string &database,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested)
{
	mongo::BSONObj* authBson = nullptr;
	if (!username.empty() && !database.empty() && !password.empty())
	{
		std::string passwordDigest = pwDigested ?
			password : mongo::DBClientWithCommands::createPasswordDigest(username, password);
		authBson = new mongo::BSONObj(BSON("user" << username <<
			"db" << database <<
			"pwd" << passwordDigest <<
			"digestPassword" << false));
	}

	return authBson;
}

void MongoDatabaseHandler::createCollection(const std::string &database, const std::string &name)
{
	mongo::DBClientBase *worker;
	if (!(database.empty() || name.empty()))
	{
		try {
			worker = workerPool->getWorker();
			if (worker)
				worker->createCollection(database + "." + name);
			else
				repoError << "Failed to create collection: cannot obtain a database worker from the pool";
		}
		catch (mongo::DBException& e)
		{
			repoError << "Failed to create collection ("
				<< database << "." << name << ":" << e.what();
		}

		workerPool->returnWorker(worker);
	}
	else
	{
		repoError << "Failed to create collection: database(value: " << database << ")/collection(value: " << name << ") name is empty!";
	}
}

void MongoDatabaseHandler::createIndex(const std::string &database, const std::string &collection, const mongo::BSONObj & obj)
{
	mongo::DBClientBase *worker;
	if (!(database.empty() || collection.empty()))
	{
		repoInfo << "Creating index for :" << database << "." << collection << " : index: " << obj;
		try {
			worker = workerPool->getWorker();
			if (worker)
				worker->createIndex(database + "." + collection, obj);
			else
				repoError << "Failed to create collection: cannot obtain a database worker from the pool";
		}
		catch (mongo::DBException& e)
		{
			repoError << "Failed to create index ("
				<< database << "." << collection << ":" << e.what();
		}

		workerPool->returnWorker(worker);
	}
	else
	{
		repoError << "Failed to create index: database(value: " << database << ")/collection(value: " << collection << ") name is empty!";
	}
}

repo::core::model::RepoBSON MongoDatabaseHandler::createRepoBSON(
	const std::string &database,
	const std::string &collection,
	const mongo::BSONObj &obj,
	const bool ignoreExtFile)
{
	repo::core::model::RepoBSON orgBson = repo::core::model::RepoBSON(obj);
	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>> > binMap;

	if (!ignoreExtFile) {
		std::vector<std::pair<std::string, std::string>> extFileList = orgBson.getFileList();
		for (const auto &pair : extFileList)
		{
			repoTrace << "Found existing external file reference, retrieving file @ " << database << "." << collection << ":" << pair.second;
			binMap[pair.first] = std::pair<std::string, std::vector<uint8_t>>(pair.second, getBigFile(database, collection, pair.second));
		}
	}

	return repo::core::model::RepoBSON(obj, binMap);
}

void MongoDatabaseHandler::disconnectHandler()
{
	if (handler)
	{
		repoInfo << "Disconnecting from database...";
		delete handler;
		handler = nullptr;
	}
	else
	{
		repoTrace << "Attempting to disconnect a handler without ever instantiating it!";
	}
}

bool MongoDatabaseHandler::dropCollection(
	const std::string &database,
	const std::string &collection,
	std::string &errMsg)
{
	bool success = false;
	mongo::DBClientBase *worker;
	if (!database.empty() || collection.empty())
	{
		try {
			worker = workerPool->getWorker();
			if (worker)
				success = worker->dropCollection(database + "." + collection);
			else
				errMsg = "Failed to count number of items in collection: cannot obtain a database worker from the pool";
		}
		catch (mongo::DBException& e)
		{
			errMsg = "Failed to drop collection ("
				+ database + "." + collection + ":" + e.what();
		}

		workerPool->returnWorker(worker);
	}
	else
	{
		errMsg = "Failed to drop collection: either database (value: " + database + ") or collection (value: " + collection + ") is empty";
	}

	return success;
}

bool MongoDatabaseHandler::dropDatabase(
	const std::string &database,
	std::string &errMsg)
{
	bool success = false;
	mongo::DBClientBase *worker;
	if (!database.empty())
	{
		try {
			worker = workerPool->getWorker();
			if (worker)
				success = worker->dropDatabase(database);
			else
				errMsg = "Failed to count number of items in collection: cannot obtain a database worker from the pool";
		}
		catch (mongo::DBException& e)
		{
			errMsg = "Failed to drop database :" + std::string(e.what());
		}

		workerPool->returnWorker(worker);
	}
	else
	{
		errMsg = "Failed to drop database: name of database is unspecified!";
	}

	return success;
}

bool MongoDatabaseHandler::dropDocument(
	const repo::core::model::RepoBSON bson,
	const std::string &database,
	const std::string &collection,
	std::string &errMsg)
{
	bool success = false;
	mongo::DBClientBase *worker;
	if (!database.empty() && !collection.empty())
	{
		try {
			worker = workerPool->getWorker();
			if (success = worker && !bson.isEmpty() && bson.hasField("_id"))
			{
				mongo::Query query = MONGO_QUERY("_id" << bson.getField("_id").toMongoElement());
				worker->remove(database + "." + collection, query, true);
			}
			else
			{
				errMsg = "Failed to drop document: id not found";
			}
		}
		catch (mongo::DBException& e)
		{
			errMsg = "Failed to drop document :" + std::string(e.what());
			success = false;
		}

		workerPool->returnWorker(worker);
	}
	else
	{
		errMsg = "Failed to drop document: either database (value: " + database + ") or collection (value: " + collection + ") is empty";
	}

	return success;
}

bool MongoDatabaseHandler::dropDocuments(
	const repo::core::model::RepoBSON criteria,
	const std::string &database,
	const std::string &collection,
	std::string &errMsg)
{
	bool success = false;
	mongo::DBClientBase *worker;
	if (!database.empty() && !collection.empty())
	{
		try {
			worker = workerPool->getWorker();
			if (success = worker && !criteria.isEmpty())
			{
				worker->remove(database + "." + collection, criteria, false);
			}
			else
			{
				errMsg = "Failed to drop documents: empty criteria";
			}
		}
		catch (mongo::DBException& e)
		{
			errMsg = "Failed to drop documents:" + std::string(e.what());
		}

		workerPool->returnWorker(worker);
	}
	else
	{
		errMsg = "Failed to drop document: either database (value: " + database + ") or collection (value: " + collection + ") is empty";
	}

	return success;
}

bool MongoDatabaseHandler::dropRawFile(
	const std::string &database,
	const std::string &collection,
	const std::string &fileName,
	std::string &errMsg
)
{
	bool success = true;
	mongo::DBClientBase *worker;

	if (fileName.empty())
	{
		errMsg = "Cannot  remove a raw file from the database with no file name!";
		return false;
	}

	if (database.empty() || collection.empty())
	{
		errMsg = "Cannot remove a raw file: database(value: " + database + ") or collection name(value: " + collection + ") is not specified!";
		return false;
	}

	try {
		worker = workerPool->getWorker();
		if (success = worker)
		{
			//store the big biary file within GridFS
			mongo::GridFS gfs(*worker, database, collection);
			//FIXME: there must be errors to catch...
			repoTrace << "removing " << fileName << " in gridfs: " << database << "." << collection;
			gfs.removeFile(fileName);
		}
		else
			errMsg = "Failed to count number of items in collection: cannot obtain a database worker from the pool";
	}
	catch (mongo::DBException &e)
	{
		success = false;
		std::string errString(e.what());
		errMsg += errString;
	}

	workerPool->returnWorker(worker);

	return success;
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

std::vector<repo::core::model::RepoBSON> MongoDatabaseHandler::findAllByCriteria(
	const std::string& database,
	const std::string& collection,
	const repo::core::model::RepoBSON& criteria)
{
	std::vector<repo::core::model::RepoBSON> data;

	if (!criteria.isEmpty())
	{
		mongo::DBClientBase *worker;
		try {
			uint64_t retrieved = 0;
			std::auto_ptr<mongo::DBClientCursor> cursor;
			worker = workerPool->getWorker();
			if (worker)
			{
				do
				{
					repoTrace << " Querying " << database << "." << collection << " with : " << criteria.toString();
					cursor = worker->query(
						database + "." + collection,
						criteria,
						0,
						retrieved);

					workerPool->returnWorker(worker);
					worker = nullptr;
					for (; cursor.get() && cursor->more(); ++retrieved)
					{
						data.push_back(createRepoBSON(database, collection, cursor->nextSafe().copy()));
					}
					worker = workerPool->getWorker();
				} while (cursor.get() && cursor->more());
			}
			else
				repoError << "Failed to count number of items in collection: cannot obtain a database worker from the pool";
		}
		catch (mongo::DBException& e)
		{
			repoError << "Error in MongoDatabaseHandler::findAllByCriteria: " << e.what();
		}
		if (worker) workerPool->returnWorker(worker);
	}
	return data;
}

repo::core::model::RepoBSON MongoDatabaseHandler::findOneByCriteria(
	const std::string& database,
	const std::string& collection,
	const repo::core::model::RepoBSON& criteria,
	const std::string& sortField)
{
	repo::core::model::RepoBSON data;

	if (!criteria.isEmpty())
	{
		mongo::DBClientBase *worker;
		try {
			uint64_t retrieved = 0;
			worker = workerPool->getWorker();
			if (worker)
			{
				auto query = mongo::Query(criteria);
				if (!sortField.empty())
					query = query.sort(sortField, -1);

				data = repo::core::model::RepoBSON(worker->findOne(
					database + "." + collection,
					query));
			}
		}
		catch (mongo::DBException& e)
		{
			repoError << "Error in MongoDatabaseHandler::findOneByCriteria: " << e.what();
		}

		workerPool->returnWorker(worker);
	}

	return data;
}

std::vector<repo::core::model::RepoBSON> MongoDatabaseHandler::findAllByUniqueIDs(
	const std::string& database,
	const std::string& collection,
	const repo::core::model::RepoBSON& uuids,
	const bool ignoreExtFiles) {
	std::vector<repo::core::model::RepoBSON> data;

	mongo::BSONArray array = mongo::BSONArray(uuids);
	int fieldsCount = array.nFields();
	if (fieldsCount > 0)
	{
		mongo::DBClientBase *worker;
		try {
			uint64_t retrieved = 0;
			std::auto_ptr<mongo::DBClientCursor> cursor;
			worker = workerPool->getWorker();
			if (worker)
			{
				do
				{
					mongo::BSONObjBuilder query;
					query << ID << BSON("$in" << array);

					cursor = worker->query(
						database + "." + collection,
						query.obj(),
						0,
						retrieved);

					workerPool->returnWorker(worker);
					worker = nullptr;
					for (; cursor.get() && cursor->more(); ++retrieved)
					{
						data.push_back(createRepoBSON(database, collection, cursor->nextSafe().copy(), ignoreExtFiles));
					}
					worker = workerPool->getWorker();
				} while (cursor.get() && cursor->more());
			}
			else
			{
				repoError << "Failed to count number of items in collection: cannot obtain a database worker from the pool";
			}

			if (fieldsCount != retrieved) {
				repoWarning << "Number of documents(" << retrieved << ") retreived by findAllByUniqueIDs did not match the number of unique IDs(" << fieldsCount << ")!";
			}
		}
		catch (mongo::DBException& e)
		{
			repoError << e.what();
		}

		if (worker) workerPool->returnWorker(worker);
	}

	return data;
}

repo::core::model::RepoBSON MongoDatabaseHandler::findOneBySharedID(
	const std::string& database,
	const std::string& collection,
	const repo::lib::RepoUUID& uuid,
	const std::string& sortField)
{
	repo::core::model::RepoBSON bson;
	mongo::DBClientBase *worker;
	try
	{
		repo::core::model::RepoBSONBuilder queryBuilder;
		queryBuilder.append("shared_id", uuid);
		//----------------------------------------------------------------------

		worker = workerPool->getWorker();
		if (worker)
		{
			auto query = mongo::Query(queryBuilder.mongoObj());
			if (!sortField.empty())
				query = query.sort(sortField, -1);

			mongo::BSONObj bsonMongo = worker->findOne(
				getNamespace(database, collection),
				query);

			workerPool->returnWorker(worker);
			worker = nullptr;
			bson = createRepoBSON(database, collection, bsonMongo);
		}
		else
		{
			repoError << "Failed to count number of items in collection: cannot obtain a database worker from the pool";
		}
	}
	catch (mongo::DBException& e)
	{
		if (worker) workerPool->returnWorker(worker);
		repoError << "Error querying the database: " << std::string(e.what());
	}

	return bson;
}

repo::core::model::RepoBSON  MongoDatabaseHandler::findOneByUniqueID(
	const std::string& database,
	const std::string& collection,
	const repo::lib::RepoUUID& uuid) {
	repo::core::model::RepoBSON bson;
	mongo::DBClientBase *worker;
	try
	{
		repo::core::model::RepoBSONBuilder queryBuilder;
		queryBuilder.append(ID, uuid);

		worker = workerPool->getWorker();
		if (worker)
		{
			mongo::BSONObj bsonMongo = worker->findOne(getNamespace(database, collection),
				mongo::Query(queryBuilder.mongoObj()));

			workerPool->returnWorker(worker);
			worker = nullptr;
			bson = createRepoBSON(database, collection, bsonMongo);
		}
		else
			repoError << "Failed to count number of items in collection: cannot obtain a database worker from the pool";
	}
	catch (mongo::DBException& e)
	{
		repoError << e.what();
		if (worker) workerPool->returnWorker(worker);
	}

	return bson;
}

std::vector<repo::core::model::RepoBSON>
MongoDatabaseHandler::getAllFromCollectionTailable(
	const std::string                             &database,
	const std::string                             &collection,
	const uint64_t                                &skip,
	const uint32_t                                &limit,
	const std::list<std::string>				  &fields,
	const std::string							  &sortField,
	const int									  &sortOrder)
{
	std::vector<repo::core::model::RepoBSON> bsons;
	mongo::DBClientBase *worker;
	try
	{
		worker = workerPool->getWorker();
		if (worker)
		{
			mongo::BSONObj tmp = fieldsToReturn(fields);

			std::auto_ptr<mongo::DBClientCursor> cursor = worker->query(
				database + "." + collection,
				sortField.empty() ? mongo::Query() : mongo::Query().sort(sortField, sortOrder),
				limit,
				skip,
				fields.size() > 0 ? &tmp : nullptr);
			workerPool->returnWorker(worker);
			worker = nullptr;
			while (cursor.get() && cursor->more())
			{
				//have to copy since the bson info gets cleaned up when cursor gets out of scope
				bsons.push_back(createRepoBSON(database, collection, cursor->nextSafe().copy()));
			}
			worker = workerPool->getWorker();
		}
		else
			repoError << "Failed to count number of items in collection: cannot obtain a database worker from the pool";
	}
	catch (mongo::DBException& e)
	{
		repoError << "Failed retrieving bsons from mongo: " << e.what();
	}
	if (worker)
		workerPool->returnWorker(worker);
	return bsons;
}

std::list<std::string> MongoDatabaseHandler::getCollections(
	const std::string &database)
{
	std::list<std::string> collections;
	mongo::DBClientBase *worker;
	try
	{
		worker = workerPool->getWorker();
		if (worker)
			collections = worker->getCollectionNames(database);
		else
			repoError << "Failed to count number of items in collection: cannot obtain a database worker from the pool";
	}
	catch (mongo::DBException& e)
	{
		repoError << e.what();
	}

	workerPool->returnWorker(worker);
	return collections;
}

std::list<std::string> MongoDatabaseHandler::getDatabases(
	const bool &sorted)
{
	std::list<std::string> list;
	mongo::DBClientBase *worker;
	try
	{
		worker = workerPool->getWorker();
		if (worker)
		{
			list = worker->getDatabaseNames();

			if (sorted)
				list.sort(&MongoDatabaseHandler::caseInsensitiveStringCompare);
		}
		else
			repoError << "Failed to count number of items in collection: cannot obtain a database worker from the pool";
	}
	catch (mongo::DBException& e)
	{
		repoError << e.what();
	}
	workerPool->returnWorker(worker);
	return list;
}

std::vector<uint8_t> MongoDatabaseHandler::getBigFile(
	const std::string &database,
	const std::string &collection,
	const std::string &fileName)
{
	auto fileManager = fileservice::FileManager::getManager();

	return fileManager->getFile(database, collection, fileName);
}

std::string MongoDatabaseHandler::getProjectFromCollection(const std::string &ns, const std::string &projectExt)
{
	size_t ind = ns.find("." + projectExt);
	std::string project;
	//just to make sure string is empty.
	project.clear();
	if (ind != std::string::npos) {
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
		repoError << e.what();
	}
	return mapping;
}

MongoDatabaseHandler* MongoDatabaseHandler::getHandler(
	std::string       &errMsg,
	const std::string &connectionString,
	const uint32_t    &maxConnections,
	const std::string &dbName,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested)
{
	if (!handler) {
		std::string msg;
		mongo::ConnectionString mongoConnectionString = mongo::ConnectionString::parse(connectionString, msg);
		if (!msg.empty()) {
			repoError << "Failed to construct connection string: " << msg;
			return nullptr;
		}

		//initialise the mongo client
		repoTrace << "Handler not present for " << mongoConnectionString.toString() << " instantiating new handler...";
		try {
			handler = new MongoDatabaseHandler(mongoConnectionString, maxConnections, dbName, username, password, pwDigested);
		}
		catch (mongo::DBException e)
		{
			if (handler)
				delete handler;
			handler = 0;
			errMsg = std::string(e.what());
			repoError << "Error establishing Mongo Handler: " << errMsg;
		}
	}
	else
	{
		repoTrace << "Found handler, returning existing handler";
	}

	return handler;
}

MongoDatabaseHandler* MongoDatabaseHandler::getHandler(
	std::string       &errMsg,
	const std::string &host,
	const int         &port,
	const uint32_t    &maxConnections,
	const std::string &dbName,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested)
{
	std::ostringstream connectionString;

	mongo::HostAndPort hostAndPort = mongo::HostAndPort(host, port >= 0 ? port : -1);

	mongo::ConnectionString mongoConnectionString = mongo::ConnectionString(hostAndPort);

	if (!handler) {
		//initialise the mongo client
		repoTrace << "Handler not present for " << mongoConnectionString.toString() << " instantiating new handler...";
		try {
			handler = new MongoDatabaseHandler(mongoConnectionString, maxConnections, dbName, username, password, pwDigested);
		}
		catch (mongo::DBException e)
		{
			if (handler)
				delete handler;
			handler = 0;
			errMsg = std::string(e.what());
			repoError << "Error establishing Mongo Handler: " << errMsg;
		}
	}
	else
	{
		repoTrace << "Found handler, returning existing handler";
	}

	return handler;
}

MongoDatabaseHandler* MongoDatabaseHandler::getHandler(
	std::string                       &errMsg,
	const std::string                 &host,
	const int                         &port,
	const uint32_t                    &maxConnections,
	const std::string                 &dbName,
	const repo::core::model::RepoBSON *credentials)
{
	std::ostringstream connectionString;

	mongo::HostAndPort hostAndPort = mongo::HostAndPort(host, port >= 0 ? port : -1);

	mongo::ConnectionString mongoConnectionString = mongo::ConnectionString(hostAndPort);

	if (!handler) {
		//initialise the mongo client
		repoTrace << "Handler not present for " << mongoConnectionString.toString() << " instantiating new handler...";
		try {
			handler = new MongoDatabaseHandler(mongoConnectionString, maxConnections, dbName, credentials);
		}
		catch (mongo::DBException e)
		{
			if (handler)
				delete handler;
			handler = 0;
			errMsg = std::string(e.what());
			repoError << "Error establishing Mongo Handler: " << errMsg;
		}
	}
	else
	{
		repoTrace << "Found handler, returning existing handler";
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
	for (std::list<std::string>::iterator it = collections.begin(); it != collections.end(); ++it) {
		std::string project = getProjectFromCollection(*it, projectExt);
		if (!project.empty())
			projects.push_back(project);
	}
	projects.sort();
	projects.unique();
	return projects;
}

std::vector<uint8_t> MongoDatabaseHandler::getRawFile(
	const std::string& database,
	const std::string& collection,
	const std::string& fname
)
{
	std::vector<uint8_t> bin;

	mongo::DBClientBase *worker;
	try {
		worker = workerPool->getWorker();
		if (worker)
		{
			mongo::GridFS gfs(*worker, database, collection);
			mongo::GridFile tmpFile = gfs.findFileByName(fname);

			repoTrace << "Getting file from GridFS: " << fname << " in : " << database << "." << collection;

			if (tmpFile.exists())
			{
				std::ostringstream oss;
				tmpFile.write(oss);

				std::string fileStr = oss.str();

				assert(sizeof(*fileStr.c_str()) == sizeof(uint8_t));

				if (!fileStr.empty())
				{
					bin.resize(fileStr.size());
					memcpy(&bin[0], fileStr.c_str(), fileStr.size());
				}
				else
				{
					repoError << "GridFS file : " << fname << " in "
						<< database << "." << collection << " is empty.";
				}
			}
			else
			{
				repoError << "Failed to find file within GridFS";
			}
		}
		else
			repoError << "Failed to count number of items in collection: cannot obtain a database worker from the pool";
	}
	catch (mongo::DBException e)
	{
		repoError << "Error fetching raw file: " << e.what();
	}

	workerPool->returnWorker(worker);

	return bin;
}

bool MongoDatabaseHandler::insertDocument(
	const std::string &database,
	const std::string &collection,
	const repo::core::model::RepoBSON &obj,
	std::string &errMsg)
{
	bool success = false;
	mongo::DBClientBase *worker;
	if (!database.empty() || collection.empty())
	{
		try {
			worker = workerPool->getWorker();
			if (worker)
			{
				worker->insert(getNamespace(database, collection), obj);
				workerPool->returnWorker(worker);
				worker = nullptr;

				//FI
//				success = storeBigFiles(database, collection, obj, errMsg);
			}
			else
				errMsg = "Failed to count number of items in collection: cannot obtain a database worker from the pool";
		}
		catch (mongo::DBException &e)
		{
			std::string errString(e.what());
			errMsg += errString;
		}
		if (worker)
			workerPool->returnWorker(worker);
	}
	else
	{
		errMsg = "Unable to insert Document, database(value : " + database + ")/collection(value : " + collection + ") name was not specified";
	}

	return success;
}

bool MongoDatabaseHandler::insertManyDocuments(
	const std::string &database,
	const std::string &collection,
	const std::vector<repo::core::model::RepoBSON> &objs,
	std::string &errMsg)
{
	bool success = false;
	mongo::DBClientBase *worker;
	if (!database.empty() || collection.empty())
	{
		try {
			worker = workerPool->getWorker();
			if (worker)
			{
				auto fileManager = fileservice::FileManager::getManager();

				fileservice::BlobFilesCreator blobCreator(fileManager, database, collection);

				for (int i = 0; i < objs.size(); i += MAX_PARALLEL_BSON) {
					std::vector<repo::core::model::RepoBSON>::const_iterator it = objs.begin() + i;
					std::vector<repo::core::model::RepoBSON>::const_iterator last = i + MAX_PARALLEL_BSON >= objs.size() ? objs.end() : it + MAX_PARALLEL_BSON;

					std::vector<mongo::BSONObj> toCommit;
					do {
						auto node = *it;
						//node.storeBinaries(blobCreator);
					} while (++it != last);

					worker->insert(getNamespace(database, collection), toCommit);
				}

				workerPool->returnWorker(worker);
				worker = nullptr;
				//FIXME
				//success = storeBigFiles(database, collection, objs, errMsg);
			}
			else
				errMsg = "Failed to count number of items in collection: cannot obtain a database worker from the pool";
		}
		catch (mongo::DBException &e)
		{
			std::string errString(e.what());
			errMsg += errString;
		}
		if (worker)
			workerPool->returnWorker(worker);
	}
	else
	{
		errMsg = "Unable to insert Document, database(value : " + database + ")/collection(value : " + collection + ") name was not specified";
	}

	return success;
}

bool MongoDatabaseHandler::insertRawFile(
	const std::string          &database,
	const std::string          &collection,
	const std::string          &fileName,
	const std::vector<uint8_t> &bin,
	std::string          &errMsg,
	const std::string          &contentType
)
{
	bool success = false;
	mongo::DBClientBase *worker;

	repoTrace << "writing raw file: " << fileName;

	if (bin.size() == 0)
	{
		errMsg = "size of file is 0!";
		return false;
	}

	if (fileName.empty())
	{
		errMsg = "Cannot store a raw file in the database with no file name!";
		return false;
	}

	if (database.empty() || collection.empty())
	{
		errMsg = "Cannot store a raw file: database(value: " + database + ") or collection name(value: " + collection + ") is not specified!";
		return false;
	}

	int retry = 0;
	while (!success && retry < 5)
	{
		try {
			worker = workerPool->getWorker();
			//store the big biary file within GridFS
			if (worker)
			{
				mongo::GridFS gfs(*worker, database, collection);
				//FIXME: there must be errors to catch...
				repoTrace << "storing " << fileName << " in gridfs: " << database << "." << collection;
				gfs.removeFile(fileName);
				mongo::BSONObj bson = gfs.storeFile((char*)&bin[0], bin.size() * sizeof(bin[0]), fileName, contentType);
				repoTrace << "returned object: " << bson.toString();
				success = true;
			}
			else
			{
				repoError << "Failed to obtain a connection with the database";
			}
		}
		catch (mongo::DBException &e)
		{
			success = false;
			std::string errString(e.what());
			errMsg = errString;
			repoError << "Failed to upload gridFS file : " << errString << " retrying in 5s...";
			boost::this_thread::sleep(boost::posix_time::seconds(5));
			retry++;
		}
		workerPool->returnWorker(worker);
	}

	return success;
}

bool MongoDatabaseHandler::performRoleCmd(
	const OPERATION                         &op,
	const repo::core::model::RepoRole       &role,
	std::string                             &errMsg)
{
	bool success = false;
	mongo::DBClientBase *worker;

	if (!role.isEmpty())
	{
		if (role.getName().empty() || role.getDatabase().empty())
		{
			errMsg += "Role bson does not contain role name/database name";
		}
		else {
			try {
				worker = workerPool->getWorker();
				if (worker)
				{
					mongo::BSONObjBuilder cmdBuilder;
					std::string roleName = role.getName();
					switch (op)
					{
					case OPERATION::INSERT:
						cmdBuilder << "createRole" << roleName;
						break;
					case OPERATION::UPDATE:
						cmdBuilder << "updateRole" << roleName;
						break;
					case OPERATION::DROP:
						cmdBuilder << "dropRole" << roleName;
					}

					if (op != OPERATION::DROP)
					{
						repo::core::model::RepoBSON privileges = role.getObjectField(REPO_ROLE_LABEL_PRIVILEGES);
						cmdBuilder.appendArray("privileges", privileges);

						repo::core::model::RepoBSON inheritedRoles = role.getObjectField(REPO_ROLE_LABEL_INHERITED_ROLES);

						cmdBuilder.appendArray("roles", inheritedRoles);
					}

					mongo::BSONObj info;
					auto cmd = cmdBuilder.obj();
					success = worker->runCommand(role.getDatabase(), cmd, info);

					std::string cmdError = info.getStringField("errmsg");
					if (!cmdError.empty())
					{
						success = false;
						errMsg += cmdError;
					}
				}
				else
					repoError << "Failed to count number of items in collection: cannot obtain a database worker from the pool";
			}
			catch (mongo::DBException &e)
			{
				success = false;
				std::string errString(e.what());
				errMsg += errString;
			}

			workerPool->returnWorker(worker);
		}
	}
	else
	{
		errMsg += "Role bson is empty";
	}

	return success;
}

bool MongoDatabaseHandler::performUserCmd(
	const OPERATION                         &op,
	const repo::core::model::RepoUser &user,
	std::string                       &errMsg)
{
	bool success = false;
	mongo::DBClientBase *worker;

	if (!user.isEmpty())
	{
		try {
			worker = workerPool->getWorker();
			if (worker)
			{
				repo::core::model::RepoBSONBuilder cmdBuilder;
				std::string username = user.getUserName();
				switch (op)
				{
				case OPERATION::INSERT:
					cmdBuilder.append("createUser", username);
					break;
				case OPERATION::UPDATE:
					cmdBuilder.append("updateUser", username);
					break;
				case OPERATION::DROP:
					cmdBuilder.append("dropUser", username);
				}

				if (op != OPERATION::DROP)
				{
					std::string pw = user.getCleartextPassword();
					if (!pw.empty())
						cmdBuilder.append("pwd", pw);

					repo::core::model::RepoBSON customData = user.getCustomDataBSON();
					if (!customData.isEmpty())
						cmdBuilder.append("customData", customData);

					//compulsory, so no point checking if it's empty
					cmdBuilder.appendArray("roles", user.getRolesBSON());
				}

				mongo::BSONObj info;
				success = worker->runCommand(ADMIN_DATABASE, cmdBuilder.mongoObj(), info);

				std::string cmdError = info.getStringField("errmsg");
				if (!cmdError.empty())
				{
					success = false;
					errMsg += cmdError;
				}
			}
			else
			{
				errMsg = "Failed to count number of items in collection: cannot obtain a database worker from the pool";
			}
		}
		catch (mongo::DBException &e)
		{
			std::string errString(e.what());
			errMsg += errString;
		}

		workerPool->returnWorker(worker);
	}
	else
	{
		errMsg += "User bson is empty";
	}

	return success;
}

bool MongoDatabaseHandler::upsertDocument(
	const std::string &database,
	const std::string &collection,
	const repo::core::model::RepoBSON &obj,
	const bool        &overwrite,
	std::string &errMsg)
{
	bool success = true;
	mongo::DBClientBase *worker;

	bool upsert = overwrite;
	try {
		worker = workerPool->getWorker();
		if (success = worker)
		{
			repo::core::model::RepoBSONBuilder queryBuilder;
			queryBuilder.append(ID, obj.getField(ID));

			mongo::BSONElement bsonID;
			obj.getObjectID(bsonID);
			mongo::Query existQuery = MONGO_QUERY("_id" << bsonID);
			mongo::BSONObj bsonMongo = worker->findOne(database + "." + collection, existQuery);

			if (bsonMongo.isEmpty())
			{
				//document doens't exist, insert the document
				upsert = true;
			}

			if (upsert)
			{
				mongo::Query query;
				query = BSON(REPO_LABEL_ID << bsonID);
				repoTrace << "query = " << query.toString();
				worker->update(getNamespace(database, collection), query, obj, true);
			}
			else
			{
				//only update fields
				mongo::BSONObjBuilder builder;
				builder << REPO_COMMAND_UPDATE << collection;

				mongo::BSONObjBuilder updateBuilder;
				updateBuilder << REPO_COMMAND_Q << BSON(REPO_LABEL_ID << bsonID);
				updateBuilder << REPO_COMMAND_U << BSON("$set" << ((mongo::BSONObj)obj).removeField(ID));
				updateBuilder << REPO_COMMAND_UPSERT << true;

				builder << REPO_COMMAND_UPDATES << BSON_ARRAY(updateBuilder.obj());
				mongo::BSONObj info;
				worker->runCommand(database, builder.obj(), info);

				if (info.hasField("writeErrors"))
				{
					repoError << info.getField("writeErrors").Array().at(0).embeddedObject().getStringField("errmsg");
					success = false;
				}
			}

			if (success) {
				workerPool->returnWorker(worker);
				worker = nullptr;
				//FIXME
				//success = storeBigFiles(database, collection, obj, errMsg);
			}
		}
		else
			errMsg = "Failed to count number of items in collection: cannot obtain a database worker from the pool";
	}
	catch (mongo::DBException &e)
	{
		success = false;
		std::string errString(e.what());
		errMsg += errString;
		if (worker) workerPool->returnWorker(worker);
	}

	return success;
}