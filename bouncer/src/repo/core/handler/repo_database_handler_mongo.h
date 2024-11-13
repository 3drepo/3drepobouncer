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

				/**
				* @param dbAddress ConnectionString that holds the address to the mongo database
				* @param maxConnections max. number of connections to the database
				* @param username user name for authentication (optional)
				* @param password password of the user (optional)
				*/
				MongoDatabaseHandler(
					const std::string& dbAddress,
					const uint32_t& maxConnections,
					const std::string& dbName,
					const std::string& username = std::string(),
					const std::string& password = std::string()
				);

				~MongoDatabaseHandler();

				/**
				 * Returns the instance of MongoDatabaseHandler
				 * @param errMsg error message if this fails
				 * @param host hostname of the database
				 * @param port port number
				 * @param number of maximum simultaneous connections
				 * @param username username for authentication
				 * @param password for authentication
				 * @return Returns the single instance
				 */
				static std::shared_ptr<MongoDatabaseHandler> getHandler(
					const std::string &connectionString,
					const uint32_t    &maxConnections = 1,
					const std::string &dbName = std::string(),
					const std::string &username = std::string(),
					const std::string &password = std::string());

				/**
				 * Returns the instance of MongoDatabaseHandler
				 * @param errMsg error message if this fails
				 * @param host hostname of the database
				 * @param port port number
				 * @param number of maximum simultaneous connections
				 * @param username username for authentication
				 * @param password for authentication
				 * @return Returns the single instance
				 */
				static std::shared_ptr<MongoDatabaseHandler> getHandler(
					const std::string &host,
					const int         &port,
					const uint32_t    &maxConnections = 1,
					const std::string &dbName = std::string(),
					const std::string &username = std::string(),
					const std::string &password = std::string());

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

				/**
				* Return the name of admin database
				* @return name of admin database
				*/
				static std::string getAdminDatabaseName()
				{
					return ADMIN_DATABASE;
				}

				/*
				*	------------- Database operations (insert/delete/update) --------------
				*/

				/**
				* Create a collection with the name specified
				* @param database name of the database
				* @param name name of the collection
				*/
				virtual void createCollection(const std::string &database, const std::string &name);

				/**
				* Create an index within the given collection
				* @param database name of the database
				* @param name name of the collection
				* @param index BSONObj specifying the index
				*/
				virtual void createIndex(const std::string &database, const std::string &collection, const repo::core::model::RepoBSON& obj);

				/**
				* Remove a collection from the database
				* @param database the database the collection resides in
				* @param collection name of the collection to drop
				* @param errMsg name of the collection to drop
				*/
				bool dropCollection(
					const std::string &database,
					const std::string &collection,
					std::string &errMsg);

				/**
				* Remove a document from the mongo database
				* @param bson document to remove
				* @param database the database the collection resides in
				* @param collection name of the collection the document is in
				* @param errMsg name of the database to drop
				*/
				bool dropDocument(
					const repo::core::model::RepoBSON bson,
					const std::string &database,
					const std::string &collection,
					std::string &errMsg);

				/**
				 * Insert a single document in database.collection
				 * @param database name
				 * @param collection name
				 * @param document to insert
				 * @param errMsg error message should it fail
				 * @return returns true upon success
				 */
				bool insertDocument(
					const std::string &database,
					const std::string &collection,
					const repo::core::model::RepoBSON &obj,
					std::string &errMsg);

				/**
				* Insert multiple document in database.collection
				* @param database name
				* @param collection name
				* @param documents to insert
				* @param errMsg error message should it fail
				* @return returns true upon success
				*/
				virtual bool insertManyDocuments(
					const std::string &database,
					const std::string &collection,
					const std::vector<repo::core::model::RepoBSON> &obj,
					std::string &errMsg,
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
				bool upsertDocument(
					const std::string &database,
					const std::string &collection,
					const repo::core::model::RepoBSON &obj,
					const bool        &overwrite,
					std::string &errMsg);

				/*
				*	------------- Query operations --------------
				*/

				/**
				* Given a list of unique IDs, find all the documents associated to them
				* @param name of database
				* @param name of collection
				* @param array of uuids in a BSON object
				* @return a vector of RepoBSON objects associated with the UUIDs given
				*/
				std::vector<repo::core::model::RepoBSON> findAllByUniqueIDs(
					const std::string& database,
					const std::string& collection,
					const std::vector<repo::lib::RepoUUID> uuids,
					const bool ignoreExtFiles = false);

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
					const repo::core::model::RepoBSON& criteria);

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
					const repo::core::model::RepoBSON& criteria,
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
				*Retrieves the document matching given Unique ID (SID), sorting is descending
				* @param database name of database
				* @param collection name of collectoin
				* @param uuid share id
				* @return returns the matching bson object
				*/
				repo::core::model::RepoBSON findOneByUniqueID(
					const std::string& database,
					const std::string& collection,
					const repo::lib::RepoUUID& uuid);

				void setFileManager(std::shared_ptr<repo::core::handler::fileservice::FileManager> manager);

			private:

				// We can work with either clients or pool as the top level, connection
				// specific, container for getting connections. pool pool is threadsafe, 
				// but acquring a client is implied to not be cheap, so for now cache a 
				//client on starting up.
				std::unique_ptr<mongocxx::pool> clientPool;

				std::weak_ptr<repo::core::handler::fileservice::FileManager> fileManager;

				/*
				* Returns the FileManager previously set with setFileManager. This method
				* does not check if the pointer is valid beforehand - it is assumed the
				* lifetime of this handler and its corresponding manager are maintained
				* correctly outside.
				*/
				std::shared_ptr < repo::core::handler::fileservice::FileManager> getFileManager();

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

				/*
				*	=========================================================================================
				*/
			};
		} /* namespace handler */
	}
}
