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
* Abstract graph for common utilities
*/

#pragma once
#include <string>
#include <vector>

#include "../bson/repo_node.h"
#include "../../handler/repo_database_handler_abstract.h"

namespace repo{
	namespace core{
		namespace model{
			class AbstractGraph
			{
			public:

				//FIXME: this has hardly any reason to exist. Consider removing
				/**
				* Constructor - instantiates a new abstract graph with settings
				*
				* @param dbHandler database handler (to read from/write to)
				* @param databaseName name of the database
				* @param projectName name of the project
				*/
				AbstractGraph(
					const std::string                                  &databaseName = std::string(),
					const std::string                                  &projectName = std::string());

				/**
				* Default Deconstructor
				*/
				virtual ~AbstractGraph();

			protected:
				std::string databaseName;/*! name of the database */
				std::string projectName; /*! name of the project */
			};
		}//namespace graph
	}//namespace manipulator
}//namespace repo
