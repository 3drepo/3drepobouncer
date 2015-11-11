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


#pragma once

#include "../../../lib/repo_stack.h"
#include "../../../lib/repo_log.h"


#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Windows.h>

#define strcasecmp _stricmp
#endif

#include <mongo/client/dbclient.h>

namespace repo{
	namespace core{
		namespace handler {
			namespace connectionPool{
				class MongoConnectionPool : repo::lib::RepoStack <mongo::DBClientBase * >
				{
				public:
					/**
					* Instantiate the pool of workers with a limited number of connections
					* @param numConnections number of connections
					*/
					MongoConnectionPool(
						const int &numConnections,
						mongo::ConnectionString dbAddress,
						mongo::BSONObj* auth) :
						maxSize(numConnections),
						dbAddress(dbAddress),
						auth(auth)
					{
						repoDebug << "Instantiating Mongo connection pool with " << maxSize << " connections...";
						//push one connected worker to ensure valid connection
						//so the caller can handle the exceptions appropriately
						std::string errMsg;

						mongo::DBClientBase *worker = dbAddress.connect(errMsg);

						if (worker)
						{
							repoDebug << "Connected to database, trying authentication..";
							for (int i = 0; i < numConnections; i++)
							{
								mongo::DBClientBase *worker = dbAddress.connect(errMsg);
								if (auth)
								{
									if (!worker->auth(auth->getStringField("db"), auth->getStringField("user"), auth->getStringField("pwd"), errMsg, auth->getField("digestPassword").boolean()))
									{
										throw mongo::DBException(errMsg, mongo::ErrorCodes::AuthenticationFailed);
									}

								}
								else
								{
									repoWarning << "No credentials found. User is not authenticated against the database!";
								}
								push(worker);
							}
						}
						else
						{
							repoDebug << "Failed to connect: " << errMsg;
							throw mongo::DBException(errMsg, 1000);
						}




					}

					MongoConnectionPool():maxSize(0){}


					~MongoConnectionPool()
					{
						delete auth;
						//free workers within the pool
						std::vector<mongo::DBClientBase*> workers = empty();
						std::vector<mongo::DBClientBase*>::iterator it;
						for (it = workers.begin(); it != workers.end(); ++it)
						{
							mongo::DBClientBase* worker = *it;
							if (worker)
								delete worker;
						}
					}

					mongo::DBClientBase* getWorker()
					{
						return pop();
					}

					void returnWorker(mongo::DBClientBase *worker)
					{
						push(worker);
					}

					mongo::DBClientBase* pop()
					{
						mongo::DBClientBase* worker = RepoStack::pop();

						if (!worker)
						{
							//worker was never used, instantiate it with a connection
							std::string tmp;
							worker = connectWorker(tmp);
						}
						else
						{
							//check worker is still connected
							if (!worker->isStillConnected())
							{
								std::string tmp;
								worker = connectWorker(tmp);
							}
						}

						return worker;
					}

					void push(mongo::DBClientBase *worker)
					{
						if (worker)
							RepoStack::push(worker);
					}
				private:
					mongo::DBClientBase* connectWorker(std::string &errMsg)
					{
						mongo::DBClientBase *worker = dbAddress.connect(errMsg);
						if (auth && !worker->auth(auth->getStringField("db"), auth->getStringField("user"), auth->getStringField("pwd"), errMsg, auth->getField("digestPassword").boolean()))
						{
							throw mongo::DBException(errMsg, mongo::ErrorCodes::AuthenticationFailed);
						}
						return worker;
					}


					const uint32_t maxSize;
					const mongo::ConnectionString dbAddress;
					const mongo::BSONObj *auth;

				};

			}
		} /* namespace handler */
	}
}

