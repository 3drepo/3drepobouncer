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

#pragma once
#include <string>
#include "../repo_bouncer_global.h"
#include "../core/model/bson/repo_bson_ref.h"

namespace repo {
	namespace lib {
		static int REPO_CONFIG_FS_DEFAULT_LEVEL = 2;
		class RepoConfig
		{
		public:
			using FileStorageEngine = repo::core::model::RepoRef::RefType;
			struct database_config_t {
				std::string addr;
				int port = 27017;
				std::string connString;
				std::string username;
				std::string password;
				bool pwDigested = false;
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

			/**
			* Instantiate Repo Config with a database connection
			* @params connString connection string
			* @params username username to login with
			* @params password password to login with
			* @param pwDigested true if password given is digested.
			*/
			REPO_API_EXPORT RepoConfig(
				const std::string &connString,
				const std::string &username,
				const std::string &password,
				const bool pwDigested = false);

			/**
			* Instantiate RepoConfig given a JSON configuration file.
			* Will throw exception if file is invalid.
			*/
			static RepoConfig REPO_API_EXPORT fromFile(const std::string &filePath);

			REPO_API_EXPORT ~RepoConfig() {}

			/**
			* Set configurations to connect to use Filesystem to store files
			* @params directory directory to the file share
			* @params level number of hierachys to use
			* @params useAsDefault use this as the default storage engine
			*/
			void REPO_API_EXPORT configureFS(
				const std::string &directory,
				const int         &level = REPO_CONFIG_FS_DEFAULT_LEVEL,
				const bool useAsDefault = true
			);

			const database_config_t getDatabaseConfig() const { return dbConf; }
			const fs_config_t getFSConfig() const { return fsConf; }

			/**
			* Get default storage engine currently configured
			* @return returns the default engine of choice for new writes
			*/
			FileStorageEngine getDefaultStorageEngine() const { return defaultStorage; }

			bool validate() const;

		private:
			database_config_t dbConf;
			fs_config_t fsConf;
			FileStorageEngine defaultStorage;
		};
	}
}
