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
			class REPO_API_EXPORT RepoScene
			{
				//FIXME: unsure as to whether i should make the graph a differen class.. struct for now.
				struct repoGraphInstance
				{
					RepoNodeSet meshes; //!< Meshes
					RepoNodeSet materials; //!< Materials
					RepoNodeSet metadata; //!< Metadata
					RepoNodeSet references; //!< References
					RepoNodeSet textures; //!< Textures
					RepoNodeSet transformations; //!< Transformations
					RepoNodeSet unknowns; //!< Unknown types

					TransformationNode *rootNode;
					//! A lookup map for the all nodes the graph contains.
					std::unordered_map<repo::lib::RepoUUID, RepoNode*, repo::lib::RepoUUIDHasher> nodesByUniqueID;
					std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher> sharedIDtoUniqueID; //** mapping of shared ID to Unique ID
					ParentMap parentToChildren; //** mapping of shared id to its children's shared id
					std::unordered_map<repo::lib::RepoUUID, RepoScene*, repo::lib::RepoUUIDHasher> referenceToScene; //** mapping of reference ID to it's scene graph
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
				* @param projectId name of the project
				*/
				RepoScene(
					const std::string                                  &database = std::string(),
					const std::string                                  &projectName = std::string());

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
				* @param unknowns Repo Node set of unknowns (optional)
				*/
				RepoScene(
					const std::vector<std::string> &refFiles,
					const RepoNodeSet              &meshes,
					const RepoNodeSet              &materials,
					const RepoNodeSet              &metadata,
					const RepoNodeSet              &textures,
					const RepoNodeSet              &transformations,
					const RepoNodeSet              &references = RepoNodeSet(),
					const RepoNodeSet              &unknowns = RepoNodeSet());

				/**
				* Default Deconstructor
				* Please note that destroying the scene graph will also destroy
				* all nodes created by the scene graph
				* and also its child graphs
				*/
				~RepoScene();

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

				void ignoreReferenceScene() {
					ignoreReferenceNodes = true;
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
				void applyScaleFactor(const float &scale);

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
				* Commit changes on the default graph into the database
				* This commits an update to project settings, a new revision node
				* and the changes within this scene
				* @param errMsg error message if this failed
				* @param userName name of the author
				* @param message message for this commit (optional)
				* @param tag tag for this commit (optional)
				* @return returns true upon success
				*/
				uint8_t commit(
					repo::core::handler::AbstractDatabaseHandler *handler,
					repo::core::handler::fileservice::FileManager *manager,
					std::string &errMsg,
					const std::string &userName,
					const std::string &message = std::string(),
					const std::string &tag = std::string(),
					const repo::lib::RepoUUID &revId = repo::lib::RepoUUID::createUUID());

				/**
				* Get the branch ID of this scene graph
				* @return returns the branch ID of this scene
				*/
				repo::lib::RepoUUID getBranchID() const
				{
					return branch;
				}

				/**
				* Get name of the database
				* @return returns name of the database if available
				*/
				std::string getDatabaseName() const
				{
					return databaseName;
				}

				/**
				* Get the revision ID of this scene graph
				* @return returns the revision ID of this scene
				*/
				repo::lib::RepoUUID getRevisionID() const
				{
					if (revNode)
						return revNode->getUniqueID();
					else
						return revision;
				}

				/**
				* If the scene is revisioned, get the owner of the model
				* if the scene is not revisioned, this function will return
				* an empty string
				* @return returns the owner, or empty string.
				*/
				std::string getOwner() const
				{
					if (revNode)
						return revNode->getAuthor();
					else
						return "";
				}

				/**
				* If the scene is revisioned, get the tag associated with the revision
				* if the reivison is not tagged or the scene is not revisioned, return empty string
				* @return returns the tag, or empty string.
				*/
				std::string getTag() const
				{
					if (revNode)
						return revNode->getTag();
					else
						return "";
				}

				/**
				* If the scene is revisioned, get the commit message associated with the revision
				* if the reivison does not have a commit message or the scene is not revisioned, return empty string
				* @return returns the tag, or empty string.
				*/
				std::string getMessage() const
				{
					if (revNode)
						return revNode->getMessage();
					else
						return commitMsg;
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
					return projectName;
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
				* Return if it the scene is of a head revision
				* @return returns true if it is head of a branch
				*/
				bool isHeadRevision() const
				{
					return headRevision;
				}

				/**
				* Check if this scene is revisioned
				* @return returns true if the scene is revisioned
				*/
				bool isRevisioned() const
				{
					return !unRevisioned;
				}

				/**
				* Set project and database names. This will overwrite the current ones
				* @param newDatabaseName new name of database.
				* @param newProjectMame new name of the project
				*/
				void setDatabaseAndProjectName(std::string newDatabaseName, std::string newProjectName);

				/**
				* Set project revision
				* @param uuid of the revision.
				*/
				void setRevision(repo::lib::RepoUUID revisionID)
				{
					headRevision = false;
					revision = revisionID;
				}

				/**
				* Set Branch
				* @param uuid of branch
				*/
				void setBranch(repo::lib::RepoUUID branchID) { branch = branchID; }

				/**
				* Set commit message
				* @param msg message to go with the commit
				*/
				void setCommitMessage(const std::string &msg) { commitMsg = msg; }

				/**
				* Set the world offset value for the model
				* models are often shifted for better viewing purposes
				* this value tells us how much to shift to put it back into
				* it's relative world coordinates.
				*/
				void setWorldOffset(
					const std::vector<double> &offset);

				/**
				* Get branch name return uuid of branch there is no name
				* return "master" if this scene is not revisioned.
				* @return name of the branch or uuid if no name
				*/
				std::string getBranchName() const;

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
				* Load revision information of the configured branch/revision
				* If setProjectRevision() and setProjectBranch was never called,
				* it will load master/head.
				* @param handler database handler to perform this action
				* @param errMsg message if it failed
				* @return return true upon success
				*/
				bool loadRevision(
					repo::core::handler::AbstractDatabaseHandler *handler,
					std::string &errMsg,
					const std::vector<ModelRevisionNode::UploadStatus> &includeStatus = {});

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
				* Update revision status, if the scene is revisioned
				* This will also update the record within the database
				* should a handler is supplied
				* @param status status of the revision
				*/
				bool updateRevisionStatus(
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
				* Get Scene from reference node
				*/
				RepoScene* getSceneFromReference(
					const GraphType &gType,
					const repo::lib::RepoUUID  &reference) const
				{
					const repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
					RepoScene* refScene = nullptr;

					std::unordered_map<repo::lib::RepoUUID, RepoScene*, repo::lib::RepoUUIDHasher >::const_iterator it = g.referenceToScene.find(reference);
					if (it != g.referenceToScene.end())
						refScene = it->second;
					return refScene;
				}

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
					const GraphType &gType) const
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
				* Get all reference nodes within current scene revision
				* @return a RepoNodeSet of references
				*/
				RepoNodeSet getAllReferences(
					const GraphType &gType) const
				{
					return  gType == GraphType::OPTIMIZED ? stashGraph.references : graph.references;
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
					const GraphType &gType) const
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
				bool hasRoot(const GraphType &gType) const {
					const repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
					return (bool)g.rootNode;
				}
				RepoNode* getRoot(const GraphType &gType) const {
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
				* Reset the change set of this RepoScene.
				* turn everything into newly added and set it as unrevisioned
				*/
				void resetChangeSet();

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
					const std::vector<repo::lib::RepoUUID> &nodesToCommit,
					const repo::lib::RepoUUID &revId,
					const GraphType &gType,
					std::string &errMsg);

				bool commitSequence(
					repo::core::handler::AbstractDatabaseHandler *handler,
					repo::core::handler::fileservice::FileManager *manager,
					const repo::lib::RepoUUID &revID,
					std::string &err
				);

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
					repo::core::handler::fileservice::FileManager *manager,
					std::string &errMsg,
					ModelRevisionNode *&newRevNode,
					const std::string &userName,
					const std::string &message,
					const std::string &tag,
					const repo::lib::RepoUUID &revId);

				/**
				* Commit a new nodes into project.sceneExt base on the
				* changes on this scene
				* @param errMsg error message if this failed
				* @return returns true upon success
				*/
				bool commitSceneChanges(
					repo::core::handler::AbstractDatabaseHandler *handler,
					const repo::lib::RepoUUID &revId,
					std::string &errMsg);

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
					const RepoNodeSet &meshes,
					const RepoNodeSet &materials,
					const RepoNodeSet &metadata,
					const RepoNodeSet &textures,
					const RepoNodeSet &transformations,
					const RepoNodeSet &references,
					const RepoNodeSet &unknowns);

				/**
				* Shift the model by the given vector
				* this alter the root node with the given translation
				* @param offset a vector 3 double denoting the vector shift
				*/
				void shiftModel(
					const std::vector<double> &offset);

				/*
				* ---------------- Scene utilities ----------------
				*/

				std::string sanitizeExt(const std::string& name) const;
				std::string sanitizeName(const std::string& name) const;
				std::string sanitizeDatabaseName(const std::string& name) const;

				/*
				* ---------------- Scene Graph settings ----------------
				*/

				std::vector<std::string> refFiles;  //Original Files that created this scene
				std::vector<RepoNode*> toRemove;
				std::vector<double> worldOffset;
				repo::lib::RepoUUID   revision;
				repo::lib::RepoUUID   branch;
				std::string commitMsg;
				bool headRevision;
				bool unRevisioned;       /*! Flag to indicate if the scene graph is revisioned (true for scene graphs from model convertor)*/
				std::string databaseName;/*! name of the database */
				std::string projectName; /*! name of the project */
				ModelRevisionNode		 *revNode;

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

				repoGraphInstance graph; //current state of the graph, given the branch/revision
				repoGraphInstance stashGraph; //current state of the optimized graph, given the branch/revision
				uint16_t status = 0; //health of the scene, 0 denotes healthy
				bool ignoreReferenceNodes = false;
				bool loadExtFiles = true;
			};
		}//namespace graph
	}//namespace manipulator
}//namespace repo
