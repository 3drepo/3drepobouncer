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
* Controller that communicates to manipulator objects.
* Typically used by Desktop Client
* loosely following the model-view-controller design pattern
*/

#pragma once

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>


#include "repo_bouncer_global.h"

#include "core/handler/repo_database_handler_mongo.h"
#include "core/model/bson/repo_bson.h"
#include "lib/repo_stack.h"
#include "lib/repo_broadcaster.h"
#include "lib/repo_listener_abstract.h"
#include "manipulator/repo_manipulator.h"
#include "manipulator/graph/repo_scene.h"


namespace repo{

	class REPO_API_EXPORT RepoToken
	{

		friend class RepoController;

		typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend > text_sink;
	public:

		/**
		* Construct a Repo token
		* @param credentials user credentials in a bson format
		* @param databaseAd database address+port as a string
		* @param databaseName database it is authenticating against
		*/
		RepoToken(
			const repo::core::model::bson::RepoBSON*  credentials,
			const std::string                         databaseAd,
			const std::string                        &databaseName) :
			credentials(credentials),
			databaseName(databaseName){};

		~RepoToken(){
			if (credentials)
				delete credentials;
		}


	private:
		const repo::core::model::bson::RepoBSON* credentials;
		const std::string databaseAd;
		const std::string databaseName;
	};


	class REPO_API_EXPORT RepoController
	{
	public:

		/**
		* LOGGING LEVEL
		*/
		enum RepoLogLevel { LOG_ALL, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_NONE };


		/**
		* Constructor
		* @param numConcurrentOps maximum number of requests it can handle concurrently
		* @param numDBConn number of concurrent connections to the database
		*/
		RepoController(
			std::vector<lib::RepoAbstractListener*> listeners = std::vector<lib::RepoAbstractListener *>(),
			const uint32_t &numConcurrentOps=1,
			const uint32_t &numDbConn=1);

		/**
		* Destructor
		*/
		~RepoController();

		/*
		*	------------- Database Authentication --------------
		*/

		/**
		* Connect to a mongo database, authenticate by the admin database
		* @param errMsg error message if failed
		* @param address address of the database
		* @param port port number
		* @param dbName name of the database within mongo to connect to
		* @param username user login name
		* @param password user password
		* @param pwDigested is given password digested (default: false)
		* @return * @return returns a void pointer to a token
		*/
		RepoToken* authenticateMongo(
			std::string       &errMsg,
			const std::string &address,
			const uint32_t    &port,
			const std::string &dbName,
			const std::string &username,
			const std::string &password,
			const bool        &pwDigested = false
			);

		/**
		* Connect to a mongo database, authenticate by the admin database
		* @param errMsg error message if failed
		* @param address address of the database
		* @param port port number
		* @param username user login name
		* @param password user password
		* @param pwDigested is given password digested (default: false)
		* @return returns a void pointer to a token
		*/
		RepoToken* authenticateToAdminDatabaseMongo(
			std::string       &errMsg,
			const std::string &address,
			const int         &port,
			const std::string &username,
			const std::string &password,
			const bool        &pwDigested = false
			)
		{
			return authenticateMongo(errMsg, address, port,
				core::handler::AbstractDatabaseHandler::ADMIN_DATABASE,
				username, password, pwDigested);
		}

		/*
		*	------------- Database info lookup --------------
		*/

		/**
		* Count the number of documents within the collection
		* @param token A RepoToken given at authentication
		* @param database name of database
		* @param collection name of collection
		* @return number of documents within the specified collection
		*/
		uint64_t countItemsInCollection(
			RepoToken            *token,
			const std::string    &database,
			const std::string    &collection);

		/**
		* Retrieve documents from a specified collection
		* due to limitations of the transfer protocol this might need
		* to be called multiple times, utilising the skip index to skip
		* the first n items.
		* @param token A RepoToken given at authentication
		* @param database name of database
		* @param collection name of collection
		* @param skip specify how many documents to skip
		* @return list of RepoBSONs representing the documents
		*/
		std::vector < repo::core::model::bson::RepoBSON >
			getAllFromCollectionContinuous(
			RepoToken            *token,
			const std::string    &database,
			const std::string    &collection,
			const uint64_t       &skip = 0);

		/**
		* Return a list of database available to the user
		* @param token A RepoToken given at authentication
		* @return returns a list of database names
		*/
		std::list<std::string> getDatabases(RepoToken *token);

		/**
		* Return a list of collections within the database
		* @param token A RepoToken given at authentication
		* @param databaseName database to get collections from
		* @return returns a list of collection names
		*/
		std::list<std::string> getCollections(
			RepoToken             *token,
			const std::string     &databaseName
			);

		/**
		* Return a CollectionStats BSON containing statistics about 
		* this collection
		* @param token A RepoToken given at authentication
		* @param database name of database
		* @param collection name of collection
		* @return returns a BSON object containing this information
		*/
		repo::core::model::bson::CollectionStats getCollectionStats(
			RepoToken            *token,
			const std::string    &database,
			const std::string    &collection);


		std::string getHostAndPort(RepoToken *token) { return token->databaseAd; }


		/*
		*	---------------- Database Retrieval -----------------------
		*/

		/**
		* Retrieve a RepoScene with a specific revision loaded.
		* @param token Authentication token
		* @param database the database the collection resides in
		* @param project name of the project
		* @param uuid if headRevision, uuid represents the branch id, 
		*              otherwise the unique id of the revision branch
		* @param headRevision true if retrieving head revision
		* @return returns a pointer to a repoScene.
		*/
		repo::manipulator::graph::RepoScene* fetchScene(
			const RepoToken      *token,
			const std::string    &database,
			const std::string    &project,
			const std::string    &uuid,
			const bool           &headRevision = false);

		/*
		*	------- Database Operations (insert/delete/update) ---------
		*/

		/**
		* Remove a collection from the database
		* @param token Authentication token
		* @param database the database the collection resides in
		* @param collection name of the collection to drop
		* @param errMsg error message if failed
		* @return returns true upon success
		*/
		bool removeCollection(
			RepoToken             *token,
			const std::string     &databaseName,
			const std::string     &collectionName,
			std::string			  &errMsg = std::string()
		);

		/**
		* Remove a database
		* @param token Authentication token
		* @param database the database the collection resides in
		* @param errMsg error message if failed
		* @return returns true upon success
		*/
		bool removeDatabase(
			RepoToken             *token,
			const std::string     &databaseName,
			std::string			  &errMsg = std::string()
		);



		/*
		*	------------- Logging --------------
		*/

		/**
		* Configure how verbose the log should be
		* The levels of verbosity are:
		* LOG_ALL - log all messages
		* LOG_DEBUG - log messages of level debug or above (use for debugging)
		* LOG_INFO - log messages of level info or above (use to filter debugging messages but want informative logging)
		* LOG_WARNING - log messages of level warning or above
		* LOG_ERROR - log messages of level error or above
		* LOG_NONE - no logging
		* @param level specify logging level
		*
		*/
		void setLoggingLevel(const RepoLogLevel &level);

		/**
		* Log to a specific file
		* @param filePath path to file
		*/
		void logToFile(const std::string &filePath);

		/*
		*	------------- Import --------------
		*/

		/**
		* Load a Repo Scene from a file
		* @param filePath path to file
		* @param config import settings(optional)
		* @return returns a pointer to Repo Scene upon success
		*/
		repo::manipulator::graph::RepoScene* loadSceneFromFile(
			const std::string &filePath,
			const repo::manipulator::modelconvertor::ModelImportConfig *config 
				= nullptr);

	private:

		/**
		* Subscribe the Broadcaster to logging core
		*/
		void subscribeBroadcasterToLog();

		/**
		* Subscribe a RepoAbstractLister to logging messages
		* @param listener object to subscribe
		*/
		void subscribeToLogger(
			std::vector<lib::RepoAbstractListener*> listeners);

		lib::RepoStack<manipulator::RepoManipulator*> workerPool;
		const uint32_t numDBConnections;

	};



}
