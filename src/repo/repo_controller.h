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
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>

#include "repo_bouncer_global.h"

#include "core/handler/repo_database_handler_mongo.h"
#include "core/model/bson/repo_bson.h"
#include "lib/repo_stack.h"
#include "lib/repo_listener_abstract.h"
#include "manipulator/repo_manipulator.h"


namespace repo{

	class REPO_API_EXPORT RepoToken
	{
		friend class RepoController;
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
		* Subscribe a RepoAbstractLister to logging messages
		* @param listener object to subscribe
		*/
		void subscribeToLogger(
			repo::lib::RepoAbstractListener *listener);

		/**
		* Subscribe a stream to the logger
		* this bypasses the RepoBroadcaster and subscribes directly to boost::log
		* @param stream pointer to the output stream to subscribe to
		*/
		void subscribeToLogger(const boost::shared_ptr< std::ostream > stream);

		/**
		* Log to a specific file
		* @param filePath path to file
		*/
		void logToFile(const std::string &filePath);



	private:

		/**
		* Subscribe the Broadcaster to logging core
		*/
		void subscribeBroadcasterToLog();
		lib::RepoStack<manipulator::RepoManipulator*> workerPool;
		const uint32_t numDBConnections;
	};



}
