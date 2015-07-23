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


#include <boost/make_shared.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/trivial.hpp>

#include "lib/repo_broadcaster.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/utility/empty_deleter.hpp>

using namespace repo;

typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend > text_sink;

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


	subscribeBroadcasterToLog();

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
	BOOST_LOG_TRIVIAL(info) << "Controller: Fetching Database....";
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

void RepoController::setLoggingLevel(const RepoLogLevel &level)
{

	boost::log::trivial::severity_level loggingLevel;
	switch (level)
	{
		case LOG_ALL:
			loggingLevel = boost::log::trivial::trace;
			break;
		case LOG_DEBUG:
			loggingLevel = boost::log::trivial::debug;
			break;
		case LOG_INFO:
			loggingLevel = boost::log::trivial::info;
			break;
		case LOG_WARNING:
			loggingLevel = boost::log::trivial::warning;
			break;
		case LOG_ERROR:
			loggingLevel = boost::log::trivial::error;
			break;
		case LOG_NONE:
			loggingLevel = boost::log::trivial::fatal;
			break;
		default:
			BOOST_LOG_TRIVIAL(error) << "Unknown log level: " << level;
			return;

	}

	BOOST_LOG_TRIVIAL(trace) << "Setting logging level to: " << level;
	if (level == LOG_NONE)
	{
		boost::log::core::get()->set_filter(
			boost::log::trivial::severity > loggingLevel);
	}
	else
	{
		boost::log::core::get()->set_filter(
			boost::log::trivial::severity >= loggingLevel);
	}
}

void RepoController::logToFile(const std::string &filePath)
{

	subscribeToLogger(boost::make_shared< std::ofstream >(filePath));

}

void RepoController::subscribeToLogger(
	lib::RepoAbstractListener *listener)
{
	BOOST_LOG_TRIVIAL(trace) << "New subscriber to the log";
	lib::RepoBroadcaster::getInstance()->subscribe(listener);

	subscribeBroadcasterToLog();
}



void RepoController::subscribeToLogger(
	const boost::shared_ptr< std::ostream > stream)
{
	// Construct the sink

	boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();

	sink->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
	sink->set_formatter
		(
		boost::log::expressions::stream 
		<< "[" << boost::log::trivial::severity << "]"
		<< " Message: " << boost::log::expressions::smessage);


	// Add a stream to write log to
	sink->locked_backend()->add_stream(stream);
	sink->locked_backend()->auto_flush(true);

	// Register the sink in the logging core
	boost::log::core::get()->add_sink(sink);

	BOOST_LOG_TRIVIAL(info) << "added new sink";
}


void RepoController::subscribeBroadcasterToLog(){
	//The broadcaster pretends to be a sink of a stream_buffer, create an
	//ostream with the buffer and add it as a sink to the logger

	lib::RepoBroadcaster *broadcaster = lib::RepoBroadcaster::getInstance();



	//std::clog.rdbuf(&sb);

	boost::shared_ptr< std::ostream > stream(
		new boost::iostreams::stream<lib::RepoBroadcaster>(*broadcaster), boost::log::empty_deleter());
	
	 //Construct the sink
	//typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend > text_sink;
	boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();


	std::cout << "test" << numDBConnections;

	subscribeToLogger(stream);


	BOOST_LOG_TRIVIAL(fatal) << " test: " << numDBConnections;
	/*std::clog.flush();
	std::clog.rdbuf(oldbuff);*/

}
