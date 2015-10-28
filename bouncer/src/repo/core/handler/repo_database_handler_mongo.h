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

#include <mongo/client/dbclient.h>

#include "repo_database_handler_abstract.h"
#include "connectionpool/repo_connection_pool_mongo.h"
#include "../model/repo_node_utils.h"
#include "../model/bson/repo_bson.h"
#include "../model/bson/repo_bson_builder.h"
#include "../model/bson/repo_bson_user.h"
#include "../../lib/repo_stack.h"

namespace repo{
	namespace core{
		namespace handler {

			class MongoDatabaseHandler : public AbstractDatabaseHandler{
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

				/*
				 *	=============================================================================================
				 */

				/*
				 *	=================================== Public Functions ========================================
				 */

				/**
				 * A Deconstructor
				 */
				~MongoDatabaseHandler();

				/**
				* Disconnects the handler and resets the instance
				* Must call this before trying to reconnect to another database!
				*/
				static void disconnectHandler()
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

				/**
				 * Returns the instance of MongoDatabaseHandler
				 * @param errMsg error message if this fails
				 * @param host hostname of the database
				 * @param port port number
				 * @param number of maximum simultaneous connections
				 * @param username username for authentication
				 * @param password for authentication
				 * @param pwDigested true if password is digested
				 * @return Returns the single instance
				 */
				static MongoDatabaseHandler* getHandler(
					std::string &errMsg,
					const std::string &host,
					const int         &port,
					const uint32_t    &maxConnections,
					const std::string &dbName,
					const std::string &username = std::string(),
					const std::string &password = std::string(),
					const bool        &pwDigested = false);


				/**
				* Returns the instance of MongoDatabaseHandler
				* @param host string containing "databaseAddress:port"
				* @return Returns null if there is no instance available
				*/
				static MongoDatabaseHandler* getHandler(
					const std::string &host)
				{
					return handler;
				}

				/**
				* Generates a BSON object containing user credentials
				* @param dbName name of the database to authenticate against
				* @param username user name for authentication
				* @param password password of the user
				* @param pwDigested true if pw is digested
				* @return returns the constructed BSON object, or 0 if username is empty
				*/
				repo::core::model::RepoBSON* createBSONCredentials(
					const std::string &dbName,
					const std::string &username,
					const std::string &password,
					const bool        &pwDigested = false)
				{
					mongo::BSONObj *mongoBSON = createAuthBSON(dbName, username, password, pwDigested);
					return mongoBSON? new repo::core::model::RepoBSON(*mongoBSON) : 0;
				}


				/*
				*	------------- Database info lookup --------------
				*/

				/**
				* Count the number of documents within the collection
				* @param database name of database
				* @param collection name of collection
				* @param errMsg errMsg if failed
				* @return number of documents within the specified collection
				*/
				uint64_t countItemsInCollection(
					const std::string &database,
					const std::string &collection,
					std::string &errMsg);

				/**
				* Retrieve documents from a specified collection
				* due to limitations of the transfer protocol this might need
				* to be called multiple times, utilising the skip index to skip
				* the first n items.
				* @param database name of database
				* @param collection name of collection
				* @param fields fields to get back from the database
				* @param sortField field to sort upon
				* @param sortOrder 1 ascending, -1 descending
				* @param skip specify how many documents to skip
				* @return list of RepoBSONs representing the documents
				*/
				std::vector<repo::core::model::RepoBSON>
					getAllFromCollectionTailable(
					const std::string                             &database,
					const std::string                             &collection,
					const uint64_t                                &skip = 0,
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
				* Get the collection statistics of the given collection
				* @param database Name of database
				* @param collection Name of collection
				* @param errMsg error message when error occurs
				* @return returns a bson object with statistical info.
				*/
				repo::core::model::CollectionStats getCollectionStats(
					const std::string    &database,
					const std::string    &collection,
					std::string          &errMsg);


				/**
				 * Get a list of all available databases, alphabetically sorted by default.
				 * @param sort the database
				 * @return returns a list of database names
				 */
				std::list<std::string> getDatabases(const bool &sorted = true);

				/** get the associated projects for the list of database.
				 * @param list of database
				 * @return returns a map of database -> list of projects
				 */
				std::map<std::string, std::list<std::string> > getDatabasesWithProjects(
					const std::list<std::string> &databases,
					const std::string &projectExt = "scene");

				/**
				 * Get a list of projects associated with a given database (aka company account).
				 * It will filter out any collections without that isn't *.projectExt
				 * @param list of database
				 * @param extension that determines it is a project (scene)
				 * @return list of projects for the database
				 */
				std::list<std::string> getProjects(const std::string &database, const std::string &projectExt);

				/**
				* Return a list of Admin database roles
				* @return a vector of Admin database roles
				*/
				std::list<std::string> getAdminDatabaseRoles()
				{
					return ADMIN_ONLY_DATABASE_ROLES;
				}

				/**
				* Return the name of admin database
				* @return name of admin database
				*/
				static std::string getAdminDatabaseName()
				{
					return ADMIN_DATABASE;
				}

				/**
				* Return a list of standard database roles
				* @return a vector of standard database roles
				*/
				std::list<std::string> getStandardDatabaseRoles()
				{
					return ANY_DATABASE_ROLES;
				}
				/*
				*	------------- Database operations (insert/delete/update) --------------
				*/


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
				* Remove a database from the mongo database
				* @param database name of the database to drop
				* @param errMsg name of the database to drop
				*/
				bool dropDatabase(
					const std::string &database,
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
				* Remove a user from the database
				* @param user user bson to remove
				* @param errmsg error message
				* @return returns true upon success
				*/
				bool dropUser(
					const repo::core::model::RepoUser &user,
					std::string                             &errmsg)
				{
					return performUserCmd(OPERATION::DROP, user, errmsg);
				}


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
				* Insert big raw file in binary format (using GridFS)
				* @param database name
				* @param collection name
				* @param fileName to insert (has to be unique)
				* @param bin raw binary of the file
				* @param errMsg error message if it fails
				* @return returns true upon success
				*/
				bool insertRawFile(
					const std::string          &database,
					const std::string          &collection,
					const std::string          &fileName,
					const std::vector<uint8_t> &bin,
					      std::string          &errMsg
					);

				/**
				* Insert a user into the database
				* @param user user bson to insert
				* @param errmsg error message
				* @return returns true upon success
				*/
				bool insertUser(
					const repo::core::model::RepoUser &user,
					std::string                             &errmsg)
				{
					return performUserCmd(OPERATION::INSERT, user, errmsg);
				}

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

				/**
				* Update a user in the database
				* @param user user bson to update
				* @param errmsg error message
				* @return returns true upon success
				*/
				bool updateUser(
					const repo::core::model::RepoUser &user,
					std::string                             &errmsg)
				{
					return performUserCmd(OPERATION::UPDATE, user, errmsg);
				}


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
					const repo::core::model::RepoBSON& uuids);


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
					const repoUUID& uuid,
					const std::string& sortField);

				/**
				*Retrieves the document matching given Unique ID (SID), sorting is descending
				* @param name of database
				* @param name of collectoin
				* @param share id
				* @return returns the matching bson object
				*/
				repo::core::model::RepoBSON findOneByUniqueID(
					const std::string& database,
					const std::string& collection,
					const repoUUID& uuid);


				/**
				* Get raw binary file from database
				* @param database name of database
				* @param collection name of collection
				* @param fname name of the file
				* @return return the raw binary as a vector of uint8_t (if found)
				*/
				std::vector<uint8_t> getRawFile(
					const std::string& database,
					const std::string& collection,
					const std::string& fname
					) ;

				/*
				 *	=============================================================================================
				 */


			protected:
				/*
				*	=================================== Protected Fields ========================================
				*/
				static MongoDatabaseHandler *handler; /* !the single instance of this class*/
				/*
				*	=============================================================================================
				*/

			private:
				/*
				 *	=================================== Private Fields ========================================
				 */

				connectionPool::MongoConnectionPool *workerPool;

				/*!
				 * Map holding database name as key and <username, password digest> as a
				 * value. User can be authenticated on any number of databases on a single
				 * connection.
				 */
				std::map<std::string, std::pair<std::string, std::string> > databasesAuthentication;

				mongo::ConnectionString dbAddress; /* !address of the database (host:port)*/

				/*
				 *	=============================================================================================
				 */

				/*
				 *	=================================== Private Functions ========================================
				 */

				/**
				 * Constructor is private because this class follows the singleton pattern
				 * @param dbAddress ConnectionString that holds the address to the mongo database
				 * @param maxConnections max. number of connections to the database
				 * @param username user name for authentication (optional)
				 * @param password password of the user (optional)
				 * @param pwDigested true if pw is digested (optional)
				 */
				MongoDatabaseHandler(
					const mongo::ConnectionString &dbAddress,
					const uint32_t                &maxConnections,
					const std::string             &dbName,
					const std::string             &username = std::string(),
					const std::string             &password = std::string(),
					const bool                    &pwDigested = false);

				/**
				* Create a Repo BSON and populate all relevant data
				* this includes getting data from GridFS
				* NOTE: the handler will only get the data from GridFS if it is
				* listed in REPO_LABEL_OVERSIZED_FILES
				* @param worker the worker to operate with
				* @param database database to store in
				* @param collection collection to store in
				* @param obj the mongo bson this repoBSON is basing from
				* @return returns a repo BSON that is fully populated
				*/
				repo::core::model::RepoBSON createRepoBSON(
					mongo::DBClientBase *worker,
					const std::string &database,
					const std::string &collection,
					const mongo::BSONObj &obj);

				/**
				* Generates a mongo BSON object for authentication
				* @param database database to authenticate against
				* @param username user name for authentication
				* @param password password of the user
				* @param pwDigested true if pw is digested
				* @return returns the constructed BSON object, or 0 if username is empty
				*/
				mongo::BSONObj* createAuthBSON(
					const std::string &database,
					const std::string &username,
					const std::string &password,
					const bool        &pwDigested);
				/**
				* Turns a list of fields that needs to be returned by the query into a bson
				* object.
				* @param list of field names to return
				* @param ID field is excluded by default unless explicitly stated in the list.
				*/
				mongo::BSONObj fieldsToReturn(
					const std::list<std::string>& fields,
					bool excludeIdField = false);

				/**
				 * Extract collection name from namespace (db.collection)
				 * @param namespace as string
				 * @return returns a string with just the collection name
				 */
				std::string getCollectionFromNamespace(const std::string &ns);


				/**
				* Get large file off GridFS
				* @param worker the worker to operate with
				* @param database database that it is stored in
				* @param collection collection that it is stored in
				* @param fileName file name in GridFS
				* @return returns true upon success
				*/
				std::vector<uint8_t> getBigFile(
					mongo::DBClientBase *worker,
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
				* Perform command on the user
				* @param op (insert, drop or update)
				* @param user user to modify
				* @param errMsg error message if failed
				* @return returns true upon success
				*/
				bool performUserCmd(
					const OPERATION                         &op,
					const repo::core::model::RepoUser &user,
					std::string                       &errMsg);


				/**
				* check if the bson object contains any big binary files
				* if yes, store them in gridFS
				* @param worker the worker to operate with
				* @param database database to store in
				* @param collection collection to store in
				* @param obj the bson object to work with
				* @param errMsg error message when failed
				* @return returns true upon success
				*/
				bool storeBigFiles(
					mongo::DBClientBase *worker,
					const std::string &database,
					const std::string &collection,
					const repo::core::model::RepoBSON &obj,
					std::string &errMsg);

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

