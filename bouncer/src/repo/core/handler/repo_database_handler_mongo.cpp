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
#include "database/repo_expressions.h"

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/query_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view_or_value.hpp>
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
const std::string repo::core::handler::MongoDatabaseHandler::SYSTEM_ROLES_COLLECTION = "system.roles";
const std::list<std::string> repo::core::handler::MongoDatabaseHandler::ANY_DATABASE_ROLES =
{ "dbAdmin", "dbOwner", "read", "readWrite", "userAdmin" };
const std::list<std::string> repo::core::handler::MongoDatabaseHandler::ADMIN_ONLY_DATABASE_ROLES =
{ "backup", "clusterAdmin", "clusterManager", "clusterMonitor", "dbAdminAnyDatabase",
"hostManager", "readAnyDatabase", "readWriteAnyDatabase", "restore", "root",
"userAdminAnyDatabase" };

MongoDatabaseHandler::MongoDatabaseHandler(
	const std::string& dbAddress,
	const std::string& username,
	const std::string& password,
	const ConnectionOptions& options) :
	AbstractDatabaseHandler(MAX_MONGO_BSON_SIZE)
{
	if (!instance) { // This global pointer should only be initialised once
		instance = std::make_unique<mongocxx::instance>();
	}
	mongocxx::uri uri(
		std::string("mongodb://") +
		username + ":" +
		password + "@" +
		dbAddress + "/?" +
		"maxConnecting=" + std::to_string(options.maxConnections) +
		"&socketTimeoutMS=" + std::to_string(options.timeout) +
		"&serverSelectionTimeoutMS=" + std::to_string(options.timeout) +
		"&connectTimeoutMS=" + std::to_string(options.timeout)
	);
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
	return this->fileManager;
}

bool MongoDatabaseHandler::caseInsensitiveStringCompare(
	const std::string& s1,
	const std::string& s2)
{
	return strcasecmp(s1.c_str(), s2.c_str()) <= 0;
}

void MongoDatabaseHandler::createCollection(const std::string &database, const std::string &name)
{
	auto client = clientPool->acquire();
	auto db = client->database(database);
	db.create_collection(name);
}

void MongoDatabaseHandler::createIndex(const std::string &database, const std::string &collection, const database::index::RepoIndex& index)
{
	if (!(database.empty() || collection.empty()))
	{
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);
		auto obj = (repo::core::model::RepoBSON)index;

		repoInfo << "Creating index for :" << database << "." << collection << " : index: " << obj.toString();

		try
		{
			col.create_index(obj.view());
		}
		catch (mongocxx::operation_exception e)
		{
			repoError << "Failed to create index (" << database << "." << collection << ":" << e.what();
			throw e;
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

void MongoDatabaseHandler::dropCollection(
	const std::string &database,
	const std::string &collection)
{
	auto client = clientPool->acquire();
	auto dbObj = client->database(database);
	auto colObj = dbObj.collection(collection);
	colObj.drop();
}

void MongoDatabaseHandler::dropDocument(
	const repo::core::model::RepoBSON bson,
	const std::string &database,
	const std::string &collection)
{
	auto client = clientPool->acquire();
	auto db = client->database(database);
	auto col = db.collection(collection);

	if (!bson.isEmpty() && bson.hasField("_id"))
	{
		bsoncxx::document::value queryDoc = make_document(kvp("_id", bson.getField("_id")));
		col.delete_one(queryDoc.view());
	}
	else
	{
		throw repo::lib::RepoException("Cannot drop a document without an _id field");
	}
}

std::vector<repo::core::model::RepoBSON> MongoDatabaseHandler::findAllByCriteria(
	const std::string& database,
	const std::string& collection,
	const database::query::RepoQuery& filter)
{
	std::vector<repo::core::model::RepoBSON> data;
	repo::core::model::RepoBSON criteria = filter;
	if (!criteria.isEmpty())
	{
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		try {

			fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

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
	const database::query::RepoQuery& filter,
	const std::string& sortField)
{
	repo::core::model::RepoBSON criteria = filter;
	if (!criteria.isEmpty())
	{
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		try {
			mongocxx::options::find options{};
			if (!sortField.empty()) {
				options.sort(make_document(kvp(sortField, -1)));
			}

			// Find document
			auto findResult = col.find_one(criteria.view(), options);
			if (findResult.has_value()) {
				fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);
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

repo::core::model::RepoBSON MongoDatabaseHandler::findOneBySharedID(
	const std::string& database,
	const std::string& collection,
	const repo::lib::RepoUUID& uuid,
	const std::string& sortField)
{
	return findOneByCriteria(database, collection, database::query::Eq(std::string("shared_id"), uuid), sortField);
}

repo::core::model::RepoBSON  MongoDatabaseHandler::findOneByUniqueID(
	const std::string& database,
	const std::string& collection,
	const repo::lib::RepoUUID& id)
{
	return findOneByCriteria(database, collection, database::query::Eq(ID, id));
}

repo::core::model::RepoBSON  MongoDatabaseHandler::findOneByUniqueID(
	const std::string& database,
	const std::string& collection,
	const std::string& id)
{
	repo::core::model::RepoBSONBuilder builder;
	builder.append(ID, id);
	auto queryDoc = builder.obj();
	return findOneByCriteria(database, collection, database::query::Eq(ID, id));
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

	std::vector<repo::core::model::RepoBSON> bsons;

	fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

	auto cursor = col.find({}, options);
	for (auto doc : cursor) {
		bsons.push_back(createRepoBSON(blobHandler, database, collection, doc));
	}

	return bsons;
}

std::list<std::string> MongoDatabaseHandler::getCollections(
	const std::string &database)
{
	auto client = clientPool->acquire();
	auto db = client->database(database);
	auto collectionsVector = db.list_collection_names();
	return std::list<std::string>(collectionsVector.begin(), collectionsVector.end());
}

std::vector<uint8_t> MongoDatabaseHandler::getBigFile(
	const std::string &database,
	const std::string &collection,
	const std::string &fileName)
{
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

std::shared_ptr<MongoDatabaseHandler> MongoDatabaseHandler::getHandler(
	const std::string &connectionString,
	const std::string& username,
	const std::string& password,
	const ConnectionOptions& options
)
{
	return std::make_shared<MongoDatabaseHandler>(connectionString, username, password, options);
}

std::shared_ptr<MongoDatabaseHandler> MongoDatabaseHandler::getHandler(
	const std::string &host,
	const int         &port,
	const std::string& username,
	const std::string& password,
	const ConnectionOptions& options
)
{
	return getHandler(host + ":" + std::to_string(port), username, password, options);
}

std::string MongoDatabaseHandler::getNamespace(
	const std::string &database,
	const std::string &collection)
{
	return database + "." + collection;
}

void MongoDatabaseHandler::insertDocument(
	const std::string &database,
	const std::string &collection,
	const repo::core::model::RepoBSON &obj)
{
	bool success = false;

	if (obj.hasOversizeFiles())
	{
		throw repo::lib::RepoException("insertDocument cannot be used with BSONs holding binary files. Use insertManyDocuments instead.");
	}

	auto client = clientPool->acquire();
	auto db = client->database(database);
	auto col = db.collection(collection);
	col.insert_one(obj.view());
}

void MongoDatabaseHandler::insertManyDocuments(
	const std::string &database,
	const std::string &collection,
	const std::vector<repo::core::model::RepoBSON> &objs,
	const Metadata& binaryStorageMetadata)
{
	auto client = clientPool->acquire();
	auto db = client->database(database);
	auto col = db.collection(collection);

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
			toCommit.push_back(node);
		} while (++it != last);

		repoInfo << "Inserting " << toCommit.size() << " documents...";
		col.insert_many(toCommit);
	}

	blobHandler.finished();
}

void MongoDatabaseHandler::upsertDocument(
	const std::string &database,
	const std::string &collection,
	const repo::core::model::RepoBSON &obj,
	const bool        &overwrite)
{
	if (obj.hasOversizeFiles())
	{
		throw repo::lib::RepoException("upsertDocument cannot be used with BSONs holding binary files.");
	}

	auto client = clientPool->acquire();
	auto db = client->database(database);
	auto col = db.collection(collection);

	bsoncxx::builder::basic::document query;

	if (overwrite)
	{
		mongocxx::options::replace options{};
		options.upsert(true);

		col.replace_one(
			query.view(),
			obj.view(),
			options
		);
	}
	else
	{
		// The following snippet makes the update document, which consists of
		// a command to set all mutable fields, and set the immutable _id field
		// only when performing an insert.

		bsoncxx::builder::basic::document set;
		bsoncxx::builder::basic::document setOnInsert;

		for (const auto& e : obj)
		{
			if (e.key() == "_id")
			{
				query.append(kvp(e.key(), e.get_value()));
				setOnInsert.append(kvp(e.key(), e.get_value()));
			}
			else
			{
				set.append(kvp(e.key(), e.get_value()));
			}
		}

		auto update = make_document(
			kvp("$set", set.view()),
			kvp("$setOnInsert", setOnInsert.view())
		);

		mongocxx::options::update options{};
		options.upsert(true);

		col.update_one(
			query.view(),
			update.view(),
			options
		);
	}
}