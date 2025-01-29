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

#include "repo_scene_builder.h"

#include "repo/core/model/bson/repo_node_transformation.h"
#include "repo/core/model/bson/repo_node_metadata.h"
#include "repo/core/model/bson/repo_node_mesh.h"
#include "repo/core/model/bson/repo_node_material.h"
#include "repo/core/model/bson/repo_node_texture.h"
#include "repo/core/model/bson/repo_bson.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/handler/database/repo_query.h"

using namespace repo::manipulator::modelutility;
using namespace repo::core::model;

struct RepoSceneBuilder::Deleter
{
	RepoSceneBuilder* builder;
	void operator()(RepoNode* n)
	{
		builder->queueNode(n);
		builder->referenceCounter--;
	}
};

RepoSceneBuilder::RepoSceneBuilder(
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
	const std::string& database,
	const std::string& project,
	const repo::lib::RepoUUID& revisionId)
	:handler(handler),
	databaseName(database),
	projectName(project),
	revisionId(revisionId),
	referenceCounter(0),
	isMissingTextures(false),
	offset({}),
	units(repo::manipulator::modelconvertor::ModelUnits::UNKNOWN)
{
}

RepoSceneBuilder::~RepoSceneBuilder()
{
	if (referenceCounter != 0)
	{
		throw std::runtime_error("RepoSceneBuilder is being destroyed with outstanding RepoNode references. Make sure all RepoNodes have gone out of scope before RepoSceneBuilder.");
	}

	if (nodesToCommit.size())
	{
		throw std::runtime_error("RepoSceneBuilder is being destroyed with outstanding nodes to commit. Call finalise() before letting RepoSceneBuilder go out of scope.");
	}
}

template<repo::core::model::RepoNodeClass T>
std::shared_ptr<T> RepoSceneBuilder::addNode(const T& node)
{
	Deleter deleter;
	deleter.builder = this;
	auto ptr = std::shared_ptr<T>(new T(node), deleter);
	ptr->setRevision(revisionId);
	referenceCounter++;
	return ptr;
}

void RepoSceneBuilder::addNode(std::unique_ptr<repo::core::model::RepoNode> node)
{
	node->setRevision(revisionId);
	queueNode(node.release());
}

void RepoSceneBuilder::addNodes(std::vector<std::unique_ptr<repo::core::model::RepoNode>> nodes)
{
	for (auto& n : nodes) {
		addNode(std::move(n));
	}
}

void RepoSceneBuilder::queueNode(RepoNode* node)
{
	nodesToCommit[node->getUniqueID()] = node;

	if (nodesToCommit.size() > 1000)
	{
		commitNodes();
	}
}

std::string RepoSceneBuilder::getSceneCollectionName()
{
	return projectName + "." + REPO_COLLECTION_SCENE;
}

void RepoSceneBuilder::commitNodes()
{
	std::vector< repo::core::model::RepoBSON> bsons;
	for (auto& n : nodesToCommit) {
		bsons.push_back(*n.second);
		delete n.second;
	}
	nodesToCommit.clear();
	handler->insertManyDocuments(databaseName, getSceneCollectionName(), bsons);

	// Commit inheritence changes

	std::vector<repo::core::handler::database::query::RepoUpdate> updates;
	for (auto u : parentUpdates) {
		updates.push_back(repo::core::handler::database::query::RepoUpdate(*u));
	}
	handler->updateOne(databaseName, getSceneCollectionName(), updates);
	parentUpdates.clear();
}

void RepoSceneBuilder::finalise()
{
	commitNodes();
}

repo::lib::RepoVector3D64 RepoSceneBuilder::getWorldOffset()
{
	return offset;
}

void RepoSceneBuilder::setWorldOffset(const repo::lib::RepoVector3D64& offset)
{
	this->offset = offset;
}

void RepoSceneBuilder::addMaterialReference(const repo_material_t& m, repo::lib::RepoUUID parentId)
{
	auto key = m.checksum();
	if (materialToUniqueId.find(key) == materialToUniqueId.end())
	{
		auto node = repo::core::model::RepoBSONFactory::makeMaterialNode(m, {}, { parentId });
		addNode(node);
		materialToUniqueId[key] = node.getUniqueID();

		if (m.hasTexture()) {
			addTextureReference(m.texturePath, node.getSharedID());
		}
	}
	else
	{
		addParent(materialToUniqueId[key], parentId);
	}
}

void RepoSceneBuilder::addTextureReference(std::string texture, repo::lib::RepoUUID parentId)
{
	if (textureToUniqueId.find(texture) == textureToUniqueId.end()) {
		auto node = createTextureNode(texture);
		if (node) {
			node->addParent(parentId);
			textureToUniqueId[texture] = node->getUniqueID();
			addNode(std::move(node));
		}
	}
	else
	{
		addParent(textureToUniqueId[texture], parentId);
	}
}

std::unique_ptr<repo::core::model::TextureNode> RepoSceneBuilder::createTextureNode(const std::string& texturePath)
{
	std::unique_ptr<repo::core::model::TextureNode> node;
	std::ifstream::pos_type size;
	std::ifstream file(texturePath, std::ios::in | std::ios::binary | std::ios::ate);
	char* memblock = nullptr;
	if (!file.is_open())
	{
		setMissingTextures();
		return node;
	}

	size = file.tellg();
	memblock = new char[size];
	file.seekg(0, std::ios::beg);
	file.read(memblock, size);
	file.close();

	node = std::make_unique<repo::core::model::TextureNode>(repo::core::model::RepoBSONFactory::makeTextureNode(
		texturePath,
		(const char*)memblock,
		size,
		1,
		0
	));

	delete[] memblock;

	return node;
}

void RepoSceneBuilder::addParent(repo::lib::RepoUUID nodeUniqueId, repo::lib::RepoUUID parentSharedId)
{
	using namespace repo::core::handler::database;

	if (nodesToCommit.find(nodeUniqueId) != nodesToCommit.end()) 
	{
		nodesToCommit[nodeUniqueId]->addParent(parentSharedId);
	}
	else
	{
		parentUpdates.push_back(new query::AddParent(nodeUniqueId, parentSharedId));
	}
}

void RepoSceneBuilder::setMissingTextures()
{
	this->isMissingTextures = true;
}

bool RepoSceneBuilder::hasMissingTextures()
{
	return isMissingTextures;
}

void RepoSceneBuilder::setUnits(repo::manipulator::modelconvertor::ModelUnits units) 
{
	this->units = units;
}

repo::manipulator::modelconvertor::ModelUnits RepoSceneBuilder::getUnits()
{
	return this->units;
}

// It is required to tell the module which specialisations to instantiate
// for addNode.

#define DECLARE_ADDNODE_SPECIALISATION(T) template std::shared_ptr<T> RepoSceneBuilder::addNode<T>(const T& node);

DECLARE_ADDNODE_SPECIALISATION(MeshNode)
DECLARE_ADDNODE_SPECIALISATION(TransformationNode)
DECLARE_ADDNODE_SPECIALISATION(MaterialNode)
DECLARE_ADDNODE_SPECIALISATION(TextureNode)
DECLARE_ADDNODE_SPECIALISATION(MetadataNode)
