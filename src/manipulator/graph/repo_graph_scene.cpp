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

#include "repo_graph_scene.h"

using namespace repo::manipulator::graph;

namespace model = repo::core::model;

SceneGraph::SceneGraph() : 
	AbstractGraph()
{

}

SceneGraph::SceneGraph(
	repo::core::handler::AbstractDatabaseHandler *dbHandler,
	std::string database,
	std::string projectName,
	std::string sceneExt,
	std::string revExt)
	: AbstractGraph(dbHandler, database, projectName),
	sceneExt(sceneExt),
	revExt(revExt),
	revNode(0)
{
	//defaults to master branch
	branch = stringToUUID(REPO_HISTORY_MASTER_BRANCH);
	//instantiate scene and revision graph

}

SceneGraph::~SceneGraph()
{
	delete revNode;
}

//bool SceneGraph::append(repo_uuid parentID, SceneGraph *scene, std::string &errMsg)
//{
//	/*
//	* FIXME:
//	* This is used for federated projects where we append a scenegraph generated from database
//	* with information based on a reference node.
//	* This can be potential chaos if we decide to commit this scene graph, changing a node from
//	* a difference scene, potentially causing weird-ness with the revision node
//	* 
//	* Probably can remedy this by tracking whether a scene is from this scene graph or 
//	* the appended scene graph via reference node, and track change requests for referenced scenes
//	* and commit those scenes when this scene is commited
//	*
//	*/
//	bool success = true;
//
//	//check parentID exists
//	if (sharedIDtoUniqueID.find(parentID) == sharedIDtoUniqueID.end()){
//		errMsg = "Node with shared ID " + UUIDtoString(parentID) + " does not exist in this scene graph!";
//		return false;
//	}
//	repo_uuid parentUniqueID = sharedIDtoUniqueID[parentID];
//	model::bson::RepoNode *refNode = nodesByUniqueID[parentUniqueID];
//
//	if (refNode && refNode->getField(REPO_NODE_LABEL_TYPE).str() != REPO_NODE_TYPE_REFERENCE)
//	{
//		//Parent is not a reference node, the logic below would not be correct.
//		errMsg = "Node associated with the parent ID is not a reference node.";
//		return false;
//	}
//	
//
//
//	if (scene && refNode)
//	{
//
//		std::vector<repo_uuid> refNodeParentIDs = refNode->getParentIDs();
//		std::vector<repo_uuid>::iterator it;
//		for (it = refNodeParentIDs.begin(); it != refNodeParentIDs.end(); ++it)
//		{
//
//			/*
//			* Tricky Business, We want to essentially replace ref node with the node of this SceneGraph
//			* So the rootNode of this scene we are appending will append to the parent of the reference Node
//			* Adding the root node of this scene to the grandParent's children set
//			* and adding grandParent to the parent of this root node.
//			* Problem is, RepoNodes are BSON objects thus immutable.
//			* so the plan is to replace the rootNode with a new rootNode with these new parents
//			* before we add the root Node to this scene.
//			*/
//			repo_uuid grandParentID = *it;
//			model::bson::RepoNode *grandParent = nodesByUniqueID[grandParentID];
//			parentToChildren[grandParentID].push_back[scene->rootNode->getSharedID()];
//			scene->rootNode->addParent(thisNodeParent);
//		}
//
//		//append the collections from the scene to this scene.
//
//		nodesByUniqueID.insert(
//			scene->nodesByUniqueID.begin(),
//			scene->nodesByUniqueID.end());
//
//		sharedIDtoUniqueID.insert(
//			scene->sharedIDtoUniqueID.begin(),
//			scene->sharedIDtoUniqueID.end());
//
//		parentToChildren.insert(
//			scene->parentToChildren.begin(),
//			scene->parentToChildren.end());
//
//		cameras.insert(cameras.end(), scene->cameras.begin(), scene->cameras.end());
//		meshes.insert(scene->meshes.begin(), scene->meshes.end());
//		materials.insert(materials.end(), scene->materials.begin(), scene->materials.end());
//		maps.insert(materials.end(), scene->maps.begin(), scene->maps.end());
//		metadata.insert(metadata.end(), scene->metadata.begin(), scene->metadata.end());
//		references.insert(references.end(), scene->references.begin(), scene->references.end());
//		textures.insert(textures.end(), scene->textures.begin(), scene->textures.end());
//		transformations.insert(scene->transformations.begin(), scene->transformations.end());
//		unknowns.insert(scene->unknowns.begin(), scene->unknowns.end());
//	}
//	else{
//		success = false;
//		errMsg = "Pointer to Scene is NULL.";
//	}
//	return success;
//}

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

	std::map<repo_uuid, model::bson::RepoNode *> nodesBySharedID;
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

		repo_uuid uniqueID = node->getUniqueID();
		repo_uuid sharedID = node->getSharedID();

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
			std::vector<repo_uuid> parentIDs = node->getParentIDs();
			std::vector<repo_uuid>::iterator it;
			for (it = parentIDs.begin(); it != parentIDs.end(); ++it)
			{
				//add itself to the parent on the "parent -> children" map
				repo_uuid parent = *it;

				//check if the parent already has an entry
				std::map<repo_uuid, std::vector<repo_uuid>>::iterator mapIt;
				mapIt = parentToChildren.find(parent);
				if (mapIt != parentToChildren.end()){
					//has an entry, add to the vector
					parentToChildren[parent].push_back(sharedID);
				}
				else{
					//no entry, create one
					std::vector<repo_uuid> children;
					children.push_back(sharedID);

					parentToChildren[parent] = children;

				}
				
			}
		} //if (!node->hasField(REPO_NODE_LABEL_PARENTS))

		nodesByUniqueID   [uniqueID] = node;
		sharedIDtoUniqueID[sharedID] = uniqueID;

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

void SceneGraph::printStatistics(std::iostream &output)
{
	output << "===================Scene Graph Statistics====================" << std::endl;
	output << "Project:\t\t\t" << databaseName << "." << projectName << std::endl;
	output << "Scene Graph Extension:\t\t" << sceneExt << std::endl;
	output << "Revision Graph Extension:\t" << revExt << std::endl << std::endl;
	//use revision node info if available
	if (revNode)
	{
		output << "Branch:\t\t\t\t" << UUIDtoString(revNode->getSharedID()) << std::endl;
		output << "Revision:\t\t\t" << UUIDtoString(revNode->getUniqueID()) << std::endl;
	}
	else{
		output << "Branch:\t\t\t\t" << UUIDtoString(branch) << std::endl;
		output << "Revision:\t\t\t" << (headRevision? "Head" : UUIDtoString(revision)) << std::endl;
	}
	if (rootNode)
	{
		output << "Scene Graph loaded:\t\ttrue" << std::endl;
		output << "# Nodes:\t\t\t" << nodesByUniqueID.size() << std::endl;
		output << "# ShareToUnique:\t\t" << sharedIDtoUniqueID.size() << std::endl;
		output << "# ParentToChildren:\t\t" << parentToChildren.size() << std::endl;
		output << "# PeferenceToScene:\t\t" << referenceToScene.size() << std::endl << std::endl;
		output << "# Cameras:\t\t\t" << cameras.size() << std::endl;
		output << "# Maps:\t\t\t\t" << maps.size() << std::endl;
		output << "# Materials:\t\t\t" << materials.size() << std::endl;
		output << "# Meshes:\t\t\t" << meshes.size() << std::endl;
		output << "# Metadata:\t\t\t" << metadata.size() << std::endl;
		output << "# References:\t\t\t" << references.size() << std::endl;
		output << "# Textures:\t\t\t" << textures.size() << std::endl;
		output << "# Transformations:\t\t" << transformations.size() << std::endl;
		output << "# Unknowns:\t\t\t" << unknowns.size() << std::endl;

	}
	else
	{
		output << "Scene Graph loaded:\t\tfalse" << std::endl;
	}

	output << "================End of Scene Graph Statistics================" << std::endl;
}
