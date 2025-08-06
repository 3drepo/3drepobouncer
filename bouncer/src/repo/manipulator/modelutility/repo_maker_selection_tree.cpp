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
#include "repo/core/handler/database/repo_query.h"
#include "repo/core/model/bson/repo_bson.h"

using namespace repo::manipulator::modelutility;

#define REPO_LABEL_VISIBILITY_STATE "toggleState"
#define REPO_VISIBILITY_STATE_SHOW "visible"
#define REPO_VISIBILITY_STATE_HIDDEN "invisible"
#define REPO_VISIBILITY_STATE_HALF_HIDDEN "parentOfInvisible"

const static std::string IFC_TYPE_SPACE_LABEL = "(IFC Space)";

SelectionTreeMaker::SelectionTreeMaker(
	const repo::core::model::RepoScene *scene,
	repo::core::handler::AbstractDatabaseHandler* handler)
	: scene(scene),
	handler(handler)
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

static std::string toggleStateToString(SelectionTree::Node::ToggleState state)
{
	switch (state) {
	case SelectionTree::Node::ToggleState::SHOW:
		return REPO_VISIBILITY_STATE_SHOW;
	case SelectionTree::Node::ToggleState::HIDDEN:
		return REPO_VISIBILITY_STATE_HIDDEN;
	case SelectionTree::Node::ToggleState::HALF_HIDDEN:
		return REPO_VISIBILITY_STATE_HALF_HIDDEN;
	}
	throw repo::lib::RepoException("Unknown visibility type");
}

void traversePtree(
	SelectionTree::Node* node, 
	bool& hasHiddenChildren,
	SelectionTree::ChildPath currentPath,
	std::vector<repo::lib::RepoUUID>& meshIds,
	const repo::core::model::RepoScene* scene,
	SelectionTreesSet& trees) 
{
    //Ensure IFC Space (if any) are put into the tree first.

	std::sort(node->children.begin(), node->children.end(),
		[](const SelectionTree::Node* a, const SelectionTree::Node* b) {
			return a->name.find(IFC_TYPE_SPACE_LABEL) != std::string::npos && b->name.find(IFC_TYPE_SPACE_LABEL) == std::string::npos;
		}
	);

	// currentPath is passed as a copy so we can update it directly

	currentPath.push_back(node->_id);
	node->path = currentPath;

	hasHiddenChildren = false;

	for (auto& child : node->children) {

		bool childIsHidden = false;
		std::vector<repo::lib::RepoUUID> childMeshes;
		traversePtree(child, childIsHidden, currentPath, childMeshes, scene, trees);
		hasHiddenChildren |= childIsHidden;
		meshIds.insert(meshIds.end(), childMeshes.begin(), childMeshes.end());
	}

	if (scene->isHiddenByDefault(node->_id) ||
		(node->name.find(IFC_TYPE_SPACE_LABEL) != std::string::npos
			&& node->type == repo::core::model::NodeType::MESH))
	{
		node->toggleState = SelectionTree::Node::ToggleState::HIDDEN;
		trees.modelSettings.hiddenNodes.push_back(node->_id);
		hasHiddenChildren = true;
	}
	else if (hasHiddenChildren)
	{
		node->toggleState = SelectionTree::Node::ToggleState::HALF_HIDDEN;
	}
	else
	{
		node->toggleState = SelectionTree::Node::ToggleState::SHOW;
	}

	if (node->type == repo::core::model::NodeType::MESH) {
		meshIds.push_back(node->_id);
	}

	trees.idToPath[node->_id] = currentPath;
	trees.idToMeshes[node->_id] = meshIds;
	trees.idToName[node->_id] = node->name;
	trees.idMap[node->_id] = node->shared_id;
}

void SelectionTreeMaker::generateSelectionTrees()
{
	trees.fullTree.container.account = scene->getDatabaseName();
	trees.fullTree.container.project = scene->getProjectName();

	repo::core::handler::database::query::RepoQueryBuilder filter;
	filter.append(repo::core::handler::database::query::Eq(REPO_NODE_REVISION_ID, scene->getRevisionID()));

	// This method builds the tree in stages. In the first the minimal number of
	// fields to fill out the tree are read for all nodes in the scene collection.
	// 
	// Once this is done - and the memory of the map will no longer change - the
	// inheritence information is used to create edges between the nodes for quick
	// traversal.

	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.includeField(REPO_NODE_LABEL_ID);
	projection.includeField(REPO_NODE_LABEL_SHARED_ID);
	projection.includeField(REPO_NODE_LABEL_PARENTS);
	projection.includeField(REPO_NODE_LABEL_TYPE);
	projection.includeField(REPO_NODE_REFERENCE_LABEL_OWNER);
	projection.includeField(REPO_NODE_REFERENCE_LABEL_PROJECT);

	auto sceneCollection = scene->getProjectName() + "." + REPO_COLLECTION_SCENE;
	auto cursor = handler->findCursorByCriteria(scene->getDatabaseName(), sceneCollection, filter, projection);

	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher> uniqueIdToParentSharedId;
	std::unordered_map<repo::lib::RepoUUID, std::vector<repo::lib::RepoUUID>, repo::lib::RepoUUIDHasher> metadataUniqueIdToParentsSharedIds;
	std::unordered_map<repo::lib::RepoUUID, SelectionTree::Node*, repo::lib::RepoUUIDHasher> sharedIdToNode;

	std::set<repo::lib::RepoUUID, repo::lib::RepoUUIDHasher> metadataUuids;

	for (auto bson : (*cursor)) {
		repo::core::model::RepoNode repo(bson);
		auto type = repo::core::model::RepoNode::parseType(
			bson.getStringField(REPO_NODE_LABEL_TYPE)
		);
		switch (type) {
		case repo::core::model::NodeType::MESH:
		case repo::core::model::NodeType::TRANSFORMATION:
		case repo::core::model::NodeType::REFERENCE:
		{
			SelectionTree::Node node;
			node.type = type;
			node.name = repo.getName();
			node._id = repo.getUniqueID();
			node.shared_id = repo.getSharedID();

			// The Selection tree is a directed acyclic tree, so nodes may only have
			// one parent.
			// parentIds are always the shared_ids, as is the convention in the database.
			if (repo.getParentIDs().size()) {
				uniqueIdToParentSharedId[node._id] = repo.getParentIDs()[0];
			}

			if (node.type == repo::core::model::NodeType::REFERENCE) {

				// For reference nodes, communicate the endpoint via the name...
				repo::core::model::ReferenceNode refNode(bson);
				auto refDb = refNode.getDatabaseName();
				node.name = (scene->getDatabaseName() == refDb ? "" : (refDb + "/")) + refNode.getProjectId();
			}

			trees.fullTree.nodes.push_back(node);
		}
		break;
		case repo::core::model::NodeType::METADATA:
		{
			// Metadata entries may have more than one parent.
			// parentIds are always the shared_ids, as is the convention in the database.
			metadataUniqueIdToParentsSharedIds[repo.getUniqueID()] = repo.getParentIDs();
		}
		break;
		}
	}

	// Now that all nodes have been read, the nodes map will not change, so we can
	// safely take references to its members.

	for (auto& node : trees.fullTree.nodes) {
		sharedIdToNode[node.shared_id] = &node;
	}

	for (auto& node : trees.fullTree.nodes) {
		auto it = sharedIdToNode.find(uniqueIdToParentSharedId[node._id]);
		if (it != sharedIdToNode.end()) {
			it->second->children.push_back(&node);
		}
		else {
			trees.fullTree.root = &node;
		}
	}

	for (auto& p : metadataUniqueIdToParentsSharedIds) {
		auto& metadataUniqueId = p.first;
		for (auto& parentSharedId : p.second) {
			sharedIdToNode[parentSharedId]->meta.push_back(metadataUniqueId);
		}
	}

	// Now that the tree is fully connected, we can update inter-dependent states
	// such as the visibility.

	// traversePtree is recursive and uses the following two parameters to pass
	// information back between levels, so we need something to provide references
	// of to start it off even if we don't care about their contents here.
	bool hasHiddenChildren = false;
	std::vector<repo::lib::RepoUUID> meshIds;
	traversePtree(trees.fullTree.root, hasHiddenChildren, {}, meshIds, scene, trees);
}

static void writePropertyTree(const SelectionTree::Node& node, const SelectionTree& tree, rapidjson::Writer<rapidjson::StringBuffer>& writer)
{
	writer.StartObject();

	writer.Key("account");	writer.String(tree.container.account);
	writer.Key("project");	writer.String(tree.container.project);

	writer.Key("type");	writer.String(nodeTypeToString(node.type));

	if (!node.name.empty()) {
		writer.Key("name"); writer.String(node.name);
	}

	writer.Key("path");	writer.String(childPathToString(node.path));
	writer.Key("_id");	writer.String(node._id.toString());	
	writer.Key("shared_id"); writer.String(node.shared_id.toString());

	if (node.children.size()) {
		writer.Key("children");
		writer.StartArray();
		for (auto& child : node.children) {
			writePropertyTree(*child, tree, writer);
		}
		writer.EndArray();
	}

	if (node.meta.size()) {

		writer.Key("meta");
		writer.StartArray();
		for (const auto& metaId : node.meta) {
			writer.String(metaId.toString());
		}
		writer.EndArray();
	}

	writer.Key("toggleState");	writer.String(toggleStateToString(node.toggleState));

	writer.EndObject();
}

std::map<std::string, std::string> serialiseSelectionTreesToJson(const SelectionTreesSet& trees)
{
	std::map<std::string, std::string> map;

	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		writer.StartObject();

		writer.Key("nodes");
		writePropertyTree(*trees.fullTree.root, trees.fullTree, writer);

		writer.Key("idToName");
		writer.StartObject();
		for (const auto& p : trees.idToName) {
			writer.Key(p.first.toString());
			writer.String(p.second);
		}
		writer.EndObject();

		writer.EndObject();

		map["fulltree.json"] = std::string(buffer.GetString(), buffer.GetSize());
	}
	
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		writer.StartObject();
		
		writer.Key("idToPath");
		writer.StartObject();
		for (auto& p : trees.idToPath) {
			writer.Key(p.first.toString());
			writer.String(childPathToString(p.second));
		}
		writer.EndObject();

		writer.EndObject();

		map["tree_path.json"] = std::string(buffer.GetString(), buffer.GetSize());
	}

	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		writer.StartObject();

		writer.Key("idMap");
		writer.StartObject();
		for (auto& p : trees.idMap) {
			writer.Key(p.first.toString());
			writer.String(p.second.toString());
		}
		writer.EndObject();

		writer.EndObject();

		map["idMap.json"] = std::string(buffer.GetString(), buffer.GetSize());
	}

	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		writer.StartObject();

		for (auto& p : trees.idToMeshes) {
			writer.Key(p.first.toString());
			writer.StartArray();
			for (const auto& meshId : p.second) {
				writer.String(meshId.toString());
			}
			writer.EndArray();
		}

		writer.EndObject();

		map["idToMeshes.json"] = std::string(buffer.GetString(), buffer.GetSize());
	}


	if (trees.modelSettings.hiddenNodes.size()) 
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		writer.StartObject();

		writer.Key("hiddenNodes");
		writer.StartArray();
		for (auto& node : trees.modelSettings.hiddenNodes) {
			writer.String(node.toString());
		}
		writer.EndArray();

		writer.EndObject();

		map["modelProperties.json"] = std::string(buffer.GetString(), buffer.GetSize());
	}

	return map;
}

std::map<std::string, std::vector<uint8_t>> SelectionTreeMaker::getSelectionTreeAsBuffer() const
{
	auto maps = serialiseSelectionTreesToJson(trees);
	std::map<std::string, std::vector<uint8_t>> buffer;
	for (const auto& map : maps)
	{
		const std::string& jsonString = map.second;
		if (!jsonString.empty())
		{
			size_t byteLength = jsonString.size() * sizeof(*jsonString.data());
			buffer[map.first] = std::vector<uint8_t>();
			buffer[map.first].resize(byteLength);
			memcpy(buffer[map.first].data(), jsonString.data(), byteLength);
		}
		else
		{
			repoError << "Failed to write selection tree into the buffer: JSON string is empty.";
		}
	}
	return buffer;
}

SelectionTreeMaker::~SelectionTreeMaker()
{
}
