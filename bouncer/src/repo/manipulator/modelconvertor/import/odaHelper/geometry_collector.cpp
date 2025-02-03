/**
*  Copyright (C) 2018 3D Repo Ltd
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

#include "geometry_collector.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace repo;
using namespace repo::manipulator::modelconvertor::odaHelper;

GeometryCollector::GeometryCollector(repo::manipulator::modelutility::RepoSceneBuilder* builder) :
	sceneBuilder(builder)
{
	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder->addNode(rootNode);
	rootNodeId = rootNode.getSharedID();
	latestMaterial = repo_material_t::DefaultMaterial();
}

GeometryCollector::~GeometryCollector()
{
	if (contexts.size()) {
		throw repo::lib::RepoSceneProcessingException("GeometryCollector is being destroyed with outstanding draw contexts.");
	}
}

void GeometryCollector::addFace(const std::vector<repo::lib::RepoVector3D64>& vertices) 
{
	contexts.top()->addFace(vertices);
}

void GeometryCollector::addFace(
	const std::vector<repo::lib::RepoVector3D64>& vertices,
	const repo::lib::RepoVector3D64& normal,
	const std::vector<repo::lib::RepoVector2D>& uvCoords) 
{
	contexts.top()->addFace(vertices, normal, uvCoords);
}

void GeometryCollector::setMaterial(const repo_material_t& material)
{
	contexts.top()->setMaterial(material);
	latestMaterial = material;
}

repo_material_t GeometryCollector::getLastMaterial()
{
	return latestMaterial;
}

void GeometryCollector::pushDrawContext(Context* ctx) 
{
	if (ctx != nullptr) {
		contexts.push(ctx);
	}
}

void GeometryCollector::popDrawContext(Context* ctx)
{
	if (ctx != nullptr) {
		if (contexts.top() != ctx)
		{
			throw repo::lib::RepoGeometryProcessingException("Attempting to pop a context that is not the last one pushed");
		}
		else
		{
			contexts.pop();
		}
	}
}

void GeometryCollector::createLayer(std::string id, std::string name, std::string parentId)
{
	if (!hasLayer(id)) {
		auto parentSharedId = rootNodeId;

		auto parentIdItr = layerIdToSharedId.find(parentId);
		if (parentIdItr != layerIdToSharedId.end()) {
			parentSharedId = parentIdItr->second;
		}

		auto node = repo::core::model::RepoBSONFactory::makeTransformationNode({}, name, { parentSharedId });
		layerIdToSharedId[id] = node.getSharedID();

		sceneBuilder->addNode(node);
	}
}

bool GeometryCollector::hasLayer(std::string id)
{
	return layerIdToSharedId.find(id) != layerIdToSharedId.end();
}

repo::lib::RepoUUID GeometryCollector::getSharedId(std::string id)
{
	return layerIdToSharedId[id];
}

void GeometryCollector::setMetadata(std::string id, std::unordered_map<std::string, repo::lib::RepoVariant> data)
{
	sceneBuilder->addNode(repo::core::model::RepoBSONFactory::makeMetaDataNode(data, {}, { layerIdToSharedId[id] }));
	layersWithMetadata.insert(id);
}

bool GeometryCollector::hasMetadata(std::string id)
{
	return layersWithMetadata.find(id) != layersWithMetadata.end();
}

void GeometryCollector::finalise()
{
	sceneBuilder->finalise();
}

void GeometryCollector::Context::setMaterial(const repo_material_t& material)
{
	auto id = material.checksum();
	auto builder = meshBuilders.find(id);
	if (builder == meshBuilders.end()) {
		meshBuilders[id] = std::make_unique<RepoMeshBuilder>(std::vector<repo::lib::RepoUUID>(), offset, material);
		this->meshBuilder = meshBuilders[id].get();
	}
	else
	{
		this->meshBuilder = builder->second.get();
	}
}

std::vector<std::pair<repo::core::model::MeshNode, repo_material_t>> GeometryCollector::Context::extractMeshes()
{
	std::vector<std::pair<repo::core::model::MeshNode, repo_material_t>> pairs;
	for (auto& p : meshBuilders) {
		std::vector<repo::core::model::MeshNode> meshes;
		p.second->extractMeshes(meshes);
		for (auto& m : meshes)
		{
			pairs.push_back({ m, p.second->getMaterial() });
		}
	}
	meshBuilders.clear();
	return pairs;
}

GeometryCollector::Context::~Context()
{
	if (meshBuilders.size()) {
		throw repo::lib::RepoGeometryProcessingException("GeometryCollector draw context is being destroyed before extract has been called.");
	}
}