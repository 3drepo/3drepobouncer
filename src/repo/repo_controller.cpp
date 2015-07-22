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


#include "repo_controller.h"

using namespace repo;

RepoController::RepoController(
	const uint32_t &numConcurrentOps,
	const uint32_t &numDbConn) :
	numDBConnections(numDbConn)
{
	for (uint32_t i = 0; i < numConcurrentOps; i++)
	{
		manipulator::RepoManipulator* worker = new manipulator::RepoManipulator();
		workerPool.push(worker);
	}
}


RepoController::~RepoController()
{

	std::vector<manipulator::RepoManipulator*> workers = workerPool.empty();
	std::vector<manipulator::RepoManipulator*>::iterator it;
	for (it = workers.begin(); it != workers.end(); ++it)
	{
		manipulator::RepoManipulator* man = *it;
		if (man)
			delete man;
	}
}

RepoToken* RepoController::authenticateMongo(
	std::string       &errMsg,
	const std::string &address,
	const uint32_t    &port,
	const std::string &dbName,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested
	)
{
	manipulator::RepoManipulator* worker = workerPool.pop();

	core::model::bson::RepoBSON* cred = 0;
	RepoToken *token = 0;
	
	std::string dbFullAd = address + ":" + std::to_string(port);

	bool success = worker->connectAndAuthenticate(errMsg, address, port, 
		numDBConnections, dbName, username, password, pwDigested);
	
	if (success)
		cred = worker->createCredBSON(dbFullAd, username, password, pwDigested);
	workerPool.push(worker);

	if (cred)
	{
		token = new RepoToken(cred, dbFullAd, dbName);
	}
	return token ;
}

std::list<std::string> RepoController::getDatabases(RepoToken *token)
{
	std::list<std::string> list;
	if (token)
	{
		if (token->databaseName == core::handler::AbstractDatabaseHandler::ADMIN_DATABASE)
		{
			manipulator::RepoManipulator* worker = workerPool.pop();

			list = worker->fetchDatabases(token->databaseAd, token->credentials);

			workerPool.push(worker);
		}
		else
		{
			//If the user is only authenticated against a single 
			//database then just return the database he/she is authenticated against.
			list.push_back(token->databaseName);
		}
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Trying to fetch database without a Repo Token!";
	}

	return list;
}

std::list<std::string>  RepoController::getCollections(
	RepoToken             *token,
	const std::string     &databaseName
	)
{
	std::list<std::string> list;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		list = worker->fetchCollections(token->databaseAd, token->credentials, databaseName);
		workerPool.push(worker);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Trying to fetch collections without a Repo Token!";
	}

	return list;

}