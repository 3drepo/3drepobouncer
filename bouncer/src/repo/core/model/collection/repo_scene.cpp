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
#include <algorithm>

#include <repo_log.h>
#include "repo/error_codes.h"
#include "repo/lib/repo_exception.h"
#include "repo/core/model/bson/repo_bson_builder.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/model/bson/repo_bson_project_settings.h"
#include "repo/core/handler/database/repo_query.h"

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

void checkParameters(const std::string& teamspace, const std::string& collection, const repo::lib::RepoUUID& revision)
{
	// http://docs.mongodb.org/manual/reference/limits/#naming-restrictions
	if(std::any_of(teamspace.begin(), teamspace.end(), dbNameCheck))
	{
		throw repo::lib::RepoInvalidConfigException("Teamspace name contains invalid characters");
	}

	// http://docs.mongodb.org/manual/reference/limits/#Restriction-on-Collection-Names
	if(std::any_of(collection.begin(), collection.end(), nameCheck))
	{
		throw repo::lib::RepoInvalidConfigException("Collection name contains invalid characters");
	}

	if (!revision)
	{
		throw repo::lib::RepoInvalidConfigException("Revision ID is invalid");
	}
}

RepoScene::RepoScene() :
	teamspace({}),
	container({}),
	revision({}),
	status(0)
{
	graph.rootNode = nullptr;
	stashGraph.rootNode = nullptr;
}

RepoScene::RepoScene(
	const std::string &teamspace,
	const std::string &container,
	const repo::lib::RepoUUID& revId)
	:
	teamspace(teamspace),
	container(container),
	revision(revId),
	status(0)
{
	graph.rootNode = nullptr;
	stashGraph.rootNode = nullptr;
	checkParameters(teamspace, container, revision);
}

RepoScene::RepoScene(
	const RepoNodeSet              &meshes,
	const RepoNodeSet              &materials,
	const RepoNodeSet              &metadata,
	const RepoNodeSet              &textures,
	const RepoNodeSet              &transformations,
	const RepoNodeSet              &unknowns
) :
	teamspace({}),
	container({}),
	revision({}),
	status(0)
{
	graph.rootNode = nullptr;
	stashGraph.rootNode = nullptr;
	populateAndUpdate(GraphType::DEFAULT, meshes, materials, metadata, textures, transformations, unknowns);
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
}

void RepoScene::setRevision(
	const std::string& teamspace,
	const std::string& container,
	const repo::lib::RepoUUID& revision
) {
	this->teamspace = teamspace;
	this->container = container;
	this->revision = revision;
	checkParameters(teamspace, container, revision);
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
	for (auto & node : nodes)
	{
		if (node->getTypeAsEnum() == NodeType::TRANSFORMATION || node->getParentIDs().size()) {
			collection->insert(node);
			addNodeToMaps(gType, node);
		}
		else {
			//Orphaned nodes detected, flag missing nodes
			setMissingNodes();
			continue;
		}
		if (gType == GraphType::DEFAULT) {
			newAdded.insert(node->getSharedID());
		}
	}
	return true;
}

void RepoScene::addNodes(
	const std::vector<RepoNode*>& nodes)
{
	std::string errMsg;
	for (auto& node : nodes)
	{
		addNodeToMaps(GraphType::DEFAULT, node);
		switch (node->getTypeAsEnum())
		{
		case NodeType::MATERIAL:
			graph.materials.insert(node);
			break;
		case NodeType::MESH:
			graph.meshes.insert(node);
			break;
		case NodeType::METADATA:
			graph.metadata.insert(node);
			break;
		case NodeType::TEXTURE:
			graph.textures.insert(node);
			break;
		case NodeType::TRANSFORMATION:
			graph.transformations.insert(node);
			break;
		case NodeType::UNKNOWN:
			graph.unknowns.insert(node);
			break;
		default:
		case NodeType::REVISION:
			throw repo::lib::RepoSceneProcessingException("Cannot add a revision node to the scene graph");
			break;
		}

		newAdded.insert(node->getSharedID());
	}
}

void RepoScene::loadRootNode(
	repo::core::handler::AbstractDatabaseHandler* handler)
{
	using namespace repo::core::handler::database;

	repo::core::handler::database::query::RepoQueryBuilder query;
	query.append(repo::core::handler::database::query::Eq(REPO_LABEL_REVISION, revision));
	query.append(repo::core::handler::database::query::Eq(REPO_LABEL_TYPE, std::string(REPO_NODE_TYPE_TRANSFORMATION)));
	query.append(repo::core::handler::database::query::Exists(REPO_NODE_LABEL_PARENTS, false));

	auto bson = handler->findOneByCriteria(teamspace, container + "." + REPO_COLLECTION_SCENE, query);

	if (bson.isEmpty()) {
		throw repo::lib::RepoSceneProcessingException(std::string("Could not find root node for ") + getDatabaseName() + "." + getProjectName() + "." + revision.toString());
	}

	std::string errMsg;
	addNodeToMaps(GraphType::DEFAULT, new repo::core::model::TransformationNode(bson));
}

void RepoScene::addNodeToMaps(
	const GraphType &gType,
	RepoNode *node)
{
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
			// If the root node has previously been loaded with loadRootNode, it will be here but
			// not in the maps
			if (*g.rootNode != *node)
			{
				//found 2 nodes with no parents...
				//they could be straggling materials. Only give an error if both are transformation
				//NOTE: this will fall apart if we ever allow root node to be something other than a transformation.

				if (node->getTypeAsEnum() == NodeType::TRANSFORMATION &&
					g.rootNode->getTypeAsEnum() == NodeType::TRANSFORMATION)
				{
					throw repo::lib::RepoSceneProcessingException("2 candidate for root node found. This is possibly an invalid Scene Graph.");
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
}

void RepoScene::addStashGraph(
	const RepoNodeSet &meshes,
	const RepoNodeSet &materials,
	const RepoNodeSet &textures,
	const RepoNodeSet &transformations)
{
	populateAndUpdate(GraphType::OPTIMIZED, meshes, materials, RepoNodeSet(),
		textures, transformations, RepoNodeSet());
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
	stashGraph.textures.clear();
	stashGraph.transformations.clear();
	stashGraph.unknowns.clear();
	stashGraph.nodesByUniqueID.clear();
	stashGraph.sharedIDtoUniqueID.clear();
	stashGraph.parentToChildren.clear();

	stashGraph.rootNode = nullptr;
}

void RepoScene::commit(
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler
)
{
	auto handlerPtr = handler.get();

	auto revNode = getRevisionNode(handlerPtr);
	revNode.updateStatus(repo::core::model::ModelRevisionNode::UploadStatus::GEN_DEFAULT);
	if (this->worldOffset.size()) {
		revNode.setCoordOffset(this->worldOffset);
	}
	updateRevisionNode(handlerPtr, revNode);
	
	commitSceneChanges(handlerPtr);

	//Succeed in commiting everything.
	//Reset the state.

	newAdded.clear();
	newRemoved.clear();
	newModified.clear();
	for (RepoNode* node : toRemove)
	{
		delete node;
	}
	toRemove.clear();

	if (frameStates.size()) {
		repoInfo << "Committing sequences...";
		commitSequence(handlerPtr, handler->getFileManager().get());
	}

	// Create and Commit revision node. The new node will have a status of
	// GEN_DEFAULT. The caller should perform the final steps to generate
	// the stash and reset the upload status before the scene will become
	// visible.

	repoInfo << "Success.";
}

void RepoScene::commitSequence(
	repo::core::handler::AbstractDatabaseHandler *handler,
	repo::core::handler::fileservice::FileManager *manager
) {
	if (frameStates.size()) {
		auto sequenceCol = container + "." + REPO_COLLECTION_SEQUENCE;
		sequence.setRevision(revision);
		handler->insertDocument(teamspace, sequenceCol, sequence);
		for (const auto& state : frameStates) {
			manager->uploadFileAndCommit(teamspace, sequenceCol, state.first, state.second);
		}

		auto taskCol = container + "." + REPO_COLLECTION_TASK;
		for (const auto &task : tasks) {
			handler->insertDocument(teamspace, taskCol, task);
		}

		if (taskList.size()) {
			manager->uploadFileAndCommit(teamspace, taskCol, sequence.getUniqueId().toString(), taskList);
		}
	}
}

void RepoScene::addErrorStatusToProjectSettings(
	repo::core::handler::AbstractDatabaseHandler *handler
)
{
	auto projectsettings = RepoProjectSettings(handler->findOneByUniqueID(teamspace, REPO_COLLECTION_SETTINGS, container));
	projectsettings.setErrorStatus();
	handler->upsertDocument(teamspace, REPO_COLLECTION_SETTINGS, projectsettings, false);
}

void RepoScene::addTimestampToProjectSettings(
	repo::core::handler::AbstractDatabaseHandler *handler
)
{
	auto projectsettings = RepoProjectSettings(handler->findOneByUniqueID(teamspace, REPO_COLLECTION_SETTINGS, container));
	projectsettings.clearErrorStatus();
	handler->upsertDocument(teamspace, REPO_COLLECTION_SETTINGS, projectsettings, false);
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

	handler->insertManyDocuments(teamspace, container + "." + ext, nodes);

	return true;
}

void RepoScene::commitSceneChanges(
	repo::core::handler::AbstractDatabaseHandler *handler)
{
	if (newAdded.size()) {
		repoInfo << "Committing " << newAdded.size() << " nodes...";

		std::vector<repo::core::model::RepoBSON> nodes;
		for (auto& id : newAdded) {
			auto node = graph.nodesByUniqueID[graph.sharedIDtoUniqueID[id]];
			node->setRevision(revision);
			nodes.push_back(*node);
		}

		handler->insertManyDocuments(teamspace, container + "." + REPO_COLLECTION_SCENE, nodes);
	}

	if (newModified.size()) {
		repoInfo << "Updating " << newModified.size() << " nodes...";

		for (auto& id : newModified) {
			auto node = graph.nodesByUniqueID[graph.sharedIDtoUniqueID[id]];
			node->setRevision(revision);
			handler->upsertDocument(teamspace, container + "." + REPO_COLLECTION_SCENE, *node, false);
		}
	}
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

repo::lib::RepoBounds RepoScene::getSceneBoundingBox() const
{
	repo::lib::RepoBounds bbox;
	GraphType gType = stashGraph.rootNode ? GraphType::OPTIMIZED : GraphType::DEFAULT;

	std::vector<float> identity = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};

	getSceneBoundingBoxInternal(gType, gType == GraphType::OPTIMIZED ? stashGraph.rootNode : graph.rootNode, identity, bbox);
	return bbox;
}

void RepoScene::getSceneBoundingBoxInternal(
	const GraphType& gType,
	const RepoNode* node,
	const repo::lib::RepoMatrix& mat,
	repo::lib::RepoBounds& bbox) const
{
	if (!node) {
		throw repo::lib::RepoSceneProcessingException("Cannot calculate bounding box of a null node");
	}
	switch (node->getTypeAsEnum())
	{
	case NodeType::TRANSFORMATION:
		{
			const TransformationNode* trans = dynamic_cast<const TransformationNode*>(node);
			auto matTransformed = mat * trans->getTransMatrix();

			for (const auto& child : getChildrenAsNodes(gType, trans->getSharedID()))
			{
				getSceneBoundingBoxInternal(gType, child, matTransformed, bbox);
			}
			break;
		}
	case NodeType::MESH:
		{
			const MeshNode* mesh = dynamic_cast<const MeshNode*>(node);
			auto newmBBox = mesh->getBoundingBox();
			MeshNode::transformBoundingBox(newmBBox, mat);
			bbox.encapsulate(newmBBox);
			break;
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

void RepoScene::loadScene(
	repo::core::handler::AbstractDatabaseHandler *handler)
{
	if (!handler) {
		throw repo::lib::RepoException("Cannot load revision with an empty database handler");
	}

	if (!revision) {
		throw repo::lib::RepoException("Cannot load empty revision");
	}

	std::vector<RepoBSON> nodes = handler->findAllByCriteria(
		teamspace, container + "." + REPO_COLLECTION_SCENE, core::handler::database::query::Eq(REPO_NODE_STASH_REF, revision), loadExtFiles
	);

	repoInfo << "# of nodes in this unoptimised scene = " << nodes.size();

	populate(GraphType::DEFAULT, handler, nodes);
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

void RepoScene::populate(
	const GraphType &gtype,
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::vector<RepoBSON> nodes)
{
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

		addNodeToMaps(gtype, node);
	} //Node Iteration
}

void RepoScene::populateAndUpdate(
	const GraphType   &gType,
	const RepoNodeSet &meshes,
	const RepoNodeSet &materials,
	const RepoNodeSet &metadata,
	const RepoNodeSet &textures,
	const RepoNodeSet &transformations,
	const RepoNodeSet &unknowns)
{
	std::string errMsg;
	repoGraphInstance &instance = gType == GraphType::OPTIMIZED ? stashGraph : graph;
	addNodeToScene(gType, meshes, errMsg, &(instance.meshes));
	addNodeToScene(gType, materials, errMsg, &(instance.materials));
	addNodeToScene(gType, metadata, errMsg, &(instance.metadata));
	addNodeToScene(gType, textures, errMsg, &(instance.textures));
	addNodeToScene(gType, transformations, errMsg, &(instance.transformations));
	addNodeToScene(gType, unknowns, errMsg, &(instance.unknowns));
}

void RepoScene::applyScaleFactor(const double& scale) {

	if(graph.rootNode == nullptr) {
		throw repo::lib::RepoException("Must create or load the root node before scaling.");
	}

	auto rootTrans = dynamic_cast<TransformationNode*>(graph.rootNode);
	std::vector<double> scalingMatrix = {
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

	newModified.insert(rootTrans->getSharedID());
}

void RepoScene::reorientateDirectXModel()
{
	if (!graph.rootNode) {
		throw repo::lib::RepoException("Must create or load the root node before reorientating.");
	}

	//Need to rotate the model by 270 degrees on the X axis
	//This is essentially swapping the 2nd and 3rd column, negating the 2nd.
	//Stash root and scene root should be identical!

	auto rootTrans = dynamic_cast<TransformationNode*>(graph.rootNode);

	//change offset relatively
	std::vector<double> rotationMatrix = {
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, -1, 0, 0,
		0, 0, 0, 1
	};

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

	newModified.insert(rootTrans->getSharedID());
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

void RepoScene::updateRevisionStatus(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const ModelRevisionNode::UploadStatus &status)
{
	auto node = getRevisionNode(handler);
	node.updateStatus(status);
	updateRevisionNode(handler, node);
}

ModelRevisionNode RepoScene::getRevisionNode(
	repo::core::handler::AbstractDatabaseHandler* handler)
{
	auto bson = handler->findOneByUniqueID(
		teamspace,
		container + "." + REPO_COLLECTION_HISTORY,
		revision
	);
	ModelRevisionNode node(bson);
	return node;
}

void RepoScene::updateRevisionNode(
	repo::core::handler::AbstractDatabaseHandler* handler,
	ModelRevisionNode& revNode)
{
	handler->upsertDocument(
		teamspace,
		container + "." + REPO_COLLECTION_HISTORY,
		revNode,
		false
	);
	if (revNode.getUploadStatus() == ModelRevisionNode::UploadStatus::COMPLETE)
	{
		handler->updateDocument(
			teamspace,
			container + "." + REPO_COLLECTION_HISTORY,
			repo::core::handler::database::query::RemoveField(revision, REPO_NODE_REVISION_LABEL_INCOMPLETE)
		);
	}
}