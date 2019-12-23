/**
*  Copyright (C) 2016 3D Repo Ltd
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
#include "repo_maker_selection_tree.h"
#include "../../core/model/bson/repo_node_reference.h"
#include "../modeloptimizer/repo_optimizer_ifc.h"

using namespace repo::manipulator::modelutility;

const static std::string REPO_LABEL_VISIBILITY_STATE = "toggleState";
const static std::string REPO_VISIBILITY_STATE_SHOW = "visible";
const static std::string REPO_VISIBILITY_STATE_HIDDEN = "invisible";
const static std::string REPO_VISIBILITY_STATE_HALF_HIDDEN = "parentOfInvisible";

SelectionTreeMaker::SelectionTreeMaker(
	const repo::core::model::RepoScene *scene)
	: scene(scene)
{
}

repo::lib::PropertyTree SelectionTreeMaker::generatePTree(
	const repo::core::model::RepoNode            *currentNode,
	std::unordered_map < std::string, std::pair < std::string, std::string >> &idMaps,
	std::vector<std::pair<std::string, std::string>>        &sharedIDToUniqueID,
	repo::lib::PropertyTree                      &idToMeshesTree,
	const std::string                            &currentPath,
	bool                                         &hiddenOnDefault,
	std::vector<std::string>                     &hiddenNode,
	std::vector<std::string>                     &meshIds) const
{
	repo::lib::PropertyTree tree;
	if (currentNode)
	{
		
		std::string idString = currentNode->getUniqueID().toString();

		repo::lib::RepoUUID sharedID = currentNode->getSharedID();
		std::string childPath = currentPath.empty() ? idString : currentPath + "__" + idString;

		auto children = scene->getChildrenAsNodes(repo::core::model::RepoScene::GraphType::DEFAULT, sharedID);
		std::vector<repo::lib::PropertyTree> childrenTrees;

		std::vector<repo::core::model::RepoNode*> childrenTypes[2];
		std::vector<repo::lib::RepoUUID> metaIDs;
		for (const auto &child : children)
		{
			if (child->getTypeAsEnum() == repo::core::model::NodeType::METADATA)
			{
				metaIDs.push_back(child->getUniqueID());
			}
			else
			{
				//Ensure IFC Space (if any) are put into the tree first.
				if (child->getName().find(IFC_TYPE_SPACE_LABEL) != std::string::npos)
					childrenTypes[0].push_back(child);
				else
					childrenTypes[1].push_back(child);

			}

		}

		bool hasHiddenChildren = false;
		for (const auto &childrenSet : childrenTypes)
		{
			for (const auto &child : childrenSet)
			{
				if (child)
				{
					switch (child->getTypeAsEnum())
					{
					case repo::core::model::NodeType::MESH:
						meshIds.push_back(child->getUniqueID().toString());
					case repo::core::model::NodeType::TRANSFORMATION:
					case repo::core::model::NodeType::CAMERA:
					case repo::core::model::NodeType::REFERENCE:
					{
						bool hiddenChild = false;
						std::vector<std::string> childrenMeshes;
						childrenTrees.push_back(generatePTree(child, idMaps, sharedIDToUniqueID, idToMeshesTree, childPath, hiddenChild, hiddenNode, childrenMeshes));
						hasHiddenChildren = hasHiddenChildren || hiddenChild;
						meshIds.insert(meshIds.end(), childrenMeshes.begin(), childrenMeshes.end());
					}
					}
				}
				else
				{
					repoDebug << "Null pointer for child node at generatePTree, current path : " << currentPath;
					repoError << "Unexpected error at selection tree generation, the tree may not be complete.";
				}
			}
		}
		std::string name = currentNode->getName();
		if (repo::core::model::NodeType::REFERENCE == currentNode->getTypeAsEnum())
		{
			if (auto refNode = dynamic_cast<const repo::core::model::ReferenceNode*>(currentNode))
			{
				auto refDb = refNode->getDatabaseName();
				name = (scene->getDatabaseName() == refDb ? "" : (refDb + "/")) + refNode->getProjectName();
			}
		}

		tree.addToTree("account", scene->getDatabaseName());
		tree.addToTree("project", scene->getProjectName());
		tree.addToTree("type", currentNode->getType());
		if (!name.empty())
			tree.addToTree("name", name);
		tree.addToTree("path", childPath);
		tree.addToTree("_id", idString);
		tree.addToTree("shared_id", sharedID.toString());
		tree.addToTree("children", childrenTrees);
		if (metaIDs.size())
			tree.addToTree("meta", metaIDs);

		if (scene->isHiddenByDefault(currentNode->getUniqueID()) ||
			(name.find(IFC_TYPE_SPACE_LABEL) != std::string::npos
			&& currentNode->getTypeAsEnum() == repo::core::model::NodeType::MESH))
		{
			tree.addToTree(REPO_LABEL_VISIBILITY_STATE, REPO_VISIBILITY_STATE_HIDDEN);
			hiddenOnDefault = true;
			hiddenNode.push_back(idString);
		}
		else if (hiddenOnDefault || hasHiddenChildren)
		{
			hiddenOnDefault = (hiddenOnDefault || hasHiddenChildren);
			tree.addToTree(REPO_LABEL_VISIBILITY_STATE, REPO_VISIBILITY_STATE_HALF_HIDDEN);
		}
		else
		{
			tree.addToTree(REPO_LABEL_VISIBILITY_STATE, REPO_VISIBILITY_STATE_SHOW);
		}

		idMaps[idString] = { name, childPath };
		sharedIDToUniqueID.push_back({ idString, sharedID.toString() });
		if (meshIds.size())
			idToMeshesTree.addToTree(idString, meshIds);
		else if (currentNode->getTypeAsEnum() == repo::core::model::NodeType::MESH){
			std::vector<repo::lib::RepoUUID> self = { currentNode->getUniqueID() };
			idToMeshesTree.addToTree(idString, self);
		}
	}
	else
	{
		repoDebug << "Null pointer at generatePTree, current path : " << currentPath;
		repoError << "Unexpected error at selection tree generation, the tree may not be complete.";
	}

	return tree;
}

std::map<std::string, std::vector<uint8_t>> SelectionTreeMaker::getSelectionTreeAsBuffer() const
{
	auto trees = getSelectionTreeAsPropertyTree();
	std::map<std::string, std::vector<uint8_t>> buffer;
	for (const auto &tree : trees)
	{
		std::stringstream ss;
		tree.second.write_json(ss);
		std::string jsonString = ss.str();
		if (!jsonString.empty())
		{
			size_t byteLength = jsonString.size() * sizeof(*jsonString.data());
			buffer[tree.first] = std::vector<uint8_t>();
			buffer[tree.first].resize(byteLength);
			memcpy(buffer[tree.first].data(), jsonString.data(), byteLength);
		}
		else
		{
			repoError << "Failed to write selection tree into the buffer: JSON string is empty.";
		}
	}

	return buffer;
}

std::map<std::string, repo::lib::PropertyTree>  SelectionTreeMaker::getSelectionTreeAsPropertyTree() const
{
	std::map<std::string, repo::lib::PropertyTree> trees;

	repo::core::model::RepoNode *root;
	if (scene && (root = scene->getRoot(repo::core::model::RepoScene::GraphType::DEFAULT)))
	{
		std::unordered_map< std::string, std::pair<std::string, std::string>> map;
		std::vector<std::string> hiddenNodes, childrenMeshes;
		bool dummy = false;
		repo::lib::PropertyTree tree, settingsTree, treePathTree, shareIDToUniqueIDMap, idToMeshes;
		std::vector<std::pair<std::string, std::string>>  sharedIDToUniqueID; 
		tree.mergeSubTree("nodes", generatePTree(root, map, sharedIDToUniqueID, idToMeshes, "", dummy, hiddenNodes, childrenMeshes));
		for (const auto pair : map)
		{
			//if there's an entry in maps it must have an entry in paths
			tree.addToTree("idToName." + pair.first, pair.second.first);			
			treePathTree.addToTree("idToPath." + pair.first, pair.second.second);
		}
		for (const auto pair : sharedIDToUniqueID)
		{
			shareIDToUniqueIDMap.addToTree("idMap." + pair.first, pair.second);
		}

		trees["fulltree.json"] = tree;
		trees["tree_path.json"] = treePathTree;
		trees["idMap.json"] = shareIDToUniqueIDMap;
		trees["idToMeshes.json"] = idToMeshes;

		if (hiddenNodes.size())
		{
			settingsTree.addToTree("hiddenNodes", hiddenNodes);
			trees["modelProperties.json"] = settingsTree;
		}
	}
	else
	{
		repoError << "Failed to generate selection tree: scene is empty or default scene is not loaded";
	}

	return trees;
}

SelectionTreeMaker::~SelectionTreeMaker()
{
}