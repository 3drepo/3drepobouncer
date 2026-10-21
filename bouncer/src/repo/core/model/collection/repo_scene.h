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

#include <unordered_map>

#include "repo/core/handler/repo_database_handler_abstract.h"
#include "repo/core/handler/fileservice/repo_file_manager.h"
#include "repo/core/model/bson/repo_bson_sequence.h"
#include "repo/core/model/bson/repo_bson_task.h"
#include "repo/core/model/bson/repo_node.h"
#include "repo/core/model/bson/repo_node_transformation.h"
#include "repo/core/model/bson/repo_node_model_revision.h"
#include "repo/lib/datastructure/repo_bounds.h"

typedef std::unordered_map<repo::lib::RepoUUID, std::vector<repo::core::model::RepoNode*>, repo::lib::RepoUUIDHasher> ParentMap;

namespace repo {
	namespace core {
		namespace model {
			/*
			* Represents the scene graph of a Container at a specific revision. RepoScene
			* can hold the entire collection in memory, or more often now, serve as a
			* pointer to a particular revision.
			* 
			* A RepoScene can be constructed first from a revision, and then populated with
			* nodes, or it can be populated and then have its revision set. In either case,
			* the scene must have both contents & an existing revision node to be valid,
			* and it must valid in order to perform operations such as commits.
			*/
			class REPO_API_EXPORT RepoScene
			{
				//FIXME: unsure as to whether i should make the graph a differen class.. struct for now.
				struct repoGraphInstance
				{
					RepoNodeSet meshes; //!< Meshes
					RepoNodeSet materials; //!< Materials
					RepoNodeSet metadata; //!< Metadata
					RepoNodeSet textures; //!< Textures
					RepoNodeSet transformations; //!< Transformations
					RepoNodeSet unknowns; //!< Unknown types

					TransformationNode *rootNode;
					//! A lookup map for the all nodes the graph contains.
					std::unordered_map<repo::lib::RepoUUID, RepoNode*, repo::lib::RepoUUIDHasher> nodesByUniqueID;
					std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher> sharedIDtoUniqueID; //** mapping of shared ID to Unique ID
					ParentMap parentToChildren; //** mapping of shared id to its children's shared id
				};

				static const std::vector<std::string> collectionsInProject;
				static const uint16_t REPO_SCENE_TEXTURE_BIT = 0x0001;
				static const uint16_t REPO_SCENE_ENTITIES_BIT = 0x0002;
			public:

				/**
				* DEFAULT - The graph that is revisioned - should be (close to) identical to original model
				* OPTIMIZED - The optimized version of the graph - for quick visualization purposes
				*/
				enum class GraphType { DEFAULT, OPTIMIZED };

				/*
				* A completely empty scene - it must be both revisioned and populated before
				* it can be committed.
				*/
				RepoScene();

				/**
				* Creates a RepoScene that points to a specific revision, that may or may
				* not yet be populated.
				*/
				RepoScene(
					const std::string &teamspace,
					const std::string &container,
					const repo::lib::RepoUUID &revision
				);

				/**
				* Used for constructing scene graphs from non-streaming model
				* importers. The scene must be revisioned before it can be
				* committed.
				* @param cameras Repo Node set of cameras
				* @param meshes  Repo Node set of meshes
				* @param materials Repo Node set of materials
				* @param metadata Repo Node set of metadata
				* @param textures Repo Node set of textures
				* @param transformations Repo Node set of transformations
				* @param unknowns Repo Node set of unknowns (optional)
				*/
				RepoScene(
					const RepoNodeSet              &meshes,
					const RepoNodeSet              &materials,
					const RepoNodeSet              &metadata,
					const RepoNodeSet              &textures,
					const RepoNodeSet              &transformations,
					const RepoNodeSet              &unknowns = RepoNodeSet());

				/**
				* Default Deconstructor
				* Please note that destroying the scene graph will also destroy
				* all nodes created by the scene graph
				* and also its child graphs
				*/
				~RepoScene();

				void setRevision(
					const std::string& teamspace,
					const std::string& container,
					const repo::lib::RepoUUID& revision
				);

				static std::vector<RepoNode*> filterNodesByType(
					const std::vector<RepoNode*> nodes,
					const NodeType filter);

				/**
				* Check if the default scene graph is ok.
				* @return returns true if it is healthy
				*/
				bool isOK() const {
					return !status;
				}

				void skipLoadingExtFiles() {
					loadExtFiles = false;
				}

				/**
				* Check if default scene graph is missing texture
				* @return returns true if missing textures
				*/
				bool isMissingTexture() const {
					return (bool)status & REPO_SCENE_TEXTURE_BIT;
				}
				/**
				* Check if default scene graph is missing some nodes due to failed import
				* @return returns true if missing nodes
				*/
				bool isMissingNodes() const;

				/**
				* Flag missing texture bit on status.
				*/
				void setMissingTexture() {
					status |= REPO_SCENE_TEXTURE_BIT;
				}

				/**
				* Flag missing nodes due to import failures
				*/
				void setMissingNodes() {
					status |= REPO_SCENE_ENTITIES_BIT;
				}

				/**
				* Add metadata that has a matching name as the transformation into the scene
				* @param metadata set of metadata to attach
				* @param exactMatch whether the name has to be an exact match or a substring will do
				* @param propagateData if set to true, propagate down the changes to all meshes of this subtree
				*/
				void addMetadata(
					RepoNodeSet &metadata,
					const bool  &exactMatch,
					const bool  &propagateData = true);

				/**
				* Add a stash graph (optimised graph) for the corresponding scene graph
				* @param camera set of cameras
				* @param meshes set of meshes
				* @param materials set of materials
				* @param textures set of textures
				* @param transformations set of transformations
				*/
				void addStashGraph(
					const RepoNodeSet &meshes,
					const RepoNodeSet &materials,
					const RepoNodeSet &textures,
					const RepoNodeSet &transformations);

				/**
				* Add error message to project settings
				*/
				void addErrorStatusToProjectSettings(
					repo::core::handler::AbstractDatabaseHandler *handler
				);

				/**
				* Add a timestamp to project settings. This is an indication that
				* the scene is successfully commited and ready to view
				* @param handler database handler to perform the commit
				*/
				void addTimestampToProjectSettings(
					repo::core::handler::AbstractDatabaseHandler *handler
				);

				/**
				* Apply a scale factor to the unoptimised scene; this will apply a scaling matrix to the root node.
				* This is typically done for units conversion.
				* @param scale the scaling factor to apply
				*/
				void applyScaleFactor(const double& scale);

				void addSequence(
					const RepoSequence &animationSequence,
					const std::unordered_map<std::string, std::vector<uint8_t>> &states
				) {
					sequence = animationSequence;
					frameStates = states;
				}

				void addSequenceTasks(
					const std::vector<RepoTask> &taskItems,
					const std::vector<uint8_t> taskListCache
				) {
					tasks = taskItems;
					taskList = taskListCache;
				}

				void setDefaultInvisible(const std::set<repo::lib::RepoUUID> &idsToHide) {
					defaultInvisible = idsToHide;
				}

				bool isHiddenByDefault(const repo::lib::RepoUUID &uniqueID) const {
					return defaultInvisible.find(uniqueID) != defaultInvisible.end();
				}

				/**
				* Clears the contents within the Stash (if there is one)
				*/
				void clearStash();

				/**
				* Commit any changes on the default graph and resets the tracked changes.
				* After this is called the status of the revision will be GEN_DEFAULT,
				* regardless of whether any changes were made or not.
				* This will also commit any sequences in the scene.
				*/
				void commit(
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler
				);

				/**
				* Get name of the database
				* @return returns name of the database if available
				*/
				std::string getDatabaseName() const
				{
					return teamspace;
				}

				/**
				* Get the revision ID of this scene graph
				* @return returns the revision ID of this scene
				*/
				repo::lib::RepoUUID getRevisionID() const
				{
					return revision;
				}

				static std::vector<std::string> getProjectExtensions()
				{
					return collectionsInProject;
				}

				/**
				* Get name of the project
				* @return returns name of the project if available
				*/
				std::string getProjectName() const
				{
					return container;
				}

				/**
				* Get the world offset shift coordinates of the model
				* @return a vector of double denoting its offset
				*/
				std::vector<double> getWorldOffset() const
				{
					return worldOffset.size() ? worldOffset : std::vector<double>({ 0, 0, 0 });
				}

				/**
				* Check if this scene is revisioned
				* @return returns true if the scene is revisioned
				*/
				bool isRevisioned() const
				{
					return !teamspace.empty() && !container.empty() && (bool)revision;
				}

				/**
				* Set the world offset value for the model
				* models are often shifted for better viewing purposes
				* this value tells us how much to shift to put it back into
				* it's relative world coordinates.
				*/
				void setWorldOffset(
					const std::vector<double> &offset);

				/**
				* Return the best graph type availalble for viewing
				* if stash is loaded, returns stash, otherwise default
				* @return returns either optimised or default
				*/
				GraphType getViewGraph() const
				{
					if (stashGraph.rootNode)
						return GraphType::OPTIMIZED;
					else
						return GraphType::DEFAULT;
				}

				/**
				* Load Scene into Scene graph object base on the
				* revision/branch setting.
				* @param handler database handler to perform this action
				* @param errMsg message if it failed
				* @return return true upon success
				*/
				void loadScene(
					repo::core::handler::AbstractDatabaseHandler *handler);

				/*
				* Loads the root node of the Default graph into the member. This method
				* assumes the Scene has a revisionId set.
				*/
				void loadRootNode(
					repo::core::handler::AbstractDatabaseHandler* handler
				);

				/**
				* Update revision status, if the scene is revisioned
				* This will also update the record within the database
				* should a handler be supplied
				* @param status status of the revision
				*/
				void updateRevisionStatus(
					repo::core::handler::AbstractDatabaseHandler *handler,
					const ModelRevisionNode::UploadStatus &status);

				/**
				* --------------------- Node Relationship ----------------------
				*/

				/**
				* Abandon child from parent (disjoint 2 nodes within the scene)
				* @param parent shared ID of parent
				* @param child shared ID of child
				* @param modifyParent modify parent to relay the information (not needed if this node is to be removed)
				* @param modifyChild modify child node to relay this information (not needed if this node is to be removed)
				*/
				void abandonChild(
					const GraphType &gType,
					const repo::lib::RepoUUID  &parent,
					const repo::lib::RepoUUID  &child,
					const bool      &modifyParent = true,
					const bool      &modifyChild = true)
				{
					return abandonChild(gType, parent, getNodeBySharedID(gType, child), modifyParent, modifyChild);
				}

				/**
				* Abandon child from parent (disjoint 2 nodes within the scene)
				* @param parent shared ID of parent
				* @param child node of child
				* @param modifyNode modify child node to relay this information (not needed if this node is to be removed)
				*/
				void abandonChild(
					const GraphType &gType,
					const repo::lib::RepoUUID  &parent,
					RepoNode  *child,
					const bool      &modifyParent = true,
					const bool      &modifyChild = true);

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
					const repo::lib::RepoUUID  &parent,
					const repo::lib::RepoUUID  &child,
					const bool      &noUpdate = false)
				{
					addInheritance(gType, getNodeByUniqueID(gType, parent), getNodeByUniqueID(gType, child), noUpdate);
				}

				/**
				* Introduce parentship to 2 nodes that already reside within the scene.
				* If either of the nodes are not found, it does nothing
				* If they already share an inheritance, it does nothing
				* @param gType which graph are the nodes
				* @param parent parent Node
				* @param child child node
				* @param noUpdate if true, it will not be treated as
				*        a change that is needed to be commited (only valid for default graph)
				*/
				void addInheritance(
					const GraphType &gType,
					const RepoNode  *parent,
					RepoNode  *child,
					const bool      &noUpdate = false);

				void addInheritance(
					const GraphType &gType,
					const RepoNodeSet &parentNodes,
					RepoNode  *child,
					const bool      &noUpdate = false);

				/**
				* Get children nodes of a specified parent
				* @param g graph to retrieve from
				* @param parent shared UUID of the parent node
				* @ return a vector of pointers to children node (potentially none)
				*/
				std::vector<RepoNode*>
					getChildrenAsNodes(
						const GraphType &g,
						const repo::lib::RepoUUID &parent) const;

				/**
				* Get children nodes of a specified parent that satisfy the filtering condition
				* @param g graph to retrieve from
				* @param parent shared UUID of the parent node
				* @param type the type of nodes to obtain
				* @ return a vector of pointers to children node (potentially none)
				*/
				std::vector<RepoNode*>
					getChildrenNodesFiltered(
						const GraphType &g,
						const repo::lib::RepoUUID &parent,
						const NodeType  &type) const;

				/**
				* Get the list of parent nodes that satisfy the filtering condition
				* @param gType graphType
				* @param node node in question
				* @param type the type of nodes to obtain
				* @return returns a vector of transformation nodes
				*/
				std::vector<RepoNode*> getParentNodesFiltered(
					const GraphType &gType,
					const RepoNode  *node,
					const NodeType  &type) const;

				/**
				* Get the texture ID that is associated with the given mesh
				* if the mesh has no texture, it returns an empty string
				* @param gType the graph type to search through
				* @param sharedID shared ID of the mesh
				* @return returns a string consisting of texture ID
				*/
				std::string getTextureIDForMesh(
					const GraphType &gType,
					const repo::lib::RepoUUID  &sharedID) const;

				/**
				* --------------------- Scene Lookup ----------------------
				*/

				/**
				* Get all material nodes within current scene revision
				* @return a RepoNodeSet of materials
				*/
				RepoNodeSet getAllMaterials(
					const GraphType &gType) const
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

				RepoNodeSet getAllSupermeshes(
					const GraphType& gType) const
				{
					if (gType == GraphType::OPTIMIZED)
					{
						return stashGraph.meshes; // All stash graph meshes are supermeshes inherently
					}
					else {
						return {};
					}
				}

				/**
				* Get all metadata nodes within current scene revision
				* @return a RepoNodeSet of metadata
				*/
				RepoNodeSet getAllMetadata(
					const GraphType &gType) const
				{
					return  gType == GraphType::OPTIMIZED ? stashGraph.metadata : graph.metadata;
				}

				/**
				* Get all texture nodes within current scene revision
				* @return a RepoNodeSet of textures
				*/
				RepoNodeSet getAllTextures(
					const GraphType &gType) const
				{
					return  gType == GraphType::OPTIMIZED ? stashGraph.textures : graph.textures;
				}

				/**
				* Get all transformation nodes within current scene revision
				* @return a RepoNodeSet of transformations
				*/
				RepoNodeSet getAllTransformations(
					const GraphType &gType = GraphType::DEFAULT) const
				{
					return  gType == GraphType::OPTIMIZED ? stashGraph.transformations : graph.transformations;
				}

				std::set<repo::lib::RepoUUID> getAllSharedIDs(
					const GraphType &gType) const;

				/**
				* Get all desecendants, of a particular type, of this shared ID
				* @param sharedID sharedID of the node in question
				* @param type type of desecendants to get
				* @return returns all child/grandchild/great grandchild etc node of type "type" for this node
				*/
				std::vector<RepoNode*> getAllDescendantsByType(
					const GraphType &gType,
					const repo::lib::RepoUUID  &sharedID,
					const NodeType  &type) const;

				/**
				* Get a bounding box for the entire scene
				* @return returns bounding box for the whole graph.
				*/
				repo::lib::RepoBounds getSceneBoundingBox() const;

				/**
				* Get all ID of nodes which are added since last revision
				* @return returns a vector of node IDs
				*/
				std::vector<repo::lib::RepoUUID> getAddedNodesID() const
				{
					return std::vector<repo::lib::RepoUUID>(newAdded.begin(), newAdded.end());
				}
				/**
				* Get all ID of nodes which are modified since last revision
				* @return returns a vector of node IDs
				*/
				std::vector<repo::lib::RepoUUID> getModifiedNodesID() const
				{
					return std::vector<repo::lib::RepoUUID>(newModified.begin(), newModified.end());
				}

				/**
				* Get all ID of nodes which are removed since last revision
				* @return returns a vector of node IDs
				*/
				std::vector<repo::lib::RepoUUID> getRemovedNodesID() const
				{
					return std::vector<repo::lib::RepoUUID>(newRemoved.begin(), newRemoved.end());
				}

				/**
				* Get allnodes which are removed since last revision
				* @return returns a vector of node removed from scene
				*/
				std::vector<RepoNode*> getRemovedNodes() const
				{
					return toRemove;
				}

				size_t getTotalNodesChanged() const
				{
					return newRemoved.size() + newAdded.size() + newModified.size();
				}

				/**
				* Get the node given the shared ID of this node
				* @param g instance of the graph to search from
				* @param sharedID shared ID of the node
				* @return returns a pointer to the node if found.
				*/
				RepoNode* getNodeBySharedID(
					const GraphType &gType,
					const repo::lib::RepoUUID &sharedID) const
				{
					const repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
					auto it = g.sharedIDtoUniqueID.find(sharedID);

					if (it == g.sharedIDtoUniqueID.end()) return nullptr;

					return getNodeByUniqueID(gType, it->second);
				}

				/**
				* Get the node given the Unique ID of this node
				* @param g instance of the graph to search from
				* @param sharedID Unique ID of the node
				* @return returns a pointer to the node if found.
				*/
				RepoNode* getNodeByUniqueID(
					const GraphType &gType,
					const repo::lib::RepoUUID &uniqueID) const
				{
					const repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
					auto it = g.nodesByUniqueID.find(uniqueID);

					if (it == g.nodesByUniqueID.end()) return nullptr;

					return it->second;
				}

				/**
				* check if Root Node exists
				* @return returns true if rootNode is not null.
				*/
				bool hasRoot(const GraphType &gType = GraphType::DEFAULT) const {
					const repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
					return (bool)g.rootNode;
				}

				RepoNode* getRoot(const GraphType &gType = GraphType::DEFAULT) const {
					const repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
					return g.rootNode;
				}

				/**
				* Return the number of nodes within the current
				* graph representation
				* @return number of nodes within the graph
				*/
				uint32_t getItemsInCurrentGraph(const GraphType &gType) {
					const repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
					return g.nodesByUniqueID.size();
				}

				/**
				* -------------- Scene Modification Functions --------------
				*/

				/**
				* Add a vector of nodes into the unoptimized scene graph
				* Tracks the modification for commit
				* @param vector of nodes to insert.
				*/
				void addNodes(const std::vector<RepoNode *> &nodes);

				/**
				* Remove a node from the scene
				* WARNING: Ensure all relationships are patched up,
				* or you may end up with orphaned nodes/disjoint trees!
				* This function will not try to patch up any orphaned children
				* The node will be deleted from memory after this call!
				* @param gtype graph type
				* @param sharedID of the node to remove
				*/
				void removeNode(
					const GraphType                   &gtype,
					const repo::lib::RepoUUID                    &sharedID);

				/**
				* Rotates the model by 270 degrees to compensate the different axis orientation
				* in directX. Commonly happens in fbx models
				*/
				void reorientateDirectXModel();

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
				* @param g which graph to insert into
				* @param node pointer to the node to add
				* @param errMsg error message if it returns false
				*/
				void addNodeToMaps(
					const GraphType& gType,
					RepoNode* node);

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
					const std::vector<repo::lib::RepoUUID> &nodesToCommit,
					const repo::lib::RepoUUID &revId,
					const GraphType &gType,
					std::string &errMsg);

				void commitSequence(
					repo::core::handler::AbstractDatabaseHandler *handler,
					repo::core::handler::fileservice::FileManager *manager
				);

				/**
				* Commit a new nodes into project.sceneExt base on the
				* changes on this scene
				* @param errMsg error message if this failed
				*/
				void commitSceneChanges(repo::core::handler::AbstractDatabaseHandler *handler);

				/**
				* Recursive function to find the scene's bounding box
				* @param gtype type of graph to navigate
				* @param node current node
				* @param mat transformation matrix
				* @param bbox boudning box (to return/update)
				*/
				void getSceneBoundingBoxInternal(
					const GraphType            &gType,
					const RepoNode             *node,
					const repo::lib::RepoMatrix   &mat,
					repo::lib::RepoBounds&bbox) const;

				ModelRevisionNode getRevisionNode(
					repo::core::handler::AbstractDatabaseHandler *handler);

				void updateRevisionNode(
					repo::core::handler::AbstractDatabaseHandler* handler,
					ModelRevisionNode& revNode);

				/**
				* populate the collections (cameras, meshes etc) with the given nodes
				* @param gtype which graph to populate
				* @param handler database handler to use for retrieval
				* @param nodes the nodes to populate with
				* @param errMsg error message when this function returns false
				*/
				void populate(
					const GraphType &gtype,
					repo::core::handler::AbstractDatabaseHandler *handler,
					std::vector<RepoBSON> nodes);

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
				*/
				void populateAndUpdate(
					const GraphType &gType,
					const RepoNodeSet &meshes,
					const RepoNodeSet &materials,
					const RepoNodeSet &metadata,
					const RepoNodeSet &textures,
					const RepoNodeSet &transformations,
					const RepoNodeSet &unknowns);

				/*
				* ---------------- Scene Graph settings ----------------
				*/

				std::vector<RepoNode*> toRemove;
				std::vector<double> worldOffset;
				repo::lib::RepoUUID revision;
				std::string teamspace;/*! previously known as database */
				std::string container; /*! previously known as project */

				RepoSequence sequence;
				std::vector<RepoTask> tasks;
				std::vector<uint8_t> taskList;
				std::unordered_map<std::string, std::vector<uint8_t>> frameStates;
				std::set<repo::lib::RepoUUID> defaultInvisible;

				/*
				* ---------------- Scene Graph Details ----------------
				*/

				//Change trackers
				std::set<repo::lib::RepoUUID> newCurrent; //new list of current (unique IDs)
				std::set<repo::lib::RepoUUID> newAdded; //list of nodes added to the new revision (shared ID)
				std::set<repo::lib::RepoUUID> newRemoved; //list of nodes removed for this revision (shared ID)
				std::set<repo::lib::RepoUUID> newModified; // list of nodes modified during this revision  (shared ID)

				repoGraphInstance graph; //current state of the graph, given the revision
				repoGraphInstance stashGraph; //current state of the optimized graph, given the revision
				uint16_t status = 0; //health of the scene, 0 denotes healthy
				bool loadExtFiles = true;
			};
		}//namespace graph
	}//namespace manipulator
}//namespace repo
