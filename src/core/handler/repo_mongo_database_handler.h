/**
 *  Copyright (C) 2014 3D Repo Ltd
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

#ifndef SRC_CORE_HANDLER_MONGODATABASEHANDLER_H_
#define SRC_CORE_HANDLER_MONGODATABASEHANDLER_H_

#include <string>
#include <iostream>
#include <sstream>
#include <boost/log/trivial.hpp>

#if defined(_WIN32) || defined(_WIN64)
	#include <WinSock2.h>
	#include <Windows.h>

	#define strcasecmp _stricmp
#endif

#include <mongo/client/dbclient.h>

#include "repo_abstract_database_handler.h"
#include "../model/repo_node_utils.h"
#include "../model/bson/repo_bson.h"
#include "../model/bson/repo_bson_builder.h"

namespace repo{
	namespace core{
		namespace handler {
			class MongoDatabaseHandler : public AbstractDatabaseHandler{
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
					 * Returns the instance of MongoDatabaseHandler
					 * @return Returns the single instance
					 */
					static MongoDatabaseHandler* getHandler(const std::string &host, const int &port);

					/*
					*	------------- Authentication --------------
					*/

				    /**
					 * Authenticates the user on the "admin" database. This gives access to all databases.
					 * @param username in the database
					 * @param password for this username
				     */
					bool authenticate(const std::string& username, const std::string& plainTextPassword);

					/** 
					 *  Authenticates the user on a given database for a specific access.
				     *  Authentication is separate for each database and the user can
					 *  authenticate on any number of databases. MongoDB password is the hex
					 *  encoding of MD5( <username> + ":mongo:" + <password_text> ).
					 * @param name of database
					 * @param username 
					 * @param password
					 * @param if the password is digested
					 */
					bool authenticate(
						const std::string& database,
						const std::string& username,
						const std::string& password,
						bool isPasswordDigested = false);

					/*
					*	------------- Database info lookup --------------
					*/

					/**
					* Get a list of all available collections.
					* Use mongo.nsGetCollection() to remove database from the returned string.
					* @param name of the database
					* @return a list of collection names
					*/
					std::list<std::string> getCollections(const std::string &database);


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
						const std::list<std::string> &databases, const std::string &projectExt);

					/**
					 * Get a list of projects associated with a given database (aka company account).
					 * It will filter out any collections without that isn't *.projectExt
					 * @param list of database
					 * @param extension that determines it is a project (scene)
					 * @return list of projects for the database
					 */ 
					std::list<std::string> getProjects(const std::string &database, const std::string &projectExt);

					/*
					*	------------- Database operations (insert/delete/update) --------------
					*/

					/**
					 * Insert a single document in database.collection
					 * @param database name
					 * @param collection name
					 * @param document to insert
					*/
					void insertDocument(
						const std::string &database,
						const std::string &collection,
						const repo::core::model::bson::RepoBSON &obj);

					/*
					*	------------- Query operations --------------
					*/

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
					repo::core::model::bson::RepoBSON findOneBySharedID(
						const std::string& database,
						const std::string& collection,
						const repo_uuid& uuid,
						const std::string& sortField);

					/*
					 *	=============================================================================================
					 */

				private:
					/*
					 *	=================================== Private Fields ========================================
					 */

					//FIXME: This needs to be a collection
					mongo::DBClientBase *worker; /* connection worker that holds connection to the database */
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
					 * Constructor is static because this class follows the singleton pattern
					 * @param IP address of the database
					 * @param connection port (typically 27017)
					 */
					MongoDatabaseHandler(const mongo::ConnectionString &dbAddress);
					

					/**
					* Turns a list of fields that needs to be returned by the query into a bson
					* object.
					* @param list of field names to return
					* @param ID field is excluded by default unless explicitly stated in the list.
					*/
					mongo::BSONObj MongoDatabaseHandler::fieldsToReturn(
						const std::list<std::string>& fields,
						bool excludeIdField=false);

					/**
					 * Extract collection name from namespace (db.collection)
					 * @param namespace as string
					 * @return returns a string with just the collection name
					 */
					std::string getCollectionFromNamespace(const std::string &ns);

					/**
					 * Given a database and collection name, returns its namespace
					 * Basically, append database + "." + collection
					 * @param database name
					 * @param collection name
					 * @return namespace
					*/
					std::string MongoDatabaseHandler::getNamespace(
						const std::string &database,
						const std::string &collection);

					/**
					* Extract database name from namespace (db.collection)
					* @param namespace as string
					* @return returns a string with just the database name
					*/
					std::string MongoDatabaseHandler::getProjectFromCollection(const std::string &ns, const std::string &projectExt);

					/*
					 * initialise the connection pool workers
					 * @param number of workers in the pool
					 * @param host connection (host:port)
					 * @returns true if a connection to the database is established
					 */
					bool intialiseWorkers();

					/**
					* Compares two strings.
					* @param string 1
					* @param string 2
					* @return returns true if string 1 matches (or is greater than) string 2
					*/
					static bool caseInsensitiveStringCompare(const std::string& s1, const std::string& s2);

					
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


//				public:
					//					MongoClientWrapper(const MongoClientWrapper&);
//
//					//! Move constructor
//					MongoClientWrapper (MongoClientWrapper &&);
//
//					//! Copy assignment. Requires separate reconnection and reauthentication.
//					MongoClientWrapper& operator= (const MongoClientWrapper&);
//
//					//! Move assignment
//					MongoClientWrapper& operator= (MongoClientWrapper&&);
//
//				    //--------------------------------------------------------------------------
//					//
//					// Static helpers
//					//
//				    //--------------------------------------------------------------------------
//
//					/*! Appends binary mongo::bdfUUID type value to a given bson builder
//						and returns the assigned uuid.
//					*/
//					static boost::uuids::uuid appendUUID(
//				        const std::string &fieldName,
//				        mongo::BSONObjBuilder &builder);
//
//					static boost::uuids::uuid appendUUID(
//				        const std::string &fieldName,
//				        const boost::uuids::uuid &val,
//				        mongo::BSONObjBuilder &builder);
//
//				    //! Compares two strings.
//				    static bool caseInsensitiveStringCompare(const std::string& s1, const std::string& s2);
//
//
//				    /*! Returns uuid representation of a given BSONElement if its binDataType is
//				     *  bdtUUID, empty uuid otherwise.
//				     */
//				    static boost::uuids::uuid retrieveUUID(const mongo::BSONElement &bse);
//
//				    static boost::uuids::uuid retrieveUUID(const mongo::BSONObj &obj);
//
//				    static std::string uuidToString(const boost::uuids::uuid &);
//
//				    //--------------------------------------------------------------------------
//					// TODO: remove
//					static int retrieveRevisionNumber(const mongo::BSONObj& obj);
//
//					//! Returns a namespace as database.collection
//				    static std::string getNamespace(
//				            const std::string &database,
//				            const std::string &collection);
//
//				    //! Returns project.history
//				    static std::string getHistoryCollectionName(const std::string& project)
//				    {   return project + "." + REPO_COLLECTION_HISTORY;}
//
//				    //! Returns project.scene
//				    static std::string getSceneCollectionName(const std::string& project)
//				    { return project + "." + REPO_COLLECTION_SCENE; }
//
//				    //--------------------------------------------------------------------------
//					//
//					// Connection and authentication
//					//
//				    //--------------------------------------------------------------------------
//
//				    /*! Connect to MongoDB at a given host and port. If port is unspecified,
//				     * connects to the default port.
//				     */
//					bool connect(const mongo::HostAndPort&);
//
//					/*!
//					 * Connect to MongoDB at a given host and port.
//					 * If port is unspecified, connects to the default port.
//					 */
//					bool connect(const std::string& host = "localhost", int port = -1);
//
//					//! Reconnect using the stored host and port.
//					bool reconnect();
//

//

//
//					bool authenticate(
//						const std::string& database,
//						const std::pair<std::string, std::string>& usernameAndDigestedPassword);
//
//					//! Attempts authentication on all databases previously authenticated successfully.
//					bool reauthenticate();
//
//				    /*! Attemps authentication using stored credentials on a specific database.
//				     * If database not found, attempts "admin" db authentication.
//				     */
//					bool reauthenticate(const std::string& /* database */);
//
//					//! Performs reconnection and reauthentication.
//					bool reconnectAndReauthenticate();
//
//					//! Reconnects and authenticates only on a specific database.
//					bool reconnectAndReauthenticate(const std::string& /* database */);
//
//				    /*! Returns the username under which the given database has been
//				     * authenticated. If not found, returns username authenticated on the
//				     * "admin" database, otherwise returns empty string.
//				     */
//					std::string getUsername(const std::string& /* database */) const;
//
//				    //--------------------------------------------------------------------------
//					//
//					// Basic DB info retrieval
//					//
//				    //--------------------------------------------------------------------------
//
//				    //! Returns a list of all available databases, alphabetically sorted by default.
//				    std::list<std::string> getDatabases(bool sorted = true);
//
//				    //! Returns a map of databases with associated projects.
//				    std::map<std::string, std::list<std::string> > getDatabasesWithProjects(
//				            const std::list<std::string> &databases);
//
//				    /*!
//				     * Returns a list of all available collections in a format "database.collection".
//				     * Use mongo.nsGetCollection() to remove database from the returned string.
//				     */
//				    std::list<std::string> getCollections(const std::string &database);
//
//				    //! Returns a list of projects associated with a given database (aka company account).
//				    std::list<std::string> getProjects(const std::string &database);
//
//				    //! Returns connection status on the admin database.
//				    mongo::BSONObj getConnectionStatus();
//
//				    //! Returns connection status on a given database.
//				    mongo::BSONObj getConnectionStatus(const std::string &database);
//
//				    //! Returns stats for a collection.
//				    repo::core::RepoCollStats getCollectionStats(const std::string &ns);
//
//				    //! Returns stats for a collection.
//				    repo::core::RepoCollStats getCollectionStats(
//				            const std::string &database,
//				            const std::string &collection);
//
//					//! Returns a collection name from namespace (db.collection)
//				    std::string nsGetCollection(const std::string &ns);
//
//					//! Returns a database name from namespace (db.collection)
//				    std::string nsGetDB(const std::string &ns);
//
//					//! Returns the number of items in a collection
//				    unsigned long long countItemsInCollection(
//				            const std::string &database,
//				            const std::string &collection);
//
//					//! Returns the number of items in a collection defined as namespace (db.collection)
//				    unsigned long long countItemsInCollection(const std::string &ns);
//
//				    //--------------------------------------------------------------------------
//					//
//					// Queries
//					//
//				    //--------------------------------------------------------------------------
//
//					std::auto_ptr<mongo::DBClientCursor> listAll(
//						const std::string& /* database */,
//						const std::string& /* collection */);
//
//					std::auto_ptr<mongo::DBClientCursor> listAllTailable(
//						const std::string& /* database */,
//						const std::string& /* collection */,
//						int skip = 0);
//
//					/*! Retrieves all objects but with a limited list of fields.
//						@param sortOrder 1 ascending, -1 descending
//					*/
//					std::auto_ptr<mongo::DBClientCursor> listAllTailable(
//						const std::string& database,
//						const std::string& collection,
//						const std::list<std::string>& fields,
//						const std::string& sortField = std::string(),
//						int sortOrder = -1,
//						int skip = 0);
//
//					//! Retrieves all objects whose ID is in the array.
//					std::auto_ptr<mongo::DBClientCursor> findAllByUniqueIDs(
//						const std::string& database,
//						const std::string& collection,
//						const mongo::BSONArray& array,
//						int skip);
//
//					// TODO: move logic to repo_core.
//					//! Retrieves fields matching given Unique ID (UID).
//					mongo::BSONObj findOneByUniqueID(
//						const std::string& database,
//						const std::string& collection,
//						const std::string& uuid,
//						const std::list<std::string>& fields);
//
//				    /*! Retrieves fields matching given Shared ID (SID), sorting is descending
//				     * (newest first).
//				     */
//					mongo::BSONObj findOneBySharedID(
//						const std::string& database,
//						const std::string& collection,
//						const std::string& uuid,
//						const std::string& sortField,
//						const std::list<std::string>& fields);
//
//				    //! Run db.eval command on the specified database.
//				    mongo::BSONElement eval(const std::string &database,
//				            const std::string &jscode);
//
//				    /*!
//				     * See http://docs.mongodb.org/manual/reference/method/db.runCommand/
//				     * http://docs.mongodb.org/manual/reference/command/
//				     * \brief runCommand
//				     * \param database
//				     * \param command
//				     */
//				    mongo::BSONObj runCommand(
//				            const std::string &database,
//				            const mongo::BSONObj &command);
//
//				    //--------------------------------------------------------------------------
//					// Deprecated
//					//
//					/*! Populates the ret vector with all BSON objs found in the collection
//						Returns true if at least one BSON obj loaded, false otherwise
//					*/
//					bool fetchEntireCollection(
//						const std::string& /* database */,
//						const std::string& /* collection */,
//						std::vector<mongo::BSONObj>& /* ret */);
//
//
//				    //--------------------------------------------------------------------------
//					// DEPRECATED
//				    //--------------------------------------------------------------------------
//					// Given dbName and collection name populates the ret vector with all BSON objs
//					// belonging to revisions listed in set
//					// Returns false if error
//				    bool fetchRevision(std::vector<mongo::BSONObj> &/* ret */,
//				                       std::string dbName,
//				                       std::string collection,
//				                       const std::set<int64_t> &revisionNumbersAncestralArray);
//				    void getRevision(std::string dbName, std::string collection, int revNumber,
//				                     int ancestor);
//
//				    //--------------------------------------------------------------------------
//					//
//					// Deletion
//					//
//				    //--------------------------------------------------------------------------
//
//				    /*! Object ID can be of any type not just OID, hence BSONElement is more
//				     * suitable
//				     */
//					bool deleteRecord(
//						const std::string& /* database */,
//						const std::string& /* collection */,
//						const mongo::BSONElement& /* recordID */ );
//
//					bool deleteAllRecords(
//						const std::string& /* database */,
//						const std::string& /* collection */);
//
//				    //! Drops given database.
//				    bool dropDatabase(const std::string& database);
//
//				    //! Drops database.collection
//				    bool dropCollection(const std::string& ns);
//
//				    //--------------------------------------------------------------------------
//					//
//					// Insertion
//					//
//				    //--------------------------------------------------------------------------
//
//					void insertRecord(
//						const std::string &database,
//						const std::string &collection,
//						const mongo::BSONObj &obj);
//
//					void insertRecords(
//						const std::string &database,
//						const std::string &collection,
//						const std::vector<mongo::BSONObj> &objs,
//						bool inReverse = false);
//
//				    void upsertRecord(const std::string &database,
//				                      const std::string &collection,
//				                      const mongo::BSONObj &obj)
//				    { updateRecord(database, collection, obj, true); }
//
//				    void updateRecord(
//				            const std::string &database,
//				            const std::string &collection,
//				            const mongo::BSONObj &obj,
//				            bool upsert = false);
//
//				    mongo::BSONObj insertFile(const std::string &database,
//				                    const std::string &project,
//				                    const std::string &filePath);
//
//				    //--------------------------------------------------------------------------
//					//
//					// Getters
//					//
//				    //--------------------------------------------------------------------------
//
//					//! Returns the host if set, "localhost" by default.
//				    std::string getHost() const { return hostAndPort.host(); }
//
//					//! Returns the port if set, 27017 by default.
//				    int getPort() const { return hostAndPort.port(); }
//
//					//! Returns the host and port as a string.
//				    std::string getHostAndPort() const { return hostAndPort.toString(true); }
//
//					//! Returns a "username@host:port" string.
//				    std::string getUsernameAtHostAndPort() const
//				    { return clientConnection.getServerAddress(); }
//
//				    //--------------------------------------------------------------------------
//					//
//					// Helpers
//					//
//				    //--------------------------------------------------------------------------
//
//					/*! Generates a BSONObj with given strings labelled as return value for query
//						Ie { string1 : 1, string2: 1 ... }
//
//						"_id" field is excluded by default unless explicitly stated in the list.
//					*/
//				    static mongo::BSONObj fieldsToReturn(
//				            const std::list<std::string>& list,
//				            bool excludeIdField = false);
//
//
//
//				    mongo::DBClientConnection clientConnection;

			};

		} /* namespace handler */
	}
}


#endif /* SRC_CORE_HANDLER_MONGODATABASEHANDLER_H_ */
