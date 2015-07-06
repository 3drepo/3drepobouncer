/**
*  Copyright (C) 2014 3D Repo Ltd
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
* A Repo project (a project consists of a scene graph and a revision graph)
*/


#pragma once
#include <string>

#include <boost/log/trivial.hpp>

#include "../core/model/repo_model_global.h"
#include "../core/model/repo_node_utils.h"
#include "../core/model/bson/repo_bson.h"
#include "../core/model/bson/repo_node_revision.h"
#include "../core/handler/repo_abstract_database_handler.h"

#include "graphs/repo_graph_revision.h"
#include "graphs/repo_graph_scene.h"


namespace repo{
	namespace manipulator{
		class RepoProject
		{
			public:
				/** 
				 * Constructor - instantiates a new project representation.
				 * Branch is set to master and Revision is set to head
				 * Use setProjectRevision() and setProjectBranch
				 * to specify which revision to load
				 *
				 * @param database handler (to read from/write to)
				 * @param name of the database
				 * @param name of the project
				 * @param extension name of the scene graph (Default: "scene")
				 * @param extension name of the revision graph (Default: "history")
				 */
				RepoProject(
					repo::core::handler::AbstractDatabaseHandler *dbHandler,
					std::string database    = std::string(),
					std::string projectName = std::string(),
					std::string sceneExt    = REPO_COLLECTION_SCENE,
					std::string revExt      = REPO_COLLECTION_HISTORY);
			
				/**
				* Default Deconstructor
				*/
				~RepoProject();
			

				/**
				* Set project revision 
				* @param uuid of the revision.
				*/
				void setProjectRevision(repo_uuid revisionID)
				{
					headRevision = false;
					revision     = revisionID;
				}


				/**
				* Set Branch
				* @param uuid of branch
				*/
				void setProjectBranch(repo_uuid branchID){ branch = branchID; }


				/**
				* Load revision information of the configured branch/revision
				* If setProjectRevision() and setProjectBranch was never called,
				* it will load master/head.
				*/
				bool loadRevision(std::string &errMsg);

				/**
				* Load Scene into Scene graph object base on the 
				* revision/branch setting. 
				*/
				void loadScene();


			private:
				std::string databaseName;/*! name of the database */
				std::string projectName; /*! name of the project */
				std::string sceneExt;    /*! extension for scene   graph (Default: scene)*/
				std::string revExt;      /*! extension for history graph (Default: history)*/
				repo_uuid   revision;
				repo_uuid   branch;
				

				bool headRevision;

				repo::core::handler::AbstractDatabaseHandler *dbHandler; /*! handler to use for db operations*/
				repo::manipulator::graph::SceneGraph		 *sceneGraph;
				repo::core::model::bson::RevisionNode		 *revNode;
		};
	}//namespace manipulator
}//namespace repo
