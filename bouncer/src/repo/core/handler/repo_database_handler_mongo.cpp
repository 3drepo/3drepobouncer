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
#include "fileservice/repo_blob_files_handler.h"
#include "repo/core/model/bson/repo_bson_builder.h"
#include "repo/core/model/bson/repo_bson_element.h"
#include "repo/lib/repo_log.h"

using namespace repo::core::handler;
using namespace bsoncxx::builder::basic;

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
	const std::string			  &dbAddress,
	const uint32_t                &maxConnections,
	const std::string             &dbName,
	const std::string             &username,
	const std::string             &password) :
	AbstractDatabaseHandler(MAX_MONGO_BSON_SIZE)
{
	// Create instance
	instance = mongocxx::instance{};

	// Create URI
	// TODO: Test needed whether this connection string is assembled in the correct format. I am only about 70% sure that it is.
	// TODO: Is using maxConnections for the max pool size the correct way here?
	bsoncxx::string::view_or_value uriString = username + ":" + password + "@" + dbAddress + "/?minPoolSize=3&maxPoolSize=" + std::to_string(maxConnections);
	mongocxx::uri uri = mongocxx::uri(uriString);

	// Create pool	
	clientPool = std::unique_ptr <mongocxx::pool>(new mongocxx::pool(uri));	
	
}


/**
* A Deconstructor
*/
MongoDatabaseHandler::~MongoDatabaseHandler()
{
	// TODO: Might not be necessary. Example just lets it get out of scope.
	clientPool->~pool();

	// Same for instance.

}

bool MongoDatabaseHandler::caseInsensitiveStringCompare(
	const std::string& s1,
	const std::string& s2)
{
	return strcasecmp(s1.c_str(), s2.c_str()) <= 0;
}

void MongoDatabaseHandler::createCollection(const std::string &database, const std::string &name)
{
	if (!(database.empty() || name.empty()))
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto nameString = bsoncxx::string::view_or_value(name);

		// Get database
		auto dbObj = client->database(dbString);

		// Create collection
		try {
			dbObj.create_collection(nameString);
		}
		catch (mongocxx::operation_exception e) {
			repoError << "Failed to create collection ("
				<< database << "." << name << ":" << e.what();
		}
	}
	else
	{
		repoError << "Failed to create collection: database(value: " << database << ")/collection(value: " << name << ") name is empty!";
	}
}

void MongoDatabaseHandler::createIndex(const std::string &database, const std::string &collection, const bsoncxx::document::view_or_value& obj)
{
	if (!(database.empty() || collection.empty()))
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		// TODO: Can the new driver's obj be printed like that?
		repoInfo << "Creating index for :" << database << "." << collection << " : index: " << obj;

		try {			
			colObj.create_index(obj);
			//worker->createIndex(database + "." + collection, obj);			
		}
		catch (mongocxx::operation_exception e)
		{
			repoError << "Failed to create index ("
				<< database << "." << collection << ":" << e.what();
		}
	}
	else
	{
		repoError << "Failed to create index: database(value: " << database << ")/collection(value: " << collection << ") name is empty!";
	}
}

// This will presumably be refactored/deleted/replaced with Sebastian's changes to RepoBSON
repo::core::model::RepoBSON createRepoBSON(
	fileservice::BlobFilesHandler &blobHandler,
	const std::string &database,
	const std::string &collection,
	const mongo::BSONObj &obj,
	const bool ignoreExtFile = false)
{
	repo::core::model::RepoBSON orgBson = repo::core::model::RepoBSON(obj);
	if (!ignoreExtFile) {
		if (orgBson.hasFileReference()) {
			auto ref = orgBson.getBinaryReference();
			auto buffer = blobHandler.readToBuffer(fileservice::DataRef::deserialise(ref));
			orgBson.initBinaryBuffer(buffer);
		}
		else if (orgBson.hasField(REPO_LABEL_OVERSIZED_FILES)) {
			// The _extRef support has been deprecated, and collections should have been
			// updated by the 5.4 migration scripts.
			// In case we have any old documents remaining, this snippet can read the data,
			// though the filenames will not be accessible.

			auto extRefbson = orgBson.getObjectField(REPO_LABEL_OVERSIZED_FILES);
			std::set<std::string> fieldNames = extRefbson.getFieldNames();
			repo::core::model::RepoBSON::BinMapping map;
			auto fileManager = fileservice::FileManager::getManager();
			for (const auto& name : fieldNames)
			{
				auto fname = extRefbson.getStringField(name);
				auto file = fileManager->getFile(database, collection, fname);
				map[name] = file;
			}
			return repo::core::model::RepoBSON(obj, map);
		}
	}
	return orgBson;
}

/// <summary>
/// Mock function as a standin for whatever magic Sebastian uses to get the documents out of RepoBSON.
/// Not to be used. Just to signal where the conversion might occur.
/// </summary>
/// <param name="bson">A BSON to be "converted"</param>
/// <returns>A document in the driver's format</returns>
bsoncxx::document::value RepoBSONToDocument(repo::core::model::RepoBSON bson) {
	return make_document(5);
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

	if (!database.empty() || collection.empty())
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		try {
			colObj.drop();

			// Operation is succesful if no exception is thrown
			success = true;
		}
		catch (mongocxx::operation_exception e)
		{
			errMsg = "Failed to drop collection ("
				+ database + "." + collection + ":" + e.what();
			success = false;
		}
	}
	else
	{
		errMsg = "Failed to drop collection: either database (value: " + database + ") or collection (value: " + collection + ") is empty";
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

	if (!database.empty() && !collection.empty())
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		try {
			if (success = !bson.isEmpty() && bson.hasField("_id"))
			{
				auto queryDoc = make_document(kvp("_id", bson.getField("_id")));
				colObj.delete_one(queryDoc);
			//	mongo::BSONElement id;
			//	bson.getObjectID(id);
			//	mongo::Query query = MONGO_QUERY("_id" << id);
			//	worker->remove(database + "." + collection, query, true);
			}
			else
			{
				errMsg = "Failed to drop document: id not found";
			}
		}
		catch (mongocxx::bulk_write_exception e)
		{
			errMsg = "Failed to drop document :" + std::string(e.what());
			success = false;
		}
	}
	else
	{
		errMsg = "Failed to drop document: either database (value: " + database + ") or collection (value: " + collection + ") is empty";
	}

	return success;
}

std::vector<repo::core::model::RepoBSON> MongoDatabaseHandler::findAllByCriteria(
	const std::string& database,
	const std::string& collection,
	const repo::core::model::RepoBSON& criteria)
{
	std::vector<repo::core::model::RepoBSON> data;

	if (!criteria.isEmpty())
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		try {		

			auto fileManager = fileservice::FileManager::getManager();
			fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);
			
			// Assemble query
			auto fieldNames = criteria.getFieldNames();
			std::vector<std::tuple<std::string, repo::core::model::RepoBSONElement>> kvps;
			for (auto fieldName : fieldNames) {
				auto kvp = bsoncxx::builder::basic::kvp(fieldName, criteria.getField(fieldName));
				kvps.push_back(kvp);
			}
			auto queryDoc = bsoncxx::builder::basic::make_document(kvps);

			// Find all documents
			auto cursor = colObj.find(queryDoc);

			for (auto doc : cursor) {
				// TODO: createRepoBSON will presumably be refactored with Sebastian's changes to RepoBSON
				data.push_back(createRepoBSON(blobHandler, database, collection, doc));
			}
		}
		catch (mongocxx::logic_error e)
		{
			repoError << "Error in MongoDatabaseHandler::findAllByCriteria: " << e.what();
		}
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
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		try {

			auto fileManager = fileservice::FileManager::getManager();
			fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

			// Assemble query
			auto fieldNames = criteria.getFieldNames();
			std::vector<std::tuple<std::string, repo::core::model::RepoBSONElement>> kvps;
			for (auto fieldName : fieldNames) {
				auto kvp = bsoncxx::builder::basic::kvp(fieldName, criteria.getField(fieldName));
				kvps.push_back(kvp);
			}
			auto queryDoc = bsoncxx::builder::basic::make_document(kvps);
			
			mongocxx::options::find options{};
			options.sort(make_document(kvp(sortField, -1)));

			// Find document
			auto findResult = colObj.find_one(queryDoc, options);

			if (findResult.has_value()) {
				auto doc = findResult.value();

				// TODO: createRepoBSON will presumably be refactored with Sebastian's changes to RepoBSON
				data = createRepoBSON(blobHandler, database, collection, doc);
			}

			
		}
		catch (mongocxx::query_exception e)
		{
			repoError << "Error in MongoDatabaseHandler::findOneByCriteria: " << e.what();
		}
	}

	return data;
}

std::vector<repo::core::model::RepoBSON> MongoDatabaseHandler::findAllByUniqueIDs(
	const std::string& database,
	const std::string& collection,
	const std::vector<repo::lib::RepoUUID> uuids,
	const bool ignoreExtFiles) {
	std::vector<repo::core::model::RepoBSON> data;

	
	int fieldsCount = uuids.size();
	if (fieldsCount > 0)
	{
		try {

			// First, acquire client from pool
			auto client = clientPool->acquire();

			// Convert into bsoncxx string format
			auto dbString = bsoncxx::string::view_or_value(database);
			auto colString = bsoncxx::string::view_or_value(collection);

			// Get database
			auto dbObj = client->database(dbString);

			// Get collection
			auto colObj = dbObj.collection(colString);


			uint64_t retrieved = 0;			
			auto fileManager = fileservice::FileManager::getManager();
			fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

			// TODO: Once this compiles someone REALLY needs to check whether this works as I understand it.
			// The documentation is a bit ambigous on this.
			auto queryDoc = make_document(kvp(ID, make_document(kvp("$in", uuids))));
			auto cursor = colObj.find(queryDoc);

			for (auto doc : cursor) {
				// TODO: createRepoBSON will presumably be refactored with Sebastian's changes to RepoBSON
				data = createRepoBSON(blobHandler, database, collection, doc);
				retrieved++;
			}


			if (fieldsCount != retrieved) {
				repoWarning << "Number of documents(" << retrieved << ") retreived by findAllByUniqueIDs did not match the number of unique IDs(" << fieldsCount << ")!";
			}
		}
		catch (mongocxx::logic_error e)
		{
			repoError << e.what();
		}
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

	try
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		// Build Query
		auto queryDoc = make_document(kvp("shared_id", uuid));

		// Create options
		mongocxx::options::find options{};
		options.sort(make_document(kvp(sortField, -1)));

		auto fileManager = fileservice::FileManager::getManager();
		fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);
		
		// Find document
		auto findResult = colObj.find_one(queryDoc, options);

		if (findResult.has_value()) {
			auto doc = findResult.value();
	//	auto query = mongo::Query(queryBuilder.obj());
	//	if (!sortField.empty())
	//		query = query.sort(sortField, -1);

			// TODO: createRepoBSON will presumably be refactored with Sebastian's changes to RepoBSON
			bson = createRepoBSON(blobHandler, database, collection, doc);
		}

	}
	catch (mongocxx::query_exception e)
	{
		repoError << "Error querying the database: " << std::string(e.what());
	}

	return bson;
}

repo::core::model::RepoBSON  MongoDatabaseHandler::findOneByUniqueID(
	const std::string& database,
	const std::string& collection,
	const repo::lib::RepoUUID& uuid) {
	repo::core::model::RepoBSON bson;

	try
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		// Build Query
		auto queryDoc = make_document(kvp(ID, uuid));


		auto fileManager = fileservice::FileManager::getManager();
		fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);
		
		// Find document
		auto findResult = colObj.find_one(queryDoc);

		if (findResult.has_value()) {
			auto doc = findResult.value();

			// TODO: createRepoBSON will presumably be refactored with Sebastian's changes to RepoBSON
			bson = createRepoBSON(blobHandler, database, collection, doc);
		}
	//	mongo::BSONObj bsonMongo = worker->findOne(getNamespace(database, collection),
	//		mongo::Query(queryBuilder.obj()));

	}
	catch (mongocxx::query_exception& e)
	{
		repoError << e.what();
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

	try
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		// Create options
		mongocxx::options::find options{};
		if(!sortField.empty())
			options.sort(make_document(kvp(sortField, sortOrder)));
		options.limit(limit);
		options.skip(skip);
		
		// If only certain fields are to be returned, prepare projection and add it to the options
		if (fields.size() > 0) {
			std::vector<std::tuple<std::string, int>> kvps;
			for (std::string field : fields) {
				auto pair = kvp(field, 1);
				kvps.push_back(pair);
			}
			auto projDoc = make_document(kvps);
			options.projection(bsoncxx::v_noabi::document::view_or_value(projDoc));
		}

		auto fileManager = fileservice::FileManager::getManager();
		fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

		// Execute query
		auto cursor = colObj.find({}, options);

		for (auto doc : cursor) {
			// TODO: createRepoBSON will presumably be refactored with Sebastian's changes to RepoBSON
			bsons.push_back(createRepoBSON(blobHandler, database, collection, doc));
		}
	}
	catch (mongocxx::logic_error e)
	{
		repoError << "Failed retrieving bsons from mongo: " << e.what();
	}

	return bsons;
}

std::list<std::string> MongoDatabaseHandler::getCollections(
	const std::string &database)
{
	std::list<std::string> collections;

	try
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);

		// Get database
		auto dbObj = client->database(dbString);
		
		auto collectionsVector = dbObj.list_collection_names();

		collections = std::list<std::string>(collectionsVector.begin(), collectionsVector.end());
	}
	catch (mongocxx::operation_exception& e)
	{
		repoError << e.what();
	}

	return collections;
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

MongoDatabaseHandler* MongoDatabaseHandler::getHandler(
	std::string       &errMsg,
	const std::string &connectionString,
	const uint32_t    &maxConnections,
	const std::string &dbName,
	const std::string &username,
	const std::string &password)
{
	if (!handler) {
		

		//initialise the mongo client
		repoTrace << "Handler not present for " << connectionString << " instantiating new handler...";
		try {
			handler = new MongoDatabaseHandler(connectionString, maxConnections, dbName, username, password);
		}
		catch (mongocxx::logic_error e) {
			if (handler)
				delete handler;
			handler = 0;
			errMsg = std::string(e.what());
			repoError << "Error establishing Mongo Handler: " << errMsg;
		}
		catch (mongocxx::exception e) {
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
	const std::string &password)
{
		
	std::string connectionString;
	if (port >= 0)
	{
		connectionString = host +":" + std::to_string(port);
	}
	else {
		repoWarning << "GetHandler called with invalid port provided. Connection is attempted without port.";
		connectionString = host;
	}
	
	if (!handler) {
		//initialise the mongo client
		repoTrace << "Handler not present for " << connectionString << " instantiating new handler...";
		try {
			handler = new MongoDatabaseHandler(connectionString, maxConnections, dbName, username, password);
		}
		catch (mongocxx::logic_error e) {
			if (handler)
				delete handler;
			handler = 0;
			errMsg = std::string(e.what());
			repoError << "Error establishing Mongo Handler: " << errMsg;
		}
		catch (mongocxx::exception e) {
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
	std::string connectionString;
	if (port >= 0)
	{
		connectionString = host + ":" + std::to_string(port);
	}
	else {
		repoWarning << "GetHandler called with invalid port provided. Connection is attempted without port.";
		connectionString = host;
	}

	// Unpack credential object	
	std::string username = credentials->getStringField("user");
	std::string password = credentials->getStringField("pwd");

	if (!handler) {
		//initialise the mongo client
		repoTrace << "Handler not present for " << connectionString << " instantiating new handler...";
		try {			
			handler = new MongoDatabaseHandler(connectionString, maxConnections, dbName, username, password);
		}
		catch (mongocxx::logic_error e) {
			if (handler)
				delete handler;
			handler = 0;
			errMsg = std::string(e.what());
			repoError << "Error establishing Mongo Handler: " << errMsg;
		}
		catch (mongocxx::exception e) {
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

bool MongoDatabaseHandler::insertDocument(
	const std::string &database,
	const std::string &collection,
	const repo::core::model::RepoBSON &obj,
	std::string &errMsg)
{
	bool success = false;

	if (obj.hasOversizeFiles())
	{
		throw repo::lib::RepoException("insertDocument cannot be used with BSONs holding binary files. Use insertManyDocuments instead.");
	}

	if (!database.empty() || collection.empty())
	{
		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		// Convert RepoBSON into the mongo format
		auto doc = RepoBSONToDocument(obj);

		try {
			colObj.insert_one(doc);			
			success = true;
		}
		catch (mongocxx::bulk_write_exception &e)
		{
			std::string errString(e.what());
			errMsg += errString;
		}
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
	std::string &errMsg,
	const Metadata& binaryStorageMetadata)
{
	bool success = false;

	if (!database.empty() || collection.empty())
	{
		try {

			// First, acquire client from pool
			auto client = clientPool->acquire();

			// Convert into bsoncxx string format
			auto dbString = bsoncxx::string::view_or_value(database);
			auto colString = bsoncxx::string::view_or_value(collection);

			// Get database
			auto dbObj = client->database(dbString);

			// Get collection
			auto colObj = dbObj.collection(colString);

			auto fileManager = fileservice::FileManager::getManager();
			fileservice::BlobFilesHandler blobHandler(fileManager, database, collection, binaryStorageMetadata);

			for (int i = 0; i < objs.size(); i += MAX_PARALLEL_BSON) {
				std::vector<repo::core::model::RepoBSON>::const_iterator it = objs.begin() + i;
				std::vector<repo::core::model::RepoBSON>::const_iterator last = i + MAX_PARALLEL_BSON >= objs.size() ? objs.end() : it + MAX_PARALLEL_BSON;
				std::vector<bsoncxx::document::value> toCommit;
				do {
					auto node = *it;
					auto data = node.getBinariesAsBuffer();
					if (data.second.size()) {
						auto ref = blobHandler.insertBinary(data.second);
						node.replaceBinaryWithReference(ref.serialise(), data.first);
					}

					// Convert RepoBSON into the mongo format
					auto doc = RepoBSONToDocument(node);

					toCommit.push_back(doc);

				} while (++it != last);

				repoInfo << "Inserting " << toCommit.size() << " documents...";

				colObj.insert_many(toCommit);				
			}

			blobHandler.finished();

			success = true;
		}

		catch (mongocxx::bulk_write_exception &e)
		{
			std::string errString(e.what());
			errMsg += errString;
		}
	}
	else
	{
		errMsg = "Unable to insert Document, database(value : " + database + ")/collection(value : " + collection + ") name was not specified";
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

	if (obj.hasOversizeFiles())
	{
		throw repo::lib::RepoException("upsertDocument cannot be used with BSONs holding binary files.");
	}

	bool upsert = overwrite;
	try {

		// First, acquire client from pool
		auto client = clientPool->acquire();

		// Convert into bsoncxx string format
		auto dbString = bsoncxx::string::view_or_value(database);
		auto colString = bsoncxx::string::view_or_value(collection);

	//	mongo::BSONElement bsonID;
	//	obj.getObjectID(bsonID);
	//	mongo::Query existQuery = MONGO_QUERY("_id" << bsonID);
	//	mongo::BSONObj bsonMongo = worker->findOne(database + "." + collection, existQuery);

		// Get database
		auto dbObj = client->database(dbString);

		// Get collection
		auto colObj = dbObj.collection(colString);

		// Build Query
		auto queryDoc = make_document(kvp(ID, obj.getField(ID)));

		// Find document
		auto docExist = colObj.find_one(queryDoc);

		if (!docExist.has_value())
		{
			//document doens't exist, insert the document
			upsert = true;
		}

		// Convert RepoBSON into the mongo format
		auto doc = RepoBSONToDocument(obj);

		if (upsert) {
			colObj.insert_one(queryDoc, doc);
		}
		else {
			colObj.update_one(queryDoc, doc);
		}
	}
	catch (mongocxx::bulk_write_exception e)
	{
		success = false;
		std::string errString(e.what());
		errMsg += errString;
	}
	catch (mongocxx::logic_error e)
	{
		success = false;
		std::string errString(e.what());
		errMsg += errString;
	}
	catch (repo::lib::RepoException& e)
	{
		success = false;
		std::string errString(e.what());
		errMsg += errString;
	}

	return success;
}

repo::core::model::RepoBSON* MongoDatabaseHandler::createBSONCredentials(
	const std::string& dbName,
	const std::string& username,
	const std::string& password,
	const bool& pwDigested)
{
	mongo::BSONObj* mongoBSON = createAuthBSON(dbName, username, password, pwDigested);
	return mongoBSON ? new repo::core::model::RepoBSON(*mongoBSON) : nullptr;
}