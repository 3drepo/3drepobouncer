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

#include "repo_connection_pool_mongo.h"

using namespace repo::core::handler::connectionPool;

MongoConnectionPool::MongoConnectionPool(
	const int &numConnections,
	mongo::ConnectionString dbAddress,
	mongo::BSONObj* auth) :
	maxSize(numConnections),
	dbAddress(dbAddress),
	auth(auth ? new mongo::BSONObj(*auth) : nullptr)
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
				repoTrace << auth->toString();
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

MongoConnectionPool::~MongoConnectionPool()
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

mongo::DBClientBase* MongoConnectionPool::pop()
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
		int attempts = 0;
		while (!worker->isStillConnected() && attempts++ < 5)
		{
			std::string tmp;
			try{
				worker = connectWorker(tmp);
			}
			catch (mongo::DBException &e)
			{
				repoInfo << "failed to connect to the database, retry in 5s...";
				boost::this_thread::sleep(boost::posix_time::seconds(5));
			}
		}

		if (!worker->isStillConnected())
		{
			//Failed to reconnect to the database push the worker back and return a nullpointer
			repoError << "Failed to reconnect to the mongo database. Retries exhausted";
			push(worker);
			worker = nullptr; 
		}
	}

	return worker;
}

mongo::DBClientBase* MongoConnectionPool::connectWorker(std::string &errMsg)
{
	mongo::DBClientBase *worker = dbAddress.connect(errMsg);
	if (!worker ||
		(auth && !worker->auth(auth->getStringField("db"), auth->getStringField("user"), auth->getStringField("pwd"), errMsg, auth->getField("digestPassword").boolean())))
	{
		throw mongo::DBException(errMsg, mongo::ErrorCodes::AuthenticationFailed);
	}
	return worker;
}