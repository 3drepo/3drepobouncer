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
#include "repo_scene.h"

#include <boost/filesystem.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <fstream>

#include "repo/lib/repo_log.h"
#include "repo/error_codes.h"
#include "repo/core/model/bson/repo_bson_builder.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/model/bson/repo_bson_project_settings.h"
#include "repo/core/handler/database/repo_expressions.h"

using namespace repo::core::model;

const std::vector<std::string> RepoScene::collectionsInProject = {
	"scene",
	"scene.files",
	"scene.chunks",
	"stash.3drepo",
	"stash.3drepo.files",
	"stash.3drepo.chunks",
	"stash.json_mpc.files",
	"stash.json_mpc.chunks",
	"stash.src",
	"stash.src.files",
	"stash.src.chunks",
	"history",
	"history.files",
	"history.chunks",
	"issues",
	"wayfinder",
	"groups",
	"sequences",
	"tasks",
};

static bool nameCheck(const char &c)
{
	return c == ' ' || c == '$' || c == '.';
}

static bool dbNameCheck(const char &c)
{
	return c == '/' || c == '\\' || c == '.' || c == ' '
		|| c == '\"' || c == '$' || c == '*' || c == '<'
		|| c == '>' || c == ':' || c == '?' || c == '|';
}

static bool extNameCheck(const char &c)
{
	return c == ' ' || c == '$';
}

std::string RepoScene::sanitizeExt(const std::string& name) const
{
	// http://docs.mongodb.org/manual/reference/limits/#Restriction-on-Collection-Names
	std::string newName(name);
	std::replace_if(newName.begin(), newName.end(), extNameCheck, '_');

	return newName;
}

std::string RepoScene::sanitizeName(const std::string& name) const
{
	// http://docs.mongodb.org/manual/reference/limits/#Restriction-on-Collection-Names
	std::string newName(name);
	std::replace_if(newName.begin(), newName.end(), nameCheck, '_');

	return newName;
}

std::string RepoScene::sanitizeDatabaseName(const std::string& name) const
{
	// http://docs.mongodb.org/manual/reference/limits/#naming-restrictions

	// Cannot contain any of /\. "$*<>:|?
	std::string newName(name);
	std::replace_if(newName.begin(), newName.end(), dbNameCheck, '_');

	return newName;
}

RepoScene::RepoScene(
	const std::string &database,
	const std::string &projectName)
	:
	databaseName(sanitizeDatabaseName(database)),
	projectName(sanitizeName(projectName)),
	headRevision(true),
	unRevisioned(false),
	revNode(0),
	status(0)
{
	graph.rootNode = nullptr;
	stashGraph.rootNode = nullptr;
	//defaults to master branch
	branch = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH);
}

RepoScene::RepoScene(
	const std::vector<std::string> &refFiles,
	const RepoNodeSet              &meshes,
	const RepoNodeSet              &materials,
	const RepoNodeSet              &metadata,
	const RepoNodeSet              &textures,
	const RepoNodeSet              &transformations,
	const RepoNodeSet              &references,
	const RepoNodeSet              &unknowns
) :
	databaseName(""),
	projectName(""),
	headRevision(true),
	unRevisioned(true),
	refFiles(refFiles),
	revNode(0),
	status(0)
{
	graph.rootNode = nullptr;
	stashGraph.rootNode = nullptr;
	branch = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH);
	populateAndUpdate(GraphType::DEFAULT, meshes, materials, metadata, textures, transformations, references, unknowns);
}

RepoScene::~RepoScene()
{
	for (auto& pair : graph.nodesByUniqueID)
	{
		delete pair.second;
	}

	for (auto& pair : stashGraph.nodesByUniqueID)
	{
		delete pair.second;
	}

	if (revNode)
		delete revNode;
}

void RepoScene::abandonChild(
	const GraphType &gType,
	const repo::lib::RepoUUID  &parent,
	RepoNode  *child,
	const bool      &modifyParent,
	const bool      &modifyChild)
{
	if (!child)
	{
		repoError << "Cannot abandon a child with nullptr!";
		return;
	}

	repoGraphInstance &g = GraphType::OPTIMIZED == gType ? stashGraph : graph;
	repo::lib::RepoUUID childSharedID = child->getSharedID();

	if (modifyParent)
	{
		auto pToCIt = g.parentToChildren.find(parent);

		if (pToCIt != g.parentToChildren.end())
		{
			std::vector<RepoNode *> children = pToCIt->second;
			auto childIt = std::find(children.begin(), children.end(), child);
			if (childIt != children.end())
			{
				children.erase(childIt);
				g.parentToChildren[parent] = children;
			}
			else
			{
				repoWarning << "Trying to abandon a child that isn't a child of the parent!";
			}
		}
	}

	if (modifyChild)
	{
		child->removeParent(parent);
	}
}

void RepoScene::addInheritance(
	const GraphType &gType,
	const RepoNode  *parentNode,
	RepoNode        *childNode,
	const bool      &noUpdate)
{
	repoGraphInstance& g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
	if (parentNode && childNode)
	{
		repo::lib::RepoUUID parentShareID = parentNode->getSharedID();
		repo::lib::RepoUUID childShareID = childNode->getSharedID();

		//add children to parentToChildren mapping
		auto childrenIT =
			g.parentToChildren.find(parentShareID);

		if (childrenIT != g.parentToChildren.end())
		{
			std::vector<RepoNode*> &children = childrenIT->second;
			//TODO: use sets for performance?
			auto childrenInd = std::find(children.begin(), children.end(), childNode);
			if (childrenInd == children.end())
			{
				children.push_back(childNode);
			}
		}
		else
		{
			g.parentToChildren[parentShareID] = std::vector<RepoNode*>();
			g.parentToChildren[parentShareID].push_back(childNode);
		}

		//add parent to children
		childNode->addParent(parentShareID);
	}
}

void RepoScene::addInheritance(
	const GraphType &gType,
	const RepoNodeSet &parentNodes,
	RepoNode        *childNode,
	const bool      &noUpdate)
{
	repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
	if (parentNodes.size() && childNode)
	{
		auto parentArr = childNode->getParentIDs();
		std::set<repo::lib::RepoUUID> currentParents(parentArr.begin(), parentArr.end());
		std::set<repo::lib::RepoUUID> parentShareIDs;
		for (const auto &parent : parentNodes) {
			auto parentShareID = parent->getSharedID();
			if (currentParents.find(parentShareID) == currentParents.end()) {
				parentShareIDs.insert(parentShareID);

				//add children to parentToChildren mapping
				auto childrenIT =
					g.parentToChildren.find(parentShareID);

				if (childrenIT != g.parentToChildren.end())
				{
					std::vector<RepoNode*> &children = childrenIT->second;
					auto childrenInd = std::find(children.begin(), children.end(), childNode);
					if (childrenInd == children.end())
					{
						children.push_back(childNode);
					}
				}
				else
				{
					g.parentToChildren[parentShareID] = std::vector<RepoNode*>();
					g.parentToChildren[parentShareID].push_back(childNode);
				}
			}
		}

		repo::lib::RepoUUID childShareID = childNode->getSharedID();

		if (parentShareIDs.size())
		{
			auto parents = std::vector<repo::lib::RepoUUID>(parentShareIDs.begin(), parentShareIDs.end());
			childNode->addParents(parents);
		}
	}
}

void RepoScene::addMetadata(
	RepoNodeSet &metadata,
	const bool  &exactMatch,
	const bool  &propagateData)
{
	std::unordered_map<std::string, std::vector<RepoNode*>> namesMap;
	//stashed version of the graph does not need to track metadata information
	for (RepoNode* transformation : graph.transformations)
	{
		std::string transformationName = transformation->getName();
		if (!exactMatch)
		{
			transformationName = transformationName.substr(0, transformationName.find(" "));
			std::transform(transformationName.begin(), transformationName.end(), transformationName.begin(), ::toupper);
		}

		if (namesMap.find(transformationName) == namesMap.end())
			namesMap[transformationName] = std::vector<RepoNode*>();

		namesMap[transformationName].push_back(transformation);
	}

	for (RepoNode* mesh : graph.meshes)
	{
		std::string name = mesh->getName();
		if (!exactMatch)
		{
			name = name.substr(0, name.find(" "));
			std::transform(name.begin(), name.end(), name.begin(), ::toupper);
		}
		if (namesMap.find(name) == namesMap.end())
			namesMap[name] = std::vector<RepoNode*>();

		namesMap[name].push_back(mesh);
	}

	for (RepoNode* meta : metadata)
	{
		// TODO: improve efficiency by storing in std::map
		std::string metaName = meta->getName();
		if (!exactMatch)
			std::transform(metaName.begin(), metaName.end(), metaName.begin(), ::toupper);

		auto nameIt = namesMap.find(metaName);

		if (nameIt != namesMap.end())
		{
			std::vector<repo::lib::RepoUUID> parents;
			repo::lib::RepoUUID metaSharedID = meta->getSharedID();
			repo::lib::RepoUUID metaUniqueID = meta->getUniqueID();
			for (auto &node : nameIt->second)
			{
				auto meshes = getAllDescendantsByType(GraphType::DEFAULT, node->getSharedID(), NodeType::MESH);
				if (propagateData && node->getTypeAsEnum() == NodeType::TRANSFORMATION && meshes.size())
				{
					for (auto &mesh : meshes)
					{
						repo::lib::RepoUUID parentSharedID = mesh->getSharedID();
						if (graph.parentToChildren.find(parentSharedID) == graph.parentToChildren.end())
							graph.parentToChildren[parentSharedID] = std::vector<RepoNode*>();

						graph.parentToChildren[parentSharedID].push_back(meta);
						parents.push_back(parentSharedID);
					}
				}
				else {
					repo::lib::RepoUUID parentSharedID = node->getSharedID();
					if (graph.parentToChildren.find(parentSharedID) == graph.parentToChildren.end())
						graph.parentToChildren[parentSharedID] = std::vector<RepoNode*>();

					graph.parentToChildren[parentSharedID].push_back(meta);
					parents.push_back(parentSharedID);
				}
			}

			meta->addParents(parents);

			graph.nodesByUniqueID[metaUniqueID] = meta;
			graph.sharedIDtoUniqueID[metaSharedID] = metaUniqueID;

			//FIXME should move this to a generic add node function...
			newAdded.insert(metaSharedID);
			newCurrent.insert(metaUniqueID);
			graph.metadata.insert(meta);
		}
		else
		{
			repoWarning << "Did not find a pairing transformation node with the same name : " << metaName;
		}
	}

	clearStash();
}

bool RepoScene::isMissingNodes() const {
	return status & REPO_SCENE_ENTITIES_BIT;
}

bool RepoScene::addNodeToScene(
	const GraphType &gType,
	const RepoNodeSet nodes,
	std::string &errMsg,
	RepoNodeSet *collection)
{
	bool success = true;

	for (auto & node : nodes)
	{
		if (node)
		{
			if (node->getTypeAsEnum() == NodeType::TRANSFORMATION || node->getParentIDs().size()) {
				collection->insert(node);
				if (!addNodeToMaps(gType, node, errMsg))
				{
					repoError << "failed to add node (" << node->getUniqueID() << " to scene graph: " << errMsg;
					success = false;
				}
			}
			else {
				//Orphaned nodes detected, flag missing nodes
				setMissingNodes();
				continue;
			}
		}
		if (gType == GraphType::DEFAULT)
			newAdded.insert(node->getSharedID());
	}

	return success;
}

void RepoScene::addNodes(
	const std::vector<RepoNode *> &nodes)
{
	std::string errMsg;
	for (auto &node : nodes)
	{
		addNodeToMaps(GraphType::DEFAULT, node, errMsg);
	}
}

bool RepoScene::addNodeToMaps(
	const GraphType &gType,
	RepoNode *node,
	std::string &errMsg)
{
	bool success = true;
	repo::lib::RepoUUID uniqueID = node->getUniqueID();
	repo::lib::RepoUUID sharedID = node->getSharedID();

	repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
	//----------------------------------------------------------------------
	//If the node has no parents it must be the rootnode
	if (!node->getParentIDs().size()) {
		if (!g.rootNode)
		{
			g.rootNode = dynamic_cast<TransformationNode*>(node);
		}
		else
		{
			//root node already exist, check if they are the same node
			if (g.rootNode == node) {
				//for some reason 2 instance of the root node reside in this scene graph - probably not game breaking.
				repoWarning << "2 instance of the (same) root node found";
			}
			else
			{
				//found 2 nodes with no parents...
				//they could be straggling materials. Only give an error if both are transformation
				//NOTE: this will fall apart if we ever allow root node to be something other than a transformation.

				if (node->getTypeAsEnum() == NodeType::TRANSFORMATION &&
					g.rootNode->getTypeAsEnum() == NodeType::TRANSFORMATION)
				{
					repoError << "2 candidate for root node found. This is possibly an invalid Scene Graph.";
				}

				if (node->getTypeAsEnum() == NodeType::TRANSFORMATION)
				{
					g.rootNode = dynamic_cast<TransformationNode*>(node);
				}
			}
		}
	}
	else {
		//has parent
		std::vector<repo::lib::RepoUUID> parentIDs = node->getParentIDs();
		std::vector<repo::lib::RepoUUID>::iterator it;
		for (it = parentIDs.begin(); it != parentIDs.end(); ++it)
		{
			//add itself to the parent on the "parent -> children" map
			repo::lib::RepoUUID parent = *it;

			//check if the parent already has an entry
			auto mapIt = g.parentToChildren.find(parent);
			if (mapIt != g.parentToChildren.end()) {
				//has an entry, add to the vector
				g.parentToChildren[parent].push_back(node);
			}
			else {
				//no entry, create one
				std::vector<RepoNode*> children;
				children.push_back(node);

				g.parentToChildren[parent] = children;
			}
		}
	} //if (!node->hasField(REPO_NODE_LABEL_PARENTS))

	g.nodesByUniqueID[uniqueID] = node;
	g.sharedIDtoUniqueID[sharedID] = uniqueID;

	return success;
}

void RepoScene::addStashGraph(
	const RepoNodeSet &meshes,
	const RepoNodeSet &materials,
	const RepoNodeSet &textures,
	const RepoNodeSet &transformations)
{
	populateAndUpdate(GraphType::OPTIMIZED, meshes, materials, RepoNodeSet(),
		textures, transformations, RepoNodeSet(), RepoNodeSet());
}

void RepoScene::clearStash()
{
	for (auto &pair : stashGraph.nodesByUniqueID)
	{
		if (pair.second)
			delete pair.second;
	}

	stashGraph.meshes.clear();
	stashGraph.materials.clear();
	stashGraph.metadata.clear();
	stashGraph.references.clear();
	stashGraph.textures.clear();
	stashGraph.transformations.clear();
	stashGraph.unknowns.clear();
	stashGraph.nodesByUniqueID.clear();
	stashGraph.sharedIDtoUniqueID.clear();
	stashGraph.parentToChildren.clear();
	stashGraph.referenceToScene.clear(); //how will this work for stash?

	stashGraph.rootNode = nullptr;
}

uint8_t RepoScene::commit(
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::core::handler::fileservice::FileManager *manager,
	std::string &errMsg,
	const std::string &userName,
	const std::string &message,
	const std::string &tag,
	const repo::lib::RepoUUID &revId)
{
	bool success = true;

	//Sanity check that everything we need is here
	if (!handler)
	{
		errMsg = "Cannot commit to the database - no database handler assigned.";
		return REPOERR_UPLOAD_FAILED;
	}

	if (databaseName.empty() || projectName.empty())
	{
		errMsg = "Cannot commit to the database - databaseName or projectName is empty (database: "
			+ databaseName
			+ " project: " + projectName + " ).";
		return REPOERR_UPLOAD_FAILED;
	}

	if (!graph.rootNode)
	{
		errMsg = "Cannot commit to the database - Scene is empty!.";
		return REPOERR_UPLOAD_FAILED;
	}

	repoInfo << "Commiting revision...";
	ModelRevisionNode* newRevNode = 0;
	if (!message.empty())
		commitMsg = message;

	if (success &= commitRevisionNode(handler, manager, errMsg, newRevNode, userName, commitMsg, tag, revId))
	{
		repoInfo << "Commited revision node, commiting scene nodes...";
		//commited the revision node, commit the modification on the scene
		if (success &= commitSceneChanges(handler, revId, errMsg))
		{
			//Succeed in commiting everything.
			//Update Revision Node and reset state.

			if (revNode)
			{
				delete revNode;
			}

			revNode = newRevNode;
			revision = newRevNode->getUniqueID();

			newAdded.clear();
			newRemoved.clear();
			newModified.clear();
			commitMsg.clear();

			refFiles.clear();
			for (RepoNode* node : toRemove)
			{
				delete node;
			}
			toRemove.clear();
			unRevisioned = false;
		}
		if (success && frameStates.size()) {
			repoInfo << "Commited Scene nodes, committing sequence";
			commitSequence(handler, manager, newRevNode->getUniqueID(), errMsg);
		}
	}

	if (success) updateRevisionStatus(handler, repo::core::model::ModelRevisionNode::UploadStatus::COMPLETE);
	//Create and Commit revision node
	repoInfo << "Succes: " << success << " msg: " << errMsg;
	return success ? REPOERR_OK : (errMsg.find("exceeds maxBsonObjectSize") != std::string::npos ? REPOERR_MAX_NODES_EXCEEDED : REPOERR_UPLOAD_FAILED);
}

bool RepoScene::commitSequence(
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::core::handler::fileservice::FileManager *manager,
	const repo::lib::RepoUUID &revID,
	std::string &err
) {
	if (frameStates.size()) {
		auto sequenceCol = projectName + "." + REPO_COLLECTION_SEQUENCE;
		sequence.setRevision(revID);
		handler->insertDocument(databaseName, sequenceCol, sequence);
		for (const auto& state : frameStates) {
			manager->uploadFileAndCommit(databaseName, sequenceCol, state.first, state.second);
		}

		auto taskCol = projectName + "." + REPO_COLLECTION_TASK;
		for (const auto &task : tasks) {
			handler->insertDocument(databaseName, taskCol, task);
		}

		if (taskList.size()) {
			manager->uploadFileAndCommit(databaseName, taskCol, sequence.getUniqueId().toString(), taskList);
		}
	}
	return true;
}

void RepoScene::addErrorStatusToProjectSettings(
	repo::core::handler::AbstractDatabaseHandler *handler
)
{
	auto projectsettings = RepoProjectSettings(handler->findOneByUniqueID(databaseName, REPO_COLLECTION_SETTINGS, projectName));
	projectsettings.setErrorStatus();
	std::string errorMsg;
	handler->upsertDocument(databaseName, REPO_COLLECTION_SETTINGS, projectsettings, false);
}

void RepoScene::addTimestampToProjectSettings(
	repo::core::handler::AbstractDatabaseHandler *handler
)
{
	auto projectsettings = RepoProjectSettings(handler->findOneByUniqueID(databaseName, REPO_COLLECTION_SETTINGS, projectName));
	projectsettings.clearErrorStatus();
	std::string errorMsg;
	handler->upsertDocument(databaseName, REPO_COLLECTION_SETTINGS, projectsettings, false);
}

bool RepoScene::commitRevisionNode(
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::core::handler::fileservice::FileManager *manager,
	std::string &errMsg,
	ModelRevisionNode *&newRevNode,
	const std::string &userName,
	const std::string &message,
	const std::string &tag,
	const repo::lib::RepoUUID &revId)
{
	bool success = true;
	std::vector<repo::lib::RepoUUID> parent;
	parent.reserve(1);

	if (!unRevisioned && !revNode)
	{
		if (!loadRevision(handler, errMsg))
			return false;
		parent.push_back(revNode->getUniqueID());
	}

	repoTrace << "Committing Revision Node....";

	// The user may import the same filename with different content, so prefix
	// the filename used for the ref node with the revision id

	std::vector<std::string> fileNames;
	for (const std::string &name : refFiles)
	{
		boost::filesystem::path filePath(name);
		fileNames.push_back(revId.toString() + sanitizeName(filePath.filename().string()));
	}

	newRevNode =
		new ModelRevisionNode(RepoBSONFactory::makeRevisionNode(userName, branch, revId,
			fileNames, parent, worldOffset, message, tag));
	newRevNode->updateStatus(ModelRevisionNode::UploadStatus::GEN_DEFAULT);

	if (newRevNode)
	{
		using namespace core::handler::database::index;

		handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_HISTORY, Descending({ REPO_NODE_REVISION_LABEL_TIMESTAMP }));
		handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_SCENE, Ascending({ REPO_NODE_REVISION_ID, "metadata.key", "metadata.value" }));
		handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_SCENE, Ascending({ "metadata.key", "metadata.value" }));
		handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_SCENE, Ascending({ REPO_NODE_REVISION_ID, REPO_NODE_LABEL_SHARED_ID, REPO_LABEL_TYPE }));
		handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_SCENE, Ascending({ REPO_NODE_LABEL_SHARED_ID }));

		for (size_t i = 0; i < refFiles.size(); i++)
		{
			std::ifstream file(refFiles[i], std::ios::binary);
			if (file.is_open())
			{
				std::string fsName = fileNames[i];

				file.seekg(0, std::ios::end);
				std::streamsize size = file.tellg();
				file.seekg(0, std::ios::beg);

				std::vector<uint8_t> rawFile(size);
				if (file.read((char*)rawFile.data(), size))
				{
					if (!(success = manager->uploadFileAndCommit(databaseName, projectName + "." + REPO_COLLECTION_RAW, fsName, rawFile)))
					{
						errMsg = "Failed to save original file into file storage: " + fsName;
						repoError << errMsg;
					}
				}
				else
				{
					repoError << "Failed to read reference file " << refFiles[i];
				}

				file.close();
			}
			else
			{
				repoError << "Failed to open reference file " << refFiles[i];
			}
		}
		}
	else
	{
		errMsg += "Failed to instantiate a new revision object!";
		return false;
	}

	if (success)
		handler->upsertDocument(databaseName, projectName + "." + REPO_COLLECTION_HISTORY, *newRevNode, true);

	return success;
}

bool RepoScene::commitNodes(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::vector<repo::lib::RepoUUID> &nodesToCommit,
	const repo::lib::RepoUUID &revId,
	const GraphType &gType,
	std::string &errMsg)
{
	bool success = true;

	bool isStashGraph = gType == GraphType::OPTIMIZED;
	repoGraphInstance &g = isStashGraph ? stashGraph : graph;
	std::string ext = isStashGraph ? REPO_COLLECTION_STASH_REPO : REPO_COLLECTION_SCENE;

	size_t count = 0;
	size_t total = nodesToCommit.size();

	repoInfo << "Committing " << total << " nodes...";

	std::vector< repo::core::model::RepoBSON> nodes;
	for (const repo::lib::RepoUUID &id : nodesToCommit)
	{
		const repo::lib::RepoUUID uniqueID = gType == GraphType::OPTIMIZED ? id : g.sharedIDtoUniqueID[id];
		RepoNode *node = g.nodesByUniqueID[uniqueID];
		node->setRevision(revId);
		nodes.push_back(*node);
	}

	handler->insertManyDocuments(databaseName, projectName + "." + ext, nodes);

	return true;
}

bool RepoScene::commitSceneChanges(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const repo::lib::RepoUUID &revId,
	std::string &errMsg)
{
	bool success = true;
	std::vector<repo::lib::RepoUUID> nodesToCommit;
	std::vector<repo::lib::RepoUUID>::iterator it;

	long count = 0;

	nodesToCommit.insert(nodesToCommit.end(), newAdded.begin(), newAdded.end());
	nodesToCommit.insert(nodesToCommit.end(), newModified.begin(), newModified.end());

	commitNodes(handler, nodesToCommit, revId, GraphType::DEFAULT, errMsg);

	return success;
}

std::vector<RepoNode*>
RepoScene::getChildrenAsNodes(
	const GraphType &gType,
	const repo::lib::RepoUUID &parent) const
{
	std::vector<RepoNode*> children;
	const repoGraphInstance &g = GraphType::OPTIMIZED == gType ? stashGraph : graph;
	auto it = g.parentToChildren.find(parent);
	return it != g.parentToChildren.end() ? it->second : std::vector<RepoNode*>();
}

std::vector<RepoNode*>
RepoScene::getChildrenNodesFiltered(
	const GraphType &gType,
	const repo::lib::RepoUUID  &parent,
	const NodeType  &type) const
{
	std::vector<RepoNode*> childrenUnfiltered = getChildrenAsNodes(gType, parent);

	return filterNodesByType(childrenUnfiltered, type);
}

std::vector<RepoNode*>
RepoScene::filterNodesByType(
	const std::vector<RepoNode*> nodes,
	const NodeType filter)
{
	std::vector<RepoNode*> filteredNodes;

	std::copy_if(nodes.begin(), nodes.end(),
		std::back_inserter(filteredNodes),
		[filter](const RepoNode* comp) { return comp->getTypeAsEnum() == filter; }
	);

	return filteredNodes;
}

std::vector<RepoNode*> RepoScene::getAllDescendantsByType(
	const GraphType &gType,
	const repo::lib::RepoUUID  &sharedID,
	const NodeType  &type) const
{
	std::vector<RepoNode*> res;
	const auto children = getChildrenAsNodes(gType, sharedID);
	for (const auto &child : children)
	{
		auto grandChildrenRes = getAllDescendantsByType(gType, child->getSharedID(), type);
		res.insert(res.end(), grandChildrenRes.begin(), grandChildrenRes.end());
		if (child->getTypeAsEnum() == type)
			res.push_back(child);
	}

	return res;
}

std::vector<RepoNode*> RepoScene::getParentNodesFiltered(
	const GraphType &gType,
	const RepoNode* node,
	const NodeType &type) const
{
	std::vector<RepoNode*> results;
	if (node)
	{
		std::vector<repo::lib::RepoUUID> parentIDs = node->getParentIDs();

		for (const repo::lib::RepoUUID &id : parentIDs)
		{
			RepoNode* node = getNodeBySharedID(gType, id);
			if (node && node->getTypeAsEnum() == type)
			{
				results.push_back(node);
			}
		}
	}
	else
	{
		repoError << "Trying to retrieve parent nodes from a null ptr child node";
	}

	return results;
}

std::string RepoScene::getBranchName() const
{
	std::string branchName("master");
	if (revNode)
	{
		branchName = revNode->getName();
		if (branchName.empty())
		{
			branchName = revNode->getUniqueID().toString();
		}
	}

	return branchName;
}

repo::lib::RepoBounds RepoScene::getSceneBoundingBox() const
{
	repo::lib::RepoBounds bbox;
	GraphType gType = stashGraph.rootNode ? GraphType::OPTIMIZED : GraphType::DEFAULT;

	std::vector<float> identity = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1, };

	getSceneBoundingBoxInternal(gType, gType == GraphType::OPTIMIZED ? stashGraph.rootNode : graph.rootNode, identity, bbox);
	return bbox;
}

void RepoScene::getSceneBoundingBoxInternal(
	const GraphType            &gType,
	const RepoNode             *node,
	const repo::lib::RepoMatrix   &mat,
	repo::lib::RepoBounds &bbox) const
{
	if (node)
	{
		switch (node->getTypeAsEnum())
		{
		case NodeType::TRANSFORMATION:
		{
			const TransformationNode *trans = dynamic_cast<const TransformationNode*>(node);
			auto matTransformed = mat * trans->getTransMatrix();

			for (const auto & child : getChildrenAsNodes(gType, trans->getSharedID()))
			{
				getSceneBoundingBoxInternal(gType, child, matTransformed, bbox);
			}
			break;
		}
		case NodeType::MESH:
		{
			const MeshNode *mesh = dynamic_cast<const MeshNode*>(node);
			auto newmBBox = mesh->getBoundingBox();
			MeshNode::transformBoundingBox(newmBBox, mat);
			bbox.encapsulate(newmBBox);
			break;
		}
		case NodeType::REFERENCE:
		{
			repoGraphInstance g = gType == GraphType::DEFAULT ? graph : stashGraph;
			auto refSceneIt = graph.referenceToScene.find(node->getSharedID());
			if (refSceneIt != graph.referenceToScene.end())
			{
				const RepoScene *refScene = refSceneIt->second;
				bbox.encapsulate(refScene->getSceneBoundingBox());
			}
			break;
		}
		}
	}
}

std::set<repo::lib::RepoUUID> RepoScene::getAllSharedIDs(
	const GraphType &gType) const
{
	std::set<repo::lib::RepoUUID> sharedIDs;

	const auto &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;

	boost::copy(
		g.sharedIDtoUniqueID | boost::adaptors::map_keys,
		std::inserter(sharedIDs, sharedIDs.begin()));

	return sharedIDs;
}

std::string RepoScene::getTextureIDForMesh(
	const GraphType &gType,
	const repo::lib::RepoUUID  &sharedID) const
{
	std::vector<RepoNode*> matNodes = getChildrenNodesFiltered(gType, sharedID, NodeType::MATERIAL);

	/*
	* NOTE:
	* This assumes there is only one texture for a mesh.
	* If this is a multipart mesh where there are multiple submesh,
	* one assumes they share the same material -> which has the same texture
	*
	* Similarly, there will not be a mixed texture/non textured sub meshes
	* in a single mesh. Thereofre checking the first MatNode is already sufficient
	* to obtain the texture ID.
	*
	* This also assumes there's a 1 to 1 mapping of texture and materials.
	*
	* This assumption is valid for the current implementation of multipart
	*/

	if (matNodes.size())
	{
		std::vector<RepoNode*> textureNodes = getChildrenNodesFiltered(
			gType, matNodes[0]->getSharedID(), NodeType::TEXTURE);
		if (textureNodes.size())
			return textureNodes[0]->getUniqueID().toString();
	}

	return "";
}

std::vector<std::string> RepoScene::getOriginalFiles() const
{
	if (revNode)
	{
		return revNode->getOrgFiles();
	}

	return refFiles;
}

bool RepoScene::loadRevision(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg,
	const std::vector<ModelRevisionNode::UploadStatus> &includeStatus) {
	bool success = true;

	if (!handler)
	{
		errMsg = "Cannot load revision with an empty database handler";
		return false;
	}

	RepoBSON bson;
	repoTrace << "loading revision : " << databaseName << "." << projectName << " head Revision: " << headRevision;
	if (headRevision) {

		using namespace repo::core::handler::database;

		query::RepoQueryBuilder query;
		if (includeStatus.size())
		{
			std::vector<int> statuses;
			std::transform(includeStatus.begin(), includeStatus.end(), std::back_inserter(statuses), [](auto s) { return (int)s; } );
			query.append(
				query::Or(
					query::Eq(REPO_NODE_REVISION_LABEL_INCOMPLETE, statuses),
					query::Exists(REPO_NODE_REVISION_LABEL_INCOMPLETE, false)
				)
			);
		}
		else
		{
			query.append(
				query::Exists(REPO_NODE_REVISION_LABEL_INCOMPLETE, false)
			);
		}
		query.append(query::Eq(REPO_NODE_LABEL_SHARED_ID, branch));

		bson = handler->findOneByCriteria(
			databaseName,
			projectName + "." + REPO_COLLECTION_HISTORY,
			query,
			REPO_NODE_REVISION_LABEL_TIMESTAMP
		);

		repoTrace << "Fetching head of revision from branch " << branch;
	}
	else {
		bson = handler->findOneByUniqueID(databaseName, projectName + "." + REPO_COLLECTION_HISTORY, revision);
		repoTrace << "Fetching revision using unique ID: " << revision;
	}

	if (bson.isEmpty()) {
		errMsg = "Failed: cannot find revision document from " + databaseName + "." + projectName + "." + REPO_COLLECTION_HISTORY;
		success = false;
	}
	else {
		revNode = new ModelRevisionNode(bson);
		worldOffset = revNode->getCoordOffset();
	}

	return success;
}

bool RepoScene::loadScene(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg) {
	bool success = true;

	if (!handler)
	{
		errMsg = "Cannot load revision with an empty database handler";
		return false;
	}

	if (!revNode) {
		//try to load revision node first.
		if (!loadRevision(handler, errMsg)) return false;
	}

	std::vector<RepoBSON> nodes = handler->findAllByCriteria(
		databaseName, projectName + "." + REPO_COLLECTION_SCENE, core::handler::database::query::Eq(REPO_NODE_STASH_REF, revNode->getUniqueID())
	);

	repoInfo << "# of nodes in this unoptimised scene = " << nodes.size();

	return populate(GraphType::DEFAULT, handler, nodes, errMsg);
}

bool RepoScene::loadStash(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg) {
	bool success = true;
	if (!handler)
	{
		errMsg += "Trying to load stash graph without a database handler!";
		return false;
	}

	if (!revNode) {
		if (!loadRevision(handler, errMsg)) return false;
	}

	std::vector<RepoBSON> nodes = handler->findAllByCriteria(
		databaseName, projectName + "." + REPO_COLLECTION_STASH_REPO, core::handler::database::query::Eq(REPO_NODE_STASH_REF, revNode->getUniqueID())
	);
	if (success = nodes.size())
	{
		repoInfo << "# of nodes in this stash scene = " << nodes.size();
		success = populate(GraphType::OPTIMIZED, handler, nodes, errMsg);
	}
	else
	{
		errMsg += "stash is empty";
	}

	return  success;
}

void RepoScene::removeNode(
	const GraphType                   &gtype,
	const repo::lib::RepoUUID                    &sharedID
)
{
	repoGraphInstance &g = gtype == GraphType::OPTIMIZED ? stashGraph : graph;
	RepoNode *node = getNodeBySharedID(gtype, sharedID);
	if (node)
	{
		//Remove entry from everything.
		g.nodesByUniqueID.erase(node->getUniqueID());
		g.sharedIDtoUniqueID.erase(sharedID);
		g.parentToChildren.erase(sharedID);

		bool keepNode = false;
		if (gtype == GraphType::DEFAULT)
		{
			//If this node was in newAdded or newModified, remove it
			std::set<repo::lib::RepoUUID>::iterator iterator;
			if ((iterator = newAdded.find(sharedID)) != newAdded.end())
			{
				newAdded.erase(iterator);
			}
			else
			{
				if ((iterator = newModified.find(sharedID)) != newModified.end())
				{
					newModified.erase(iterator);
				}

				newRemoved.insert(sharedID);
				keepNode = true;
			}
		}

		//remove from the nodes sets
		switch (node->getTypeAsEnum())
		{
		case NodeType::MATERIAL:
			g.materials.erase(node);
			break;
		case NodeType::MESH:
			g.meshes.erase(node);
			break;
		case NodeType::METADATA:
			g.metadata.erase(node);
			break;
		case NodeType::REFERENCE:
		{
			g.references.erase(node);
			//Since it's reference node, also delete the referenced scene
			RepoScene *s = g.referenceToScene[sharedID];
			delete s;
			g.referenceToScene.erase(sharedID);
		}
		break;
		case NodeType::TEXTURE:
			g.textures.erase(node);
			break;
		case NodeType::TRANSFORMATION:
			g.transformations.erase(node);
			break;
		case NodeType::UNKNOWN:
			g.unknowns.erase(node);
			break;
		default:
			repoError << "Unexpected node type: " << (int)node->getTypeAsEnum();
		}

		if (keepNode)
		{
			//add node onto the toRemove list
			toRemove.push_back(node);
		}
		else
			delete node;
	}
	else
	{
		repoError << "Trying to delete a node that doesn't exist!";
	}
}

bool RepoScene::populate(
	const GraphType &gtype,
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::vector<RepoBSON> nodes,
	std::string &errMsg)
{
	bool success = true;

	repoGraphInstance &g = gtype == GraphType::OPTIMIZED ? stashGraph : graph;

	std::unordered_map<repo::lib::RepoUUID, RepoNode *, repo::lib::RepoUUIDHasher> nodesBySharedID;
	for (std::vector<RepoBSON>::const_iterator it = nodes.begin();
		it != nodes.end(); ++it)
	{
		RepoBSON obj = *it;
		RepoNode *node = NULL;

		std::string nodeType = obj.getStringField(REPO_NODE_LABEL_TYPE);

		if (REPO_NODE_TYPE_TRANSFORMATION == nodeType)
		{
			node = new TransformationNode(obj);
			g.transformations.insert(node);
		}
		else if (REPO_NODE_TYPE_MESH == nodeType)
		{
			node = new MeshNode(obj);
			g.meshes.insert(node);
		}
		else if (REPO_NODE_TYPE_MATERIAL == nodeType)
		{
			node = new MaterialNode(obj);
			g.materials.insert(node);
		}
		else if (REPO_NODE_TYPE_TEXTURE == nodeType)
		{
			node = new TextureNode(obj);
			g.textures.insert(node);
		}
		else if (REPO_NODE_TYPE_REFERENCE == nodeType)
		{
			node = new ReferenceNode(obj);
			g.references.insert(node);
		}
		else if (REPO_NODE_TYPE_METADATA == nodeType)
		{
			node = new MetadataNode(obj);
			g.metadata.insert(node);
		}
		else {
			//UNKNOWN TYPE - instantiate it with generic RepoNode
			node = new RepoNode(obj);
			g.unknowns.insert(node);
		}

		success &= addNodeToMaps(gtype, node, errMsg);
	} //Node Iteration

	//deal with References
	RepoNodeSet::iterator refIt;
	//Make sure it is propagated into the repoScene if it exists in revision node

	if (g.references.size()) worldOffset.clear();
	if (!ignoreReferenceNodes)
	{
		for (const auto &node : g.references)
		{
			ReferenceNode* reference = (ReferenceNode*)node;

			//construct a new RepoScene with the information from reference node and append this g to the Scene
			std::string spDbName = reference->getDatabaseName();
			if (spDbName.empty()) spDbName = databaseName;
			RepoScene *refg = new RepoScene(spDbName, reference->getProjectId());
			if (reference->useSpecificRevision())
				refg->setRevision(reference->getProjectRevision());
			else
				refg->setBranch(reference->getProjectRevision());

			if (!loadExtFiles) {
				refg->skipLoadingExtFiles();
			}

			//Try to load the stash first, if fail, try scene.
			if (loadExtFiles && refg->loadStash(handler, errMsg) || refg->loadScene(handler, errMsg))
			{
				g.referenceToScene[reference->getSharedID()] = refg;
				auto refOffset = refg->getWorldOffset();
				if (!worldOffset.size())
				{
					worldOffset = refOffset;
				}
			}
			else {
				repoWarning << "Failed to load reference node for ref ID " << reference->getUniqueID() << ": " << errMsg;
			}
		}
	}

	repoTrace << "World Offset = [" << worldOffset[0] << " , " << worldOffset[1] << ", " << worldOffset[2] << " ]";
	//Now that we know the world Offset, make sure the referenced scenes are shifted accordingly
	for (const auto &node : g.references)
	{
		ReferenceNode* reference = (ReferenceNode*)node;
		auto parent = reference->getParentIDs().at(0);
		auto refScene = g.referenceToScene[reference->getSharedID()];
		if (refScene)
		{
			auto refOffset = refScene->getWorldOffset();
			//Back to world coord of subProject
			std::vector<std::vector<float>> backToSubWorld =
			{ { 1., 0., 0., (float)refOffset[0] },
			{ 0., 1., 0., (float)refOffset[1] },
			{ 0., 0., 1., (float)refOffset[2] },
			{ 0., 0., 0., 1 } };
			std::vector<std::vector<float>> toFedWorldTrans =
			{ { 1., 0., 0., (float)-worldOffset[0] },
			{ 0., 1., 0., (float)-worldOffset[1] },
			{ 0., 0., 1., (float)-worldOffset[2] },
			{ 0., 0., 0., 1. } };

			//parent - ref
			//Becomes: toFedWorld - parent - toSubWorld - ref

			auto parentNode = getNodeBySharedID(GraphType::DEFAULT, parent);
			auto grandParent = parentNode->getParentIDs().at(0);
			auto grandParentNode = getNodeBySharedID(GraphType::DEFAULT, grandParent);
			auto toFedWorld = new TransformationNode(RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(toFedWorldTrans), "trans", { grandParent }));
			auto toSubWorld = new TransformationNode(RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(backToSubWorld), "trans", { parent }));
			std::vector<RepoNode*> newNodes;
			newNodes.push_back(toFedWorld);
			newNodes.push_back(toSubWorld);
			addNodes(newNodes);
			addInheritance(GraphType::DEFAULT, toSubWorld, reference);
			addInheritance(GraphType::DEFAULT, toFedWorld, parentNode);
			abandonChild(GraphType::DEFAULT, grandParent, parentNode);
			abandonChild(GraphType::DEFAULT, parent, reference);
			newModified.clear(); //We're still loading the scene, there shouldn't be anything here anyway.
		}
	}

	return success;
}

void RepoScene::populateAndUpdate(
	const GraphType   &gType,
	const RepoNodeSet &meshes,
	const RepoNodeSet &materials,
	const RepoNodeSet &metadata,
	const RepoNodeSet &textures,
	const RepoNodeSet &transformations,
	const RepoNodeSet &references,
	const RepoNodeSet &unknowns)
{
	std::string errMsg;
	repoGraphInstance &instance = gType == GraphType::OPTIMIZED ? stashGraph : graph;
	addNodeToScene(gType, meshes, errMsg, &(instance.meshes));
	addNodeToScene(gType, materials, errMsg, &(instance.materials));
	addNodeToScene(gType, metadata, errMsg, &(instance.metadata));
	addNodeToScene(gType, textures, errMsg, &(instance.textures));
	addNodeToScene(gType, transformations, errMsg, &(instance.transformations));
	addNodeToScene(gType, references, errMsg, &(instance.references));
	addNodeToScene(gType, unknowns, errMsg, &(instance.unknowns));
}

void RepoScene::applyScaleFactor(const float &scale) {
	if (graph.rootNode)
	{
		auto rootTrans = dynamic_cast<TransformationNode*>(graph.rootNode);
		std::vector<float> scalingMatrix = {
			scale, 0, 0, 0,
			0, scale, 0, 0,
			0, 0, scale, 0,
			0, 0, 0, 1
		};

		rootTrans->applyTransformation(repo::lib::RepoMatrix(scalingMatrix));

		//Clear the stash as bounding boxes in mesh mappings are no longer valid like this.
		clearStash();

		if (worldOffset.size())
		{
			for (int i = 0; i < worldOffset.size(); ++i) {
				worldOffset[i] *= scale;
			}
		}
	}
}

void RepoScene::reorientateDirectXModel()
{
	//Need to rotate the model by 270 degrees on the X axis
	//This is essentially swapping the 2nd and 3rd column, negating the 2nd.
	repoTrace << "Reorientating model...";
	//Stash root and scene root should be identical!
	if (graph.rootNode)
	{
		auto rootTrans = dynamic_cast<TransformationNode*>(graph.rootNode);

		//change offset relatively
		std::vector<float> rotationMatrix = { 1, 0, 0, 0,
			0, 0, 1, 0,
			0, -1, 0, 0,
			0, 0, 0, 1 };

		rootTrans->applyTransformation(repo::lib::RepoMatrix(rotationMatrix));

		//Clear the stash as bounding boxes in mesh mappings are no longer valid like this.
		clearStash();

		//Apply the rotation on the offset
		if (worldOffset.size())
		{
			auto temp = worldOffset[2];
			worldOffset[2] = -worldOffset[1];
			worldOffset[1] = temp;
		}
	}
}

void RepoScene::resetChangeSet()
{
	newRemoved.clear();
	newModified.clear();
	newAdded.clear();
	newCurrent.clear();
	std::vector<repo::lib::RepoUUID> sharedIds;
	//boost::copy(graph.nodesByUniqueID | boost::adaptors::map_keys, std::back_inserter(newCurrent));
	boost::copy(graph.sharedIDtoUniqueID | boost::adaptors::map_keys, std::back_inserter(sharedIds));
	newAdded.insert(sharedIds.begin(), sharedIds.end());
	revNode = nullptr;
	unRevisioned = true;
	databaseName = projectName = "";
}

void RepoScene::setDatabaseAndProjectName(std::string newDatabaseName, std::string newProjectName)
{
	databaseName = sanitizeDatabaseName(newDatabaseName);
	projectName = sanitizeName(newProjectName);
}

void RepoScene::setWorldOffset(
	const std::vector<double> &offset)
{
	if (offset.size() > 0)
	{
		if (worldOffset.size())
		{
			worldOffset.clear();
		}
		worldOffset.insert(worldOffset.end(), offset.begin(), offset.end());
	}
	else
	{
		repoWarning << "Trying to set world off set with no values. Ignoring...";
	}
}

void RepoScene::shiftModel(
	const std::vector<double> &offset)
{
	std::vector<float> transMat = { 1, 0, 0, (float)offset[0],
		0, 1, 0, (float)offset[1],
		0, 0, 1, (float)offset[2],
		0, 0, 0, 1 };
	if (graph.rootNode)
	{
		graph.rootNode->applyTransformation(transMat);
	}

	if (stashGraph.rootNode)
	{
		stashGraph.rootNode->applyTransformation(transMat);
	}
}

bool RepoScene::updateRevisionStatus(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const ModelRevisionNode::UploadStatus &status)
{
	bool success = false;
	if (revNode)
	{
		revNode->updateStatus(status);
		handler->upsertDocument(databaseName, projectName + "." + REPO_COLLECTION_HISTORY, *revNode, true);
		repoInfo << "rev node status is: " << (int)revNode->getUploadStatus();
		success = true;
	}
	else
	{
		repoError << "Trying to update the status of a revision when the scene is not revisioned!";
	}

	return success;
}