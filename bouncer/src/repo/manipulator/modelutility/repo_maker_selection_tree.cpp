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

using namespace repo::manipulator::modelutility; 

SelectionTreeMaker::SelectionTreeMaker(
	const repo::core::model::RepoScene *scene)
	: scene(scene)
{

}

repo::lib::PropertyTree SelectionTreeMaker::generatePTree(
	const repo::core::model::RepoNode            *currentNode,
	std::unordered_map<std::string, 
	        std::pair<std::string, std::string>> &idMaps,
	const std::string                            &currentPath) const
{
	repo::lib::PropertyTree tree;
	if (currentNode)
	{
		std::string idString = UUIDtoString(currentNode->getUniqueID());
		repoUUID sharedID = currentNode->getSharedID();
		std::string childPath = currentPath.empty()? idString : currentPath + "__" + idString;


		auto children = scene->getChildrenAsNodes(repo::core::model::RepoScene::GraphType::DEFAULT, sharedID);
		std::vector<repo::lib::PropertyTree> childrenTrees;

		for (const auto &child : children)
		{
			if (child)
			{
				switch (child->getTypeAsEnum())
				{
				case repo::core::model::NodeType::MESH:
				case repo::core::model::NodeType::TRANSFORMATION:
				case repo::core::model::NodeType::CAMERA:
				case repo::core::model::NodeType::REFERENCE:
					childrenTrees.push_back(generatePTree(child, idMaps, childPath));				
				}

			}
			else
			{
				repoDebug << "Null pointer for child node at generatePTree, current path : " << currentPath;
				repoError << "Unexpected error at selection tree generation, the tree may not be complete.";

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

		if (name.empty())
			name = idString;
				
		tree.addToTree("account", scene->getDatabaseName());
		tree.addToTree("project", scene->getProjectName());
		tree.addToTree("name", name);
		tree.addToTree("path", childPath);
		tree.addToTree("_id", idString);
		tree.addToTree("shared_id", UUIDtoString(sharedID));
		tree.addToTree("children", childrenTrees);
		idMaps[idString] = { name, childPath };
		
		
	}
	else
	{
		repoDebug << "Null pointer at generatePTree, current path : " << currentPath;
		repoError << "Unexpected error at selection tree generation, the tree may not be complete.";

	}

	return tree;
}

std::vector<uint8_t> SelectionTreeMaker::getSelectionTreeAsBuffer() const
{
	auto tree = getSelectionTreeAsPropertyTree();
	std::stringstream ss;
	tree.write_json(ss);
	std::string jsonString = ss.str();
	auto buffer = std::vector<uint8_t>();
	if (!jsonString.empty())
	{				
		size_t byteLength = jsonString.size() * sizeof(*jsonString.data());
		buffer.resize(byteLength);
		memcpy(buffer.data(), jsonString.data(), byteLength);
	}
	else
	{
		repoError << "Failed to write selection tree into the buffer: JSON string is empty.";
	}

	return buffer;
}

repo::lib::PropertyTree  SelectionTreeMaker::getSelectionTreeAsPropertyTree() const
{
	repo::lib::PropertyTree tree;

	repo::core::model::RepoNode *root;
	if (scene && (root = scene->getRoot(repo::core::model::RepoScene::GraphType::DEFAULT)))
	{
		std::unordered_map< std::string, std::pair<std::string, std::string>> map;
		tree.mergeSubTree("nodes", generatePTree(root, map, ""));
		for (const auto pair : map)
		{
			//if there's an entry in maps it must have an entry in paths
			tree.addToTree("idToName." + pair.first, pair.second.first);
			tree.addToTree("idToPath." + pair.first, pair.second.second);
		}
	}
	else
	{
		repoError << "Failed to generate selection tree: scene is empty or default scene is not loaded";
	}

	return tree;
}


SelectionTreeMaker::~SelectionTreeMaker()
{
}
