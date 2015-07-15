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
* A Scene graph representation of a collection
*/

#pragma once

#include <map>
#include <boost/log/trivial.hpp>

#include "repo_graph_abstract.h"

#include "../../core/handler/repo_database_handler_abstract.h"

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
					* Used for loading scene graphs from database
					* Constructor - instantiates a new scene graph representation.
					* Branch is set to master and Revision is set to head
					* Use setRevision() and setBranch()
					* to specify which revision to load
					* NOTE: This merely sets up the settings of the scene graph
					* the graph itself is not loaded until loadScene() is called!
					*
					* @param dbHandler database handler (to read from/write to)
					* @param database name  of the database
					* @param projectName name of the project
					* @param sceneExt extension name of the scene graph (Default: "scene")
					* @param revExt extension name of the revision graph (Default: "history")
					*/
					SceneGraph(
						repo::core::handler::AbstractDatabaseHandler *dbHandler,
						const std::string                                  &database = std::string(),
						const std::string                                  &projectName = std::string(),
						const std::string                                  &sceneExt = REPO_COLLECTION_SCENE,
						const std::string                                  &revExt = REPO_COLLECTION_HISTORY);

					/**
					* Used for constructing scene graphs from model creators
					* Constructor - instantiates a new scene graph representation.
					* This scene will be flagged as not revisioned. A database handler
					* will need to be set before it can be commited into the database
					* NOTE: database handler, project name, database name will need to be 
					* set before this scene graph can be commited to the database.
					* @param cameras Repo Node set of cameras
					* @param meshes  Repo Node set of meshes
					* @param materials Repo Node set of materials
					* @param metadata Repo Node set of metadata
					* @param textures Repo Node set of textures
					* @param transformations Repo Node set of transformations
					* @param references Repo Node set of references (optional)
					* @param maps Repo Node set of maps (optional)
					* @param unknowns Repo Node set of unknowns (optional)
					* @param sceneExt extension name of the scene when it is saved into the database (optional)
					* @param revExt   extension name of the revision when it is saved into the database (optional)
					*/
					SceneGraph(
						const repo::core::model::bson::RepoNodeSet &cameras, 
						const repo::core::model::bson::RepoNodeSet &meshes, 
						const repo::core::model::bson::RepoNodeSet &materials, 
						const repo::core::model::bson::RepoNodeSet &metadata, 
						const repo::core::model::bson::RepoNodeSet &textures, 
						const repo::core::model::bson::RepoNodeSet &transformations,
						const repo::core::model::bson::RepoNodeSet &references = repo::core::model::bson::RepoNodeSet(),
						const repo::core::model::bson::RepoNodeSet &maps = repo::core::model::bson::RepoNodeSet(),
						const repo::core::model::bson::RepoNodeSet &unknowns = repo::core::model::bson::RepoNodeSet(),
						const std::string                          &sceneExt = REPO_COLLECTION_SCENE,
						const std::string                          &revExt = REPO_COLLECTION_HISTORY);

					/**
					* Default Deconstructor
					* Please note that destroying the scene graph will also destroy
					* all nodes created by the scene graph
					* and also its child graphs
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

					/**
					* Prints the statics of this Scene graph to an IO stream
					* This is mostly a debugging function to check the scene graph has done 
					* what you have expected.
					* @param the file stream to print the info into
					*/
					void printStatistics(std::iostream &output);


					/**
					* -------------- Scene Modification Functions --------------
					*/

					/**
					* Add a vector of nodes into the scene graph
					* Tracks the modification for commit
					* @param vector of nodes to insert.
					*/
					void addNodes(std::vector<repo::core::model::bson::RepoNode *> nodes);

					/**
					* ----------------------------------------------------------
					*/


				protected:
					/**
					* Add Nodes to scene. If
					* @param node pointer to the node to add
					* @param errMsg error message if it returns false
					* @param collection the collection for the node type if known.
					* @return returns true if succeeded
					*/
					bool SceneGraph::addNodeToScene(
						const repo::core::model::bson::RepoNodeSet nodes,
						std::string &errMsg,
						repo::core::model::bson::RepoNodeSet *collection
					);
					/**
					* Add node to the following maps: UniqueID -> Node, SharedID -> UniqueID,
					* Parent->Children.
					* It will also assign root node if the node has no parent.
					* @param node pointer to the node to add
					* @param errMsg error message if it returns false
					* @return returns true if succeeded
					*/
					bool SceneGraph::addNodeToMaps(repo::core::model::bson::RepoNode *node, std::string &errMsg);
					
					/**
					* populate the collections (cameras, meshes etc) with the given nodes
					* @params new nodes to add to scene graph
					* @return returns true if scene graph populated with no errors
					*/
					bool populate(std::vector<repo::core::model::bson::RepoBSON> nodes, std::string errMsg);

					/**
					* Populate the collections with the given node sets
					* This populates the scene graph information and also track the nodes that are added.
					* i.e. this assumes the nodes did not exist in the previous revision (if any)
					* @param cameras Repo Node set of cameras
					* @param meshes  Repo Node set of meshes
					* @param materials Repo Node set of materials
					* @param metadata Repo Node set of metadata
					* @param textures Repo Node set of textures
					* @param transformations Repo Node set of transformations
					* @return returns true if scene graph populated with no errors
					*/
					void SceneGraph::populateAndUpdate(
						const repo::core::model::bson::RepoNodeSet &cameras,
						const repo::core::model::bson::RepoNodeSet &meshes,
						const repo::core::model::bson::RepoNodeSet &materials,
						const repo::core::model::bson::RepoNodeSet &metadata,
						const repo::core::model::bson::RepoNodeSet &textures,
						const repo::core::model::bson::RepoNodeSet &transformations,
						const repo::core::model::bson::RepoNodeSet &references,
						const repo::core::model::bson::RepoNodeSet &maps,
						const repo::core::model::bson::RepoNodeSet &unknowns);


					/*
					* ---------------- Scene Graph settings ----------------
					*/
			
					std::string sceneExt;    /*! extension for scene   graph (Default: scene)*/
					std::string revExt;      /*! extension for history graph (Default: history)*/
					repo_uuid   revision;
					repo_uuid   branch;
					bool headRevision;
					bool unRevisioned;       /*! Flag to indicate if the scene graph is revisioned (true for scene graphs from model creator)*/

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

					//Change trackers
					std::vector<repo_uuid> new_current; //new list of current (unique IDs)
					std::vector<repo_uuid> new_added; //list of nodes added to the new revision (shared ID)
					std::vector<repo_uuid> new_removed; //list of nodes removed for this revision (shared ID)
					std::vector<repo_uuid> new_modified; // list of nodes modified during this revision  (shared ID)



			};
		}//namespace graph
	}//namespace manipulator
}//namespace repo


