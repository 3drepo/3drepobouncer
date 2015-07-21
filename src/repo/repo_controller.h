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

#include "repo_bouncer_global.h"

#include "core/handler/repo_database_handler_mongo.h"
#include "core/model/bson/repo_bson.h"
#include "lib/repo_stack.h"
#include "manipulator/repo_manipulator.h"


namespace repo{

	class REPO_API_EXPORT RepoToken
	{
		friend class RepoController;
	public:

		/**
		* Construct a Repo token
		* @param credentials user credentials in a bson format
		* @param handler database handler
		*/
		RepoToken(
			const repo::core::model::bson::RepoBSON* credentials,
			const repo::core::handler::AbstractDatabaseHandler* handler) :
			credentials(credentials),
			handler(handler){};

		~RepoToken(){}


	private:
		const repo::core::model::bson::RepoBSON* credentials;
		const repo::core::handler::AbstractDatabaseHandler* handler;
	};


	class REPO_API_EXPORT RepoController
	{
	public:
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

	private:
		lib::RepoStack<manipulator::RepoManipulator*> workerPool;
		const uint32_t numDBConnections;
	};



}
