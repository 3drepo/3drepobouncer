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
#include "manipulator/modelconvertor/import/repo_model_import_assimp.h"
#include "manipulator/modelconvertor/export/repo_model_export_assimp.h"
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>



using namespace repo;

typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend > text_sink;

RepoController::RepoController(
	std::vector<lib::RepoAbstractListener*> listeners,
	const uint32_t &numConcurrentOps,
	const uint32_t &numDbConn) :
	numDBConnections(numDbConn)
{

	for (uint32_t i = 0; i < numConcurrentOps; i++)
	{
		manipulator::RepoManipulator* worker = new manipulator::RepoManipulator();
		workerPool.push(worker);
	}

	if (listeners.size() > 0)
	{
		subscribeToLogger(listeners);

		subscribeBroadcasterToLog();
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

void RepoController::commitScene(
	const RepoToken                     *token,
	repo::manipulator::graph::RepoScene *scene)
{
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		worker->commitScene(token->databaseAd, token->credentials, scene);
		workerPool.push(worker);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Trying to commit to the database without a Repo Token!";
	}
}

uint64_t RepoController::countItemsInCollection(
	const RepoToken            *token,
	const std::string    &database,
	const std::string    &collection)
{
	uint64_t numItems;
	BOOST_LOG_TRIVIAL(trace) << "Controller: Counting number of items in the collection";

	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		numItems = worker->countItemsInCollection(token->databaseAd, token->credentials, database, collection);
		workerPool.push(worker);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Trying to fetch database without a Repo Token!";
	}

	return numItems;
}

repo::manipulator::graph::RepoScene* RepoController::fetchScene(
	const RepoToken      *token,
	const std::string    &database,
	const std::string    &collection,
	const std::string    &uuid,
	const bool           &headRevision)
{
	repo::manipulator::graph::RepoScene* scene = 0;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		scene = worker->fetchScene(token->databaseAd, token->credentials,
			database, collection, stringToUUID(uuid), headRevision);

		workerPool.push(worker);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Trying to fetch scene without a Repo Token!";
	}

	return scene;
}

std::vector < repo::core::model::bson::RepoBSON >
	RepoController::getAllFromCollectionContinuous(
	    const RepoToken      *token,
		const std::string    &database,
		const std::string    &collection,
		const uint64_t       &skip)
{
	BOOST_LOG_TRIVIAL(trace) << "Controller: Fetching BSONs from " 
		<< database << "."  << collection << "....";
	std::vector<repo::core::model::bson::RepoBSON> vector;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		vector = worker->getAllFromCollectionTailable(token->databaseAd, token->credentials,
			database, collection, skip);

		workerPool.push(worker);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Trying to fetch BSONs from a collection without a Repo Token!";
	}

	BOOST_LOG_TRIVIAL(trace) << "Obtained " << vector.size() << " bson objects.";

	return vector;
}

std::list<std::string> RepoController::getDatabases(RepoToken *token)
{
	BOOST_LOG_TRIVIAL(trace) << "Controller: Fetching Database....";
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
	const RepoToken       *token,
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

repo::core::model::bson::CollectionStats RepoController::getCollectionStats(
	const RepoToken      *token,
	const std::string    &database,
	const std::string    &collection)
{
	repo::core::model::bson::CollectionStats stats;

	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		std::string errMsg;
		stats = worker->getCollectionStats(token->databaseAd,
			token->credentials, database, collection, errMsg);
		workerPool.push(worker);

		if (!errMsg.empty())
			BOOST_LOG_TRIVIAL(error) << errMsg;
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Trying to get collections stats without a Repo Token!";

	}

	return stats;
}

bool RepoController::removeCollection(
	const RepoToken             *token,
	const std::string     &databaseName,
	const std::string     &collectionName,
	std::string			  &errMsg
	)
{
	bool success = false;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->dropCollection(token->databaseAd, 
			token->credentials, databaseName, collectionName, errMsg);
		workerPool.push(worker);
	}
	else
	{
		errMsg = "Trying to fetch collections without a Repo Token!"; 
		BOOST_LOG_TRIVIAL(error) << errMsg;
		
	}

	return success;
}


bool RepoController::removeDatabase(
	const RepoToken       *token,
	const std::string     &databaseName,
	std::string			  &errMsg
	)
{
	bool success = false;
	if (token)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		success = worker->dropDatabase(token->databaseAd,
			token->credentials, databaseName, errMsg);
		workerPool.push(worker);
	}
	else
	{
		errMsg = "Trying to fetch collections without a Repo Token!";
		BOOST_LOG_TRIVIAL(error) << errMsg;

	}

	return success;
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

	boost::log::add_file_log
		(
		boost::log::keywords::file_name = filePath + "_%N",
		boost::log::keywords::rotation_size = 10 * 1024 * 1024,
		boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
		boost::log::keywords::format = "[%TimeStamp%]: %Message%"
		);

}

void RepoController::subscribeToLogger(
	std::vector<lib::RepoAbstractListener*> listeners)
{
	BOOST_LOG_TRIVIAL(trace) << "New subscriber to the log";
	lib::RepoBroadcaster *broadcaster = lib::RepoBroadcaster::getInstance();
	
	for (auto listener: listeners)
		broadcaster->subscribe(listener);
}

void RepoController::subscribeBroadcasterToLog(){
	BOOST_LOG_TRIVIAL(info) << "subscribeBroadcasterToLog()";
	lib::RepoBroadcaster *broadcaster = lib::RepoBroadcaster::getInstance();

	boost::iostreams::stream<lib::RepoBroadcaster> *streamptr =
		new boost::iostreams::stream<lib::RepoBroadcaster>(*broadcaster);


	boost::shared_ptr< std::ostream > stream(
		streamptr, boost::log::empty_deleter());

	boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();


	sink->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
	//FIXME: better format!
	sink->set_formatter
		(
		boost::log::expressions::stream
		<< "[" << boost::log::trivial::severity << "] "
		<< boost::log::expressions::smessage);


	sink->locked_backend()->add_stream(stream);
	sink->locked_backend()->auto_flush(true);

	// Register the sink in the logging core
	boost::log::core::get()->add_sink(sink);

	BOOST_LOG_TRIVIAL(trace) << "Subscribed broadcaster to log";
}


repo::manipulator::graph::RepoScene* RepoController::createFederatedScene(
	const std::map<repo::core::model::bson::TransformationNode, repo::core::model::bson::ReferenceNode> &fedMap)
{

	repo::manipulator::graph::RepoScene* scene = nullptr;
	if (fedMap.size() > 0)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		scene = worker->createFederatedScene(fedMap);
		workerPool.push(worker);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Trying to federate a new scene graph with no references!";
	}

	return scene;
}

std::string RepoController::getSupportedExportFormats()
{
	//This needs to be updated if we support more than assimp
	return repo::manipulator::modelconvertor::AssimpModelExport::getSupportedFormats();
}

std::string RepoController::getSupportedImportFormats()
{
	//This needs to be updated if we support more than assimp
	return repo::manipulator::modelconvertor::AssimpModelImport::getSupportedFormats();
}

repo::manipulator::graph::RepoScene* 
	RepoController::loadSceneFromFile(
	const std::string                                          &filePath,
	const repo::manipulator::modelconvertor::ModelImportConfig *config)
{

	std::string errMsg;
	repo::manipulator::graph::RepoScene *scene = nullptr;

	if (!filePath.empty())
	{
		manipulator::RepoManipulator* worker = workerPool.pop();
		scene = worker->loadSceneFromFile(filePath, errMsg, config);
		workerPool.push(worker);
		if (!scene)
			BOOST_LOG_TRIVIAL(error) << "Failed ot load scene from file: " << errMsg;
		
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Trying to load from an empty file path!";

	}

	return scene;
}


bool RepoController::saveSceneToFile(
	const std::string &filePath,
	const repo::manipulator::graph::RepoScene* scene)
{
	bool success = true;
	if (scene)
	{
		manipulator::RepoManipulator* worker = workerPool.pop();

		worker->saveSceneToFile(filePath, scene);
		workerPool.push(worker);

	}
	else{
		BOOST_LOG_TRIVIAL(error) << "RepoController::saveSceneToFile: NULL pointer to scene!";
		success = false;
	}

	return success;
}