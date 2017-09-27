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
#include "../../core/model/bson/repo_node_revision.h"
#include "../../core/handler/repo_database_handler_abstract.h"

namespace repo{
	namespace manipulator{
		class StatisticsGenerator
		{
		public:
			/**
			* Create a scene cleaner
			* cleans up any incomplete scene information within the database
			* @params dbName database name
			* @param projectName project name
			* @param handler database handler to the database
			*/
			StatisticsGenerator(
				repo::core::handler::AbstractDatabaseHandler *handler
				);
			~StatisticsGenerator();

			/**
			* Get a list of users and print the result in the given filepath
			* @params outputFilePath
			*/
			void getUserList(const std::string &outputFilePath);

			/**
			* Generate database statistics and print the result in the given filepath
			* @params outputFilePath
			*/
			void getDatabaseStatistics(const std::string &outputFilePath);

		private:
			repo::core::handler::AbstractDatabaseHandler *handler;
		};
	}
}
