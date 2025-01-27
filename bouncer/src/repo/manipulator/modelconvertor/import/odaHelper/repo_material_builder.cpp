/*
*  Copyright(C) 2025 3D Repo Ltd
*
*  This program is free software : you can redistribute it and / or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or(at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.If not, see <http://www.gnu.org/licenses/>.
*/

#include "repo_material_builder.h"
#include "repo/lib/repo_exception.h"

#include "repo/core/model/bson/repo_bson_factory.h"

#include <fstream>

using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace repo::core::model;

void RepoMaterialBuilder::addMaterialReference(repo_material_t material, repo::lib::RepoUUID parentId)
{
	auto key = material.checksum();

	auto itr = materials.find(key);
	if (itr == materials.end())
	{
		materials[key] = std::make_unique<repo::core::model::MaterialNode>(RepoBSONFactory::makeMaterialNode(material, {}, { parentId }));
	}
	else
	{
		itr->second->addParent(parentId);
	}

	if (!material.texturePath.empty())
	{
		createTextureNode(material.texturePath, materials[key]->getSharedID());
	}
}

std::vector<std::unique_ptr<repo::core::model::RepoNode>> RepoMaterialBuilder::extract()
{
	std::vector<std::unique_ptr<repo::core::model::RepoNode>> nodes;

	for (auto& p : materials)
	{
		nodes.push_back(std::move(p.second));
	}
	materials.clear();

	for (auto& p : textures) 
	{
		nodes.push_back(std::move(p.second));
	}
	textures.clear();

	return nodes;
}

RepoMaterialBuilder::~RepoMaterialBuilder()
{
	if (materials.size() || textures.size()) {
		throw repo::lib::RepoSceneProcessingException("Destroying RepoMaterialBuilder with material or texture nodes that have not been extracted.");
	}
}

void RepoMaterialBuilder::createTextureNode(const std::string& texturePath, repo::lib::RepoUUID parent)
{
	auto texturePtr = textures.find(texturePath);

	if (texturePtr == textures.end()) {

		std::ifstream::pos_type size;
		std::ifstream file(texturePath, std::ios::in | std::ios::binary | std::ios::ate);
		char* memblock = nullptr;
		if (!file.is_open())
		{
			missingTextures = true;
			return;
		}

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

		texnode.addParent(parent);

		textures[texturePath] = std::make_unique<repo::core::model::TextureNode>(texnode);
	}
	else
	{
		texturePtr->second->addParent(parent);
	}
}
