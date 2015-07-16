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

#include "repo_graph_scene.h"
#include "../../core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::graph;

namespace model = repo::core::model;

SceneGraph::SceneGraph() : 
	AbstractGraph()
{

}

SceneGraph::SceneGraph(
	repo::core::handler::AbstractDatabaseHandler *dbHandler,
	const std::string &database,
	const std::string &projectName,
	const std::string &sceneExt,
	const std::string &revExt)
	: AbstractGraph(dbHandler, database, projectName),
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

SceneGraph::SceneGraph(
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
	: AbstractGraph(0, "", ""),
	sceneExt(sceneExt),
	revExt(revExt),
	headRevision(true),
	unRevisioned(true),
	revNode(0)
{
	branch = stringToUUID(REPO_HISTORY_MASTER_BRANCH);
	populateAndUpdate(cameras, meshes, materials, metadata, textures, transformations, references, maps, unknowns);
}

SceneGraph::~SceneGraph()
{
	//TODO: delete nodes instantiated and scene graph instantiated?

	if (revNode)
		delete revNode;
}

bool SceneGraph::addNodeToScene(
	const repo::core::model::bson::RepoNodeSet nodes, 
	std::string &errMsg,
	 repo::core::model::bson::RepoNodeSet *collection)
{
	bool success = true;
	repo::core::model::bson::RepoNodeSet::iterator nodeIterator;
	if (nodes.size() > 0)
	{
		newAdded.reserve(nodes.size());

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
			newAdded.push_back(node->getSharedID());
		}
	}

	

	return success;
}

bool SceneGraph::addNodeToMaps(model::bson::RepoNode *node, std::string &errMsg)
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
				errMsg = "2 possible candidate for root node found. This is an invalid Scene Graph.";
				success = false;
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

bool SceneGraph::commit(
	std::string &errMsg, 
	const std::string &userName, 
	const std::string &message,
	const std::string &tag)
{

	bool success = true;

	//Sanity check that everything we need is here
	if (!dbHandler)
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

	if (success &= commitProjectSettings(errMsg, userName))
	{
		BOOST_LOG_TRIVIAL(info) << "Commited project settings, commiting revision...";
		model::bson::RevisionNode *newRevNode = 0;
		if (success &= commitRevisionNode(errMsg, newRevNode, userName, message, tag))
		{
			BOOST_LOG_TRIVIAL(info) << "Commited revision node, commiting scene nodes...";
			//commited the revision node, commit the modification on the scene
			if (success &= commitSceneChanges(errMsg))
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

				unRevisioned = false;
			}
		}
	}
	//Create and Commit revision node
	return success;
}

bool SceneGraph::commitProjectSettings(
	std::string &errMsg,
	const std::string &userName)
{
	bool success = true;
	model::bson::RepoProjectSettings * projectSettings = 
		model::bson::RepoBSONFactory::makeRepoProjectSettings(projectName, userName);
	
	success = (projectSettings) != 0;
	
	if (success)
	{
		success &= dbHandler->upsertDocument(databaseName, REPO_COLLECTION_SETTINGS, *projectSettings, false, errMsg);
	}

	return success;

}

bool SceneGraph::commitRevisionNode(
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
		if (!loadRevision(errMsg))
			return false;
		parent.push_back(revNode->getUniqueID());
	}

	std::vector<repoUUID> uniqueIDs;

	//using boost's range adaptor, retrieve all the keys within this map
	//http://www.boost.org/doc/libs/1_53_0/libs/range/doc/html/range/reference/adaptors/reference/map_keys.html
	boost::copy(
		nodesByUniqueID | boost::adaptors::map_keys,
		std::back_inserter(uniqueIDs));

	newRevNode =
		model::bson::RepoBSONFactory::makeRevisionNode(userName, branch, uniqueIDs,
		newAdded, newRemoved, newModified, parent, message, tag);


	if (!newRevNode)
	{
		errMsg += "Failed to instantiate a new revision object!";
		return false;
	}


	return success && dbHandler->insertDocument(databaseName, projectName +"." + revExt, *newRevNode, errMsg);
}

bool SceneGraph::commitSceneChanges(
	std::string &errMsg)
{
	bool success = true;
	std::vector<repoUUID> nodesToCommit;
	std::vector<repoUUID>::iterator it;

	long count = 0;
	
	nodesToCommit.reserve(newAdded.size() + newModified.size() + newRemoved.size());

	nodesToCommit.insert(nodesToCommit.end(), newAdded.begin(), newAdded.end());
	nodesToCommit.insert(nodesToCommit.end(), newModified.begin(), newModified.end());
	nodesToCommit.insert(nodesToCommit.end(), newRemoved.begin(), newRemoved.end());

	BOOST_LOG_TRIVIAL(info) << "Commiting addedNodes...." << newAdded.size() << " nodes";
	
	for (it = nodesToCommit.begin(); it != nodesToCommit.end(); ++it)
	{
		model::bson::RepoNode *node = nodesByUniqueID[sharedIDtoUniqueID[*it]];
		if (node->objsize() > dbHandler->documentSizeLimit())
		{
			success = false;
			errMsg += "Node '" + UUIDtoString(node->getUniqueID()) + "' over 16MB in size is not committed.";
		}
		else
			success &= dbHandler->insertDocument(databaseName, projectName + "."+ sceneExt, *node, errMsg);
	}


	return success;
}

bool SceneGraph::loadRevision(std::string &errMsg){
	bool success = true;
	model::bson::RepoBSON bson;
	if (headRevision){
		bson = dbHandler->findOneBySharedID(databaseName, projectName + "." +
			revExt, branch, REPO_NODE_REVISION_LABEL_TIMESTAMP);
	}
	else{
		bson = dbHandler->findOneByUniqueID(databaseName, projectName + "." + revExt, revision);
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

bool SceneGraph::loadScene(std::string &errMsg){
	bool success = true;
	if (!revNode){
		//try to load revision node first.
		if (!loadRevision(errMsg)) return false;
	}

	//Get the relevant nodes from the scene graph using the unique IDs stored in this revision node
	model::bson::RepoBSON idArray = revNode->getObjectField(REPO_NODE_REVISION_LABEL_CURRENT_UNIQUE_IDS);
	std::vector<model::bson::RepoBSON> nodes = dbHandler->findAllByUniqueIDs(
		databaseName, projectName + "." + sceneExt, idArray);

	BOOST_LOG_TRIVIAL(info) << "# of nodes in this scene = " << nodes.size();

	return populate(nodes, errMsg);

}

bool SceneGraph::populate(std::vector<model::bson::RepoBSON> nodes, std::string errMsg)
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

		//construct a new SceneGraph with the information from reference node and append this graph to the Scene
		SceneGraph *refGraph = new SceneGraph(dbHandler, databaseName, reference->getProjectName(), sceneExt, revExt);
		refGraph->setRevision(reference->getRevisionID());

		if (refGraph->loadScene(errMsg)){
			referenceToScene[reference->getSharedID()];
		}
		else{
			BOOST_LOG_TRIVIAL(warning) << "Failed to load reference node for ref ID" << reference->getUniqueID();
		}

	}
	return success;
}

void SceneGraph::populateAndUpdate(
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

void SceneGraph::printStatistics(std::iostream &output)
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
