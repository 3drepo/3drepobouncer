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

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/query_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>

using namespace repo::core::handler;
using namespace repo::core::handler::fileservice;
using namespace bsoncxx::builder::basic;

// mongocxx requires an instance to be created before using the driver, which
// must remain alive until all other mongocxx objects are destroyed. Only one
// instance must be created in a given program, even if the multiple instances
// have non-overlapping lifetimes. This pointer is initialised the first time
// a MongoDatabaseHandler is constructed, and destroyed when it goes out of
// scope (on the program exit).

std::unique_ptr<mongocxx::instance> instance;

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

MongoDatabaseHandler::MongoDatabaseHandler(
	const std::string			  &dbAddress,
	const uint32_t                &maxConnections,
	const std::string             &dbName,
	const std::string             &username,
	const std::string             &password) :
	AbstractDatabaseHandler(MAX_MONGO_BSON_SIZE)
{
	if (!instance) { // This global pointer should only be initialised once
		instance = std::make_unique<mongocxx::instance>();
	}
	mongocxx::uri uri(std::string("mongodb://") + username + ":" + password + "@" + dbAddress + "/?minPoolSize=3&maxPoolSize=" + std::to_string(maxConnections));
	clientPool = std::make_unique<mongocxx::pool>(uri);
}

MongoDatabaseHandler::~MongoDatabaseHandler()
{
}

void MongoDatabaseHandler::setFileManager(std::shared_ptr<FileManager> manager)
{
	this->fileManager = manager;
}

std::shared_ptr<FileManager> MongoDatabaseHandler::getFileManager()
{
	return this->fileManager.lock();
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
		try 
		{
			auto client = clientPool->acquire();
			auto db = client->database(database);
			db.create_collection(name);
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

void MongoDatabaseHandler::createIndex(const std::string &database, const std::string &collection, const repo::core::model::RepoBSON& obj)
{
	if (!(database.empty() || collection.empty()))
	{
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		repoInfo << "Creating index for :" << database << "." << collection << " : index: " << obj.toString();

		try 
		{
			col.create_index(obj.view());
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


/*
 * This helper function resolves the binary files for a given document.Any
 * document that might have file mappings should be passed through here
 * before being returned as a RepoBSON.
 */
repo::core::model::RepoBSON createRepoBSON(
	fileservice::BlobFilesHandler &blobHandler,
	const std::string &database,
	const std::string &collection,
	const bsoncxx::document::view& obj,
	const bool ignoreExtFile = false)
{
	repo::core::model::RepoBSON orgBson = repo::core::model::RepoBSON(obj);
	if (!ignoreExtFile) {
		if (orgBson.hasFileReference()) {
			auto ref = orgBson.getBinaryReference();
			auto buffer = blobHandler.readToBuffer(fileservice::DataRef::deserialise(ref));
			orgBson.initBinaryBuffer(buffer);
		}
	}
	return orgBson;
}

bool MongoDatabaseHandler::dropCollection(
	const std::string &database,
	const std::string &collection,
	std::string &errMsg)
{
	bool success = false;

	if (!database.empty() || collection.empty())
	{
		auto client = clientPool->acquire();
		auto dbObj = client->database(database);
		auto colObj = dbObj.collection(collection);

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
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		try {
			if (success = !bson.isEmpty() && bson.hasField("_id"))
			{
				bsoncxx::document::value queryDoc = make_document(kvp("_id", bson.getField("_id")));
				col.delete_one(queryDoc.view());
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
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		try {		

			fileservice::BlobFilesHandler blobHandler(getFileManager(), database, collection);
			
			// Find all documents
			auto cursor = col.find(criteria.view());
			for (auto& doc : cursor) {
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
	if (!criteria.isEmpty())
	{
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		try {
			fileservice::BlobFilesHandler blobHandler(getFileManager(), database, collection);
			
			mongocxx::options::find options{};
			options.sort(make_document(kvp(sortField, -1)));

			// Find document
			auto findResult = col.find_one(criteria.view(), options);
			if (findResult.has_value()) {
				return createRepoBSON(blobHandler, database, collection, findResult.value());
			}
		}
		catch (mongocxx::query_exception e)
		{
			repoError << "Error in MongoDatabaseHandler::findOneByCriteria: " << e.what();
		}
	}

	return repo::core::model::RepoBSON(make_document());
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
			auto client = clientPool->acquire();
			auto dbObj = client->database(database);
			auto col = dbObj.collection(collection);

			uint64_t retrieved = 0;			
			fileservice::BlobFilesHandler blobHandler(getFileManager(), database, collection);

			// To search for UUIDs, convert them to strings for the query document

			bsoncxx::builder::basic::array uids;
			for (auto& u : uuids) {
				uids.append(u.toString());
			}

			// TODO: Once this compiles someone REALLY needs to check whether this works as I understand it.
			// The documentation is a bit ambigous on this.
			auto queryDoc = make_document(kvp(ID, make_document(kvp("$in", uids))));
			auto cursor = col.find(queryDoc.view());

			for (auto doc : cursor) {
				data.push_back(createRepoBSON(blobHandler, database, collection, doc));
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
	try
	{
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		// Build Query
		repo::core::model::RepoBSONBuilder builder;
		builder.append("shared_id", uuid);
		auto queryDoc = builder.obj();

		// Create options
		mongocxx::options::find options{};
		options.sort(make_document(kvp(sortField, -1)));

		fileservice::BlobFilesHandler blobHandler(getFileManager(), database, collection);
		
		// Find document
		auto findResult = col.find_one(queryDoc.view(), options);

		if (findResult.has_value()) {
			auto doc = findResult.value();
			return createRepoBSON(blobHandler, database, collection, doc);
		}

	}
	catch (mongocxx::query_exception e)
	{
		repoError << "Error querying the database: " << std::string(e.what());
	}

	return repo::core::model::RepoBSON(make_document());
}

repo::core::model::RepoBSON  MongoDatabaseHandler::findOneByUniqueID(
	const std::string& database,
	const std::string& collection,
	const repo::lib::RepoUUID& uuid) {

	try
	{
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		repo::core::model::RepoBSONBuilder builder;
		builder.append(ID, uuid);
		auto queryDoc = builder.obj();

		auto findResult = col.find_one(queryDoc.view());

		if (findResult.has_value()) {
			fileservice::BlobFilesHandler blobHandler(getFileManager(), database, collection);
			return createRepoBSON(blobHandler, database, collection, findResult.value());
		}
	}
	catch (mongocxx::query_exception& e)
	{
		repoError << e.what();
	}

	return repo::core::model::RepoBSON(make_document());
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
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		mongocxx::options::find options{};
		if (!sortField.empty()) 
		{
			options.sort(make_document(kvp(sortField, sortOrder)));
		}
		options.limit(limit);
		options.skip(skip);

		// Append the projection (as a document) - this should go onto the options
		if (fields.size() > 0)
		{
			bsoncxx::builder::basic::document projection;
			for (auto& n : fields)
			{
				projection.append(kvp(n, 1));
			}
			options.projection(projection.extract());
		}

		fileservice::BlobFilesHandler blobHandler(getFileManager(), database, collection);

		auto cursor = col.find({}, options);

		for (auto doc : cursor) {
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
		auto client = clientPool->acquire();
		auto db = client->database(database);
		
		auto collectionsVector = db.list_collection_names();

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
	return getFileManager()->getFile(database, collection, fileName);
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

std::shared_ptr<MongoDatabaseHandler> MongoDatabaseHandler::getHandler(
	const std::string &connectionString,
	const uint32_t    &maxConnections,
	const std::string &dbName,
	const std::string &username,
	const std::string &password)
{
	return std::make_shared<MongoDatabaseHandler>(connectionString, maxConnections, dbName, username, password);
}

std::shared_ptr<MongoDatabaseHandler> MongoDatabaseHandler::getHandler(
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
	return std::make_shared<MongoDatabaseHandler>(connectionString, maxConnections, dbName, username, password);
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
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		try 
		{
			col.insert_one(obj.view());
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
			auto client = clientPool->acquire();
			auto db = client->database(database);
			auto col = db.collection(collection);

			fileservice::BlobFilesHandler blobHandler(getFileManager(), database, collection, binaryStorageMetadata);

			for (int i = 0; i < objs.size(); i += MAX_PARALLEL_BSON) {
				std::vector<repo::core::model::RepoBSON>::const_iterator it = objs.begin() + i;
				std::vector<repo::core::model::RepoBSON>::const_iterator last = i + MAX_PARALLEL_BSON >= objs.size() ? objs.end() : it + MAX_PARALLEL_BSON;
				std::vector<bsoncxx::document::view_or_value> toCommit;
				do {
					auto node = *it;
					auto data = node.getBinariesAsBuffer();
					if (data.second.size()) {
						auto ref = blobHandler.insertBinary(data.second);
						node.replaceBinaryWithReference(ref.serialise(), data.first);
					}
					toCommit.push_back(node.view()); // Get the view to commit - this must be done before the method returns in case the original RepoBSON goes out of scope

				} while (++it != last);

				repoInfo << "Inserting " << toCommit.size() << " documents...";
				col.insert_many(toCommit);				
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
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		auto queryDoc = make_document(kvp(ID, obj.getField(ID)));

		auto existing = col.find_one(queryDoc.view());
		if (!existing.has_value())
		{
			upsert = true;
		}

		if (upsert) {
			col.insert_one(obj.view());
		}
		else {
			col.update_one(
				queryDoc.view(), 
				make_document(kvp("$set", obj.view()))
			);
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