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
					* DEFAULT - The graph that is revisioned - should be (close to) identical to original model
					* OPTIMIZED - The optimized version of the graph - for quick visualization purposes
					*/
					enum class GraphType { DEFAULT, OPTIMIZED};

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
					* @param stashExt extension name of the stash (Default: "stash.3drepo")
					*/
					RepoScene(
						const std::string                                  &database = std::string(),
						const std::string                                  &projectName = std::string(),
						const std::string                                  &sceneExt = REPO_COLLECTION_SCENE,
						const std::string                                  &revExt = REPO_COLLECTION_HISTORY,
						const std::string                                  &stashExt = REPO_COLLECTION_REPOSTASH,
						const std::string                                  &rawExt = REPO_COLLECTION_RAW);

					/**
					* Used for constructing scene graphs from model convertors
					* Constructor - instantiates a new scene graph representation.
					* This scene will be flagged as not revisioned. A database handler
					* will need to be set before it can be commited into the database
					* NOTE: database handler, project name, database name will need to be 
					* set before this scene graph can be commited to the database.
					* @param refFiles vector of files created this scene
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
					* @param stashExt extension name of the stash (Default: "stash.3drepo")
					*/
					RepoScene(
						const std::vector<std::string> &refFiles,
						const RepoNodeSet              &cameras, 
						const RepoNodeSet              &meshes, 
						const RepoNodeSet              &materials, 
						const RepoNodeSet              &metadata, 
						const RepoNodeSet              &textures, 
						const RepoNodeSet              &transformations,
						const RepoNodeSet              &references = RepoNodeSet(),
						const RepoNodeSet              &maps = RepoNodeSet(),
						const RepoNodeSet              &unknowns = RepoNodeSet(),
						const std::string              &sceneExt = REPO_COLLECTION_SCENE,
						const std::string              &revExt = REPO_COLLECTION_HISTORY,
						const std::string              &stashExt = REPO_COLLECTION_REPOSTASH,
						const std::string              &rawExt = REPO_COLLECTION_RAW);

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
					* Add a stash graph (optimised graph) for the corresponding scene graph
					* @param camera set of cameras
					* @param meshes set of meshes
					* @param materials set of materials
					* @param textures set of textures
					* @param transformations set of transformations
					*/
					void addStashGraph(
						const RepoNodeSet &cameras,
						const RepoNodeSet &meshes,
						const RepoNodeSet &materials,
						const RepoNodeSet &textures,
						const RepoNodeSet &transformations);

					/**
					* Clears the contents within the Stash (if there is one)
					*/
					void clearStash();

					/**
					* Commit changes on the default graph into the database
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
					* Commit the stash representation into the database
					* This can only happen if there is a stash representation
					* and the graph is already commited (i.e. there is a revision node)
					* @param errMsg error message if this failed
					* @return returns true upon success
					*/
					bool commitStash(
						repo::core::handler::AbstractDatabaseHandler *handler,
						std::string &errMsg);

					/**
					* Get name of the database
					* @return returns name of the database if available
					*/
					std::string getDatabaseName() const
					{
						return databaseName;
					}

					/**
					* Get raw extension for this project
					* @return returns the raw extension 
					*/
					std::string getRawExtension() const
					{
						return rawExt;
					}

					/**
					* Get name of the project
					* @return returns name of the project if available
					*/
					std::string getProjectName() const
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
					* @param handler database handler to perform this action
					* @param errMsg message if it failed
					* @return return true upon success
					*/
					bool loadRevision(
						repo::core::handler::AbstractDatabaseHandler *handler,
						std::string &errMsg);

					/**
					* Load Scene into Scene graph object base on the
					* revision/branch setting.
					* @param handler database handler to perform this action
					* @param errMsg message if it failed
					* @return return true upon success
					*/
					bool loadScene(
						repo::core::handler::AbstractDatabaseHandler *handler, 
						std::string &errMsg);

					/**
					* Load stash into scene base on the revision setting
					* @param handler database handler to perform this action
					* @param errMsg message if it failed
					* @return return true upon success
					*/
					bool loadStash(
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
					* Introduce parentship to 2 nodes that already reside within the scene.
					* If either of the nodes are not found, it does nothing
					* If they already share an inheritance, it does nothing
					* @param gType which graph are the nodes
					* @param parent UNIQUE uuid of the parent
					* @param child UNIQUE uuid of the child
					* @param noUpdate if true, it will not be treated as 
					*        a change that is needed to be commited (only valid for default graph)
					*/
					void addInheritance(
						const GraphType &gType,
						const repoUUID  &parent,
						const repoUUID  &child,
						const bool      &noUpdate = false);

					/**
					* Get children nodes of a specified parent
					* @param parent shared UUID of the parent node
					* @ return a vector of pointers to children node (potentially none)
					*/
					std::vector<RepoNode*>
						getChildrenAsNodes(const repoUUID &parent) const
					{
						return getChildrenAsNodes(GraphType::DEFAULT, parent);
					}

					/**
					* Get children nodes of a specified parent
					* @param g graph to retrieve from
					* @param parent shared UUID of the parent node
					* @ return a vector of pointers to children node (potentially none)
					*/
					std::vector<RepoNode*>
						getChildrenAsNodes(
						const GraphType &g,
						const repoUUID &parent) const;

					/**
					* Get Scene from reference node
					*/
					RepoScene* getSceneFromReference(
						const GraphType &gType, 
						const repoUUID  &reference) const
					{
						repoGraphInstance g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
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
					RepoNodeSet getAllCameras(
						const GraphType &gType = GraphType::DEFAULT) const
					{
						return  gType == GraphType::OPTIMIZED ? stashGraph.cameras : graph.cameras;
					}

					/**
					* Get all material nodes within current scene revision
					* @return a RepoNodeSet of materials
					*/
					RepoNodeSet getAllMaterials(
						const GraphType &gType = GraphType::DEFAULT) const
					{
						return  gType == GraphType::OPTIMIZED ? stashGraph.materials : graph.materials;
					}

					/**
					* Get all mesh nodes within current scene revision
					* @return a RepoNodeSet of meshes
					*/
					RepoNodeSet getAllMeshes(
						const GraphType &gType = GraphType::DEFAULT) const
					{
						return  gType == GraphType::OPTIMIZED ? stashGraph.meshes : graph.meshes;
					}

					/**
					* Get all texture nodes within current scene revision
					* @return a RepoNodeSet of textures
					*/
					RepoNodeSet getAllTextures(
						const GraphType &gType = GraphType::DEFAULT) const
					{
						return  gType == GraphType::OPTIMIZED ? stashGraph.textures : graph.textures;
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

						return getNodeBySharedID(GraphType::DEFAULT, sharedID);
					}

					/**
					* Get the node given the shared ID of this node
					* @param g instance of the graph to search from
					* @param sharedID shared ID of the node
					* @return returns a pointer to the node if found.
					*/
					RepoNode* getNodeBySharedID(
						const GraphType &gType,
						const repoUUID &sharedID) const
					{
						repoGraphInstance g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
						auto it = g.sharedIDtoUniqueID.find(sharedID);

						if (it == g.sharedIDtoUniqueID.end()) return nullptr;

						return g.nodesByUniqueID.at(it->second);
					}

					/**
					* Get the node given the Unique ID of this node
					* @param g instance of the graph to search from
					* @param sharedID Unique ID of the node
					* @return returns a pointer to the node if found.
					*/
					RepoNode* getNodeByUniqueID(
						const GraphType &gType,
						const repoUUID &uniqueID) const
					{
						repoGraphInstance g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
						auto it = g.nodesByUniqueID.find(uniqueID);

						if (it == g.nodesByUniqueID.end()) return nullptr;

						return it->second;
					}

					/**
					* check if Root Node exists
					* @return returns true if rootNode is not null.
					*/
					bool hasRoot(const GraphType &gType = GraphType::DEFAULT) const {
						repoGraphInstance g = gType == GraphType::OPTIMIZED ? stashGraph : graph; 
						return (bool)g.rootNode;
					}
					RepoNode* getRoot(const GraphType &gType = GraphType::DEFAULT) const {
						repoGraphInstance g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
						return g.rootNode;
					}

					/**
					* Return the number of nodes within the current
					* graph representation
					* @return number of nodes within the graph
					*/
					uint32_t getItemsInCurrentGraph(const GraphType &gType = GraphType::DEFAULT) {
						repoGraphInstance g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
						return g.nodesByUniqueID.size();
					}

					/**
					* Get the file names of the original files that are stored within the database
					* @return returns a vector of file names
					*/
					std::vector<std::string> getOriginalFiles() const;

					/**
					* -------------- Scene Modification Functions --------------
					*/

					/**
					* Add a vector of nodes into the unoptimized scene graph 
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
					* @param gType which graph to add the nodes onto (default or optimized)
					* @param node pointer to the node to add
					* @param errMsg error message if it returns false
					* @param collection the collection for the node type if known.
					* @return returns true if succeeded
					*/
					bool addNodeToScene(
						const GraphType &gType,
						const RepoNodeSet nodes,
						std::string &errMsg,
						RepoNodeSet *collection);

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
						return addNodeToMaps(GraphType::DEFAULT, node, errMsg);
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
						const GraphType &gType,
						RepoNode *node, 
						std::string &errMsg);
					
					/**
					* Commit a vector of nodes into the database
					* @param handler database handler to perform the commit
					* @param nodesToCommit vector of uuids of nodes to commit
					* @param graphType which graph did the nodes come from
					* @param errMsg error message if this failed
					* @return returns true upon success
					*/
					bool commitNodes(
						repo::core::handler::AbstractDatabaseHandler *handler,
						const std::vector<repoUUID> &nodesToCommit,
						const GraphType &gType,
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
					* @param gtype which graph to populate
					* @param handler database handler to use for retrieval
					* @param nodes the nodes to populate with
					* @param errMsg error message when this function returns false
					* @return returns true if scene graph populated with no errors
					*/
					bool populate(
						const GraphType &gtype,
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
						populateAndUpdate(GraphType::DEFAULT, cameras, meshes, materials, metadata,
							textures, transformations, references, maps, unknowns);
					}

					/**
					* Populate the collections with the given node sets
					* This populates the scene graph information and also track the nodes that are added.
					* i.e. this assumes the nodes did not exist in the previous revision (if any)
					* @param gType specify which graph (graph or stash) to insert into
					* @param cameras Repo Node set of cameras
					* @param meshes  Repo Node set of meshes
					* @param materials Repo Node set of materials
					* @param metadata Repo Node set of metadata
					* @param textures Repo Node set of textures
					* @param transformations Repo Node set of transformations
					* @return returns true if scene graph populated with no errors
					*/
					void populateAndUpdate(
						const GraphType &gType,
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
					std::string stashExt;      /*! extension for optimized graph (Default: stash.3drepo)*/
					std::string rawExt;      /*! extension for raw file dumps (e.g. original files) (Default: raw)*/
					std::vector<std::string> refFiles;  //Original Files that created this scene
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

			};
		}//namespace graph
	}//namespace manipulator
}//namespace repo


