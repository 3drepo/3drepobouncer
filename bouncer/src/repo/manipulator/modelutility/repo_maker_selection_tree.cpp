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
#include "repo/core/model/bson/repo_node_reference.h"

#define RAPIDJSON_HAS_STDSTRING 1

#include "repo/manipulator/modelutility/rapidjson/rapidjson.h"
#include "repo/manipulator/modelutility/rapidjson/document.h"
#include "repo/manipulator/modelutility/rapidjson/writer.h"
#include "repo/manipulator/modelutility/rapidjson/stringbuffer.h"

using namespace repo::manipulator::modelutility;

#define REPO_LABEL_VISIBILITY_STATE "toggleState"
#define REPO_VISIBILITY_STATE_SHOW "visible"
#define REPO_VISIBILITY_STATE_HIDDEN "invisible"
#define REPO_VISIBILITY_STATE_HALF_HIDDEN "parentOfInvisible"

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
		rapidjson::StringBuffer b;
		rapidjson::Writer<rapidjson::StringBuffer> writer(b);
		tree.second.Accept(writer);

		std::string jsonString = b.GetString();
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

static rapidjson::Value getAsPropertyTree(const SelectionTree::Node& node, const SelectionTree& tree, rapidjson::Document::AllocatorType& allocator)
{
	rapidjson::Value ptree(rapidjson::kObjectType);

	ptree.AddMember("account", tree.container.account, allocator);
	ptree.AddMember("project", tree.container.project, allocator);

	ptree.AddMember("type", nodeTypeToString(node.type), allocator);
	if (!node.name.empty()) {
		ptree.AddMember("name", node.name, allocator);
	}
	ptree.AddMember("path", childPathToString(node.path), allocator);
	ptree.AddMember("_id", node._id.toString(), allocator);
	ptree.AddMember("shared_id", node.shared_id.toString(), allocator);
	
	rapidjson::Value children(rapidjson::kArrayType);
	for (auto& child : node.children) {
		children.PushBack(getAsPropertyTree(child, tree, allocator), allocator);
	}
	ptree.AddMember("children", children, allocator);

	if (node.meta.size()) {
		rapidjson::Value meta(rapidjson::kArrayType);
		for (const auto& metaId : node.meta) {
			meta.PushBack(rapidjson::Value(metaId.toString(), allocator).Move(), allocator);
		}
		ptree.AddMember("meta", meta, allocator);
	}

	ptree.AddMember(REPO_LABEL_VISIBILITY_STATE, getAsString(node.toggleState), allocator);

	return ptree;
}

std::map<std::string, rapidjson::Document> SelectionTreeMaker::getSelectionTreeAsPropertyTree() const
{
	std::map<std::string, rapidjson::Document> map;

	{
		rapidjson::Document fullTree;
		fullTree.SetObject();
		rapidjson::Document::AllocatorType& allocator = fullTree.GetAllocator();

		fullTree.AddMember("nodes", getAsPropertyTree(trees.fullTree.nodes, trees.fullTree, allocator), allocator);

		rapidjson::Value idToName(rapidjson::kObjectType);
		for (auto& p : trees.idToName) {
			idToName.AddMember(
				rapidjson::Value(p.first.toString(), allocator).Move(), 
				rapidjson::Value(p.second, allocator).Move(),
				allocator);
		}
		fullTree.AddMember("idToName", idToName, allocator);

		map["fulltree.json"] = std::move(fullTree);
	}
	
	{
		rapidjson::Document treePathTree;
		treePathTree.SetObject();
		rapidjson::Document::AllocatorType& allocator = treePathTree.GetAllocator();

		rapidjson::Value idToPath(rapidjson::kObjectType);
		for (auto& p : trees.idToPath) {
			idToPath.AddMember(
				rapidjson::Value(p.first.toString(), allocator).Move(),
				rapidjson::Value(childPathToString(p.second), allocator).Move(),
				allocator);
		}

		treePathTree.AddMember("idToPath", idToPath, allocator);

		map["tree_path.json"] = std::move(treePathTree);
	}

	{
		rapidjson::Document shareIDToUniqueIDMap;
		shareIDToUniqueIDMap.SetObject();
		rapidjson::Document::AllocatorType& allocator = shareIDToUniqueIDMap.GetAllocator();

		rapidjson::Value idMap(rapidjson::kObjectType);
		for (auto& p : trees.idMap) {
			idMap.AddMember(
				rapidjson::Value(p.first.toString(), allocator).Move(),
				rapidjson::Value(p.second.toString(), allocator).Move(),
				allocator);
		}

		shareIDToUniqueIDMap.AddMember("idMap", idMap, allocator);

		map["idMap.json"] = std::move(shareIDToUniqueIDMap);
	}

	{
		rapidjson::Document idToMeshes;
		idToMeshes.SetObject();
		rapidjson::Document::AllocatorType& allocator = idToMeshes.GetAllocator();

		for (auto& p : trees.idToMeshes) {
			rapidjson::Value meshArray(rapidjson::kArrayType);
			for (const auto& meshId : p.second) {
				meshArray.PushBack(rapidjson::Value(meshId.toString(), allocator).Move(), allocator);
			}

			idToMeshes.AddMember(
				rapidjson::Value(p.first.toString(), allocator).Move(),
				meshArray, 
				allocator);
		}

		map["idToMeshes.json"] = std::move(idToMeshes);
	}


	if (trees.modelSettings.hiddenNodes.size()) {

		rapidjson::Document settingsTree;
		settingsTree.SetObject();
		rapidjson::Document::AllocatorType& allocator = settingsTree.GetAllocator();

		rapidjson::Value hiddenNodes(rapidjson::kArrayType);
		for(auto& node : trees.modelSettings.hiddenNodes) {
			hiddenNodes.PushBack(rapidjson::Value(node.toString(), allocator).Move(), allocator);
		}

		settingsTree.AddMember("hiddenNodes", hiddenNodes, allocator);

		map["modelProperties.json"] = std::move(settingsTree);
	}

	return map;
}

SelectionTreeMaker::~SelectionTreeMaker()
{
}