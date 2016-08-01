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

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <fstream>

#include "../../../lib/repo_log.h"
#include "../bson/repo_bson_builder.h"
#include "../bson/repo_bson_factory.h"

using namespace repo::core::model;

const std::vector<std::string> RepoScene::collectionsInProject = { "scene", "scene.files", "scene.chunks", "stash.3drepo", "stash.3drepo.files", "stash.3drepo.chunks", "stash.x3d", "stash.x3d.files",
"stash.json_mpc.files", "stash.json_mpc.chunks", "stash.x3d.chunks", "stash.gltf", "stash.gltf.files", "stash.gltf.chunks", "stash.src", "stash.src.files", "stash.src.chunks", "history",
"history.files", "history.chunks", "issues", "wayfinder" };

RepoScene::RepoScene(
	const std::string &database,
	const std::string &projectName,
	const std::string &sceneExt,
	const std::string &revExt,
	const std::string &stashExt,
	const std::string &rawExt,
	const std::string &issuesExt,
	const std::string &srcExt,
	const std::string &gltfExt,
	const std::string &x3dExt,
	const std::string &jsonExt)
	: AbstractGraph(database, projectName),
	sceneExt(sanitizeExt(sceneExt)),
	revExt(sanitizeExt(revExt)),
	stashExt(sanitizeExt(stashExt)),
	rawExt(sanitizeExt(rawExt)),
	issuesExt(sanitizeExt(issuesExt)),
	srcExt(sanitizeExt(srcExt)),
	gltfExt(sanitizeExt(gltfExt)),
	x3dExt(sanitizeExt(x3dExt)),
	jsonExt(sanitizeExt(jsonExt)),
	headRevision(true),
	unRevisioned(false),
	revNode(0),
	status(0)
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
	const std::string              &rawExt,
	const std::string              &issuesExt,
	const std::string              &srcExt,
	const std::string              &gltfExt,
	const std::string              &x3dExt,
	const std::string              &jsonExt
	)
	: AbstractGraph("", ""),
	sceneExt(sanitizeExt(sceneExt)),
	revExt(sanitizeExt(revExt)),
	stashExt(sanitizeExt(stashExt)),
	rawExt(sanitizeExt(rawExt)),
	issuesExt(sanitizeExt(issuesExt)),
	srcExt(sanitizeExt(srcExt)),
	gltfExt(sanitizeExt(gltfExt)),
	x3dExt(sanitizeExt(x3dExt)),
	jsonExt(sanitizeExt(jsonExt)),
	headRevision(true),
	unRevisioned(true),
	refFiles(refFiles),
	revNode(0),
	status(0)
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
	repoUUID childSharedID = child->getSharedID();

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
		//We only need a new unique ID if this graph is revisioned,
		//And the child node in question is not a added/modified node already
		bool needNewId = !unRevisioned
			&& (newAdded.find(childSharedID) == newAdded.end()
			|| newModified.find(childSharedID) == newModified.end());

		auto nodeWithoutParent = child->cloneAndRemoveParent(parent, needNewId);
		this->modifyNode(gType, child, &nodeWithoutParent, true);
	}
}

void RepoScene::addInheritance(
	const GraphType &gType,
	const RepoNode  *parentNode,
	RepoNode        *childNode,
	const bool      &noUpdate)
{
	repoGraphInstance &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;
	//stash has no sense of version control, so only default graph needs to track changes
	bool trackChanges = !noUpdate && gType == GraphType::DEFAULT;

	if (parentNode && childNode)
	{
		repoUUID parentShareID = parentNode->getSharedID();
		repoUUID childShareID = childNode->getSharedID();

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
		std::vector<repoUUID> parents = childNode->getParentIDs();
		//TODO: use sets for performance?
		auto parentInd = std::find(parents.begin(), parents.end(), parentShareID);
		if (parentInd == parents.end())
		{
			RepoNode childWithParent = childNode->cloneAndAddParent(parentShareID);

			if (trackChanges)
			{
				//this is considered a change on the node, we need to make a new node with new uniqueID
				modifyNode(GraphType::DEFAULT, childNode, &childWithParent);
			}
			else
			{
				//not tracking, just swap the content
				childNode->swap(childWithParent);
			}
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

		repoInfo << "Pushing " << transformationName << " into namesMap";
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

		repoInfo << "Pushing " << name << " into namesMap";
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
			std::vector<repoUUID> parents;
			repoUUID metaSharedID = meta->getSharedID();
			repoUUID metaUniqueID = meta->getUniqueID();
			for (auto &node : nameIt->second)
			{
				if (propagateData && node->getTypeAsEnum() == NodeType::TRANSFORMATION)
				{
					auto meshes = getAllDescendantsByType(GraphType::DEFAULT, node->getSharedID(), NodeType::MESH);
					for (auto &mesh : meshes)
					{
						repoUUID parentSharedID = mesh->getSharedID();
						if (graph.parentToChildren.find(parentSharedID) == graph.parentToChildren.end())
							graph.parentToChildren[parentSharedID] = std::vector<RepoNode*>();

						graph.parentToChildren[parentSharedID].push_back(meta);
						parents.push_back(parentSharedID);
					}
				}
				else{
					repoUUID parentSharedID = node->getSharedID();
					if (graph.parentToChildren.find(parentSharedID) == graph.parentToChildren.end())
						graph.parentToChildren[parentSharedID] = std::vector<RepoNode*>();

					graph.parentToChildren[parentSharedID].push_back(meta);
					parents.push_back(parentSharedID);
				}
			}

			*meta = meta->cloneAndAddParent(parents);

			graph.nodesByUniqueID[metaUniqueID] = meta;
			graph.sharedIDtoUniqueID[metaSharedID] = metaUniqueID;

			//FIXME should move this to a generic add node function...
			newAdded.insert(metaSharedID);
			newCurrent.insert(metaUniqueID);
			graph.metadata.insert(meta);

			repoTrace << "Found pairing transformation! Metadata " << metaName << " added into the scene graph.";
		}
		else
		{
			repoWarning << "Did not find a pairing transformation node with the same name : " << metaName;
		}
	}

	clearStash();
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
			auto mapIt = g.parentToChildren.find(parent);
			if (mapIt != g.parentToChildren.end()){
				//has an entry, add to the vector
				g.parentToChildren[parent].push_back(node);
			}
			else{
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

	if (databaseName.empty() || projectName.empty())
	{
		errMsg = "Cannot commit to the database - databaseName or projectName is empty (database: "
			+ databaseName
			+ " project: " + projectName + " ).";
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
				handler->createCollection(databaseName, projectName + "." + issuesExt);

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
	if (success) updateRevisionStatus(handler, repo::core::model::RevisionNode::UploadStatus::COMPLETE);
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

	bool success = handler->insertDocument(
		databaseName, REPO_COLLECTION_SETTINGS, projectSettings, errMsg);

	if (!success)
	{
		//check that the error occurred because of duplicated index (i.e. there's already an entry for projects)
		RepoBSON criteria = BSON(REPO_LABEL_ID << projectName);

		RepoBSON doc = handler->findOneByCriteria(databaseName, REPO_COLLECTION_SETTINGS, criteria);

		//If it already exist, that's fine.
		success = !doc.isEmpty();
		if (success)
		{
			repoTrace << "This project already has a project settings entry, skipping project settings commit...";
			errMsg.clear();
		}
	}

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

	// Using a more standard transform to cope with use of unordered_map
	for (auto& keyVal : graph.nodesByUniqueID)
	{
		uniqueIDs.push_back(keyVal.first);
	}

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
		/*newAddedV, newRemovedV, newModifiedV,*/ fileNames, parent, worldOffset, message, tag));
	*newRevNode = newRevNode->cloneAndUpdateStatus(RevisionNode::UploadStatus::GEN_DEFAULT);

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

	return success && handler->insertDocument(databaseName, projectName + "." + revExt, *newRevNode, errMsg);
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

	repoInfo << "Committing " << total << " nodes...";

	for (const repoUUID &id : nodesToCommit)
	{
		if (++count % 500 == 0 || count == total - 1)
		{
			repoInfo << "Committing " << count << " of " << total;
		}

		const repoUUID uniqueID = gType == GraphType::OPTIMIZED ? id : g.sharedIDtoUniqueID[id];
		RepoNode *node = g.nodesByUniqueID[uniqueID];
		if (node->objsize() > handler->documentSizeLimit())
		{
			//Try to extract binary data out of the bson to shrink it.
			RepoNode shrunkNode = node->cloneAndShrink();
			if (shrunkNode.objsize() > handler->documentSizeLimit())
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
		updateRevisionStatus(handler, repo::core::model::RevisionNode::UploadStatus::GEN_REPO_STASH);
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

		auto success = commitNodes(handler, nodes, GraphType::OPTIMIZED, errMsg);
		if (success)
			updateRevisionStatus(handler, repo::core::model::RevisionNode::UploadStatus::COMPLETE);

		return success;
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
	const repoGraphInstance &g = GraphType::OPTIMIZED == gType ? stashGraph : graph;
	auto it = g.parentToChildren.find(parent);
	return it != g.parentToChildren.end() ? it->second : std::vector<RepoNode*>();
}

std::vector<RepoNode*>
RepoScene::getChildrenNodesFiltered(
const GraphType &gType,
const repoUUID  &parent,
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
	for (RepoNode* n : nodes)
	{
		if (n && n->getTypeAsEnum() == filter)
		{
			filteredNodes.push_back(n);
		}
	}
	return filteredNodes;
}

std::vector<RepoNode*> RepoScene::getAllDescendantsByType(
	const GraphType &gType,
	const repoUUID  &sharedID,
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

std::vector<repo_vector_t> RepoScene::getSceneBoundingBox() const
{
	std::vector<repo_vector_t> bbox;
	GraphType gType = stashGraph.rootNode ? GraphType::OPTIMIZED : GraphType::DEFAULT;

	std::vector<float> identity = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1, };

	getSceneBoundingBoxInternal(gType, gType == GraphType::OPTIMIZED ? stashGraph.rootNode : graph.rootNode, identity, bbox);
	repoTrace << "Scene bounding box: {" << bbox[0].x << "," << bbox[0].y << "," << bbox[0].z << "}{" << bbox[1].x << "," << bbox[1].y << "," << bbox[1].z << "}";
	return bbox;
}

void RepoScene::getSceneBoundingBoxInternal(
	const GraphType            &gType,
	const RepoNode             *node,
	const std::vector<float>   &mat,
	std::vector<repo_vector_t> &bbox) const
{
	if (node)
	{
		switch (node->getTypeAsEnum())
		{
		case NodeType::TRANSFORMATION:
		{
			const TransformationNode *trans = dynamic_cast<const TransformationNode*>(node);
			auto matTransformed = matMult(mat, trans->getTransMatrix(false));

			for (const auto & child : getChildrenAsNodes(gType, trans->getSharedID()))
			{
				getSceneBoundingBoxInternal(gType, child, matTransformed, bbox);
			}
			break;
		}
		case NodeType::MESH:
		{
			const MeshNode *mesh = dynamic_cast<const MeshNode*>(node);
			//FIXME: can we figure this out from the existing bounding box?
			MeshNode transedMesh = mesh->cloneAndApplyTransformation(mat);
			auto newmBBox = transedMesh.getBoundingBox();

			if (bbox.size())
			{
				if (newmBBox[0].x < bbox[0].x)
					bbox[0].x = newmBBox[0].x;
				if (newmBBox[0].y < bbox[0].y)
					bbox[0].y = newmBBox[0].y;
				if (newmBBox[0].z < bbox[0].z)
					bbox[0].z = newmBBox[0].z;

				if (newmBBox[1].x > bbox[1].x)
					bbox[1].x = newmBBox[1].x;
				if (newmBBox[1].y > bbox[1].y)
					bbox[1].y = newmBBox[1].y;
				if (newmBBox[1].z > bbox[1].z)
					bbox[1].z = newmBBox[1].z;
			}
			else
			{
				//no bbox yet
				bbox.push_back(newmBBox[0]);
				bbox.push_back(newmBBox[1]);
			}

			break;
		}
		case NodeType::REFERENCE:
		{
			repoGraphInstance g = gType == GraphType::DEFAULT ? graph : stashGraph;
			auto refSceneIt = graph.referenceToScene.find(node->getSharedID());
			if (refSceneIt != graph.referenceToScene.end())
			{
				const RepoScene *refScene = refSceneIt->second;
				const std::vector<repo_vector_t> refSceneBbox = refScene->getSceneBoundingBox();

				if (bbox.size())
				{
					if (refSceneBbox[0].x < bbox[0].x)
						bbox[0].x = refSceneBbox[0].x;
					if (refSceneBbox[0].y < bbox[0].y)
						bbox[0].y = refSceneBbox[0].y;
					if (refSceneBbox[0].z < bbox[0].z)
						bbox[0].z = refSceneBbox[0].z;

					if (refSceneBbox[1].x > bbox[1].x)
						bbox[1].x = refSceneBbox[1].x;
					if (refSceneBbox[1].y > bbox[1].y)
						bbox[1].y = refSceneBbox[1].y;
					if (refSceneBbox[1].z > bbox[1].z)
						bbox[1].z = refSceneBbox[1].z;
				}
				else
				{
					//no bbox yet
					bbox.push_back(refSceneBbox[0]);
					bbox.push_back(refSceneBbox[1]);
				}
			}
			break;
		}
		}
	}
}

std::set<repoUUID> RepoScene::getAllSharedIDs(
	const GraphType &gType) const
{
	std::set<repoUUID> sharedIDs;

	const auto &g = gType == GraphType::OPTIMIZED ? stashGraph : graph;

	boost::copy(
		g.sharedIDtoUniqueID | boost::adaptors::map_keys,
		std::inserter(sharedIDs, sharedIDs.begin()));

	return sharedIDs;
}

std::string RepoScene::getTextureIDForMesh(
	const GraphType &gType,
	const repoUUID  &sharedID) const
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
			return UUIDtoString(textureNodes[0]->getUniqueID());
	}

	return "";
}

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
		RepoBSONBuilder critBuilder;
		critBuilder.append(REPO_NODE_LABEL_SHARED_ID, branch);
		critBuilder << REPO_NODE_REVISION_LABEL_INCOMPLETE << BSON("$exists" << false);

		bson = handler->findOneByCriteria(databaseName, projectName + "." +
			revExt, critBuilder.obj(), REPO_NODE_REVISION_LABEL_TIMESTAMP);
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
		worldOffset = revNode->getCoordOffset();
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

void RepoScene::modifyNode(
	const GraphType                   &gtype,
	RepoNode                          *nodeToChange,
	RepoNode                          *newNode,
	const bool						  &overwrite)
{
	if (!nodeToChange || !newNode)
	{
		repoError << "Failed to modify node in scene (node is nullptr)";
		return;
	}
	repoGraphInstance &g = gtype == GraphType::OPTIMIZED ? stashGraph : graph;

	repoUUID sharedID = nodeToChange->getSharedID();

	RepoNode updatedNode;

	//generate new UUID if it  is not in list, otherwise use the current one.
	bool isInList = gtype == GraphType::OPTIMIZED ||
		(newAdded.find(sharedID) != newAdded.end() || newModified.find(sharedID) != newModified.end());
	updatedNode = overwrite ? *newNode : RepoNode(nodeToChange->cloneAndAddFields(newNode, !isInList));

	repoUUID newUniqueID = updatedNode.getUniqueID();

	if (gtype == GraphType::DEFAULT && !isInList)
	{
		newModified.insert(sharedID);
		newCurrent.erase(nodeToChange->getUniqueID());
		newCurrent.insert(newUniqueID);
	}

	//update shared to unique ID  and uniqueID to node mapping
	g.sharedIDtoUniqueID[sharedID] = newUniqueID;
	g.nodesByUniqueID.erase(nodeToChange->getUniqueID());
	g.nodesByUniqueID[newUniqueID] = nodeToChange;

	nodeToChange->swap(updatedNode);
}

void RepoScene::removeNode(
	const GraphType                   &gtype,
	const repoUUID                    &sharedID
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

	std::unordered_map<repoUUID, RepoNode *, RepoUUIDHasher> nodesBySharedID;
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
	//Make sure it is propagated into the repoScene if it exists in revision node
	worldOffset = getWorldOffset();
	for (const auto &node : g.references)
	{
		ReferenceNode* reference = (ReferenceNode*)node;

		//construct a new RepoScene with the information from reference node and append this g to the Scene
		std::string spDbName = reference->getDatabaseName();
		if (spDbName.empty()) spDbName = databaseName;
		RepoScene *refg = new RepoScene(spDbName, reference->getProjectName(), sceneExt, revExt);
		if (reference->useSpecificRevision())
			refg->setRevision(reference->getRevisionID());
		else
			refg->setBranch(reference->getRevisionID());

		//Try to load the stash first, if fail, try scene.
		if (refg->loadStash(handler, errMsg) || refg->loadScene(handler, errMsg))
		{
			g.referenceToScene[reference->getSharedID()] = refg;
			auto refOffset = refg->getWorldOffset();
			if (refOffset.size() >= worldOffset.size())
			{
				for (size_t offIdx = 0; offIdx < worldOffset.size(); ++offIdx)
				{
					if (worldOffset[offIdx] > refOffset[offIdx])
						worldOffset[offIdx] = refOffset[offIdx];
				}
			}
		}
		else{
			repoWarning << "Failed to load reference node for ref ID" << reference->getUniqueID() << ": " << errMsg;
		}
	}
	repoTrace << "World Offset = [" << worldOffset[0] << " , " << worldOffset[1] << ", " << worldOffset[2] << " ]";
	//Now that we know the world Offset, make sure the referenced scenes are shifted accordingly
	for (const auto &node : g.references)
	{
		ReferenceNode* reference = (ReferenceNode*)node;
		auto refScene = g.referenceToScene[reference->getSharedID()];
		auto refOffset = refScene->getWorldOffset();
		std::vector<double> dOffset = { refOffset[0] - worldOffset[0], refOffset[1] - worldOffset[1], refOffset[2] - worldOffset[2] };
		repoTrace << "delta Offset = [" << dOffset[0] << " , " << dOffset[1] << ", " << dOffset[2] << " ]";
		refScene->shiftModel(dOffset);
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
	repoTrace << "Reorientating model...";
	//Stash root and scene root should be identical!
	if (graph.rootNode)
	{
		auto rootTrans = dynamic_cast<TransformationNode*>(graph.rootNode);
		std::vector<float> mat = rootTrans->getTransMatrix(false);
		if (mat.size() == 16)
		{
			//change offset relatively
			std::vector<float> rotationMatrix = { 1, 0, 0, 0,
				0, 0, 1, 0,
				0, -1, 0, 0,
				0, 0, 0, 1 };

			TransformationNode newRoot = rootTrans->cloneAndApplyTransformation(rotationMatrix);
			modifyNode(GraphType::DEFAULT, rootTrans->getSharedID(), &newRoot);

			/*if (stashGraph.rootNode)
			{
			modifyNode(GraphType::OPTIMIZED, stashGraph.rootNode->getSharedID(), &newRoot);
			}*/

			//Clear the stash as bounding boxes in mesh mappings are no longer valid like this.
			clearStash();

			//Apply the rotation on the offset
			auto temp = worldOffset[2];
			worldOffset[2] = -worldOffset[1];
			worldOffset[1] = temp;
		}
		else
		{
			repoError << "Root Transformation is not a 4x4 matrix!";
		}
	}
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
		auto translatedRoot = graph.rootNode->cloneAndApplyTransformation(transMat);
		graph.rootNode->swap(translatedRoot);
	}

	if (stashGraph.rootNode)
	{
		auto translatedRoot = stashGraph.rootNode->cloneAndApplyTransformation(transMat);
		stashGraph.rootNode->swap(translatedRoot);
	}
}

void RepoScene::updateRevisionStatus(
	repo::core::handler::AbstractDatabaseHandler *handler,
	const RevisionNode::UploadStatus &status)
{
	if (revNode)
	{
		auto updatedRev = revNode->cloneAndUpdateStatus(status);

		if (handler)
		{
			//update revision node
			std::string errMsg;
			handler->upsertDocument(databaseName, projectName + "." + revExt, updatedRev, true, errMsg);
		}

		revNode->swap(updatedRev);
		repoInfo << "rev node status is: " << (int)revNode->getUploadStatus();
	}
	else
	{
		repoError << "Trying to update the status of a revision when the scene is not revisioned!";
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