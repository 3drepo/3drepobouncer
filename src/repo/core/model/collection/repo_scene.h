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

#include "repo_graph_abstract.h"

#include "../../handler/repo_database_handler_abstract.h"

#include "../bson/repo_node.h"
#include "../bson/repo_node_camera.h"
#include "../bson/repo_node_map.h"
#include "../bson/repo_node_material.h"
#include "../bson/repo_node_mesh.h"
#include "../bson/repo_node_metadata.h"
#include "../bson/repo_node_reference.h"
#include "../bson/repo_node_revision.h"
#include "../bson/repo_node_texture.h"
#include "../bson/repo_node_transformation.h"


namespace repo{
	namespace core{
		namespace model{
			class REPO_API_EXPORT RepoScene : public AbstractGraph
			{

				//FIXME: unsure as to whether i should make the graph a differen class.. struct for now.
				struct repoGraphInstance
				{

					RepoNodeSet cameras; //!< Cameras
					RepoNodeSet meshes; //!< Meshes
					RepoNodeSet materials; //!< Materials
					RepoNodeSet maps; //!< Maps
					RepoNodeSet metadata; //!< Metadata
					RepoNodeSet references; //!< References
					RepoNodeSet textures; //!< Textures
					RepoNodeSet transformations; //!< Transformations
					RepoNodeSet unknowns; //!< Unknown types

					RepoNode *rootNode;
					//! A lookup map for the all nodes the graph contains.
					std::map<repoUUID, RepoNode*> nodesByUniqueID;
					std::map<repoUUID, repoUUID> sharedIDtoUniqueID; //** mapping of shared ID to Unique ID
					std::map<repoUUID, std::vector<repoUUID>> parentToChildren; //** mapping of shared id to its children's shared id
					std::map<repoUUID, RepoScene*> referenceToScene; //** mapping of reference ID to it's scene graph
				};

				public:

					/**
					* Used for loading scene graphs from database
					* Constructor - instantiates a new scene graph representation.
					* Branch is set to master and Revision is set to head
					* Use setRevision() and setBranch()
					* to specify which revision to load
					* NOTE: This merely sets up the settings of the scene graph
					* the graph itself is not loaded until loadScene() is called!
					*
					* @param database name  of the database
					* @param projectName name of the project
					* @param sceneExt extension name of the scene graph (Default: "scene")
					* @param revExt extension name of the revision graph (Default: "history")
					*/
					RepoScene(
						const std::string                                  &database = std::string(),
						const std::string                                  &projectName = std::string(),
						const std::string                                  &sceneExt = REPO_COLLECTION_SCENE,
						const std::string                                  &revExt = REPO_COLLECTION_HISTORY);

					/**
					* Used for constructing scene graphs from model convertors
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
					RepoScene(
						const RepoNodeSet &cameras, 
						const RepoNodeSet &meshes, 
						const RepoNodeSet &materials, 
						const RepoNodeSet &metadata, 
						const RepoNodeSet &textures, 
						const RepoNodeSet &transformations,
						const RepoNodeSet &references = RepoNodeSet(),
						const RepoNodeSet &maps = RepoNodeSet(),
						const RepoNodeSet &unknowns = RepoNodeSet(),
						const std::string                          &sceneExt = REPO_COLLECTION_SCENE,
						const std::string                          &revExt = REPO_COLLECTION_HISTORY);

					/**
					* Default Deconstructor
					* Please note that destroying the scene graph will also destroy
					* all nodes created by the scene graph
					* and also its child graphs
					*/
					~RepoScene();


					/**
					* Add metadata that has a matching name as the transformation into the scene
					* @param metadata set of metadata to attach
					* @param exactMatch whether the name has to be an exact match or a substring will do
					*/
					void addMetadata(
						RepoNodeSet &metadata,
						const bool        &exactMatch);

					/**
					* Commit changes into the database
					* This commits an update to project settings, a new revision node
					* and the changes within this scene
					* @param errMsg error message if this failed
					* @param userName name of the author
					* @param message message for this commit (optional)
					* @param tag tag for this commit (optional)
					* @return returns true upon success
					*/
					bool commit(
						repo::core::handler::AbstractDatabaseHandler *handler,
						std::string &errMsg,
						const std::string &userName,
						const std::string &message=std::string(),
						const std::string &tag=std::string());

					/**
					* Get name of the database
					* @return returns name of the database if available
					*/
					std::string getDatabaseName()
					{
						return databaseName;
					}

					/**
					* Get name of the project
					* @return returns name of the project if available
					*/
					std::string getProjectName()
					{
						return projectName;
					}

					/**
					* Set project and database names. This will overwrite the current ones
					* @param newDatabaseName new name of database.
					* @param newProjectMame new name of the project
					*/
					void setDatabaseAndProjectName(std::string newDatabaseName, std::string newProjectName)
					{
						databaseName = newDatabaseName;
						projectName = newProjectName;
					}

					/**
					* Set project revision
					* @param uuid of the revision.
					*/
					void setRevision(repoUUID revisionID)
					{
						headRevision = false;
						revision = revisionID;
					}
					
					/**
					* Set Branch
					* @param uuid of branch
					*/
					void setBranch(repoUUID branchID){ branch = branchID; }

					/**
					* Set commit message
					* @param msg message to go with the commit
					*/
					void setCommitMessage(const std::string &msg) { commitMsg = msg; }

					/**
					* Get branch name return uuid of branch there is no name
					* return "master" if this scene is not revisioned.
					* @return name of the branch or uuid if no name
					*/
					std::string getBranchName() const;


					/**
					* Load revision information of the configured branch/revision
					* If setProjectRevision() and setProjectBranch was never called,
					* it will load master/head.
					* @param handler database handler to load from
					* @param error messages will be returned here
					* @return returns true upon success.
					*/
					bool loadRevision(
						repo::core::handler::AbstractDatabaseHandler *handler,
						std::string &errMsg);

					/**
					* Load Scene into Scene graph object base on the
					* revision/branch setting.
					* @param error messages will be returned here
					* @return returns true upon success.
					*/
					bool loadScene(
						repo::core::handler::AbstractDatabaseHandler *handler, 
						std::string &errMsg);

					/**
					* Prints the statics of this Scene graph to an IO stream
					* This is mostly a debugging function to check the scene graph has done 
					* what you have expected.
					* @param the file stream to print the info into
					*/
					void printStatistics(std::iostream &output);

					/**
					* --------------------- Node Relationship ----------------------
					*/

					/**
					* Get children nodes of a specified parent
					* @param parent shared UUID of the parent node
					* @ return a vector of pointers to children node (potentially none)
					*/
					std::vector<RepoNode*>
						getChildrenAsNodes(const repoUUID &parent) const
					{
						return getChildrenAsNodes(graph, parent);
					}

					/**
					* Get children nodes of a specified parent
					* @param g graph to retrieve from
					* @param parent shared UUID of the parent node
					* @ return a vector of pointers to children node (potentially none)
					*/
					std::vector<RepoNode*>
						RepoScene::getChildrenAsNodes(
						const repoGraphInstance &g,
						const repoUUID &parent) const;

					/**
					* Get Scene from reference node
					*/
					RepoScene* getSceneFromReference(const repoUUID &reference) const
					{
						repoGraphInstance g = queryStashGraph ? stashGraph : graph;
						RepoScene* refScene = nullptr;

						std::map<repoUUID, RepoScene*>::const_iterator it = g.referenceToScene.find(reference);
						if (it != g.referenceToScene.end())
							refScene = it->second;
						return refScene;
					}

					/**
					* --------------------- Scene Lookup ----------------------
					*/


					/**
					* Get all camera nodes within current scene revision
					* @return a RepoNodeSet of materials
					*/
					RepoNodeSet getAllCameras() const
					{
						return queryStashGraph? stashGraph.cameras:  graph.cameras;
					}

					/**
					* Get all material nodes within current scene revision
					* @return a RepoNodeSet of materials
					*/
					RepoNodeSet getAllMaterials() const
					{
						return queryStashGraph ? stashGraph.materials : graph.materials;
					}

					/**
					* Get all mesh nodes within current scene revision
					* @return a RepoNodeSet of meshes
					*/
					RepoNodeSet getAllMeshes() const
					{
						return queryStashGraph ? stashGraph.meshes : graph.meshes;
					}

					/**
					* Get all texture nodes within current scene revision
					* @return a RepoNodeSet of textures
					*/
					RepoNodeSet getAllTextures() const
					{
						return queryStashGraph ? stashGraph.textures : graph.textures;
					}

					/**
					* Get all ID of nodes which are modified since last revision
					* @return returns a vector of node IDs
					*/
					std::vector<repoUUID> getModifiedNodesID() const;

					/**
					* Get the node given the shared ID of this node
					* @param sharedID shared ID of the node
					* @return returns a pointer to the node if found.
					*/
					RepoNode* getNodeBySharedID(
						const repoUUID &sharedID) const
					{

						return getNodeBySharedID(graph, sharedID);
					}

					/**
					* Get the node given the shared ID of this node
					* @param g instance of the graph to search from
					* @param sharedID shared ID of the node
					* @return returns a pointer to the node if found.
					*/
					RepoNode* getNodeBySharedID(
						const repoGraphInstance &g,
						const repoUUID &sharedID) const
					{

						auto it = g.sharedIDtoUniqueID.find(sharedID);

						if (it == g.sharedIDtoUniqueID.end()) return nullptr;

						return g.nodesByUniqueID.at(it->second);
					}

					/**
					* check if Root Node exists
					* @return returns true if rootNode is not null.
					*/
					bool hasRoot() const { 
						repoGraphInstance g = queryStashGraph ? stashGraph : graph; 
						return (bool)g.rootNode;
					}
					RepoNode* getRoot() const { 
						repoGraphInstance g = queryStashGraph ? stashGraph : graph;
						return g.rootNode;
					}

					/**
					* Return the number of nodes within the current
					* graph representation
					* @return number of nodes within the graph
					*/
					uint32_t getItemsInCurrentGraph() { 
						repoGraphInstance g = queryStashGraph ? stashGraph : graph; 
						return g.nodesByUniqueID.size();
					}

					/**
					* -------------- Scene Modification Functions --------------
					*/

					/**
					* Add a vector of nodes into the scene graph
					* Tracks the modification for commit
					* @param vector of nodes to insert.
					*/
					void addNodes(std::vector<RepoNode *> nodes);

					/**
					* Modify a node with the information within the new node.
					* This will also update revision related information within 
					* the scene (if necessary)
					* @param sharedID of the node to modify
					* @param node modified version node
					* @param overwrite if true, overwrite the node with modified version, 
					*        otherwise just update with its contents (default: false)
					*/

					void modifyNode(
						const repoUUID                    &sharedID,
						RepoNode *node,
						const bool                        &overwrite=false);

					/**
					* ----------------------------------------------------------
					*/

				protected:
					/**
					* Add Nodes to scene.
					* @param node pointer to the node to add
					* @param errMsg error message if it returns false
					* @param collection the collection for the node type if known.
					* @return returns true if succeeded
					*/
					bool addNodeToScene(
						const RepoNodeSet nodes,
						std::string &errMsg,
						RepoNodeSet *collection
					);
					/**
					* Add node to the following maps: UniqueID -> Node, SharedID -> UniqueID,
					* Parent->Children.
					* It will also assign root node if the node has no parent.
					* @param node pointer to the node to add
					* @param errMsg error message if it returns false
					* @return returns true if succeeded
					*/
					bool addNodeToMaps(RepoNode *node, std::string &errMsg)
					{
						//add to unoptimised graph by default
						return addNodeToMaps(graph, node, errMsg);
					}

					/**
					* Add node to the following maps: UniqueID -> Node, SharedID -> UniqueID,
					* Parent->Children.
					* It will also assign root node if the node has no parent.
					* @param g which graph to insert into
					* @param node pointer to the node to add
					* @param errMsg error message if it returns false
					* @return returns true if succeeded
					*/
					bool addNodeToMaps(
						repoGraphInstance &g, 
						RepoNode *node, 
						std::string &errMsg);
					
					/**
					* Commit a project settings base on the
					* changes on this scene
					* @param errMsg error message if this failed
					* @param userName user name of the owner
					* @return returns true upon success
					*/
					bool commitProjectSettings(
						repo::core::handler::AbstractDatabaseHandler *handler,
						std::string &errMsg,
						const std::string &userName);

					/**
					* Commit a revision node into project.revExt base on the
					* changes on this scene
					* @param errMsg error message if this failed
					* @param newRevNode a reference to the revNode that will be created in this function
					* @param userName name of the author
					* @param message message describing this commit
					* @param tag tag for this commit
					* @return returns true upon success
					*/
					bool commitRevisionNode(
						repo::core::handler::AbstractDatabaseHandler *handler,
						std::string &errMsg,
						RevisionNode *&newRevNode,
						const std::string &userName,
						const std::string &message,
						const std::string &tag);

					/**
					* Commit a new nodes into project.sceneExt base on the
					* changes on this scene
					* @param errMsg error message if this failed
					* @return returns true upon success
					*/
					bool commitSceneChanges(
						repo::core::handler::AbstractDatabaseHandler *handler,
						std::string &errMsg);

					/**
					* populate the collections (cameras, meshes etc) with the given nodes
					* @params new nodes to add to scene graph
					* @return returns true if scene graph populated with no errors
					*/
					bool populate(
						repo::core::handler::AbstractDatabaseHandler *handler, 
						std::vector<RepoBSON> nodes, 
						std::string &errMsg);

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
					void populateAndUpdate(
						const RepoNodeSet &cameras,
						const RepoNodeSet &meshes,
						const RepoNodeSet &materials,
						const RepoNodeSet &metadata,
						const RepoNodeSet &textures,
						const RepoNodeSet &transformations,
						const RepoNodeSet &references,
						const RepoNodeSet &maps,
						const RepoNodeSet &unknowns)
					{
						//populate the non optimised graph by default
						populateAndUpdate(graph, cameras, meshes, materials, metadata,
							textures, transformations, references, maps, unknowns);
					}

					/**
					* Populate the collections with the given node sets
					* This populates the scene graph information and also track the nodes that are added.
					* i.e. this assumes the nodes did not exist in the previous revision (if any)
					* @param instance specify which graph (graph or stash) to insert into
					* @param cameras Repo Node set of cameras
					* @param meshes  Repo Node set of meshes
					* @param materials Repo Node set of materials
					* @param metadata Repo Node set of metadata
					* @param textures Repo Node set of textures
					* @param transformations Repo Node set of transformations
					* @return returns true if scene graph populated with no errors
					*/
					void populateAndUpdate(
						repoGraphInstance &instance,
						const RepoNodeSet &cameras,
						const RepoNodeSet &meshes,
						const RepoNodeSet &materials,
						const RepoNodeSet &metadata,
						const RepoNodeSet &textures,
						const RepoNodeSet &transformations,
						const RepoNodeSet &references,
						const RepoNodeSet &maps,
						const RepoNodeSet &unknowns);


					/*
					* ---------------- Scene Graph settings ----------------
					*/
			
					std::string sceneExt;    /*! extension for scene   graph (Default: scene)*/
					std::string revExt;      /*! extension for history graph (Default: history)*/
					repoUUID   revision;
					repoUUID   branch;
					std::string commitMsg;
					bool headRevision;
					bool unRevisioned;       /*! Flag to indicate if the scene graph is revisioned (true for scene graphs from model convertor)*/

					RevisionNode		 *revNode;

					/*
					* ---------------- Scene Graph Details ----------------
					*/


					//Change trackers
					std::set<repoUUID> newCurrent; //new list of current (unique IDs)
					std::set<repoUUID> newAdded; //list of nodes added to the new revision (shared ID)
					std::set<repoUUID> newRemoved; //list of nodes removed for this revision (shared ID)
					std::set<repoUUID> newModified; // list of nodes modified during this revision  (shared ID)

					repoGraphInstance graph; //current state of the graph, given the branch/revision
					repoGraphInstance stashGraph; //current state of the optimized graph, given the branch/revision

					//FIXME: this is to stop everything else from breaking for now. may want to remove this and
					// patch everything outside in the future
					bool queryStashGraph; //if true, all generic queries query stashed version of the graph
			};
		}//namespace graph
	}//namespace manipulator
}//namespace repo


