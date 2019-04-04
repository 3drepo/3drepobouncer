/**
*  Copyright (C) 2016 3D Repo Ltd
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
using namespace repo::lib;


RepoConfig::RepoConfig(
	const std::string &databaseAddr,
	const int &port,
	const std::string &username,
	const std::string &password,
	const bool pwDigested) : defaultStorage(FileStorageEngine::GRIDFS)
{
	dbConf.addr = databaseAddr;
	dbConf.port = port;
	dbConf.username = username;
	dbConf.password = password;
	dbConf.pwDigested = pwDigested;
}

void RepoConfig::configureS3(
	const std::string &bucketName,
	const std::string &bucketRegion,
	const bool useAsDefault)
{
	s3Conf.bucketName = bucketName;
	s3Conf.bucketRegion = bucketRegion;
	s3Conf.configured = true;

	if (useAsDefault) defaultStorage = FileStorageEngine::S3;
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

bool RepoConfig::validate() const{
	const bool dbOk = !dbConf.addr.empty() && (dbConf.username.empty() || !dbConf.password.empty()) && dbConf.port > 0;
	const bool s3Ok = !s3Conf.configured || (!s3Conf.bucketName.empty() && !s3Conf.bucketRegion.empty());
	const bool fsOk = !fsConf.configured || !fsConf.dir.empty();

	return dbOk && s3Ok && fsOk;
}