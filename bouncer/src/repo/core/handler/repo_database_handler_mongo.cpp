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
#include "repo/lib/repo_exception.h"
#include "database/repo_expressions.h"

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
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

// The 'admin' database for checking a connection - this can be any database
// against which getCollections is cheap and for which permissions are
// representative.
#define ADMIN "admin"

class MongoDatabaseHandler::MongoDatabaseHandlerException : public repo::lib::RepoException
{
public:
	MongoDatabaseHandlerException(const MongoDatabaseHandler& handler, const std::string& method, const std::string& db, const std::string collection)
		: RepoException("MongoDatabaseHandler exception: in " + method + " on: " + db + "." + collection + getUri(handler))
	{
		errorCode = REPOERR_AUTH_FAILED; //If no outer exception sets the return code, signal this is database operation/connection problem.
	}

	MongoDatabaseHandlerException(const MongoDatabaseHandler& handler, const std::string& method, const std::string& db)
		: RepoException("MongoDatabaseHandler exception: in " + method + " on: " + db + getUri(handler))
	{
		errorCode = REPOERR_AUTH_FAILED;
	}

	MongoDatabaseHandlerException(const MongoDatabaseHandler& handler, const std::string& msg)
		: RepoException("MongoDatabaseHandler exception: " + msg + getUri(handler))
	{
		errorCode = REPOERR_AUTH_FAILED;
	}

	MongoDatabaseHandlerException(const std::string& msg)
		: RepoException("MongoDatabaseHandler exception: " + msg)
	{
		errorCode = REPOERR_AUTH_FAILED;
	}

private:
	std::string getUri(const MongoDatabaseHandler& handler)
	{
		try
		{
			auto pool = handler.clientPool->acquire();
			auto uri = pool->uri().to_string();
			auto to = uri.find("@");
			if (to != std::string::npos)
			{
				for (auto i = uri.find("://") + 3; i < to; i++) {
					uri[i] = '*';
				}
			}
			return std::string(" Uri: ") + uri;
		}
		catch (...)
		{
			return std::string(" Uri: Unable to get URI");
		}
	}
};

MongoDatabaseHandler::MongoDatabaseHandler(
	const std::string& connectionString,
	const std::string& username,
	const std::string& password,
	const ConnectionOptions& options) :
	AbstractDatabaseHandler(MAX_MONGO_BSON_SIZE)
{
	if (!instance) { // This global pointer should only be initialised once
		instance = std::make_unique<mongocxx::instance>();
	}

	auto s = connectionString; // (A copy of the string we can modify)

	// The only valid use of @  in a Mongo connection string is to delimit
	// credentials (special characters are percent encoded).

	if (s.find("@") == std::string::npos && !username.empty() && !password.empty())
	{
		// The connection string doesn't have credentials, so add them here if
		// provided in separate config members. It is permissable to have no
		// username and password, if, for example, TLS is configured instead.

		s.insert(s.find("://") + 3, username + ":" + password + "@");
	}

	std::string optionsPrefix = s.find("?") == std::string::npos ? "/?" : "&";
	s += optionsPrefix + "maxConnecting=" + std::to_string(options.maxConnections) +
		"&socketTimeoutMS=" + std::to_string(options.timeout) +
		"&serverSelectionTimeoutMS=" + std::to_string(options.timeout) +
		"&connectTimeoutMS=" + std::to_string(options.timeout);

	mongocxx::uri uri(s);
	clientPool = std::make_unique<mongocxx::pool>(uri);
}

MongoDatabaseHandler::~MongoDatabaseHandler()
{
}

void MongoDatabaseHandler::testConnection()
{
	try {
		// The current version of this test is to enumerate the admins database, which
		// should always be available, require similar permissions to being able to
		// create collections etc, but should not have too many entries that waste
		// time returning.
		getCollections(ADMIN);
	}
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException("testConnection() failed.")); // (The nested exception will print the URI)
	}
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

void MongoDatabaseHandler::createIndex(const std::string& database, const std::string& collection, const database::index::RepoIndex& index)
{
	try
	{
		if (!(database.empty() || collection.empty()))
		{
			auto client = clientPool->acquire();
			auto db = client->database(database);
			auto col = db.collection(collection);
			auto obj = (repo::core::model::RepoBSON)index;

			repoInfo << "Creating index for :" << database << "." << collection << " : index: " << obj.toString();

			col.create_index(obj.view());
		}
	}
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "createIndex", database, collection));
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
	try
	{
		if (!database.empty() && !collection.empty())
		{
			auto client = clientPool->acquire();
			auto dbObj = client->database(database);
			auto colObj = dbObj.collection(collection);
			colObj.drop();
		}
	}
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "dropCollection", database, collection));
	}
}

void MongoDatabaseHandler::dropDocument(
	const repo::core::model::RepoBSON bson,
	const std::string &database,
	const std::string &collection)
{
	try
	{
		if (database.empty() || collection.empty())
		{
			return;
		}

		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);

		auto value = bson.find("_id");
		if (value != bson.end())
		{
			bsoncxx::document::value queryDoc = make_document(kvp("_id", (*value).get_value()));
			col.delete_one(queryDoc.view());
		}
		else
		{
			throw repo::lib::RepoException("Cannot drop a document without an _id field");
		}
	}
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "dropDocument", database, collection));
	}
}

std::vector<repo::core::model::RepoBSON> MongoDatabaseHandler::findAllByCriteria(
	const std::string& database,
	const std::string& collection,
	const database::query::RepoQuery& filter)
{
	try
	{
		std::vector<repo::core::model::RepoBSON> data;
		repo::core::model::RepoBSON criteria = filter;
		if (!criteria.isEmpty() && !database.empty() && !collection.empty())
		{
			auto client = clientPool->acquire();
			auto db = client->database(database);
			auto col = db.collection(collection);

			fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

			// Find all documents
			auto cursor = col.find(criteria.view());
			for (auto& doc : cursor) {
				data.push_back(createRepoBSON(blobHandler, database, collection, doc));
			}
		}
		return data;
	}
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "findAllByCriteria", database, collection));
	}
}

repo::core::model::RepoBSON MongoDatabaseHandler::findOneByCriteria(
	const std::string& database,
	const std::string& collection,
	const database::query::RepoQuery& filter,
	const std::string& sortField)
{
	try
	{
		repo::core::model::RepoBSON criteria = filter;
		if (!criteria.isEmpty() && !database.empty() && !collection.empty())
		{
			auto client = clientPool->acquire();
			auto db = client->database(database);
			auto col = db.collection(collection);

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
		return {};
	}
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "findOneByCriteria", database, collection));
	}
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
	try
	{
		std::vector<repo::core::model::RepoBSON> bsons;

		if (database.empty() || collection.empty())
		{
			return bsons;
		}

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


		fileservice::BlobFilesHandler blobHandler(fileManager, database, collection);

		auto cursor = col.find({}, options);
		for (auto doc : cursor) {
			bsons.push_back(createRepoBSON(blobHandler, database, collection, doc));
		}

		return bsons;
	}
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "getAllFromCollectionTailable", database, collection));
	}
}

std::list<std::string> MongoDatabaseHandler::getCollections(
	const std::string &database)
{
	try
	{
		if (database.empty())
		{
			return {};
		}
		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto collectionsVector = db.list_collection_names();
		return std::list<std::string>(collectionsVector.begin(), collectionsVector.end());
	}
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "getCollections", database));
	}
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
	return getHandler("mongodb://" + host + ":" + std::to_string(port), username, password, options);
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
	try
	{
		if (obj.hasOversizeFiles())
		{
			throw repo::lib::RepoException("insertDocument cannot be used with BSONs holding binary files. Use insertManyDocuments instead.");
		}

		auto client = clientPool->acquire();
		auto db = client->database(database);
		auto col = db.collection(collection);
		col.insert_one(obj.view());
	}
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "insertDocument", database, collection));
	}
}

void MongoDatabaseHandler::insertManyDocuments(
	const std::string &database,
	const std::string &collection,
	const std::vector<repo::core::model::RepoBSON> &objs,
	const Metadata& binaryStorageMetadata)
{
	try
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
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "insertManyDocuments", database, collection));
	}
}

void MongoDatabaseHandler::upsertDocument(
	const std::string &database,
	const std::string &collection,
	const repo::core::model::RepoBSON &obj,
	const bool        &overwrite)
{
	try
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
			query.append(kvp(ID, obj.find(ID)->get_value()));

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
				if (e.key() == ID)
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
	catch (...)
	{
		std::throw_with_nested(MongoDatabaseHandlerException(*this, "upsertDocument", database, collection));
	}
}