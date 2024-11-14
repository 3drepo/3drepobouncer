/**
*  Copyright (C) 2019 3D Repo Ltd
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
* A (attempt to be) thread safe stack essentially that would push/pop items as required
* Incredibly weird, I know.
*/

#include "repo_config.h"
#include "repo_exception.h"
#include "repo_log.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
using namespace repo::lib;

RepoConfig::RepoConfig(
	const std::string &databaseAddr,
	const int &port,
	const std::string &username,
	const std::string &password) : defaultStorage(FileStorageEngine::FS)
{
	dbConf.addr = databaseAddr;
	dbConf.port = port;
	dbConf.username = username;
	dbConf.password = password;
}

RepoConfig::RepoConfig(
	const std::string &connString,
	const std::string &username,
	const std::string &password) : defaultStorage(FileStorageEngine::FS)
{
	dbConf.connString = connString;
	dbConf.username = username;
	dbConf.password = password;
}

void RepoConfig::configureFS(
	const std::string &directory,
	const int         &level,
	const bool useAsDefault)
{
	fsConf.dir = directory;
	fsConf.nLevel = level;
	fsConf.configured = true;

	if (useAsDefault) defaultStorage = FileStorageEngine::FS;
}

RepoConfig RepoConfig::fromFile(const std::string &filePath) {
	boost::property_tree::ptree jsonTree;
	try {
		boost::property_tree::read_json(filePath, jsonTree);
	}
	catch (const std::exception &e) {
		throw RepoException("Failed to read configuration file [" + filePath + "] : " + e.what());
	}

	//Read database configurations
	auto dbTree = jsonTree.get_child_optional("db");

	if (!dbTree) {
		throw RepoException("Cannot find entry 'db' within configuration file.");
	}

	auto dbAddr = dbTree->get<std::string>("dbhost", "");
	auto dbPort = dbTree->get<int>("dbport", -1);
	auto dbConn = dbTree->get<std::string>("connectionString", "");
	auto username = dbTree->get<std::string>("username", "");
	auto password = dbTree->get<std::string>("password", "");

	auto useHostAndPort = !dbAddr.empty() && dbPort > 0;
	if (!useHostAndPort && dbConn.empty()) {
		throw RepoException("Database address and port not specified within configuration file.");
	}

	repo::lib::RepoConfig config = useHostAndPort ? RepoConfig(dbAddr, dbPort, username, password) : RepoConfig(dbConn, username, password);

	auto useAsDefault = jsonTree.get<std::string>("defaultStorage", "");

	//Read FS configuirations if found
	auto fsTree = jsonTree.get_child_optional("fs");

	if (fsTree) {
		auto path = fsTree->get<std::string>("path", "");
		auto level = fsTree->get<int>("level", REPO_CONFIG_FS_DEFAULT_LEVEL);
		if (!path.empty())
			config.configureFS(path, level, useAsDefault == "fs" || useAsDefault.empty());
	}

	return config;
}

bool RepoConfig::validate() const {
	const bool validDBConn = !dbConf.connString.empty() || (!dbConf.addr.empty() && dbConf.port > 0);
	const bool dbOk = validDBConn && (dbConf.username.empty() == dbConf.password.empty());
	const bool fsOk = !fsConf.configured || (!fsConf.dir.empty() && fsConf.nLevel >= 0);

	return dbOk && fsOk;
}