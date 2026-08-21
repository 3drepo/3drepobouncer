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
#include "repo_test_database_info.h"
#include "repo_test_mesh_utils.h"
#include <repo/core/model/bson/repo_node_metadata.h>
#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_node_material.h>
#include <repo/core/model/bson/repo_node_texture.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/lib/datastructure/repo_variant_utils.h>
#include <repo/manipulator/modelutility/repo_scene_builder.h>
#include <set>

using namespace repo::core::handler::database;
using namespace repo::core::model;
using namespace testing;

std::vector<SceneUtils::NodeInfo> SceneUtils::findNodesByMetadata(std::string key, std::string value)
{
	std::vector<repo::core::model::RepoNode*> transforms;
	std::vector<repo::core::model::RepoNode*> meshes;
	std::set<repo::lib::RepoUUID> parents;
	for (auto& n : scene->getAllMetadata(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		auto m = dynamic_cast<MetadataNode*>(n);
		auto metadata = m->getAllMetadata();
		if (boost::apply_visitor(repo::lib::StringConversionVisitor(), metadata[key]) == value) {
			for (auto p : m->getParentIDs()) {

				auto n = scene->getNodeBySharedID(repo::core::model::RepoScene::GraphType::DEFAULT, p);

				if (n->getTypeAsEnum() == repo::core::model::NodeType::MESH) {
					meshes.push_back(n);
				}
				else if (n->getTypeAsEnum() == repo::core::model::NodeType::TRANSFORMATION) {
					transforms.push_back(n);
				}
			}
		}
	}

	std::vector<NodeInfo> nodes;

	for (auto& t : transforms) {
		nodes.push_back(getNodeInfo(t));
		parents.insert(t->getSharedID());
	}

	for (auto& m : meshes) {
		if (m->getName().size()) {
			nodes.push_back(getNodeInfo(m));
			continue;
		}

		bool hasParentInList = false;
		for (auto& p : transforms) {
			for (auto& mp : m->getParentIDs()) {
				if (p->getSharedID() == mp) {
					hasParentInList = true;
				}
			}
		}

		if (!hasParentInList) {
			nodes.push_back(getNodeInfo(m));
		}
	}

	return nodes;
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

std::string testing::SceneUtils::getTeamspaceName()
{
	return scene->getDatabaseName();
}

std::string testing::SceneUtils::getContainerName()
{
	return scene->getProjectName();
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

SceneUtils::NodeInfo testing::SceneUtils::findNodeByUniqueId(repo::lib::RepoUUID uniqueId)
{
	return getNodeInfo(scene->getNodeByUniqueID(repo::core::model::RepoScene::GraphType::DEFAULT, uniqueId));
}

std::vector<SceneUtils::NodeInfo> SceneUtils::getChildNodes(repo::core::model::RepoNode* node, Filter filter)
{
	std::vector<NodeInfo> nodes;
	for (auto& n : scene->getChildrenAsNodes(repo::core::model::RepoScene::GraphType::DEFAULT, node->getSharedID()))
	{
		if (filter.size() == 0 || std::find(filter.begin(), filter.end(), n->getTypeAsEnum()) != filter.end()) {
			nodes.push_back(getNodeInfo(n));
		}
	}
	return nodes;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::getParentNodes(repo::core::model::RepoNode* node, Filter filter)
{
	std::vector<NodeInfo> nodes;
	for (auto& pid : node->getParentIDs()) {
		auto p = scene->getNodeBySharedID(repo::core::model::RepoScene::GraphType::DEFAULT, pid);
		if(filter.size() == 0 || std::find(filter.begin(), filter.end(), p->getTypeAsEnum()) != filter.end())
		{
			nodes.push_back(getNodeInfo(p));
		}
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

std::vector<SceneUtils::NodeInfo> SceneUtils::NodeInfo::getMeshes(repo::core::model::MeshNode::Primitive primitive)
{
	std::vector<SceneUtils::NodeInfo> meshNodes;
	for (auto& n : getMeshes()) {
		if (auto m = dynamic_cast<MeshNode*>(n.node)) {
			if (m->getPrimitive() == primitive) {
				meshNodes.push_back(n);
			}
		}
	}
	return meshNodes;
}


std::vector<SceneUtils::NodeInfo> SceneUtils::NodeInfo::getMeshes()
{
	std::vector<SceneUtils::NodeInfo> meshNodes;
	for (auto& c : scene->getChildNodes(node, { repo::core::model::NodeType::MESH }))
	{
		meshNodes.push_back(c);
	}
	if (dynamic_cast<MeshNode*>(this->node)) {
		meshNodes.push_back(*this);
	}
	return meshNodes;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::NodeInfo::getMeshesRecursive()
{
	std::vector<SceneUtils::NodeInfo> meshNodes;
	for (auto& c : scene->getChildNodes(node, { repo::core::model::NodeType::TRANSFORMATION, repo::core::model::NodeType::MESH }))
	{
		auto childMeshNodes = c.getMeshesRecursive();
		meshNodes.insert(meshNodes.end(), childMeshNodes.begin(), childMeshNodes.end());
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

repo::lib::RepoBounds SceneUtils::NodeInfo::getProjectBounds()
{
	repo::lib::RepoBounds bounds;
	for (auto& m : getMeshesInProjectCoordinates()) {
		bounds.encapsulate(m.getBoundingBox());
	}
	return bounds;
}

bool SceneUtils::NodeInfo::hasTextures()
{
	return getTextures().size();
}

std::vector<SceneUtils::NodeInfo> SceneUtils::NodeInfo::getTextures()
{
	std::vector<MeshNode*> meshes;
	std::vector<MaterialNode*> materials;
	std::vector<SceneUtils::NodeInfo> textures;

	if (auto n = dynamic_cast<TransformationNode*>(node)) {
		for (auto m : getMeshes()) {
			meshes.push_back(dynamic_cast<MeshNode*>(m.node));
		}
	}

	if (auto mesh = dynamic_cast<MeshNode*>(node)) {
		meshes.push_back(mesh);
	}
	
	for (auto m : meshes) {
		for (auto c : scene->getChildNodes(m, { repo::core::model::NodeType::MATERIAL })) {
			materials.push_back(dynamic_cast<MaterialNode*>(c.node));
		}
	}
	
	if(auto material = dynamic_cast<MaterialNode*>(node)) {
		materials.push_back(material);
	}
	
	for (auto m : materials) {
		for (auto& c : scene->getChildNodes(m, { repo::core::model::NodeType::TEXTURE })) {
			textures.push_back(c);
		}
	}

	return textures;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::NodeInfo::getSiblings(Filter p, Filter s)
{
	std::vector<SceneUtils::NodeInfo> siblings;
	for (auto& p : this->getParents(p)) {
		for (auto& c : scene->getChildNodes(p.node, s)) {
			if (c != *this) {
				siblings.push_back(c);
			}
		}
	}
	return siblings;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::getMeshes()
{
	std::vector<SceneUtils::NodeInfo> meshes;
	for (auto n : scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT)) {
		meshes.push_back(getNodeInfo(n));
	}
	return meshes;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::getTransformations()
{
	std::vector<SceneUtils::NodeInfo> transforms;
	for (auto n : scene->getAllTransformations(repo::core::model::RepoScene::GraphType::DEFAULT)) {
		transforms.push_back(getNodeInfo(n));
	}
	return transforms;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::getMetadataNodes()
{
	std::vector<SceneUtils::NodeInfo> metadata;
	for (auto n : scene->getAllMetadata(repo::core::model::RepoScene::GraphType::DEFAULT)) {
		metadata.push_back(getNodeInfo(n));
	}
	return metadata;
}

std::unordered_map<std::string, repo::lib::RepoVariant> SceneUtils::NodeInfo::getMetadata()
{
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
	for (const auto& c : getMetadataNodes()) {
		auto m = dynamic_cast<MetadataNode*>(c.node)->getAllMetadata();
		metadata.insert(m.begin(), m.end());
	}
	return metadata;
}

std::vector<SceneUtils::NodeInfo> SceneUtils::NodeInfo::getMetadataNodes()
{
	return scene->getChildNodes(node, { repo::core::model::NodeType::METADATA });
}

repo::lib::repo_material_t SceneUtils::NodeInfo::getMaterial()
{
	for (auto& c : scene->getChildNodes(node, {repo::core::model::NodeType::MATERIAL}))
	{
		return dynamic_cast<MaterialNode*>(c.node)->getMaterialStruct();
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

bool SceneUtils::NodeInfo::hasTransparency()
{
	for (auto& c : getColours()) {
		if (c.a < 0.9999) {
			return true;
		}
	}
	return false;
}

std::vector<std::string> SceneUtils::NodeInfo::getChildNames()
{
	std::vector<std::string> names;
	for (auto c : scene->getChildNodes(node, { repo::core::model::NodeType::MESH, repo::core::model::NodeType::TRANSFORMATION }))
	{
		if (!c.node->getName().empty()) {
			names.push_back(c.node->getName());
		}
	}
	return names;
}

SceneUtils::NodeInfo SceneUtils::NodeInfo::getParent() const
{
	auto parents = scene->getParentNodes(node, { repo::core::model::NodeType::TRANSFORMATION });
	return parents[0];
}

std::vector<SceneUtils::NodeInfo> SceneUtils::NodeInfo::getParents(SceneUtils::Filter filter) const
{
	auto parents = scene->getParentNodes(node, filter);
	return parents;
}

std::string SceneUtils::NodeInfo::getPath() const
{
	auto parents = scene->getParentNodes(node, { repo::core::model::NodeType::TRANSFORMATION });
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

ContainerUtils::ContainerUtils(
	std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
	const std::string& database,
	const std::string& container,
	const repo::lib::RepoUUID& revisionId
) :
	handler(handler),
	database(database),
	container(container),
	revisionId(revisionId)
{
}

std::vector<repo::core::model::RepoBSON> ContainerUtils::getRootNodes()
{
	query::RepoQueryBuilder query;
	query.append(query::Eq(REPO_NODE_REVISION_ID, revisionId));
	query.append(query::Exists(REPO_NODE_LABEL_PARENTS, false));
	return handler->findAllByCriteria(
		database,
		container + "." + REPO_COLLECTION_SCENE,
		query,
		false
	);
}

std::vector<repo::core::model::RepoRef> ContainerUtils::getAllRefNodes()
{
	std::set<std::string> refNames;

	{
		query::RepoQueryBuilder query;
		query.append(query::Eq(REPO_NODE_REVISION_ID, revisionId));
		query.append(query::Exists(REPO_LABEL_BINARY_REFERENCE, true));

		auto cursor = handler->findCursorByCriteria(
			database,
			container + "." + REPO_COLLECTION_SCENE,
			query
		);

		for (auto& doc : *cursor) {
			auto reference = doc.getBinaryReference();
			auto name = reference.getStringField(REPO_LABEL_BINARY_FILENAME);
			refNames.insert(name);
		}
	}

	std::vector<repo::core::model::RepoRef> refNodes;

	{
		auto cursor = handler->findCursorByCriteria(
			database,
			container + "." + REPO_COLLECTION_SCENE + "." + REPO_COLLECTION_EXT_REF,
			query::Eq(REPO_LABEL_ID, refNames)
		);

		for (auto& doc : *cursor) {
			refNodes.push_back(repo::core::model::RepoRef(doc));
		}
	}

	return refNodes;
}

void ContainerUtils::deleteNode(const repo::lib::RepoUUID& id)
{
	handler->dropDocuments(
		database,
		container + "." + REPO_COLLECTION_SCENE,
		query::Eq(REPO_NODE_LABEL_ID, id)
	);
}

void ContainerUtils::updateRevision(const repo::core::model::RevisionNode& revision)
{
	handler->upsertDocument(
		database,
		container + "." + REPO_COLLECTION_HISTORY,
		revision,
		false
	);
}

std::vector<repo::core::model::ModelRevisionNode> ContainerUtils::getRevisions()
{
	
	std::vector<repo::core::model::ModelRevisionNode> revisions;
	auto docs = handler->findAllByCriteria(
		database,
		container + "." + REPO_COLLECTION_HISTORY,
		query::Eq(REPO_NODE_LABEL_TYPE, REPO_NODE_TYPE_REVISION),
		false
	);
	for (auto& d : docs) {
		revisions.push_back(repo::core::model::ModelRevisionNode(d));
	}
	return revisions;
}

void testing::makeRandomScene(
	std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler,
	const std::string& db,
	const std::string& project,
	const repo::lib::RepoUUID& revisionId
)
{
	using namespace repo::core::model;

	repo::manipulator::modelutility::RepoSceneBuilder builder(
		handler, db, project, revisionId);

	builder.setUnits(repo::lib::ModelUnits::METRES);

	auto root = builder.addNode(
		RepoBSONFactory::makeTransformationNode({}, "root"));

	for (int b = 0; b < 5; ++b)
	{
		// Branch transformation under root
		auto branch = builder.addNode(
			RepoBSONFactory::makeTransformationNode(
				{}, "branch_" + std::to_string(b), {root->getSharedID()}));

		for (int m = 0; m < 2; ++m)
		{
			auto mesh = repo::test::utils::mesh::createRandomMesh(
				30 + rand() % 100,	// nVertices
				false,              // hasUV
				3,                  // primitiveSize (triangles)
				"",                 // grouping
				{branch->getSharedID()});

			builder.addNode(*mesh);
		}
	}

	root.reset(); // equivalent to the root going out of scope so it is queued for writing.
	builder.finalise();
}
