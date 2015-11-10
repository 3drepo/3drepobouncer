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

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/assign.hpp>

#include <fstream>

#include "repo_scene.h"
#include "../bson/repo_bson_factory.h"
#include "../../../lib/repo_log.h"

using namespace repo::core::model;

const std::vector<std::string> RepoScene::collectionsInProject = { "scene", "stash.3drepo",
"stash.src", "history", "issues", "wayfinder" };

RepoScene::RepoScene(
	const std::string &database,
	const std::string &projectName,
	const std::string &sceneExt,
	const std::string &revExt,
	const std::string &stashExt,
	const std::string &rawExt)
	: AbstractGraph(database, projectName),
	sceneExt(sanitizeName(sceneExt)),
	revExt(sanitizeName(revExt)),
	stashExt(sanitizeName(stashExt)),
	rawExt(sanitizeName(rawExt)),
	headRevision(true),
	unRevisioned(false),
	revNode(0)
{
	graph.rootNode = nullptr;
	stashGraph.rootNode = nullptr;
	//defaults to master branch
	branch = stringToUUID(REPO_HISTORY_MASTER_BRANCH);

}

RepoScene::RepoScene(
	const std::vector<std::string> &refFiles,
	const RepoNodeSet              &cameras,
	const RepoNodeSet              &meshes,
	const RepoNodeSet              &materials,
	const RepoNodeSet              &metadata,
	const RepoNodeSet              &textures,
	const RepoNodeSet              &transformations,
	const RepoNodeSet              &references,
	const RepoNodeSet              &maps,
	const RepoNodeSet              &unknowns,
	const std::string              &sceneExt,
	const std::string              &revExt,
	const std::string              &stashExt,
	const std::string              &rawExt)
	: AbstractGraph("", ""),
	sceneExt(sanitizeName(sceneExt)),
	revExt(sanitizeName(revExt)),
	stashExt(sanitizeName(stashExt)),
	rawExt(sanitizeName(rawExt)),
	headRevision(true),
	unRevisioned(true),
	refFiles(refFiles),
	revNode(0)
{
	graph.rootNode = nullptr;
	stashGraph.rootNode = nullptr;
	branch = stringToUUID(REPO_HISTORY_MASTER_BRANCH);
	populateAndUpdate(GraphType::DEFAULT, cameras, meshes, materials, metadata, textures, transformations, references, maps, unknowns);
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
	const repoUUID  &parent,
	const repoUUID  &child,
	const bool      &modifyNode)
{
	repoGraphInstance g = GraphType::OPTIMIZED == gType ? stashGraph : graph;
	auto pToCIt = g.parentToChildren.find(parent);
	if (pToCIt != g.parentToChildren.end())
	{
		std::vector<repoUUID> children = pToCIt->second;
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

	repoTrace << "modifyNode = " << modifyNode;
	if (modifyNode)
	{
		//Remove parent from the child node
		RepoNode *node = getNodeBySharedID(gType, child);
		if (node)
		{
			//We only need a new unique ID if this graph is revisioned,
			//And the child node in question is not a added/modified node already
			bool needNewId = !unRevisioned
				&& (newAdded.find(child) == newAdded.end()
				|| newModified.find(child) == newModified.end());
			repoUUID oldUUID = node->getUniqueID();
			auto nodeWithoutParent = node->cloneAndRemoveParent(parent, needNewId);

			this->modifyNode(gType, node->getSharedID(), new RepoNode(nodeWithoutParent), true);

		}
	}
}

void RepoScene::addInheritance(
	const GraphType &gType,
	const repoUUID  &parent,
	const repoUUID  &child,
	const bool      &noUpdate)
{
	repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
	//stash has no sense of version control, so only default graph needs to track changes
	bool trackChanges = !noUpdate && gType == GraphType::DEFAULT;

	//check if both nodes exist in the graph
	RepoNode *parentNode = getNodeByUniqueID(gType, parent);
	RepoNode *childNode = getNodeByUniqueID(gType, child);

	if (parentNode && childNode)
	{
		repoUUID parentShareID = parentNode->getSharedID();
		repoUUID childShareID = childNode->getSharedID();


		//add children to parentToChildren mapping
		std::map<repoUUID, std::vector<repoUUID>>::iterator childrenIT =
			g.parentToChildren.find(parentShareID);

		if (childrenIT != g.parentToChildren.end())
		{
			std::vector<repoUUID> &children = childrenIT->second;
			//TODO: use sets for performance?
			auto childrenInd = std::find(children.begin(), children.end(), childShareID);
			if (childrenInd == children.end())
			{
				children.push_back(childShareID);
			}
		}
		else
		{
			g.parentToChildren[parentShareID] = std::vector<repoUUID>();
			g.parentToChildren[parentShareID].push_back(childShareID);
		}


		//add parent to children
		std::vector<repoUUID> parents = childNode->getParentIDs();
		//TODO: use sets for performance?
		auto parentInd = std::find(parents.begin(), parents.end(), parentShareID);
		if (parentInd == parents.end())
		{
			RepoNode childWithParent = childNode->cloneAndAddParent(parentShareID);

			if (trackChanges)
			{
				//this is considered a change on the node, we need to make a new node with new uniqueID
				modifyNode(GraphType::DEFAULT, childShareID, new RepoNode(childWithParent));
			}
			else
			{
				//not tracking, just swap the content
				childNode->swap(childWithParent);
			}

		}
	}
	else
	{
		repoDebug << "Parent: " << UUIDtoString(parent) << " child: " << UUIDtoString(child);
		repoError << "Unable to add parentship:" << (parentNode ? "child" : "parent") << " [" << UUIDtoString(parent) << "] node not found.";
	}

}


void RepoScene::addMetadata(
	RepoNodeSet &metadata,
	const bool  &exactMatch)
{

	std::map<std::string, RepoNode*> transMap;
	//stashed version of the graph does not need to track metadata information
	for (RepoNode* transformation : graph.transformations)
	{
		std::string transformationName = transformation->getName();
		if (!exactMatch)
		{
			transformationName = transformationName.substr(0, transformationName.find(" "));
			std::transform(transformationName.begin(), transformationName.end(), transformationName.begin(), ::toupper);
		}

		transMap[transformationName] = transformation;
	}

	for (RepoNode* meta : metadata)
	{
		// TODO: improve efficiency by storing in std::map
		std::string metaName = meta->getName();
		if (!exactMatch)
			std::transform(metaName.begin(), metaName.end(), metaName.begin(), ::toupper);

		auto nameIt = transMap.find(metaName);

		if (nameIt != transMap.end())
		{
			RepoNode *transformation = nameIt->second;
			repoUUID transSharedID = transformation->getSharedID();
			repoUUID metaSharedID = meta->getSharedID();
			repoUUID metaUniqueID = meta->getUniqueID();

			if (graph.parentToChildren.find(transSharedID) == graph.parentToChildren.end())
				graph.parentToChildren[transSharedID] = std::vector<repoUUID>();

			graph.parentToChildren[transSharedID].push_back(metaSharedID);

			*meta = meta->cloneAndAddParent(transSharedID);


			graph.nodesByUniqueID[metaUniqueID] = meta;
			graph.sharedIDtoUniqueID[metaSharedID] = metaUniqueID;


			//FIXME should move this to a generic add node function...
			newAdded.insert(metaSharedID);
			newCurrent.insert(metaUniqueID);
			graph.metadata.insert(meta);

			repoTrace << "Found pairing transformation! Metadata " << metaName <<  " added into the scene graph.";
		}
		else
		{
			repoWarning << "Did not find a pairing transformation node with the same name : " << metaName;
		}
	}
}


bool RepoScene::addNodeToScene(
	const GraphType &gType,
	const RepoNodeSet nodes,
	std::string &errMsg,
	 RepoNodeSet *collection)
{
	bool success = true;
	RepoNodeSet::iterator nodeIterator;
	if (nodes.size() > 0)
	{

		collection->insert(nodes.begin(), nodes.end());
		for (nodeIterator = nodes.begin(); nodeIterator != nodes.end(); ++nodeIterator)
		{
			RepoNode * node = *nodeIterator;
			if (node)
			{

				if (!addNodeToMaps(gType, node, errMsg))
				{
					repoError << "failed to add node (" << node->getUniqueID() << " to scene graph: " << errMsg;
					success = false;
				}
			}
			if (gType == GraphType::DEFAULT)
				newAdded.insert(node->getSharedID());
		}
	}



	return success;
}

bool RepoScene::addNodeToMaps(
	const GraphType &gType,
	RepoNode *node,
	std::string &errMsg)
{
	bool success = true;
	repoUUID uniqueID = node->getUniqueID();
	repoUUID sharedID = node->getSharedID();


	repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
	//----------------------------------------------------------------------
	//If the node has no parents it must be the rootnode
	if (!node->hasField(REPO_NODE_LABEL_PARENTS)){
		if (!g.rootNode)
			g.rootNode = node;
		else{
			//root node already exist, check if they are the same node
			if (g.rootNode == node){
				//for some reason 2 instance of the root node reside in this scene graph - probably not game breaking.
				repoWarning << "2 instance of the (same) root node found";
			}
			else{
				//found 2 nodes with no parents...
				//they could be straggling materials. Only give an error if both are transformation
				//NOTE: this will fall apart if we ever allow root node to be something other than a transformation.

				if (node->getTypeAsEnum() == NodeType::TRANSFORMATION &&
					g.rootNode->getTypeAsEnum() == NodeType::TRANSFORMATION)
					repoError << "2 candidate for root node found. This is possibly an invalid Scene Graph.";

				if (node->getTypeAsEnum() == NodeType::TRANSFORMATION)
				{
					g.rootNode = node;
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
			mapIt = g.parentToChildren.find(parent);
			if (mapIt != g.parentToChildren.end()){
				//has an entry, add to the vector
				g.parentToChildren[parent].push_back(sharedID);
			}
			else{
				//no entry, create one
				std::vector<repoUUID> children;
				children.push_back(sharedID);

				g.parentToChildren[parent] = children;

			}

		}
	} //if (!node->hasField(REPO_NODE_LABEL_PARENTS))

	g.nodesByUniqueID[uniqueID] = node;
	g.sharedIDtoUniqueID[sharedID] = uniqueID;

	return success;
}

void RepoScene::addStashGraph(
	const RepoNodeSet &cameras,
	const RepoNodeSet &meshes,
	const RepoNodeSet &materials,
	const RepoNodeSet &textures,
	const RepoNodeSet &transformations)
{
	populateAndUpdate(GraphType::OPTIMIZED, cameras, meshes, materials, RepoNodeSet(),
		textures, transformations, RepoNodeSet(), RepoNodeSet(), RepoNodeSet());
}

void RepoScene::clearStash()
{

	for (auto &pair : stashGraph.nodesByUniqueID)
	{
		if (pair.second)
			delete pair.second;
	}

	stashGraph.cameras.clear();
	stashGraph.meshes.clear();
	stashGraph.materials.clear();
	stashGraph.maps.clear();
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
		repoInfo << "Commited project settings, commiting revision...";
		RevisionNode *newRevNode = 0;
		if (!message.empty())
			commitMsg = message;

		if (success &= commitRevisionNode(handler, errMsg, newRevNode, userName, commitMsg, tag))
		{
			repoInfo << "Commited revision node, commiting scene nodes...";
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

				refFiles.clear();
				for (RepoNode* node : toRemove)
				{
					delete node;
				}
				toRemove.clear();
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

	RepoProjectSettings projectSettings =
		RepoBSONFactory::makeRepoProjectSettings(projectName, userName);

	bool success = handler->upsertDocument(
		databaseName, REPO_COLLECTION_SETTINGS, projectSettings, false, errMsg);


	return success;

}

bool RepoScene::commitRevisionNode(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg,
	RevisionNode *&newRevNode,
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
	//revision node should only track non optimised graph members.
	boost::copy(
		graph.nodesByUniqueID | boost::adaptors::map_keys,
		std::back_inserter(uniqueIDs));


	//convert the sets to vectors
	std::vector<repoUUID> newAddedV(newAdded.begin(), newAdded.end());
	std::vector<repoUUID> newRemovedV(newRemoved.begin(), newRemoved.end());
	std::vector<repoUUID> newModifiedV(newModified.begin(), newModified.end());

	repoTrace << "Committing Revision Node....";

	repoTrace << "New revision: #current = " << uniqueIDs.size() << " #added = " << newAddedV.size()
		<< " #deleted = " << newRemovedV.size() << " #modified = " << newModifiedV.size();

	std::vector<std::string> fileNames;
	for (const std::string &name : refFiles)
	{
		boost::filesystem::path filePath(name);
		fileNames.push_back(sanitizeName(filePath.filename().string()));
	}

	newRevNode =
		new RevisionNode(RepoBSONFactory::makeRevisionNode(userName, branch, uniqueIDs,
		newAddedV, newRemovedV, newModifiedV, fileNames, parent, message, tag));

	if (newRevNode)
	{
		//Creation of the revision node will append unique id onto the filename (e.g. <uniqueID>chair.obj)
		//we need to store the file in GridFS under the new name
		std::vector<std::string> newRefFileNames = newRevNode->getOrgFiles();
		for (size_t i = 0; i < refFiles.size(); i++)
		{
			std::ifstream file(refFiles[i], std::ios::binary);

			if (file.is_open())
			{
				//newRefFileNames should be at the same index as the refFile. but double check this!
				std::string gridFSName = newRefFileNames[i];
				if (gridFSName.find(fileNames[i]) == std::string::npos)
				{
					//fileNames[i] is not a substring of newName, try to find it
					for (const std::string &name : newRefFileNames)
					{
						if (gridFSName.find(fileNames[i]) != std::string::npos)
						{
							gridFSName = name;
						}
					}

					if (gridFSName == newRefFileNames[i])
					{
						//could not find the matching name (theoretically this should never happen). Skip this file.
						repoError << "Cannot find matching file name for : " << refFiles[i] << " skipping...";
						continue;
					}
				}


				file.seekg(0, std::ios::end);
				std::streamsize size = file.tellg();
				file.seekg(0, std::ios::beg);

				std::vector<uint8_t> rawFile(size);
				if (file.read((char*)rawFile.data(), size))
				{
					std::string errMsg;
					if (!handler->insertRawFile(databaseName, projectName + "." + rawExt, gridFSName, rawFile, errMsg))
					{
						repoError << "Failed to save original file into the database: " << errMsg;
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


	return success && handler->insertDocument(databaseName, projectName +"." + revExt, *newRevNode, errMsg);
}

bool RepoScene::commitNodes(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const std::vector<repoUUID> &nodesToCommit,
	const GraphType &gType,
	std::string &errMsg)
{
	bool success = true;

	bool isStashGraph = gType == GraphType::OPTIMIZED;
	repoGraphInstance &g = isStashGraph ? stashGraph : graph;
	std::string ext = isStashGraph ? stashExt : sceneExt;

	size_t count = 0;
	size_t total = nodesToCommit.size();

	repoTrace << "Committing " << total << " nodes...";

	for (const repoUUID &id : nodesToCommit)
	{
		if (++count % 100 == 0 || count == total -1)
		{
			repoTrace << "Committing " << id << " (" << count << " of " << total << ")";
		}

		const repoUUID uniqueID = gType == GraphType::OPTIMIZED ? id : g.sharedIDtoUniqueID[id];
		RepoNode *node = g.nodesByUniqueID[uniqueID];
		if (node->objsize() > handler->documentSizeLimit())
		{

			//Try to extract binary data out of the bson to shrink it.
			RepoNode shrunkNode = node->cloneAndShrink();
			if (shrunkNode.objsize() >  handler->documentSizeLimit())
			{
				success = false;
				errMsg += "Node '" + UUIDtoString(node->getUniqueID()) + "' over 16MB in size is not committed.";

			}
			else
			{
				node->swap(shrunkNode);
				success &= handler->insertDocument(databaseName, projectName + "." + ext, *node, errMsg);
			}

		}
		else
			success &= handler->insertDocument(databaseName, projectName + "." + ext, *node, errMsg);
	}

	return success;
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
	//There is nothign to commit on removed nodes
	//nodesToCommit.insert(nodesToCommit.end(), newRemoved.begin(), newRemoved.end());

	
	commitNodes(handler, nodesToCommit, GraphType::DEFAULT, errMsg);


	return success;
}

bool RepoScene::commitStash(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg)
{

	/*
	* Don't bother if:
	* 1. root node is null (not instantiated)
	* 2. revnode is null (unoptimised scene graph needs to be commited first
	*/

	repoUUID rev;
	if (!revNode)
	{
		errMsg += "Revision node not found, make sure the default scene graph is commited";
		return false;
	}
	else
	{
		rev = revNode->getUniqueID();
	}
	if (stashGraph.rootNode)
	{
		//Add rev id onto the stash nodes before committing.
		std::vector<repoUUID> nodes;
		RepoBSONBuilder builder;
		builder.append(REPO_NODE_STASH_REF, rev);
		RepoBSON revID = builder.obj(); // this should be RepoBSON?

		for (auto &pair : stashGraph.nodesByUniqueID)
		{
			nodes.push_back(pair.first);
			*pair.second = pair.second->cloneAndAddFields(&revID, false);

		}

		return commitNodes(handler, nodes, GraphType::OPTIMIZED, errMsg);
	}
	else
	{
		//Not neccessarily an error. Make it visible for debugging purposes
		repoDebug << "Stash graph not commited. Root node is nullptr!";
		return true;
	}

}

std::vector<RepoNode*>
RepoScene::getChildrenAsNodes(
	const GraphType &gType,
	const repoUUID &parent) const
{
	std::vector<RepoNode*> children;
	repoGraphInstance g = GraphType::OPTIMIZED == gType ? stashGraph : graph;
	std::map<repoUUID, std::vector<repoUUID>>::const_iterator it = g.parentToChildren.find(parent);
	if (it != g.parentToChildren.end())
	{
		for (auto child : it->second)
		{
			children.push_back(g.nodesByUniqueID.at(g.sharedIDtoUniqueID.at(child)));
		}
	}
	return children;
}

std::vector<RepoNode*>
RepoScene::getChildrenNodesFiltered(
	const GraphType &gType,
	const repoUUID  &parent,
	const NodeType  &type) const
{
	std::vector<RepoNode*> childrenUnfiltered = getChildrenAsNodes(gType, parent);

	std::vector<RepoNode*> children;

	for (RepoNode* child : childrenUnfiltered)
	{
		if (child && child->getTypeAsEnum() == type)
		{
			children.push_back(child);
		}
	}
	return children;
}

std::vector<RepoNode*> RepoScene::getParentNodesFiltered(
	const GraphType &gType,
	const RepoNode* node, 
	const NodeType &type) const
{
	std::vector<repoUUID> parentIDs = node->getParentIDs();
	std::vector<RepoNode*> results;
	for (const repoUUID &id : parentIDs)
	{
		RepoNode* node = getNodeBySharedID(gType, id);
		if (node && node->getTypeAsEnum() == type)
		{
			results.push_back(node);
		}
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
			branchName = UUIDtoString(revNode->getUniqueID());
		}
	}

	return branchName;
}

//
//std::vector<repoUUID> RepoScene::getModifiedNodesID() const
//{
//	repoTrace << "getting modified nodes...";
//	std::vector<repoUUID> ids(newAdded.begin(), newAdded.end());
//
//	ids.insert(ids.end(), newModified.begin(), newModified.end());
//	ids.insert(ids.end(), newRemoved.begin() , newRemoved.end());
//	repoTrace << "Added: " <<
//		newAdded.size() << " modified: " <<
//		newModified.size() << " removed: " <<
//		newRemoved.size();
//
//	repoTrace << "# modified nodes : " << ids.size();
//	return ids;
//}

std::vector<std::string> RepoScene::getOriginalFiles() const
{

	if (revNode)
	{
		return revNode->getOrgFiles();
	}
	return std::vector<std::string>();
}

bool RepoScene::loadRevision(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg){
	bool success = true;

	if (!handler)
		return false;

	RepoBSON bson;
	repoTrace << "loading revision : " << databaseName << "." << projectName << " head Revision: " << headRevision;
	if (headRevision){
		bson = handler->findOneBySharedID(databaseName, projectName + "." +
			revExt, branch, REPO_NODE_REVISION_LABEL_TIMESTAMP);
		repoTrace << "Fetching head of revision from branch " << UUIDtoString(branch);
	}
	else{
		bson = handler->findOneByUniqueID(databaseName, projectName + "." + revExt, revision);
		repoTrace << "Fetching revision using unique ID: " << UUIDtoString(revision);
	}

	if (bson.isEmpty()){
		errMsg = "Failed: cannot find revision document from " + databaseName + "." + projectName + "." + revExt;
		success = false;
	}
	else{
		revNode = new RevisionNode(bson);
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
	RepoBSON idArray = revNode->getObjectField(REPO_NODE_REVISION_LABEL_CURRENT_UNIQUE_IDS);
	std::vector<RepoBSON> nodes = handler->findAllByUniqueIDs(
		databaseName, projectName + "." + sceneExt, idArray);

	repoInfo << "# of nodes in this unoptimised scene = " << nodes.size();

	return populate(GraphType::DEFAULT, handler, nodes, errMsg);

}

bool RepoScene::loadStash(
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::string &errMsg){
	bool success = true;

	if (!handler)
	{
		errMsg += "Trying to load stash graph without a database handler!";
		return false;
	}

	if (!revNode){
		if (!loadRevision(handler, errMsg)) return false;
	}

	//Get the relevant nodes from the scene graph using the unique IDs stored in this revision node
	RepoBSONBuilder builder;
	builder.append(REPO_NODE_STASH_REF, revNode->getUniqueID());

	std::vector<RepoBSON> nodes = handler->findAllByCriteria(databaseName, projectName + "." + stashExt, builder.obj());

	repoInfo << "# of nodes in this stash scene = " << nodes.size();

	return populate(GraphType::OPTIMIZED, handler, nodes, errMsg) && nodes.size() > 0;

}

void RepoScene::modifyNode(
	const GraphType                   &gtype,
	const repoUUID                    &sharedID,
	RepoNode                          *node,
	const bool						  &overwrite)
{

	repoGraphInstance &g = gtype == GraphType::OPTIMIZED ? stashGraph : graph;
	if (g.sharedIDtoUniqueID.find(sharedID) != g.sharedIDtoUniqueID.end())
	{
		RepoNode* nodeToChange = g.nodesByUniqueID[g.sharedIDtoUniqueID[sharedID]];
		//generate new UUID if it  is not in list, otherwise use the current one.
		RepoNode updatedNode;
		bool isInList = gtype == GraphType::DEFAULT &&
			( newAdded.find(sharedID) != newAdded.end() || newModified.find(sharedID) != newModified.end());
		
		updatedNode = overwrite? *node : RepoNode(nodeToChange->cloneAndAddFields(node, !isInList));

		if (!isInList)
		{
			newModified.insert(sharedID);
			newCurrent.erase(nodeToChange->getUniqueID());
			newCurrent.insert(updatedNode.getUniqueID());

		}


		//update shared to unique ID  and uniqueID to node mapping
		g.sharedIDtoUniqueID[sharedID] = updatedNode.getUniqueID();
		g.nodesByUniqueID.erase(nodeToChange->getUniqueID());
		g.nodesByUniqueID[updatedNode.getUniqueID()] = nodeToChange;

		nodeToChange->swap(updatedNode);


	}
	else{
		repoError << "Trying to update a node " << sharedID << " that doesn't exist in the scene!";
	}

}

void RepoScene::removeNode(
	const GraphType                   &gtype,
	const repoUUID                    &sharedID
	)
{
	repoGraphInstance &g = gtype == GraphType::OPTIMIZED ? stashGraph : graph;
	RepoNode *node = getNodeBySharedID(gtype, sharedID);
	//Remove entry from everything.
	g.nodesByUniqueID.erase(node->getUniqueID());
	g.sharedIDtoUniqueID.erase(sharedID);
	g.parentToChildren.erase(sharedID);
	
	
	bool keepNode = false;
	if (gtype == GraphType::DEFAULT)
	{

		//If this node was in newAdded or newModified, remove it
		std::set<repoUUID>::iterator iterator;
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
	case NodeType::CAMERA:
		g.cameras.erase(node);
		break;
	case NodeType::MAP:
		g.maps.erase(node);
		break;
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


bool RepoScene::populate(
	const GraphType &gtype,
	repo::core::handler::AbstractDatabaseHandler *handler,
	std::vector<RepoBSON> nodes,
	std::string &errMsg)
{
	bool success = true;

	repoGraphInstance &g = gtype == GraphType::OPTIMIZED ? stashGraph : graph;

	std::map<repoUUID, RepoNode *> nodesBySharedID;
	for (std::vector<RepoBSON>::const_iterator it = nodes.begin();
		it != nodes.end(); ++it)
	{
		RepoBSON obj = *it;
		RepoNode *node = NULL;

		std::string nodeType = obj.getField(REPO_NODE_LABEL_TYPE).str();

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
		else if (REPO_NODE_TYPE_CAMERA == nodeType)
		{
			node = new CameraNode(obj);
			g.cameras.insert(node);
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
		else if (REPO_NODE_TYPE_MAP == nodeType)
		{
			node = new MapNode(obj);
			g.maps.insert(node);
		}
		else{
			//UNKNOWN TYPE - instantiate it with generic RepoNode
			node = new RepoNode(obj);
			g.unknowns.insert(node);
		}

		success &= addNodeToMaps(gtype, node, errMsg);

	} //Node Iteration


	//deal with References
	RepoNodeSet::iterator refIt;
	for (const auto &node : g.references)
	{
		ReferenceNode* reference = (ReferenceNode*) node;

		//construct a new RepoScene with the information from reference node and append this g to the Scene
		RepoScene *refg = new RepoScene(databaseName, reference->getProjectName(), sceneExt, revExt);
		if (reference->useSpecificRevision())
			refg->setRevision(reference->getRevisionID());
		else
			refg->setBranch(reference->getRevisionID());

		//Try to load the stash first, if fail, try scene.
		if (refg->loadStash(handler, errMsg) || refg->loadScene(handler, errMsg))
		{
			g.referenceToScene[reference->getSharedID()] = refg;
		}
		else{
			repoWarning << "Failed to load reference node for ref ID" << reference->getUniqueID() << ": " << errMsg;
		}

	}
	return success;
}

void RepoScene::populateAndUpdate(
	const GraphType   &gType,
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

	std::string errMsg;
	repoGraphInstance &instance = gType == GraphType::OPTIMIZED ? stashGraph : graph;
	addNodeToScene(gType, cameras, errMsg, &(instance.cameras));
	addNodeToScene(gType, meshes, errMsg, &(instance.meshes));
	addNodeToScene(gType, materials, errMsg, &(instance.materials));
	addNodeToScene(gType, metadata, errMsg, &(instance.metadata));
	addNodeToScene(gType, textures, errMsg, &(instance.textures));
	addNodeToScene(gType, transformations, errMsg, &(instance.transformations));
	addNodeToScene(gType, references, errMsg, &(instance.references));
	addNodeToScene(gType, maps, errMsg, &(instance.maps));
	addNodeToScene(gType, unknowns, errMsg, &(instance.unknowns));

}


void RepoScene::reorientateDirectXModel()
{
	//Need to rotate the model by 270 degrees on the X axis
	//This is essentially swapping the 2nd and 3rd column, negating the 2nd.

	//Stash root and scene root should be identical!
	if (graph.rootNode)
	{
		TransformationNode rootTrans = TransformationNode(*graph.rootNode);
		std::vector<float> mat =  rootTrans.getTransMatrix();
		if (mat.size() == 16)
		{
			std::vector<std::vector<float>> newMatrix;

			for (uint32_t row = 0; row < 4; row++)
			{
				std::vector<float> matRow;
				
				matRow.push_back( mat[row * 4]);
				matRow.push_back(-mat[row * 4 + 2]);
				matRow.push_back( mat[row * 4 + 1]);
				matRow.push_back( mat[row * 4 + 3]);

				newMatrix.push_back(matRow);
			}



			TransformationNode newRoot = RepoBSONFactory::makeTransformationNode(newMatrix, rootTrans.getName());
			modifyNode(GraphType::DEFAULT, rootTrans.getSharedID(), new TransformationNode(newRoot));
			if (stashGraph.rootNode)
				modifyNode(GraphType::OPTIMIZED, stashGraph.rootNode->getSharedID(), new TransformationNode(newRoot));
		}
		else
		{
			repoError << "Root Transformation is not a 4x4 matrix!";
		}

	}
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
	if (graph.rootNode)
	{
		output << "Scene Graph loaded:\t\t\ttrue" << std::endl;
		output << "\t# Nodes:\t\t\t" << graph.nodesByUniqueID.size() << std::endl;
		output << "\t# ShareToUnique:\t\t" << graph.sharedIDtoUniqueID.size() << std::endl;
		output << "\t# ParentToChildren:\t\t" << graph.parentToChildren.size() << std::endl;
		output << "\t# ReferenceToScene:\t\t" << graph.referenceToScene.size() << std::endl << std::endl;
		output << "\t# Cameras:\t\t\t" << graph.cameras.size() << std::endl;
		output << "\t# Maps:\t\t\t\t" << graph.maps.size() << std::endl;
		output << "\t# Materials:\t\t\t" << graph.materials.size() << std::endl;
		output << "\t# Meshes:\t\t\t" << graph.meshes.size() << std::endl;
		output << "\t# Metadata:\t\t\t" << graph.metadata.size() << std::endl;
		output << "\t# References:\t\t\t" << graph.references.size() << std::endl;
		output << "\t# Textures:\t\t\t" << graph.textures.size() << std::endl;
		output << "\t# Transformations:\t\t" << graph.transformations.size() << std::endl;
		output << "\t# Unknowns:\t\t\t" << graph.unknowns.size() << std::endl << std::endl;

		output << "Uncommitted changes:" << std::endl;
		output << "\tAdded:\t\t\t\t" << newAdded.size() << std::endl;
		output << "\tDeleted:\t\t\t" << newRemoved.size() << std::endl;
		output << "\tModified:\t\t\t" << newModified.size() << std::endl;
	}
	else
	{
		output << "Scene Graph loaded:\t\t\tfalse" << std::endl;
	}
	if (stashGraph.rootNode)
	{
		output << "Stash Graph loaded:\t\t\ttrue" << std::endl;
		output << "\t# Nodes:\t\t\t" << stashGraph.nodesByUniqueID.size() << std::endl;
		output << "\t# ShareToUnique:\t\t" << stashGraph.sharedIDtoUniqueID.size() << std::endl;
		output << "\t# ParentToChildren:\t\t" << stashGraph.parentToChildren.size() << std::endl;
		output << "\t# ReferenceToScene:\t\t" << stashGraph.referenceToScene.size() << std::endl << std::endl;
		output << "\t# Cameras:\t\t\t" << stashGraph.cameras.size() << std::endl;
		output << "\t# Maps:\t\t\t\t" << stashGraph.maps.size() << std::endl;
		output << "\t# Materials:\t\t\t" << stashGraph.materials.size() << std::endl;
		output << "\t# Meshes:\t\t\t" << stashGraph.meshes.size() << std::endl;
		output << "\t# Metadata:\t\t\t" << stashGraph.metadata.size() << std::endl;
		output << "\t# References:\t\t\t" << stashGraph.references.size() << std::endl;
		output << "\t# Textures:\t\t\t" << stashGraph.textures.size() << std::endl;
		output << "\t# Transformations:\t\t" << stashGraph.transformations.size() << std::endl;
		output << "\t# Unknowns:\t\t\t" << stashGraph.unknowns.size() << std::endl << std::endl;
	}
	else
	{
		output << "Stash Graph loaded:\t\t\tfalse" << std::endl;
	}

	output << "================End of Scene Graph Statistics================" << std::endl;
}
