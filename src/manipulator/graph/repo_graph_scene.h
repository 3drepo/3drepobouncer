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
* A Scene graph representation of a collection
*/

#pragma once

#include <map>
#include <boost/log/trivial.hpp>

#include "repo_graph_abstract.h"

#include "../../core/handler/repo_abstract_database_handler.h"

#include "../../core/model/bson/repo_node.h"
#include "../../core/model/bson/repo_node_camera.h"
#include "../../core/model/bson/repo_node_map.h"
#include "../../core/model/bson/repo_node_material.h"
#include "../../core/model/bson/repo_node_mesh.h"
#include "../../core/model/bson/repo_node_metadata.h"
#include "../../core/model/bson/repo_node_reference.h"
#include "../../core/model/bson/repo_node_revision.h"
#include "../../core/model/bson/repo_node_texture.h"
#include "../../core/model/bson/repo_node_transformation.h"


namespace repo{
	namespace manipulator{
		namespace graph{
			class SceneGraph : public AbstractGraph
			{
				public:
					/**
					* Default Constructor
					*/
					SceneGraph();

					/**
					* Constructor - instantiates a new scene graph representation.
					* Branch is set to master and Revision is set to head
					* Use setRevision() and setBranch()
					* to specify which revision to load
					* NOTE: This merely sets up the settings of the scene graph
					* the graph itself is not loaded until loadScene() is called!
					*
					* @param database handler (to read from/write to)
					* @param name of the database
					* @param name of the project
					* @param extension name of the scene graph (Default: "scene")
					* @param extension name of the revision graph (Default: "history")
					*/
					SceneGraph(
						repo::core::handler::AbstractDatabaseHandler *dbHandler,
						std::string database = std::string(),
						std::string projectName = std::string(),
						std::string sceneExt = REPO_COLLECTION_SCENE,
						std::string revExt = REPO_COLLECTION_HISTORY);

					/**
					* Default Deconstructor
					*/
					~SceneGraph();

					/**
					* Set project revision
					* @param uuid of the revision.
					*/
					void setRevision(repo_uuid revisionID)
					{
						headRevision = false;
						revision = revisionID;
					}


					/**
					* Set Branch
					* @param uuid of branch
					*/
					void setBranch(repo_uuid branchID){ branch = branchID; }


					/**
					* Load revision information of the configured branch/revision
					* If setProjectRevision() and setProjectBranch was never called,
					* it will load master/head.
					* @param error messages will be returned here
					* @return returns true upon success.
					*/
					bool loadRevision(std::string &errMsg);

					/**
					* Load Scene into Scene graph object base on the
					* revision/branch setting.
					* @param error messages will be returned here
					* @return returns true upon success.
					*/
					bool loadScene(std::string &errMsg);

					///**
					//* Append the given scene graph to the appointed parent
					//* @param the shared ID of the parend node this graph should append to
					//* @param the scene graph to append
					//* @param error messages will be returned here
					//* @return returns true upon success.
					//*/
					//bool append(repo_uuid parentID, SceneGraph *scene, std::string &errMsg);

					/**
					* Prints the statics of this Scene graph to an IO stream
					* This is mostly a debugging function to check the scene graph has done 
					* what you have expected.
					* @param the file stream to print the info into
					*/
					void printStatistics(std::iostream &output);


				protected:
					/**
					* populate the collections (cameras, meshes etc) with the given nodes
					* @params new nodes to add to scene graph
					* @return returns true if scene graph populated with no errors
					*/
					bool populate(std::vector<repo::core::model::bson::RepoBSON> nodes, std::string errMsg);


					/*
					* ---------------- Scene Graph settings ----------------
					*/
			
					std::string sceneExt;    /*! extension for scene   graph (Default: scene)*/
					std::string revExt;      /*! extension for history graph (Default: history)*/
					repo_uuid   revision;
					repo_uuid   branch;
					bool headRevision;

					repo::core::model::bson::RevisionNode		 *revNode;

					/*
					* ---------------- Scene Graph Details ----------------
					*/

					repo::core::model::bson::RepoNodeSet cameras; //!< Cameras
					repo::core::model::bson::RepoNodeSet meshes; //!< Meshes
					repo::core::model::bson::RepoNodeSet materials; //!< Materials
					repo::core::model::bson::RepoNodeSet maps; //!< Maps
					repo::core::model::bson::RepoNodeSet metadata; //!< Metadata
					repo::core::model::bson::RepoNodeSet references; //!< References
					repo::core::model::bson::RepoNodeSet textures; //!< Textures
					repo::core::model::bson::RepoNodeSet transformations; //!< Transformations
					repo::core::model::bson::RepoNodeSet unknowns; //!< Unknown types

					std::map<repo_uuid, repo_uuid> sharedIDtoUniqueID; //** mapping of shared ID to Unique ID
					std::map<repo_uuid, std::vector<repo_uuid>> parentToChildren; //** mapping of shared id to its children's shared id
					std::map<repo_uuid, SceneGraph> referenceToScene; //** mapping of reference ID to it's scene graph



			};
		}//namespace graph
	}//namespace manipulator
}//namespace repo


