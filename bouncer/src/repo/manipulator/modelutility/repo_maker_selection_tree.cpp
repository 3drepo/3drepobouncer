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

using namespace repo::manipulator::modelutility;

const static std::string REPO_LABEL_VISIBILITY_STATE = "toggleState";
const static std::string REPO_VISIBILITY_STATE_SHOW = "visible";
const static std::string REPO_VISIBILITY_STATE_HIDDEN = "invisible";
const static std::string REPO_VISIBILITY_STATE_HALF_HIDDEN = "parentOfInvisible";
const static std::string IFC_TYPE_SPACE_LABEL = "(IFC Space)";

SelectionTreeMaker::SelectionTreeMaker(
	const repo::core::model::RepoScene *scene)
	: scene(scene)
{
	generateSelectionTrees();
}

static std::string nodeTypeToString(repo::core::model::NodeType type)
{
	switch (type) {
	case repo::core::model::NodeType::MESH:
		return REPO_NODE_TYPE_MESH;
	case repo::core::model::NodeType::TRANSFORMATION:
		return REPO_NODE_TYPE_TRANSFORMATION;
	case repo::core::model::NodeType::REFERENCE:
		return REPO_NODE_TYPE_REFERENCE;
	case repo::core::model::NodeType::METADATA:
		return REPO_NODE_TYPE_METADATA;
	case repo::core::model::NodeType::MATERIAL:
		return REPO_NODE_TYPE_MATERIAL;
	case repo::core::model::NodeType::REVISION:
		return REPO_NODE_TYPE_REVISION;
	case repo::core::model::NodeType::TEXTURE:
		return REPO_NODE_TYPE_TEXTURE;
	default:
		return "UNKNOWN";
	}
}

static std::string childPathToString(const SelectionTree::ChildPath& path)
{
	std::string result;
	for (size_t i = 0; i < path.size(); ++i) {
		result += path[i].toString();
		if (i != path.size() - 1) {
			result += "__";
		}
	}
	return result;
}

static std::vector<std::string> getAsString(const std::vector<repo::lib::RepoUUID>& uuids)
{
	std::vector<std::string> array;
	for (auto& uuid : uuids) {
		array.push_back(uuid.toString());
	}
	return array;
}

static std::string getAsString(SelectionTree::Node::ToggleState state)
{
	switch (state) {
	case SelectionTree::Node::ToggleState::SHOW:
		return REPO_VISIBILITY_STATE_SHOW;
	case SelectionTree::Node::ToggleState::HIDDEN:
		return REPO_VISIBILITY_STATE_HIDDEN;
	case SelectionTree::Node::ToggleState::HALF_HIDDEN:
		return REPO_VISIBILITY_STATE_HALF_HIDDEN;
	}
	throw new repo::lib::RepoException("Unknown visibility type");
}

SelectionTree::Node SelectionTreeMaker::generatePTree(
	const repo::core::model::RepoNode			*currentNode,
	std::unordered_map<std::string, std::pair<std::string, SelectionTree::ChildPath>> &idMaps,
	IdMap							&uniqueIDToSharedId,
	IdToMeshes									&idToMeshesTree,
	std::vector<repo::lib::RepoUUID>			currentPath,
	bool										&hiddenOnDefault,
	std::vector<repo::lib::RepoUUID>			&hiddenNode,
	std::vector<repo::lib::RepoUUID>			&meshIds) const
{
	SelectionTree::Node tree;
	if (currentNode)
	{
		repo::lib::RepoUUID sharedID = currentNode->getSharedID();

		// currentPath is passed as a copy so we can update it directly

		currentPath.push_back(currentNode->getUniqueID());

		auto children = scene->getChildrenAsNodes(repo::core::model::RepoScene::GraphType::DEFAULT, sharedID);
		std::vector<SelectionTree::Node> childrenTrees;

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
					case repo::core::model::NodeType::REFERENCE:
					{
						bool hiddenChild = false;
						std::vector<repo::lib::RepoUUID> childrenMeshes;
						childrenTrees.push_back(generatePTree(child, idMaps, uniqueIDToSharedId, idToMeshesTree, currentPath, hiddenChild, hiddenNode, childrenMeshes));
						hasHiddenChildren = hasHiddenChildren || hiddenChild;
						meshIds.insert(meshIds.end(), childrenMeshes.begin(), childrenMeshes.end());
					}
					}
				}
				else
				{
					repoDebug << "Null pointer for child node at generatePTree, current path : " << childPathToString(currentPath);
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
				name = (scene->getDatabaseName() == refDb ? "" : (refDb + "/")) + refNode->getProjectId();
			}
		}

		tree.type = currentNode->getTypeAsEnum();
		tree.name = name;
		tree.path = currentPath;
		tree._id = currentNode->getUniqueID();
		tree.shared_id = sharedID;
		tree.children = childrenTrees;
		tree.meta = metaIDs;

		std::string idString = currentNode->getUniqueID().toString();

		if (scene->isHiddenByDefault(currentNode->getUniqueID()) ||
			(name.find(IFC_TYPE_SPACE_LABEL) != std::string::npos
				&& currentNode->getTypeAsEnum() == repo::core::model::NodeType::MESH))
		{
			tree.toggleState = SelectionTree::Node::ToggleState::HIDDEN;
			hiddenOnDefault = true;
			hiddenNode.push_back(currentNode->getUniqueID());
		}
		else if (hiddenOnDefault || hasHiddenChildren)
		{
			hiddenOnDefault = (hiddenOnDefault || hasHiddenChildren);
			tree.toggleState = SelectionTree::Node::ToggleState::HALF_HIDDEN;
		}
		else
		{
			tree.toggleState = SelectionTree::Node::ToggleState::SHOW;
		}

		idMaps[idString] = { name, currentPath };
		uniqueIDToSharedId[currentNode->getUniqueID()] = sharedID;
		if (meshIds.size())
			idToMeshesTree[currentNode->getUniqueID()] = meshIds;
		else if (currentNode->getTypeAsEnum() == repo::core::model::NodeType::MESH) {
			std::vector<repo::lib::RepoUUID> self = { currentNode->getUniqueID() };
			idToMeshesTree[currentNode->getUniqueID()] = self;
		}
	}
	else
	{
		repoDebug << "Null pointer at generatePTree, current path : " << childPathToString(currentPath);
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

void SelectionTreeMaker::generateSelectionTrees()
{
	trees.fullTree.container.account = scene->getDatabaseName();
	trees.fullTree.container.project = scene->getProjectName();

	repo::core::model::RepoNode* root;
	if (scene && (root = scene->getRoot(repo::core::model::RepoScene::GraphType::DEFAULT)))
	{
		std::unordered_map<std::string, std::pair<std::string, SelectionTree::ChildPath>> map;
		IdToMeshes idToMeshes;
		std::vector<repo::lib::RepoUUID> childrenMeshes;
		std::vector<repo::lib::RepoUUID> hiddenNodes;
		bool dummy = false;
		repo::lib::PropertyTree settingsTree, treePathTree, shareIDToUniqueIDMap;
		IdMap  uniqueIdToSharedId;

		trees.fullTree.nodes = generatePTree(root, map, uniqueIdToSharedId, idToMeshes, {}, dummy, hiddenNodes, childrenMeshes);
		trees.idMap = uniqueIdToSharedId;

		for (const auto pair : map)
		{
			trees.idToName[pair.first] = pair.second.first;
			trees.idToPath[pair.first] = pair.second.second;
		}

		trees.idToMeshes = idToMeshes;
		trees.modelSettings.hiddenNodes = hiddenNodes;
	}
	else
	{
		throw new repo::lib::RepoException("Failed to generate selection tree: scene is empty or default scene is not loaded");
	}
}

static repo::lib::PropertyTree getAsPropertyTree(const SelectionTree::Node& node, const SelectionTree& tree) 
{
	repo::lib::PropertyTree ptree;

	ptree.addToTree("account", tree.container.account);
	ptree.addToTree("project", tree.container.project);

	ptree.addToTree("type", nodeTypeToString(node.type));
	if (!node.name.empty()) {
		ptree.addToTree("name", node.name);
	}
	ptree.addToTree("path", childPathToString(node.path));
	ptree.addToTree("_id", node._id.toString());
	ptree.addToTree("shared_id", node.shared_id.toString());
	
	std::vector<repo::lib::PropertyTree> children;
	for (auto& child : node.children) {
		children.push_back(getAsPropertyTree(child, tree));
	}
	ptree.addToTree("children", children);

	if (node.meta.size()) {
		ptree.addToTree("meta", getAsString(node.meta));
	}

	ptree.addToTree(REPO_LABEL_VISIBILITY_STATE, getAsString(node.toggleState));

	return ptree;
}

std::map<std::string, repo::lib::PropertyTree> SelectionTreeMaker::getSelectionTreeAsPropertyTree() const
{
	std::map<std::string, repo::lib::PropertyTree> map;

	repo::lib::PropertyTree fullTree;
	fullTree.mergeSubTree("nodes", getAsPropertyTree(trees.fullTree.nodes, trees.fullTree));
	for (auto& p : trees.idToName) {
		fullTree.addToTree("idToName." + p.first.toString(), p.second);
	}

	repo::lib::PropertyTree treePathTree;
	for (auto& p : trees.idToPath) {
		treePathTree.addToTree("idToPath." + p.first.toString(), childPathToString(p.second));
	}

	repo::lib::PropertyTree shareIDToUniqueIDMap;
	for (auto& p : trees.idMap) {
		shareIDToUniqueIDMap.addToTree("idMap." + p.first.toString(), p.second.toString());
	}

	repo::lib::PropertyTree idToMeshes;
	for (auto& p : trees.idToMeshes) {
		idToMeshes.addToTree(p.first.toString(), getAsString(p.second));
	}

	map["fulltree.json"] = fullTree;
	map["tree_path.json"] = treePathTree;
	map["idMap.json"] = shareIDToUniqueIDMap;
	map["idToMeshes.json"] = idToMeshes;

	if (trees.modelSettings.hiddenNodes.size()) {
		repo::lib::PropertyTree settingsTree;
		settingsTree.addToTree("hiddenNodes", getAsString(trees.modelSettings.hiddenNodes));
		map["modelProperties.json"] = settingsTree;
	}

	return map;
}

SelectionTreeMaker::~SelectionTreeMaker()
{
}