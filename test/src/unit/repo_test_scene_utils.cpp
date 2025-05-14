/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo_test_scene_utils.h"
#include <repo/core/model/bson/repo_node_metadata.h>
#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_node_material.h>
#include <repo/core/model/bson/repo_node_texture.h>
#include <repo/lib/datastructure/repo_variant_utils.h>

using namespace repo::core::model;
using namespace testing;

std::vector<SceneUtils::NodeInfo> SceneUtils::findNodesByMetadata(std::string key, std::string value)
{
	std::vector<NodeInfo> info;
	for (auto& n : scene->getAllMetadata(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		auto m = dynamic_cast<MetadataNode*>(n);
		auto metadata = m->getAllMetadata();
		if (boost::apply_visitor(repo::lib::StringConversionVisitor(), metadata[key]) == value) {
			for (auto p : m->getParentIDs()) {
				info.push_back(getNodeInfo(scene->getNodeBySharedID(repo::core::model::RepoScene::GraphType::DEFAULT, p)));
			}
		}
	}
	return info;
}

SceneUtils::NodeInfo SceneUtils::findNodeByMetadata(std::string key, std::string value)
{
	auto nodes = findNodesByMetadata(key, value);
	if (nodes.size() > 1) {
		throw std::runtime_error("Found too many matching nodes for call.");
	}
	return nodes[0];
}

SceneUtils::NodeInfo SceneUtils::getRootNode()
{
	return getNodeInfo(scene->getRoot(repo::core::model::RepoScene::GraphType::DEFAULT));
}

bool SceneUtils::isPopulated()
{
	return scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT).size() > 0;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::findTransformationNodesByName(std::string name)
{
	std::vector<NodeInfo> nodes;
	for (auto& n : scene->getAllTransformations(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		if (n->getName() == name) {
			nodes.push_back(getNodeInfo(n));
		}
	}
	return nodes;
}

SceneUtils::NodeInfo SceneUtils::findTransformationNodeByName(std::string name)
{
	auto nodes = findTransformationNodesByName(name);
	if (nodes.size() > 1) {
		throw std::runtime_error("Found too many matching nodes for call.");
	}
	return nodes[0];
}

std::vector<SceneUtils::NodeInfo> SceneUtils::findLeafNodes(std::string name)
{
	std::vector<NodeInfo> nodes;

	for (auto& n : scene->getAllTransformations(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		if (n->getName() == name) {
			auto i = getNodeInfo(n);
			if (i.isLeaf())
			{
				nodes.push_back(i);
			}
		}
	}

	for (auto& n : scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		if (n->getName() == name) {
			auto i = getNodeInfo(n);
			nodes.push_back(i);
		}
	}

	return nodes;
}

SceneUtils::NodeInfo SceneUtils::findLeafNode(std::string name)
{
	auto nodes = findLeafNodes(name);

	if (nodes.size() > 1) {
		throw std::runtime_error("Found too many matching nodes for call.");
	}

	return nodes[0];
}

std::vector<SceneUtils::NodeInfo> SceneUtils::getChildNodes(repo::core::model::RepoNode* node, bool ignoreMeta)
{
	std::vector<NodeInfo> nodes;
	for (auto& n : scene->getChildrenAsNodes(repo::core::model::RepoScene::GraphType::DEFAULT, node->getSharedID()))
	{
		if (ignoreMeta && n->getTypeAsEnum() == repo::core::model::NodeType::METADATA) {
			continue;
		}
		nodes.push_back(getNodeInfo(n));
	}
	return nodes;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::getParentNodes(repo::core::model::RepoNode* node)
{
	std::vector<NodeInfo> nodes;
	for (auto& n : scene->getParentNodesFiltered(repo::core::model::RepoScene::GraphType::DEFAULT, node, repo::core::model::NodeType::TRANSFORMATION))
	{
		nodes.push_back(getNodeInfo(n));
	}
	return nodes;
}

SceneUtils::NodeInfo SceneUtils::getNodeInfo(repo::core::model::RepoNode* node)
{
	NodeInfo info(node, this);
	for (auto child : scene->getChildrenAsNodes(repo::core::model::RepoScene::GraphType::DEFAULT, node->getSharedID()))
	{
		if (dynamic_cast<MeshNode*>(child)) {
			info.numMeshes++;

			if (!child->getName().empty()) {
				info.numVisibleChildren++;
			}
		}

		if (dynamic_cast<TransformationNode*>(child))
		{
			info.numVisibleChildren++;
		}
	}
	return info;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::NodeInfo::getMeshes()
{
	std::vector<SceneUtils::NodeInfo> meshNodes;
	for (auto& c : scene->getChildNodes(node, true))
	{
		if (dynamic_cast<MeshNode*>(c.node)) {
			meshNodes.push_back(c);
		}
	}
	if (dynamic_cast<MeshNode*>(this->node)) {
		meshNodes.push_back(*this);
	}
	return meshNodes;
}

repo::core::model::MeshNode SceneUtils::NodeInfo::getMeshInProjectCoordinates()
{
	auto mesh = dynamic_cast<repo::core::model::MeshNode*>(node);
	if (!mesh) {
		auto meshes = getMeshes();
		if (meshes.size() > 1) {
			throw std::runtime_error("getMeshInProjectCoordinates called for transform node with more than one mesh.");
		}
		mesh = dynamic_cast<repo::core::model::MeshNode*>(meshes[0].node);
	}

	return mesh->cloneAndApplyTransformation(
		repo::lib::RepoMatrix::translate(repo::lib::RepoVector3D64(scene->scene->getWorldOffset())) * scene->getWorldTransform(node)
	);
}

std::vector<repo::core::model::MeshNode> SceneUtils::NodeInfo::getMeshesInProjectCoordinates()
{
	std::vector<repo::core::model::MeshNode> meshes;
	for (auto& m : getMeshes()) {
		meshes.push_back(m.getMeshInProjectCoordinates());
	}
	return meshes;
}


std::vector<SceneUtils::NodeInfo> SceneUtils::NodeInfo::getTextures()
{
	std::vector<MeshNode*> meshes;
	std::vector<MaterialNode*> materials;
	std::vector<SceneUtils::NodeInfo> textures;

	if (auto mesh = dynamic_cast<MeshNode*>(node)) {
		meshes.push_back(mesh);
	}
	
	for (auto m : meshes) {
		for (auto c : scene->getChildNodes(m, true)) {
			if (auto material = dynamic_cast<MaterialNode*>(c.node)) {
				materials.push_back(material);
			}
		}
	}
	
	if(auto material = dynamic_cast<MaterialNode*>(node)) {
		materials.push_back(material);
	}
	
	for (auto m : materials) {
		for (auto& c : scene->getChildNodes(m, true)) {
			if (dynamic_cast<TextureNode*>(c.node)) {
				textures.push_back(c);
			}
		}
	}

	return textures;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::getMeshes()
{
	std::vector<SceneUtils::NodeInfo> meshes;
	for (auto n : scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT)) {
		meshes.push_back(getNodeInfo(n));
	}
	return meshes;
}

std::unordered_map<std::string, repo::lib::RepoVariant> SceneUtils::NodeInfo::getMetadata()
{
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
	for (auto c : scene->getChildNodes(node, false))
	{
		if (dynamic_cast<MetadataNode*>(c.node)) {
			auto m = dynamic_cast<MetadataNode*>(c.node)->getAllMetadata();
			metadata.insert(m.begin(), m.end());
		}
	}
	return metadata;
}

repo::lib::repo_material_t SceneUtils::NodeInfo::getMaterial()
{
	for (auto& c : scene->getChildNodes(node, true))
	{
		if (dynamic_cast<MaterialNode*>(c.node)) {
			return dynamic_cast<MaterialNode*>(c.node)->getMaterialStruct();
		}
	}
	throw std::runtime_error("Node does not have a material");
}

std::vector<repo::lib::repo_color4d_t> SceneUtils::NodeInfo::getColours()
{
	std::vector<repo::lib::repo_color4d_t> colours;
	for (auto c : getMeshes()) {
		colours.push_back(repo::lib::repo_color4d_t(c.getMaterial().diffuse, c.getMaterial().opacity));
	}
	return colours;
}

std::vector<std::string> SceneUtils::NodeInfo::getChildNames()
{
	std::vector<std::string> names;
	for (auto c : scene->getChildNodes(node, true))
	{
		if (!c.node->getName().empty()) {
			names.push_back(c.node->getName());
		}
	}
	return names;
}

SceneUtils::NodeInfo SceneUtils::NodeInfo::getParent() const
{
	auto parents = scene->getParentNodes(node);
	return parents[0];
}

std::string SceneUtils::NodeInfo::getPath() const
{
	auto parents = scene->getParentNodes(node);
	if (parents.size()) {
		return parents[0].getPath() + "->" + name();
	}
	else
	{
		return name();
	}
}

repo::lib::RepoMatrix SceneUtils::getWorldTransform(repo::core::model::RepoNode* node)
{
	repo::lib::RepoMatrix m;
	if (auto t = dynamic_cast<repo::core::model::TransformationNode*>(node)) {
		m = t->getTransMatrix();
	}
	auto child = node;
	while(true){
		auto parents = scene->getParentNodesFiltered(
			repo::core::model::RepoScene::GraphType::DEFAULT,
			child,
			repo::core::model::NodeType::TRANSFORMATION);
		if (!parents.size()) {
			break;
		}
		if (parents.size() > 1) {
			throw repo::lib::RepoException("SceneUtils::getWorldTransform does not support instancing. Call getWorldTransform from each instances' TransformationNode one by one.");
		}
		m = dynamic_cast<repo::core::model::TransformationNode*>(parents[0])->getTransMatrix() * m;
		child = parents[0];
	}
	return m;
}
