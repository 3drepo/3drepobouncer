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

#pragma once

#include <string>
#include <iostream>
#include <sstream>

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Windows.h>

#define strcasecmp _stricmp
#endif

#include "repo_database_handler_abstract.h"
#include "repo/lib/repo_stack.h"

#include <mongocxx/client-fwd.hpp>
#include <mongocxx/pool-fwd.hpp>
#include <bsoncxx/document/view-fwd.hpp>
#include <mongocxx/pipeline.hpp>
#include <mongocxx/cursor.hpp>

namespace repo {
	namespace core {
		namespace model {
			class RepoBSON; // Forward declaration for document type
		}
		namespace handler {
			namespace fileservice{
				class Metadata; // Forward declaration for alias
				class FileManager;
			}

			class MongoDatabaseHandler : public AbstractDatabaseHandler {
				enum class OPERATION { DROP, INSERT, UPDATE };

			public:
				/*
				*	=================================== Public Fields ========================================
				*/
				static const std::string ID; //! "_id"
				static const std::string UUID;//! "uuid"
				static const std::string ADMIN_DATABASE;//! "admin"
				static const std::string SYSTEM_ROLES_COLLECTION;//! "system.roles"
				static const std::string AUTH_MECH;//! Authentication mechanism. currently MONGO-CR since mongo v2.6
				//! Built in any database roles. See http://docs.mongodb.org/manual/reference/built-in-roles/
				static const std::list<std::string> ANY_DATABASE_ROLES;
				//! Built in admin database roles. See http://docs.mongodb.org/manual/reference/built-in-roles/
				static const std::list<std::string> ADMIN_ONLY_DATABASE_ROLES;

				class MongoCursor;

				struct ConnectionOptions
				{
					uint32_t maxConnections = 1;
					uint32_t timeout = 10000; // Common timeout for socket, connection and server selection, in milliseconds

					ConnectionOptions(){} // Explicit default constructor required for gcc
				};

				/**
				* @param dbAddress ConnectionString that holds the address to the mongo database
				* @param maxConnections max. number of connections to the database
				* @param username user name for authentication (optional)
				* @param password password of the user (optional)
				*/
				MongoDatabaseHandler(
					const std::string& connectionString,
					const std::string& username,
					const std::string& password,
					const ConnectionOptions& options = ConnectionOptions()
				);

				~MongoDatabaseHandler();

				/**
				 * Returns a new instance of MongoDatabaseHandler
				 * @param host hostname of the database
				 * @param port port number
				 * @param number of maximum simultaneous connections
				 * @param username username for authentication
				 * @param password for authentication
				 * @return Returns the single instance
				 */
				static std::shared_ptr<MongoDatabaseHandler> getHandler(
					const std::string &connectionString,
					const std::string& username,
					const std::string& password,
					const ConnectionOptions& options = ConnectionOptions()
				);

				/**
				 * Returns a new instance of MongoDatabaseHandler
				 * @param host hostname of the database
				 * @param port port number
				 * @param number of maximum simultaneous connections
				 * @param username username for authentication
				 * @param password for authentication
				 */
				static std::shared_ptr<MongoDatabaseHandler> getHandler(
					const std::string &host,
					const int         &port,
					const std::string &username,
					const std::string &password,
					const ConnectionOptions& options = ConnectionOptions()
				);

				/*
				*	------------- Database info lookup --------------
				*/

				/**
				* Retrieve documents from a specified collection
				* due to limitations of the transfer protocol this might need
				* to be called multiple times, utilising the skip index to skip
				* the first n items.
				* @param database name of database
				* @param collection name of collection
				* @param skip number of maximum items to skip (default is 0)
				* @param limit number of maximum items to return (default is 0)
				* @param fields fields to get back from the database
				* @param sortField field to sort upon
				* @param sortOrder 1 ascending, -1 descending
				*/
				std::vector<repo::core::model::RepoBSON>
					getAllFromCollectionTailable(
						const std::string                             &database,
						const std::string                             &collection,
						const uint64_t                                &skip = 0,
						const uint32_t								  &limit = 0,
						const std::list<std::string>				  &fields = std::list<std::string>(),
						const std::string							  &sortField = std::string(),
						const int									  &sortOrder = -1);

				/**
				* Get a list of all available collections.
				* Use mongo.nsGetCollection() to remove database from the returned string.
				* @param name of the database
				* @return a list of collection names
				*/
				std::list<std::string> getCollections(const std::string &database);

				/*
				*	------------- Database operations (insert/delete/update) --------------
				*/

				/**
				* Create an index within the given collection
				* @param database name of the database
				* @param name name of the collection
				* @param index BSONObj specifying the index
				*/
				virtual void createIndex(const std::string &database, const std::string &collection, const database::index::RepoIndex& index);

				/**
				* Create an index within the given collection
				* @param database name of the database
				* @param name name of the collection
				* @param index BSONObj specifying the index
				* @param bool whether this is a sparse index
				*/
				virtual void createIndex(const std::string& database, const std::string& collection, const database::index::RepoIndex& index, bool sparse);

				/**
				* Remove a collection from the database
				* @param database the database the collection resides in
				* @param collection name of the collection to drop
				* @param errMsg name of the collection to drop
				*/
				void dropCollection(
					const std::string &database,
					const std::string &collection);

				/**
				* Remove a document from the mongo database
				* @param bson document to remove
				* @param database the database the collection resides in
				* @param collection name of the collection the document is in
				* @param errMsg name of the database to drop
				*/
				void dropDocument(
					const repo::core::model::RepoBSON bson,
					const std::string &database,
					const std::string &collection);

				/**
				 * Insert a single document in database.collection
				 * @param database name
				 * @param collection name
				 * @param document to insert
				 * @param errMsg error message should it fail
				 * @return returns true upon success
				 */
				void insertDocument(
					const std::string &database,
					const std::string &collection,
					const repo::core::model::RepoBSON &obj);

				/**
				* Insert multiple document in database.collection
				* @param database name
				* @param collection name
				* @param documents to insert
				* @param errMsg error message should it fail
				* @return returns true upon success
				*/
				virtual void insertManyDocuments(
					const std::string &database,
					const std::string &collection,
					const std::vector<repo::core::model::RepoBSON> &obj,
					const Metadata& metadata = {});

				/**
				* Update/insert a single document in database.collection
				* If the document exists, update it, if it doesn't, insert it
				* @param database name
				* @param collection name
				* @param document to insert
				* @param if it is an update, overwrites the document instead of updating the fields it has
				* @param errMsg error message should it fail
				* @return returns true upon success
				*/
				void upsertDocument(
					const std::string &database,
					const std::string &collection,
					const repo::core::model::RepoBSON &obj,
					const bool        &overwrite);

				/*
				*	------------- Query operations --------------
				*/

				/**
				* Given a search criteria,  find all the documents that passes this query
				* @param database name of database
				* @param collection name of collection
				* @param criteria search criteria in a bson object
				* @return a vector of RepoBSON objects satisfy the given criteria
				*/
				std::vector<repo::core::model::RepoBSON> findAllByCriteria(
					const std::string& database,
					const std::string& collection,
					const database::query::RepoQuery& criteria);

				/**
				* Given a search criteria,  find all the documents that passes this query
				* @param database name of database
				* @param collection name of collection
				* @param criteria search criteria in a bson object
				* @param projection to define the fiels in the returned document
				* @return a vector of RepoBSON objects satisfy the given criteria
				*/
				std::vector<repo::core::model::RepoBSON> findAllByCriteria(
					const std::string& database,
					const std::string& collection,
					const database::query::RepoQuery& filter,
					const repo::core::model::RepoBSON projection);

				/**
				* Given a search criteria,  find one documents that passes this query
				* @param database name of database
				* @param collection name of collection
				* @param criteria search criteria in a bson object
				* @param sortField field to sort
				* @return a RepoBSON objects satisfy the given criteria
				*/
				repo::core::model::RepoBSON findOneByCriteria(
					const std::string& database,
					const std::string& collection,
					const database::query::RepoQuery& criteria,
					const std::string& sortField = ""
				);

				/**
				*Retrieves the first document matching given Shared ID (SID), sorting is descending
				* (newest first)
				* @param name of database
				* @param name of collectoin
				* @param share id
				* @param field to sort by
				* @param fields to retrieve
				* @return returns a bson object containing those fields
				*/
				repo::core::model::RepoBSON findOneBySharedID(
					const std::string& database,
					const std::string& collection,
					const repo::lib::RepoUUID& uuid,
					const std::string& sortField);

				/**
				*Retrieves the document matching given Unique ID
				* @param database name of database
				* @param collection name of collection
				* @param uuid share id
				* @return returns the matching bson object
				*/
				repo::core::model::RepoBSON findOneByUniqueID(
					const std::string& database,
					const std::string& collection,
					const repo::lib::RepoUUID& uuid);

				/**
				*Retrieves the document matching given Unique ID, where the type of the Id
				* is a string.
				* @param database name of database
				* @param collection name of collection
				* @param uuid share id
				* @return returns the matching bson object
				*/
				repo::core::model::RepoBSON findOneByUniqueID(
					const std::string& database,
					const std::string& collection,
					const std::string& id);

				size_t count(
					const std::string& database,
					const std::string& collection,
					const database::query::RepoQuery& criteria);

				std::unique_ptr<repo::core::handler::database::Cursor> getAllByCriteria(
					const std::string& database,
					const std::string& collection,
					const database::query::RepoQuery& criteria
				);

				void updateManyWithAggregate(
					const std::string& database,
					const std::string& collection,
					const repo::core::model::RepoBSON filter,
					const mongocxx::pipeline& pipeline);

				void updateOne(
					const std::string& database,
					const std::string& collection,
					const repo::core::model::RepoBSON filter,
					const repo::core::model::RepoBSON update);

				void updateMany(
					const std::string& database,
					const std::string& collection,
					const repo::core::model::RepoBSON filter,
					const repo::core::model::RepoBSON update);

				mongocxx::v_noabi::cursor specialFind(
					const std::string& database,
					const std::string& collection,
					const repo::core::model::RepoBSON filter,
					const repo::core::model::RepoBSON projection);

				mongocxx::v_noabi::cursor specialFindPerf(
					const std::string& database,
					const std::string& collection,
					const repo::core::model::RepoBSON filter,
					const repo::core::model::RepoBSON projection,
					long long& duration);

				repo::core::model::RepoBSON rawFindOne(
					const std::string& database,
					const std::string& collection,
					const repo::core::model::RepoBSON filter,
					const repo::core::model::RepoBSON projection);

				mongocxx::v_noabi::cursor rawCursorFind(
					const std::string& database,
					const std::string& collection,
					const repo::core::model::RepoBSON filter,
					const repo::core::model::RepoBSON projection);

				std::unique_ptr<database::Cursor> runAggregatePipeline(
					const std::string& database,
					const std::string& collection,
					const mongocxx::pipeline& pipeline);

				std::unique_ptr<repo::core::handler::database::Cursor> getTransformsForLeaf(
					const std::string& database,
					const std::string& collection,
					const repo::lib::RepoUUID& id);

				std::unique_ptr<repo::core::handler::database::Cursor> getTransformsForAllLeaves(
					const std::string& database,
					const std::string& collection);

				std::unique_ptr<database::BulkWriteContext> getBulkWriteContext(
					const std::string& database,
					const std::string& collection);

				void setFileManager(std::shared_ptr<repo::core::handler::fileservice::FileManager> manager);

				std::shared_ptr<repo::core::handler::fileservice::FileManager> getFileManager();

				/*
				* If a RepoBSON comes from somewhere else, populate the binaries.
				*/
				void loadBinaries(const std::string& database,
					const std::string& collection, 
					repo::core::model::RepoBSON& bson);

				/**
				* This method performs an arbitrary operation against the database to check
				* if the connection is available and authenticated. The method is synchronous,
				* and will throw an exception describing the failure mode if there is one. If
				* it returns successfully, it means it was possible to communicate with the
				* database for at least that instant.
				* A successful check once does not mean the connection cannot be lost in the
				* future - this check is used mainly to verify the database configuration,
				* before bouncer gets a long way into an expensive job.
				*/
				void testConnection();

			private:

				// We can work with either clients or pool as the top level, connection
				// specific, container for getting connections. pool pool is threadsafe.
				std::unique_ptr<mongocxx::pool> clientPool;

				// The fileManager is used in the storage of certain member types, such
				// as large vectors of binary data. It must be set using setFileManager
				// before documents containing such members are uploaded.
				std::shared_ptr<repo::core::handler::fileservice::FileManager> fileManager;

				/**
				* Get large file off GridFS
				* @param database database that it is stored in
				* @param collection collection that it is stored in
				* @param fileName file name in GridFS
				* @return returns true upon success
				*/
				std::vector<uint8_t> getBigFile(
					const std::string &database,
					const std::string &collection,
					const std::string &fileName);

				/**
				 * Given a database and collection name, returns its namespace
				 * Basically, append database + "." + collection
				 * @param database name
				 * @param collection name
				 * @return namespace
				 */
				std::string getNamespace(
					const std::string &database,
					const std::string &collection);

				/**
				* Extract database name from namespace (db.collection)
				* @param namespace as string
				* @return returns a string with just the database name
				*/
				std::string getProjectFromCollection(const std::string &ns, const std::string &projectExt);

				/**
				* Compares two strings.
				* @param string 1
				* @param string 2
				* @return returns true if string 1 matches (or is greater than) string 2
				*/
				static bool caseInsensitiveStringCompare(const std::string& s1, const std::string& s2);

				class MongoDatabaseHandlerException;
				friend MongoDatabaseHandlerException;

				class MongoWriteContext;

				/*
				*	=========================================================================================
				*/
			};
		} /* namespace handler */
	}
}
