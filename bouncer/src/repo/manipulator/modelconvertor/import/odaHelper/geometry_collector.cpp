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

using namespace repo::manipulator::modelconvertor::odaHelper;

GeometryCollector::GeometryCollector(repo::manipulator::modelutility::RepoSceneBuilder* builder) :
	meshBuilderDeleter(this),
	meshBuilder(nullptr, meshBuilderDeleter),
	sceneBuilder(builder)
{
	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	sceneBuilder->addNode(rootNode);
	rootNodeId = rootNode.getSharedID();
}

GeometryCollector::RepoMeshBuilderPtr GeometryCollector::makeMeshBuilder(
	std::vector<repo::lib::RepoUUID> parentIds,
	repo::lib::RepoVector3D64 offset,
	repo_material_t material)
{
	return std::move(std::unique_ptr<RepoMeshBuilder, RepoMeshBuilderDeleter&>(
		new RepoMeshBuilder(parentIds, offset, material),
		meshBuilderDeleter)
	);
}

repo_material_t GeometryCollector::GetDefaultMaterial() const {
	repo_material_t material;
	material.shininess = 0.0;
	material.shininessStrength = 0.0;
	material.opacity = 1;
	material.specular = { 0, 0, 0, 0 };
	material.diffuse = { 0.5f, 0.5f, 0.5f, 0 };
	material.emissive = material.diffuse;

	return material;
}

void GeometryCollector::setLayer(std::string id)
{
	auto parentIds = std::vector<repo::lib::RepoUUID>({ layerIdToSharedId[id] }); // (There is only ever actually one parent)

	if (!meshBuilder)
	{
		meshBuilder = makeMeshBuilder(parentIds, offset, GetDefaultMaterial());
	}
	else if (meshBuilder->getParents() != parentIds)
	{
		meshBuilder = makeMeshBuilder(parentIds, offset, meshBuilder->getMaterial());
	}
}

void GeometryCollector::setMaterial(const repo_material_t& material, bool missingTexture)
{
	// setLayer must always be called before the material

	if (missingTexture) 
	{
		sceneBuilder->setMissingTextures();
	}

	if (meshBuilder->getMaterial().checksum() != material.checksum())
	{
		meshBuilder = makeMeshBuilder(meshBuilder->getParents(), offset, material);
	}
}

void GeometryCollector::createLayer(std::string id, std::string name, std::string parentId)
{
	if (hasLayer(id)) {
		throw repo::lib::RepoSceneProcessingException("createLayer called for the same id twice");
	}
	auto parentSharedId = rootNodeId;

	auto parentIdItr = layerIdToSharedId.find(parentId);
	if (parentIdItr != layerIdToSharedId.end()) {
		parentSharedId = parentIdItr->second;
	}

	auto node = repo::core::model::RepoBSONFactory::makeTransformationNode({}, name, { parentSharedId });
	sceneBuilder->addNode(node);

	layerIdToSharedId[id] = node.getSharedID();
}

bool GeometryCollector::hasLayer(std::string id)
{
	return layerIdToSharedId.find(id) != layerIdToSharedId.end();
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

void GeometryCollector::RepoMeshBuilderDeleter::operator()(RepoMeshBuilder* builder)
{
	std::vector<repo::core::model::MeshNode> meshes;
	builder->extractMeshes(meshes);
	for (auto m : meshes) {
		processor->materialBuilder.addMaterialReference(builder->getMaterial(), m.getSharedID()); // (If it is not obvious, the logic of this loop ensures that a given material is only ever added if it is reference at least once)
		processor->sceneBuilder->addNode(m);
	}
	delete builder;
}

void GeometryCollector::finalise()
{
	meshBuilder = nullptr;
	std::vector<repo::core::model::MaterialNode> materials;
	materialBuilder.extract(materials);
	for (auto m : materials)
	{
		sceneBuilder->addNode(m);
	}
}

/*
repo::core::model::TextureNode GeometryCollector::createTextureNode(const std::string& texturePath)
{
	std::ifstream::pos_type size;
	std::ifstream file(texturePath, std::ios::in | std::ios::binary | std::ios::ate);
	char *memblock = nullptr;
	if (!file.is_open())
		return repo::core::model::TextureNode();

	size = file.tellg();
	memblock = new char[size];
	file.seekg(0, std::ios::beg);
	file.read(memblock, size);
	file.close();

	auto texnode = repo::core::model::RepoBSONFactory::makeTextureNode(
		texturePath,
		(const char*)memblock,
		size,
		1,
		0
	);

	delete[] memblock;

	return texnode;
}
*/