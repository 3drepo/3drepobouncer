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
		namespace modelutility{
			class SceneCleaner
			{
			public:
				/**
				* Create a scene cleaner
				* cleans up any incomplete scene information within the database
				* @params dbName database name
				* @param projectName project name
				* @param handler database handler to the database
				*/
				SceneCleaner(
					const std::string                            &dbName,
					const std::string                            &projectName,
					repo::core::handler::AbstractDatabaseHandler *handler
					);
				~SceneCleaner();

				bool execute();

			private:
				const std::string dbName, projectName;
				repo::core::handler::AbstractDatabaseHandler *handler;

				/**
				* Remove the revision given from the database
				* @param revNode revision bson
				*/
				bool cleanUpRevision(
					const repo::core::model::RevisionNode &revNode);

				/**
				* Get a list of incomplete revision from the database
				* incomplete revisions contains a flag "incomplete" within the bson
				*/
				std::vector<repo::core::model::RepoBSON> getIncompleteRevisions();
			};
		}
	}
}
