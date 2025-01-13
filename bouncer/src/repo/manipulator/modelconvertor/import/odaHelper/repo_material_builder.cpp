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

#include "repo/core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace repo::core::model;

void RepoMaterialBuilder::addMaterialReference(repo_material_t material, repo::lib::RepoUUID parentId)
{
	auto key = material.checksum();

	auto itr = materials.find(key);
	if (itr == materials.end())
	{
		materials[key] = RepoBSONFactory::makeMaterialNode(material, {}, { parentId });
	}
	else
	{
		itr->second.addParent(parentId);
	}
}

void RepoMaterialBuilder::extract(std::vector<repo::core::model::MaterialNode>& nodes)
{
	for (auto p : materials)
	{
		nodes.push_back(p.second);
	}
}