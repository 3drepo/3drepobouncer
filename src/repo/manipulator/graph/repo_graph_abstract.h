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

#include "../../core/model/bson/repo_node.h"
#include "../../core/handler/repo_database_handler_abstract.h"


namespace repo{
	namespace manipulator{
		namespace graph{
			class AbstractGraph
			{
			public:


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

				/**
				* check if Root Node exists
				* @return returns true if rootNode is not null.
				*/
				bool hasRoot() const { return (bool)rootNode; }
				repo::core::model::bson::RepoNode* getRoot() const { return rootNode; }

				/**
				* Return the number of nodes within the current 
				* graph representation
				* @return number of nodes within the graph
				*/
				uint32_t getItemsInCurrentGraph() { return nodesByUniqueID.size(); }

			protected:
				std::string databaseName;/*! name of the database */
				std::string projectName; /*! name of the project */

				repo::core::model::bson::RepoNode *rootNode;
				//! A lookup map for the all nodes the graph contains.
				std::map<repoUUID, repo::core::model::bson::RepoNode*> nodesByUniqueID;
			};
		}//namespace graph
	}//namespace manipulator
}//namespace repo
