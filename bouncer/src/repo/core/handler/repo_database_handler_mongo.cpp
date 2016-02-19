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

#include <regex>
#include <unordered_map>

#include "repo_database_handler_mongo.h"
#include "../../lib/repo_log.h"

using namespace repo::core::handler;

static uint64_t MAX_MONGO_BSON_SIZE=16777216L;
//------------------------------------------------------------------------------

const std::string repo::core::handler::MongoDatabaseHandler::ID = "_id";
const std::string repo::core::handler::MongoDatabaseHandler::UUID = "uuid";
const std::string repo::core::handler::MongoDatabaseHandler::ADMIN_DATABASE = "admin";
const std::string repo::core::handler::MongoDatabaseHandler::SYSTEM_ROLES_COLLECTION = "system.roles";
const std::list<std::string> repo::core::handler::MongoDatabaseHandler::ANY_DATABASE_ROLES =
{ "dbAdmin", "dbOwner", "read", "readWrite", "userAdmin" };
const std::list<std::string> repo::core::handler::MongoDatabaseHandler::ADMIN_ONLY_DATABASE_ROLES =
{ "backup", "clusterAdmin", "clusterManager", "clusterMonitor", "dbAdminAnyDatabase",
"hostManager", "readAnyDatabase", "readWriteAnyDatabase", "restore", "root",
"userAdminAnyDatabase" };

//------------------------------------------------------------------------------

MongoDatabaseHandler* MongoDatabaseHandler::handler = NULL;

MongoDatabaseHandler::MongoDatabaseHandler(
	const mongo::ConnectionString &dbAddress,
	const uint32_t                &maxConnections,
	const std::string             &dbName,
	const std::string             &username,
	const std::string             &password,
	const bool                    &pwDigested) :
	AbstractDatabaseHandler(MAX_MONGO_BSON_SIZE)
{
	mongo::client::initialize();
	workerPool = new connectionPool::MongoConnectionPool(maxConnections, dbAddress, createAuthBSON(dbName, username, password, pwDigested));
}

/**
* A Deconstructor
*/
MongoDatabaseHandler::~MongoDatabaseHandler()
{
	if (workerPool)
		delete workerPool;
}

bool MongoDatabaseHandler::caseInsensitiveStringCompare(
	const std::string& s1,
	const std::string& s2)
{
	return strcasecmp(s1.c_str(), s2.c_str()) <= 0;
}


uint64_t MongoDatabaseHandler::countItemsInCollection(
	const std::string &database,
	const std::string &collection,
	std::string &errMsg)
{
	uint64_t numItems = 0;
	mongo::DBClientBase *worker;
	try{

		worker = workerPool->getWorker();
		numItems = worker->count(database + "." + collection);
	}
	catch (mongo::DBException& e)
	{
		errMsg =  "Failed to count num. items within "
			+ database + "." + collection + ":" + e.what();
		repoError << errMsg;
	}

	workerPool->returnWorker(worker);

	return numItems;

}

mongo::BSONObj* MongoDatabaseHandler::createAuthBSON(
	const std::string &database,
	const std::string &username, 
	const std::string &password,
	const bool        &pwDigested)
{
	mongo::BSONObj* authBson = nullptr;
	if (!username.empty() && !database.empty() && !password.empty())
	{

		std::string passwordDigest = pwDigested ?
		password : mongo::DBClientWithCommands::createPasswordDigest(username, password);
		authBson = new mongo::BSONObj(BSON("user" << username <<
			"db" << database <<
			"pwd" << passwordDigest <<
			"digestPassword" << false));
	}

	return authBson;
}

void MongoDatabaseHandler::createCollection(const std::string &database, const std::string &name)
{
	mongo::DBClientBase *worker;
	try{

		worker = workerPool->getWorker();
		worker->createCollection(database + "." + name);
	}
	catch (mongo::DBException& e)
	{
		repoError << "Failed to create collection ("
			<< database << "." << name << ":" << e.what();
	}

	workerPool->returnWorker(worker);
}

repo::core::model::RepoBSON MongoDatabaseHandler::createRepoBSON(
	mongo::DBClientBase *worker,
	const std::string &database,
	const std::string &collection,
	const mongo::BSONObj &obj)
{
	repo::core::model::RepoBSON orgBson = repo::core::model::RepoBSON(obj);

	std::vector<std::pair<std::string, std::string>> extFileList = orgBson.getFileList();

	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>> > binMap;

	for (const auto &pair : extFileList)
	{
		repoTrace << "Found existing GridFS reference, retrieving file @ " << database << "." << collection << ":" << pair.first;
		binMap[pair.first] = std::pair<std::string, std::vector<uint8_t>>(pair.second, getBigFile(worker, database, collection, pair.second));
	}

	return repo::core::model::RepoBSON(obj, binMap);
}

void MongoDatabaseHandler::disconnectHandler()
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

bool MongoDatabaseHandler::dropCollection(
	const std::string &database,
	const std::string &collection,
	std::string &errMsg)
{

	bool success = true;
	mongo::DBClientBase *worker;
	try{

		worker = workerPool->getWorker();
		worker->dropCollection(database + "." + collection);
	}
	catch (mongo::DBException& e)
	{
		repoError << "Failed to drop collection ("
			<< database << "." << collection << ":" << e.what();
	}

	workerPool->returnWorker(worker);

	return success;
}

bool MongoDatabaseHandler::dropDatabase(
	const std::string &database,
	std::string &errMsg)
{
	bool success = true;
	mongo::DBClientBase *worker;
	try{

		worker = workerPool->getWorker();
		worker->dropDatabase(database);
	}
	catch (mongo::DBException& e)
	{
		repoError << "Failed to drop database :" << e.what();
	}

	workerPool->returnWorker(worker);

	return success;
}

bool MongoDatabaseHandler::dropDocument(
	const repo::core::model::RepoBSON bson,
	const std::string &database,
	const std::string &collection,
	std::string &errMsg)
{
	bool success = true;
	mongo::DBClientBase *worker;
	try{
		worker = workerPool->getWorker();
		mongo::BSONElement bsonID;
		bson.getObjectID(bsonID);
		mongo::Query query = MONGO_QUERY("_id" << bsonID);
		worker->remove(database + "." + collection, query, true);


	}
	catch (mongo::DBException& e)
	{
		repoError << "Failed to drop document :" << e.what();
	}

	workerPool->returnWorker(worker);

	return true;
}

mongo::BSONObj MongoDatabaseHandler::fieldsToReturn(
	const std::list<std::string>& fields,
	bool excludeIdField)
{
	mongo::BSONObjBuilder fieldsToReturn;
	std::list<std::string>::const_iterator it;
	for (it = fields.begin(); it != fields.end(); ++it)
	{
		fieldsToReturn << *it << 1;
		excludeIdField = excludeIdField && ID != *it;
	}
	if (excludeIdField)
		fieldsToReturn << ID << 0;

	return fieldsToReturn.obj();
}

std::vector<repo::core::model::RepoBSON> MongoDatabaseHandler::findAllByCriteria(
	const std::string& database,
	const std::string& collection,
	const repo::core::model::RepoBSON& criteria)
{
	std::vector<repo::core::model::RepoBSON> data;

	if (!criteria.isEmpty())
	{
		mongo::DBClientBase *worker;
		try{
			uint64_t retrieved = 0;
			std::auto_ptr<mongo::DBClientCursor> cursor;
			worker = workerPool->getWorker();
			do
			{
				cursor = worker->query(
					database + "." + collection,
					criteria,
					0,
					retrieved);

				for (; cursor.get() && cursor->more(); ++retrieved)
				{
					data.push_back(createRepoBSON(worker, database, collection, cursor->nextSafe().copy()));
				}
			} while (cursor.get() && cursor->more());

		}
		catch (mongo::DBException& e)
		{
			repoError << "Error in MongoDatabaseHandler::findAllByCriteria: " << e.what();
		}

		workerPool->returnWorker(worker);
	}

	return data;
}

repo::core::model::RepoBSON MongoDatabaseHandler::findOneByCriteria(
	const std::string& database,
	const std::string& collection,
	const repo::core::model::RepoBSON& criteria,
	const std::string& sortField)
{
	repo::core::model::RepoBSON data;

	if (!criteria.isEmpty())
	{
		mongo::DBClientBase *worker;
		try{
			uint64_t retrieved = 0;
			worker = workerPool->getWorker();
			auto query = mongo::Query(criteria);
			if (!sortField.empty())
				query = query.sort(sortField, -1);
			
			data = repo::core::model::RepoBSON(worker->findOne(
				database + "." + collection,
				query));

		}
		catch (mongo::DBException& e)
		{
			repoError << "Error in MongoDatabaseHandler::findOneByCriteria: " << e.what();
		}

		workerPool->returnWorker(worker);
	}

	return data;
}

std::vector<repo::core::model::RepoBSON> MongoDatabaseHandler::findAllByUniqueIDs(
	const std::string& database,
	const std::string& collection,
	const repo::core::model::RepoBSON& uuids){

	std::vector<repo::core::model::RepoBSON> data;

	mongo::BSONArray array = mongo::BSONArray(uuids);

	int fieldsCount = array.nFields();
	if (fieldsCount > 0)
	{
		mongo::DBClientBase *worker;
		try{
			uint64_t retrieved = 0;
			std::auto_ptr<mongo::DBClientCursor> cursor;
			worker = workerPool->getWorker();
			do
			{

				mongo::BSONObjBuilder query;
				query << ID << BSON("$in" << array);


				cursor = worker->query(
					database + "." + collection,
					query.obj(),
					0,
					retrieved);

				for (; cursor.get() && cursor->more(); ++retrieved)
				{
					data.push_back(createRepoBSON(worker, database, collection, cursor->nextSafe().copy()));
				}
			} while (cursor.get() && cursor->more());

			if (fieldsCount != retrieved){
				repoError << "Number of documents("<< retrieved<<") retreived by findAllByUniqueIDs did not match the number of unique IDs(" <<  fieldsCount <<")!";
			}
		}
		catch (mongo::DBException& e)
		{
			repoError << e.what();
		}

		workerPool->returnWorker(worker);
	}



	return data;
}

repo::core::model::RepoBSON MongoDatabaseHandler::findOneBySharedID(
	const std::string& database,
	const std::string& collection,
	const repoUUID& uuid,
	const std::string& sortField)
{

	repo::core::model::RepoBSON bson;
	mongo::DBClientBase *worker;
	try
	{
		repo::core::model::RepoBSONBuilder queryBuilder;
		queryBuilder.append("shared_id", uuid);
		//----------------------------------------------------------------------
	
		worker = workerPool->getWorker();
		mongo::BSONObj bsonMongo = worker->findOne(
			getNamespace(database, collection),
			mongo::Query(queryBuilder.obj()).sort(sortField, -1));

		bson = createRepoBSON(worker, database, collection, bsonMongo);
	}
	catch (mongo::DBException& e)
	{
		repoError << "Error querying the database: "<< std::string(e.what());
	}

	workerPool->returnWorker(worker);
	return bson;
}

repo::core::model::RepoBSON  MongoDatabaseHandler::findOneByUniqueID(
	const std::string& database,
	const std::string& collection,
	const repoUUID& uuid){

	repo::core::model::RepoBSON bson;
	mongo::DBClientBase *worker;
	try
	{
		repo::core::model::RepoBSONBuilder queryBuilder;
		queryBuilder.append(ID, uuid);

		worker = workerPool->getWorker();
		mongo::BSONObj bsonMongo = worker->findOne(getNamespace(database, collection),
			mongo::Query(queryBuilder.obj()));

		bson = createRepoBSON(worker, database, collection, bsonMongo);
	}
	catch (mongo::DBException& e)
	{
		repoError << e.what();
	}

	workerPool->returnWorker(worker);
	return bson;
}

std::vector<repo::core::model::RepoBSON>
	MongoDatabaseHandler::getAllFromCollectionTailable(
		const std::string                             &database,
		const std::string                             &collection,
		const uint64_t                                &skip,
		const uint32_t                                &limit,
		const std::list<std::string>				  &fields,
		const std::string							  &sortField,
		const int									  &sortOrder)
{
	std::vector<repo::core::model::RepoBSON> bsons;
	mongo::DBClientBase *worker;
	try
	{

		worker = workerPool->getWorker();
		
		mongo::BSONObj tmp = fieldsToReturn(fields);

		std::auto_ptr<mongo::DBClientCursor> cursor = worker->query(
			database + "." + collection,
			sortField.empty() ? mongo::Query() : mongo::Query().sort(sortField, sortOrder),
			0,
			skip,
			fields.size() > 0 ? &tmp : nullptr);

		while (cursor.get() && cursor->more())
		{
			//have to copy since the bson info gets cleaned up when cursor gets out of scope
			bsons.push_back(createRepoBSON(worker, database, collection, cursor->nextSafe().copy()));
		}
	}
	catch (mongo::DBException& e)
	{
		repoError << "Failed retrieving bsons from mongo: " << e.what();
	}

	workerPool->returnWorker(worker);
	return bsons;

}

std::list<std::string> MongoDatabaseHandler::getCollections(
	const std::string &database)
{
	std::list<std::string> collections;
	mongo::DBClientBase *worker;
	try
	{
		worker = workerPool->getWorker();
		collections = worker->getCollectionNames(database);
	}
	catch (mongo::DBException& e)
	{
		repoError << e.what();
	}

	workerPool->returnWorker(worker);
	return collections;
}

repo::core::model::CollectionStats MongoDatabaseHandler::getCollectionStats(
	const std::string    &database,
	const std::string    &collection,
	std::string          &errMsg)
{
	mongo::BSONObj info;
	mongo::DBClientBase *worker;
	try {
		mongo::BSONObjBuilder builder;
		builder.append("collstats", collection);
		builder.append("scale", 1); // 1024 == KB 		

		worker = workerPool->getWorker();
		worker->runCommand(database, builder.obj(), info);
	}
	catch (mongo::DBException &e)
	{
		errMsg = e.what();
		repoError << "Failed to retreive collection stats for" << database 
			<< "." << collection << " : " << errMsg;
	}

	workerPool->returnWorker(worker);
	return repo::core::model::CollectionStats(info);
}

std::list<std::string> MongoDatabaseHandler::getDatabases(
	const bool &sorted)
{
	std::list<std::string> list;
	mongo::DBClientBase *worker;
	try
	{
		worker = workerPool->getWorker();
		list = worker->getDatabaseNames();

		if (sorted)
			list.sort(&MongoDatabaseHandler::caseInsensitiveStringCompare);
	}
	catch (mongo::DBException& e)
	{
		repoError << e.what();
	}
	workerPool->returnWorker(worker);
	return list;
}

std::vector<uint8_t> MongoDatabaseHandler::getBigFile(
	mongo::DBClientBase *worker,
	const std::string &database,
	const std::string &collection,
	const std::string &fileName)
{
	mongo::GridFS gfs(*worker, database, collection);
	mongo::GridFile tmpFile = gfs.findFileByName(fileName);

	std::vector<uint8_t> bin;
	if (tmpFile.exists())
	{
		std::ostringstream oss;
		tmpFile.write(oss);

		std::string fileStr = oss.str();
		
		assert(sizeof(*fileStr.c_str()) == sizeof(uint8_t));

		if (!fileStr.empty())
		{
			bin.resize(fileStr.size());
			memcpy(&bin[0], fileStr.c_str(), fileStr.size());

		}
		else
		{
			repoError << "GridFS file : " << fileName << " in " 
				<< database << "." << collection << " is empty.";
 		}
	}
	else
	{
		repoError << "Failed to find file within GridFS";
	}


	return bin;
}

std::string MongoDatabaseHandler::getProjectFromCollection(const std::string &ns, const std::string &projectExt)
{
	size_t ind = ns.find("." + projectExt);
	std::string project;
	//just to make sure string is empty.
	project.clear();
	if (ind != std::string::npos){
		project = ns.substr(0, ind);
	}
	return project;
}

std::map<std::string, std::list<std::string> > MongoDatabaseHandler::getDatabasesWithProjects(
	const std::list<std::string> &databases, const std::string &projectExt)
{
	std::map<std::string, std::list<std::string> > mapping;
	try
	{
		for (std::list<std::string>::const_iterator it = databases.begin();
			it != databases.end(); ++it)
		{
			std::string database = *it;
			std::list<std::string> projects = getProjects(database, projectExt);
			mapping.insert(std::make_pair(database, projects));
		}
	}
	catch (mongo::DBException& e)
	{
		repoError << e.what();
	}
	return mapping;
}

MongoDatabaseHandler* MongoDatabaseHandler::getHandler(
	std::string       &errMsg,
	const std::string &host, 
	const int         &port,
	const uint32_t    &maxConnections,
	const std::string &dbName,
	const std::string &username,
	const std::string &password,
	const bool        &pwDigested)
{

	std::ostringstream connectionString;

	mongo::HostAndPort hostAndPort = mongo::HostAndPort(host, port >= 0 ? port : -1);

	mongo::ConnectionString mongoConnectionString = mongo::ConnectionString(hostAndPort);


	if(!handler){
		//initialise the mongo client
		repoTrace << "Handler not present for " << mongoConnectionString.toString() << " instantiating new handler...";
		try{
			handler = new MongoDatabaseHandler(mongoConnectionString, maxConnections, dbName, username, password, pwDigested);
		}
		catch (mongo::DBException e)
		{
			if (handler)
				delete handler;
			handler = 0;
			errMsg = std::string(e.what());
			repoError << "Error establishing Mongo Handler: " << errMsg;
		}
	}
	else
	{
		repoTrace << "Found handler, returning existing handler";
	}


	return handler;
}

std::string MongoDatabaseHandler::getNamespace(
	const std::string &database, 
	const std::string &collection)
{	
	return database + "." + collection;
}

std::list<std::string> MongoDatabaseHandler::getProjects(const std::string &database, const std::string &projectExt)
{
	// TODO: remove db.info from the list (anything that is not a project basically)
	std::list<std::string> collections = getCollections(database);
	std::list<std::string> projects;
	for (std::list<std::string>::iterator it = collections.begin(); it != collections.end(); ++it){
		std::string project = getProjectFromCollection(*it, projectExt);
		if (!project.empty())
			projects.push_back(project);
	}
	projects.sort();
	projects.unique();
	return projects;
}

std::vector<uint8_t> MongoDatabaseHandler::getRawFile(
	const std::string& database,
	const std::string& collection,
	const std::string& fname
	)
{
	bool success = true;
	mongo::DBClientBase *worker;

	worker = workerPool->getWorker();
	mongo::GridFS gfs(*worker, database, collection);
	mongo::GridFile tmpFile = gfs.findFileByName(fname);

	repoTrace << "Getting file from GridFS: " << fname << " in : " << database << "." << collection;

	std::vector<uint8_t> bin;
	if (tmpFile.exists())
	{
		std::ostringstream oss;
		tmpFile.write(oss);

		std::string fileStr = oss.str();

		assert(sizeof(*fileStr.c_str()) == sizeof(uint8_t));

		if (!fileStr.empty())
		{
			bin.resize(fileStr.size());
			memcpy(&bin[0], fileStr.c_str(), fileStr.size());

		}
		else
		{
			repoError << "GridFS file : " << fname << " in "
				<< database << "." << collection << " is empty.";
		}
	}
	else
	{
		repoError << "Failed to find file within GridFS";
	}


	workerPool->returnWorker(worker);

	return bin;
}

bool MongoDatabaseHandler::insertDocument(
	const std::string &database,
	const std::string &collection,
	const repo::core::model::RepoBSON &obj,
	std::string &errMsg)
{
	bool success = true;
	mongo::DBClientBase *worker;
	try{
		worker = workerPool->getWorker();
		worker->insert(getNamespace(database, collection), obj);

		success &= storeBigFiles(worker, database, collection, obj, errMsg);

	}
	catch (mongo::DBException &e)
	{
		success = false;
		std::string errString(e.what());
		errMsg += errString;
	}

	workerPool->returnWorker(worker);

	return success;
}

bool MongoDatabaseHandler::insertRawFile(
	const std::string          &database,
	const std::string          &collection,
	const std::string          &fileName,
	const std::vector<uint8_t> &bin,
	      std::string          &errMsg,
	const std::string          &contentType
	)
{
	bool success = true;
	mongo::DBClientBase *worker;

	repoTrace << "writing raw file: " << fileName;

	if (bin.size() == 0)
	{
		errMsg = "size of file is 0!";
		return false;
	}

	if (fileName.empty())
	{
		errMsg = "Cannot store a raw file in the database with no file name!";
		return false;
	}

	try{
		worker = workerPool->getWorker();
		//store the big biary file within GridFS
		mongo::GridFS gfs(*worker, database, collection);
		//FIXME: there must be errors to catch...
		repoTrace << "storing " << fileName << " in gridfs: " << database << "." << collection;
		mongo::BSONObj bson = gfs.storeFile((char*)&bin[0], bin.size() * sizeof(bin[0]), fileName, contentType);

		repoTrace << "returned object: " << bson.toString();
	}
	catch (mongo::DBException &e)
	{
		success = false;
		std::string errString(e.what());
		errMsg += errString;
	}

	workerPool->returnWorker(worker);

	return success;
}

bool MongoDatabaseHandler::performRoleCmd(
	const OPERATION                         &op,
	const repo::core::model::RepoRole       &role,
	std::string                             &errMsg)
{
	bool success = true;
	mongo::DBClientBase *worker;

	if (!role.isEmpty())
	{
		if (role.getName().empty() || role.getDatabase().empty())
		{
			errMsg += "Role bson does not contain role name/database name";
		}
		else{
			try{
				worker = workerPool->getWorker();
				mongo::BSONObjBuilder cmdBuilder;
				std::string roleName = role.getName();
				switch (op)
				{
				case OPERATION::INSERT:
					cmdBuilder << "createRole" << roleName;
					break;
				case OPERATION::UPDATE:
					cmdBuilder << "updateRole" << roleName;
					break;
				case OPERATION::DROP:
					cmdBuilder << "dropRole" << roleName;
				}

				if (op != OPERATION::DROP)
				{

					repo::core::model::RepoBSON privileges = role.getObjectField(REPO_ROLE_LABEL_PRIVILEGES);
					cmdBuilder.appendArray("privileges", privileges);

					repo::core::model::RepoBSON inheritedRoles = role.getObjectField(REPO_ROLE_LABEL_INHERITED_ROLES);

					cmdBuilder.appendArray("roles", inheritedRoles);
				}


				mongo::BSONObj info;
				auto cmd = cmdBuilder.obj();
				worker->runCommand(role.getDatabase(), cmd, info);

				repoTrace << "Role command : " << cmd;

				std::string cmdError = info.getStringField("errmsg");
				if (!cmdError.empty())
				{
					success = false;
					errMsg += cmdError;
				}

			}
			catch (mongo::DBException &e)
			{
				success = false;
				std::string errString(e.what());
				errMsg += errString;
			}

			workerPool->returnWorker(worker);
		}
		
	}
	else
	{
		errMsg += "Role bson is empty";
	}


	return success;
}

bool MongoDatabaseHandler::performUserCmd(
	const OPERATION                         &op,
	const repo::core::model::RepoUser &user,
	std::string                       &errMsg)
{
	bool success = true;
	mongo::DBClientBase *worker;

	if (!user.isEmpty())
	{
		try{
			worker = workerPool->getWorker();
			repo::core::model::RepoBSONBuilder cmdBuilder;
			std::string username = user.getUserName();
			switch (op)
			{
			case OPERATION::INSERT:
				cmdBuilder << "createUser" << username;
				break;
			case OPERATION::UPDATE:
				cmdBuilder << "updateUser" << username;
				break;
			case OPERATION::DROP:
				cmdBuilder << "dropUser" << username;
			}

			if (op != OPERATION::DROP)
			{
				std::string pw = user.getCleartextPassword();
				if (!pw.empty())
					cmdBuilder << "pwd" << pw;

				repo::core::model::RepoBSON customData = user.getCustomDataBSON();
				if (!customData.isEmpty())
					cmdBuilder << "customData" << customData;

				//compulsory, so no point checking if it's empty
				cmdBuilder.appendArray("roles", user.getRolesBSON());
			}
	

			mongo::BSONObj info;
			worker->runCommand(ADMIN_DATABASE, cmdBuilder.obj(), info);

			std::string cmdError = info.getStringField("errmsg");
			if (!cmdError.empty())
			{
				success = false;
				errMsg += cmdError;
			}

		}
		catch (mongo::DBException &e)
		{
			success = false;
			std::string errString(e.what());
			errMsg += errString;
		}

		workerPool->returnWorker(worker);
	}
	else
	{
		errMsg += "User bson is empty";
	}


	return success;
}

bool MongoDatabaseHandler::upsertDocument(
	const std::string &database,
	const std::string &collection,
	const repo::core::model::RepoBSON &obj,
	const bool        &overwrite,
	std::string &errMsg)
{
	bool success = true;
	mongo::DBClientBase *worker;

	bool upsert = overwrite;
	try{
		worker = workerPool->getWorker();

	
		repo::core::model::RepoBSONBuilder queryBuilder;
		queryBuilder << ID << obj.getField(ID);

		mongo::BSONElement bsonID;
		obj.getObjectID(bsonID);
		mongo::Query existQuery = MONGO_QUERY("_id" << bsonID);
		mongo::BSONObj bsonMongo = worker->findOne(database + "." + collection, existQuery);

		if (bsonMongo.isEmpty())
		{
			//document doens't exist, insert the document
			upsert = true;
		}

		
		if (upsert)
		{
			mongo::Query query;
			query = BSON(REPO_LABEL_ID << bsonID);
			repoTrace << "query = " << query.toString();
			worker->update(getNamespace(database, collection), query, obj, true);
		}
		else
		{
			//only update fields
			mongo::BSONObjBuilder builder;
			builder << REPO_COMMAND_UPDATE << collection;

			mongo::BSONObjBuilder updateBuilder;
			updateBuilder << REPO_COMMAND_Q << BSON(REPO_LABEL_ID << bsonID);
			updateBuilder << REPO_COMMAND_U << BSON("$set" << obj.removeField(ID));
			updateBuilder << REPO_COMMAND_UPSERT << true;

			builder << REPO_COMMAND_UPDATES << BSON_ARRAY(updateBuilder.obj());
			mongo::BSONObj info;
			worker->runCommand(database, builder.obj(), info);

			if (info.hasField("writeErrors"))
			{
				repoError << info.getField("writeErrors").Array().at(0).embeddedObject().getField("errmsg");
				success = false;
			}
		}

		if (success)
			success = storeBigFiles(worker, database, collection, obj, errMsg);

			
	}
	catch (mongo::DBException &e)
	{
		success = false;
		std::string errString(e.what());
		errMsg += errString;
	}

	workerPool->returnWorker(worker);
	return success;
}


bool MongoDatabaseHandler::storeBigFiles(
	mongo::DBClientBase *worker,
	const std::string &database,
	const std::string &collection,
	const repo::core::model::RepoBSON &obj,
	std::string &errMsg
	)
{
	bool success = true;


	//insert files into gridFS if applicable
	if (obj.hasOversizeFiles())
	{
		const std::vector<std::pair<std::string,std::string>> fNames = obj.getFileList();
		repoTrace << "storeBigFiles: #oversized files: " << fNames.size();

		for (const auto &file : fNames)
		{
			std::vector<uint8_t> binary = obj.getBigBinary(file.first);
			if (binary.size())
			{
				//store the big biary file within GridFS
				mongo::GridFS gfs(*worker, database, collection);
				//FIXME: there must be errors to catch...
				repoTrace << "storing " << file.second << "("<< file.first << ") in gridfs: " << database << "." << collection;
				mongo::BSONObj bson = gfs.storeFile((char*)&binary[0], binary.size() * sizeof(binary[0]), file.second);

				repoTrace << "returned object: " << bson.toString();

			}
			else
			{
				repoError << "A oversized entry exist but binary not found!";
				success = false;
			}
		}
	}

	return true;
}