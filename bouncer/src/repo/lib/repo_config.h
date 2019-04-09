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

#pragma once
#include <string>
#include "../repo_bouncer_global.h"


namespace repo{
	namespace lib{
		class RepoConfig
		{

		public:
			enum class FileStorageEngine { GRIDFS, S3, FS }; //FIXME: this should live in filemanager?
			enum class CompressionType { NONE }; //FIXME: this belongs in filemanager

			struct database_config_t {
				std::string addr;
				int port = 27017;
				std::string username;
				std::string password;
				bool pwDigested = false;
			};

			struct s3_config_t {
				std::string bucketName;
				std::string bucketRegion;
				bool configured = false;
			};

			struct fs_config_t {
				std::string dir;
				int nLevel;
				bool configured = false;
			};
		
			/**
			* Instantiate Repo Config with a database connection
			* @params databaseAddr database address
			* @params port database port
			* @params username username to login with
			* @params password password to login with
			* @param pwDigested true if password given is digested.
			*/
			REPO_API_EXPORT RepoConfig(
				const std::string &databaseAddr,
				const int &port,
				const std::string &username,
				const std::string &password,
				const bool pwDigested = false);

			REPO_API_EXPORT ~RepoConfig() {}

			/**
			* Set configurations to connect to S3 to store files
			* @params bucketName name of the bucket
			* @params bucketRegion name of the region the bucket sits in
			* @params useAsDefault use this as the default storage engine
			*/
			void REPO_API_EXPORT configureS3(
				const std::string &bucketName,
				const std::string &bucketRegion,
				const bool useAsDefault = true);

			/**
			* Set configurations to connect to use Filesystem to store files
			* @params directory directory to the file share
			* @params level number of hierachys to use
			* @params useAsDefault use this as the default storage engine
			*/
			void REPO_API_EXPORT configureFS(
				const std::string &directory,
				const int         &level = 2,
				const bool useAsDefault = true
				);			

			const database_config_t getDatabaseConfig() const { return dbConf; }
			const s3_config_t getS3Config() const { return s3Conf; }
			const fs_config_t getFSConfig() const { return fsConf; }

			/**
			* Get default storage engine currently configured
			* @return returns the default engine of choice for new writes
			*/
			FileStorageEngine getDefaultStorageEngine() const { return defaultStorage; }

			bool validate() const;

		private:
			database_config_t dbConf;
			s3_config_t s3Conf;
			fs_config_t fsConf;
			FileStorageEngine defaultStorage;
			CompressionType compression = CompressionType::NONE;

		};
	}
}
