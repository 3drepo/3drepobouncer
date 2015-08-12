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

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>

#include "repo_scene.h"
#include "../../core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::graph;

namespace model = repo::core::model;

RepoScene::RepoScene(
	const std::string &database,
	const std::string &projectName,
	const std::string &sceneExt,
	const std::string &revExt)
	: AbstractGraph(database, projectName),
	sceneExt(sceneExt),
	revExt(revExt),
	headRevision(true),
	unRevisioned(false),
	revNode(0)
{
	//defaults to master branch
	branch = stringToUUID(REPO_HISTORY_MASTER_BRANCH);
	//instantiate scene and revision graph

}

RepoScene::RepoScene(
	const repo::core::model::bson::RepoNodeSet &cameras,
	const repo::core::model::bson::RepoNodeSet &meshes,
	const repo::core::model::bson::RepoNodeSet &materials,
	const repo::core::model::bson::RepoNodeSet &metadata,
	const repo::core::model::bson::RepoNodeSet &textures,
	const repo::core::model::bson::RepoNodeSet &transformations,
	const repo::core::model::bson::RepoNodeSet &references,
	const repo::core::model::bson::RepoNodeSet &maps,
	const repo::core::model::bson::RepoNodeSet &unknowns,
	const std::string                          &sceneExt,
	const std::string                          &revExt)
	: AbstractGraph("", ""),
	sceneExt(sceneExt),
	revExt(revExt),
	headRevision(true),
	unRevisioned(true),
	revNode(0)
{
	branch = stringToUUID(REPO_HISTORY_MASTER_BRANCH);
	populateAndUpdate(cameras, meshes, materials, metadata, textures, transformations, references, maps, unknowns);
}

RepoScene::~RepoScene()
{
	for (auto& pair : nodesByUniqueID)
	{
		delete pair.second;
	}

	if (revNode)
		delete revNode;
}

bool RepoScene::addNodeToScene(
	const repo::core::model::bson::RepoNodeSet nodes, 
	std::string &errMsg,
	 repo::core::model::bson::RepoNodeSet *collection)
{
	bool success = true;
	repo::core::model::bson::RepoNodeSet::iterator nodeIterator;
	if (nodes.size() > 0)
	{

		collection->insert(nodes.begin(), nodes.end());
		for (nodeIterator = nodes.begin(); nodeIterator != nodes.end(); ++nodeIterator)
		{
			model::bson::RepoNode * node = *nodeIterator;
			if (node)
			{

				if (!addNodeToMaps(node, errMsg))
				{
					BOOST_LOG_TRIVIAL(error) << "failed to add node (" << node->getUniqueID() << " to scene graph: " << errMsg;
					success = false;
				}
			}
			newAdded.insert(node->getSharedID());
		}
	}

	

	return success;
}

bool RepoScene::addNodeToMaps(model::bson::RepoNode *node, std::string &errMsg)
{
	bool success = true; 
	repoUUID uniqueID = node->getUniqueID();
	repoUUID sharedID = node->getSharedID();

	//----------------------------------------------------------------------
	//If the node has no parents it must be the rootnode
	if (!node->hasField(REPO_NODE_LABEL_PARENTS)){
		if (!rootNode)
			rootNode = node;
		else{
			//root node already exist, check if they are the same node
			if (rootNode == node){
				//for some reason 2 instance of the root node reside in this scene graph - probably not game breaking.
				BOOST_LOG_TRIVIAL(warning) << "2 instance of the root node found";
			}
			else{
				//2 root nodes?!
				BOOST_LOG_TRIVIAL(error) << "Found 2 root nodes! (" << rootNode->getUniqueID() << " and  " << node->getUniqueID() << ")";
				errMsg = "2 possible candidate for root node found. This is possibly an invalid Scene Graph.";
				//if only one of them is transformation then take that one

				if (node->getTypeAsEnum() == model::bson::NodeType::TRANSFORMATION)
				{
					rootNode = node;
				}
			}
		}
	}
	else{
		//has parent
		std::vector<repoUUID> parentIDs = node->getParentIDs();
		std::vector<repoUUID>::iterator it;
		for (it = parentIDs.begin(); it != parentIDs.end(); ++it)
		{
			//add itself to the parent on the "parent -> children" map
			repoUUID parent = *it;

			//check if the parent already has an entry
			std::map<repoUUID, std::vector<repoUUID>>::iterator mapIt;
			mapIt = parentToChildren.find(parent);
			if (mapIt != parentToChildren.end()){
				//has an entry, add to the vector
				parentToChildren[parent].push_back(sharedID);
			}
			else{
				//no entry, create one
				std::vector<repoUUID> children;
				children.push_back(sharedID);

				parentToChildren[parent] = children;

			}

		}
	} //if (!node->hasField(REPO_NODE_LABEL_PARENTS))

	nodesByUniqueID[uniqueID] = node;
	sharedIDtoUniqueID[sharedID] = uniqueID;

	return success;
}

bool RepoScene::commit(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg, 
	const std::string &userName, 
	const std::string &message,
	const std::string &tag)
{

	bool success = true;

	//Sanity check that everything we need is here
	if (!handler)
	{
		errMsg = "Cannot commit to the database - no database handler assigned.";
		return false;
	}
	
	if (databaseName.empty() | projectName.empty())
	{
		errMsg = "Cannot commit to the database - databaseName or projectName is empty (database: " 
			+ databaseName 
			+ " project: " + projectName +  " ).";
		return false;
	}

	if (success &= commitProjectSettings(handler, errMsg, userName))
	{
		BOOST_LOG_TRIVIAL(info) << "Commited project settings, commiting revision...";
		model::bson::RevisionNode *newRevNode = 0;
		if (!message.empty())
			commitMsg = message;

		if (success &= commitRevisionNode(handler, errMsg, newRevNode, userName, commitMsg, tag))
		{
			BOOST_LOG_TRIVIAL(info) << "Commited revision node, commiting scene nodes...";
			//commited the revision node, commit the modification on the scene
			if (success &= commitSceneChanges(handler, errMsg))
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

				unRevisioned = false;
			}
		}
	}
	//Create and Commit revision node
	return success;
}

bool RepoScene::commitProjectSettings(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg,
	const std::string &userName)
{

	model::bson::RepoProjectSettings projectSettings = 
		model::bson::RepoBSONFactory::makeRepoProjectSettings(projectName, userName);
	
	bool success = handler->upsertDocument(
		databaseName, REPO_COLLECTION_SETTINGS, projectSettings, false, errMsg);
	

	return success;

}

bool RepoScene::commitRevisionNode(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg,
	model::bson::RevisionNode *&newRevNode,
	const std::string &userName,
	const std::string &message,
	const std::string &tag)
{

	bool success = true;
	std::vector<repoUUID> parent;
	parent.reserve(1);

	if (!unRevisioned && !revNode)
	{
		if (!loadRevision(handler, errMsg))
			return false;
		parent.push_back(revNode->getUniqueID());
	}

	std::vector<repoUUID> uniqueIDs;

	//using boost's range adaptor, retrieve all the keys within this map
	//http://www.boost.org/doc/libs/1_53_0/libs/range/doc/html/range/reference/adaptors/reference/map_keys.html
	boost::copy(
		nodesByUniqueID | boost::adaptors::map_keys,
		std::back_inserter(uniqueIDs));


	//convert the sets to vectors


	std::vector<repoUUID> newAddedV(newAdded.begin(), newAdded.end());
	std::vector<repoUUID> newRemovedV(newRemoved.begin(), newRemoved.end());
	std::vector<repoUUID> newModifiedV(newModified.begin(), newModified.end());

	BOOST_LOG_TRIVIAL(trace) << "Committing Revision Node....";

	BOOST_LOG_TRIVIAL(trace) << "New revision: #current = " << uniqueIDs.size() << " #added = " << newAddedV.size()
		<< " #deleted = " << newRemovedV.size() << " #modified = " << newModifiedV.size();

	newRevNode =
		new model::bson::RevisionNode(model::bson::RepoBSONFactory::makeRevisionNode(userName, branch, uniqueIDs,
		newAddedV, newRemovedV, newModifiedV, parent, message, tag));


	if (!newRevNode)
	{
		errMsg += "Failed to instantiate a new revision object!";
		return false;
	}


	return success && handler->insertDocument(databaseName, projectName +"." + revExt, *newRevNode, errMsg);
}

bool RepoScene::commitSceneChanges(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg)
{
	bool success = true;
	std::vector<repoUUID> nodesToCommit;
	std::vector<repoUUID>::iterator it;

	long count = 0;

	nodesToCommit.insert(nodesToCommit.end(), newAdded.begin(), newAdded.end());
	nodesToCommit.insert(nodesToCommit.end(), newModified.begin(), newModified.end());
	nodesToCommit.insert(nodesToCommit.end(), newRemoved.begin(), newRemoved.end());

	BOOST_LOG_TRIVIAL(info) << "Commiting addedNodes...." << newAdded.size() << " nodes";
	
	for (it = nodesToCommit.begin(); it != nodesToCommit.end(); ++it)
	{
	
		model::bson::RepoNode *node = nodesByUniqueID[sharedIDtoUniqueID[*it]];
		if (node->objsize() > handler->documentSizeLimit())
		{
			success = false;
			errMsg += "Node '" + UUIDtoString(node->getUniqueID()) + "' over 16MB in size is not committed.";
		}
		else
			success &= handler->insertDocument(databaseName, projectName + "."+ sceneExt, *node, errMsg);
	}


	return success;
}

std::vector<repo::core::model::bson::RepoNode*> 
RepoScene::getChildrenAsNodes(
 const repoUUID &parent) 
{
	std::vector<repo::core::model::bson::RepoNode*> children;

	std::map<repoUUID, std::vector<repoUUID>>::iterator it = parentToChildren.find(parent);
	if (it != parentToChildren.end())
	{
		for (auto child : it->second)
		{
			children.push_back(nodesByUniqueID[sharedIDtoUniqueID[child]]);
		}
	}
	return children;
}

std::string RepoScene::getBranchName() const
{
	std::string branchName("master");
	if (revNode)
	{
		branchName = revNode->getName();
		if (branchName.empty())
		{
			branchName = UUIDtoString(revNode->getUniqueID());
		}
	}

	return branchName;
}


std::vector<repoUUID> RepoScene::getModifiedNodesID() const
{
	std::vector<repoUUID> ids(newAdded.begin(), newAdded.end());

	ids.insert(ids.end(), newModified.begin(), newModified.end());
	ids.insert(ids.end(), newRemoved.begin() , newRemoved.end());

	return ids;
}

bool RepoScene::loadRevision(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg){
	bool success = true;

	if (!handler)
		return false;

	model::bson::RepoBSON bson;
	if (headRevision){
		bson = handler->findOneBySharedID(databaseName, projectName + "." +
			revExt, branch, REPO_NODE_REVISION_LABEL_TIMESTAMP);
	}
	else{
		bson = handler->findOneByUniqueID(databaseName, projectName + "." + revExt, revision);
	}

	if (bson.isEmpty()){
		errMsg = "Failed: cannot find revision document from " + databaseName + "." + projectName + "." + revExt;
		success = false;
	}
	else{
		revNode = new model::bson::RevisionNode(bson);
	}

	return success;
}

bool RepoScene::loadScene(
	repo::core::handler::AbstractDatabaseHandler *handler, 
	std::string &errMsg){
	bool success = true;
	
	if (!handler) return false;

	if (!revNode){
		//try to load revision node first.
		if (!loadRevision(handler, errMsg)) return false;
	}

	//Get the relevant nodes from the scene graph using the unique IDs stored in this revision node
	model::bson::RepoBSON idArray = revNode->getObjectField(REPO_NODE_REVISION_LABEL_CURRENT_UNIQUE_IDS);
	std::vector<model::bson::RepoBSON> nodes = handler->findAllByUniqueIDs(
		databaseName, projectName + "." + sceneExt, idArray);

	BOOST_LOG_TRIVIAL(info) << "# of nodes in this scene = " << nodes.size();

	return populate(handler, nodes, errMsg);

}

void RepoScene::modifyNode(
	const repoUUID                    &sharedID,
	repo::core::model::bson::RepoNode *node,
	const bool                        &overwrite)
{
	//model::bson::RepoNode* updatedNode = nullptr;
	if (sharedIDtoUniqueID.find(sharedID) != sharedIDtoUniqueID.end())
	{
		model::bson::RepoNode* nodeToChange = nodesByUniqueID[sharedIDtoUniqueID[sharedID]];

		//check if the node is already in the "to modify" list
		bool isInList = newAdded.find(sharedID) != newAdded.end() || newModified.find(sharedID) != newModified.end();

		//generate new UUID if it  is not in list, otherwise use the current one.
		model::bson::RepoNode updatedNode = model::bson::RepoNode(nodeToChange->cloneAndAddFields(node, !isInList));

		if (!isInList)
		{
			newModified.insert(sharedID);
			newCurrent.erase(newCurrent.find(nodeToChange->getUniqueID()));
			newCurrent.insert(updatedNode.getUniqueID());
		}

		nodeToChange->swap(updatedNode);

	}
	else{
		BOOST_LOG_TRIVIAL(error) << "Trying to update a node " << sharedID << " that doesn't exist in the scene!";
	}
	
}


bool RepoScene::populate(
	repo::core::handler::AbstractDatabaseHandler *handler, 
	std::vector<model::bson::RepoBSON> nodes, 
	std::string &errMsg)
{
	bool success = true;

	std::map<repoUUID, model::bson::RepoNode *> nodesBySharedID;
	for (std::vector<model::bson::RepoBSON>::const_iterator it = nodes.begin();
		it != nodes.end(); ++it)
	{
		model::bson::RepoBSON obj = *it;
		model::bson::RepoNode *node = NULL;

		std::string nodeType = obj.getField(REPO_NODE_LABEL_TYPE).str();

		if (REPO_NODE_TYPE_TRANSFORMATION == nodeType)
		{
			node = new model::bson::TransformationNode(obj);
			transformations.insert(node);
		}
		else if (REPO_NODE_TYPE_MESH == nodeType)
		{
			node = new model::bson::MeshNode(obj);
			meshes.insert(node);
		}
		else if (REPO_NODE_TYPE_MATERIAL == nodeType)
		{
			node = new model::bson::MaterialNode(obj);
			materials.insert(node);
		}
		else if (REPO_NODE_TYPE_TEXTURE == nodeType)
		{
			node = new model::bson::TextureNode(obj);
			textures.insert(node);
		}
		else if (REPO_NODE_TYPE_CAMERA == nodeType)
		{
			node = new model::bson::CameraNode(obj);
			cameras.insert(node);
		}
		else if (REPO_NODE_TYPE_REFERENCE == nodeType)
		{
			node = new model::bson::ReferenceNode(obj);
			references.insert(node);
		}
		else if (REPO_NODE_TYPE_METADATA == nodeType)
		{
			node = new model::bson::MetadataNode(obj);
			metadata.insert(node);
		}
		else if (REPO_NODE_TYPE_MAP == nodeType)
		{
			node = new model::bson::MapNode(obj);
			maps.insert(node);
		}
		else{
			//UNKNOWN TYPE - instantiate it with generic RepoNode
			node = new model::bson::RepoNode(obj);
			unknowns.insert(node);
		}

		success &= addNodeToMaps(node, errMsg);

	} //Node Iteration


	//deal with References
	model::bson::RepoNodeSet::iterator refIt;
	for (refIt = references.begin(); refIt != references.end(); ++refIt)
	{
		model::bson::ReferenceNode *reference = (model::bson::ReferenceNode*)*refIt;

		//construct a new RepoScene with the information from reference node and append this graph to the Scene
		RepoScene *refGraph = new RepoScene(databaseName, reference->getProjectName(), sceneExt, revExt);
		refGraph->setRevision(reference->getRevisionID());

		if (refGraph->loadScene(handler, errMsg)){
			referenceToScene[reference->getSharedID()] = refGraph;
		}
		else{
			BOOST_LOG_TRIVIAL(warning) << "Failed to load reference node for ref ID" << reference->getUniqueID();
		}

	}
	return success;
}

void RepoScene::populateAndUpdate(
	const repo::core::model::bson::RepoNodeSet &cameras,
	const repo::core::model::bson::RepoNodeSet &meshes,
	const repo::core::model::bson::RepoNodeSet &materials,
	const repo::core::model::bson::RepoNodeSet &metadata,
	const repo::core::model::bson::RepoNodeSet &textures,
	const repo::core::model::bson::RepoNodeSet &transformations,
	const repo::core::model::bson::RepoNodeSet &references,
	const repo::core::model::bson::RepoNodeSet &maps,
	const repo::core::model::bson::RepoNodeSet &unknowns)
{

	std::string errMsg;

	addNodeToScene(cameras        , errMsg, &(this->cameras));
	addNodeToScene(meshes         , errMsg, &(this->meshes));
	addNodeToScene(materials      , errMsg, &(this->materials));
	addNodeToScene(metadata       , errMsg, &(this->metadata));
	addNodeToScene(textures       , errMsg, &(this->textures));
	addNodeToScene(transformations, errMsg, &(this->transformations));
	addNodeToScene(references     , errMsg, &(this->references));
	addNodeToScene(maps           , errMsg, &(this->maps));
	addNodeToScene(unknowns       , errMsg, &(this->unknowns));

}

void RepoScene::printStatistics(std::iostream &output)
{
	output << "===================Scene Graph Statistics====================" << std::endl;
	output << "Project:\t\t\t\t" << databaseName << "." << projectName << std::endl;
	output << "Scene Graph Extension:\t\t\t" << sceneExt << std::endl;
	output << "Revision Graph Extension:\t\t" << revExt << std::endl << std::endl;
	//use revision node info if available

	if (unRevisioned)
	{
		output << "Revision:\t\t\t\tNot Revisioned" << std::endl;
	}	
	else
	{
		if (revNode)
		{
			output << "Branch:\t\t\t\t\t" << UUIDtoString(revNode->getSharedID()) << std::endl;
			output << "Revision:\t\t\t\t" << UUIDtoString(revNode->getUniqueID()) << std::endl;
		}
		else{
			output << "Branch:\t\t\t\t\t" << UUIDtoString(branch) << std::endl;
			output << "Revision:\t\t\t\t" << (headRevision ? "Head" : UUIDtoString(revision)) << std::endl;
		}
	}
	if (rootNode)
	{
		output << "Scene Graph loaded:\t\t\ttrue" << std::endl;
		output << "\t# Nodes:\t\t\t" << nodesByUniqueID.size() << std::endl;
		output << "\t# ShareToUnique:\t\t" << sharedIDtoUniqueID.size() << std::endl;
		output << "\t# ParentToChildren:\t\t" << parentToChildren.size() << std::endl;
		output << "\t# ReferenceToScene:\t\t" << referenceToScene.size() << std::endl << std::endl;
		output << "\t# Cameras:\t\t\t" << cameras.size() << std::endl;
		output << "\t# Maps:\t\t\t\t" << maps.size() << std::endl;
		output << "\t# Materials:\t\t\t" << materials.size() << std::endl;
		output << "\t# Meshes:\t\t\t" << meshes.size() << std::endl;
		output << "\t# Metadata:\t\t\t" << metadata.size() << std::endl;
		output << "\t# References:\t\t\t" << references.size() << std::endl;
		output << "\t# Textures:\t\t\t" << textures.size() << std::endl;
		output << "\t# Transformations:\t\t" << transformations.size() << std::endl;
		output << "\t# Unknowns:\t\t\t" << unknowns.size() << std::endl << std::endl;

		output << "Uncommitted changes:" << std::endl;
		output << "\tAdded:\t\t\t\t" << newAdded.size() << std::endl;
		output << "\tDeleted:\t\t\t" << newRemoved.size() << std::endl;
		output << "\tModified:\t\t\t" << newModified.size() << std::endl;
	}
	else
	{
		output << "Scene Graph loaded:\t\t\tfalse" << std::endl;
	}

	output << "================End of Scene Graph Statistics================" << std::endl;
}
