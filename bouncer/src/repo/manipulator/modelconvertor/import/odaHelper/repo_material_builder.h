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

#pragma once

#include "repo/lib/datastructure/repo_structs.h"

namespace repo {

	namespace core {
		namespace model{
			class RepoNode;
			class MaterialNode;
			class TextureNode;
		}
	}

	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {

				/*
				* RepoMaterialBuilder is used to build MaterialNodes from repo_material_t
				* instances, where references to the nodes can be added over multiple calls.
				* The node management itself is hidden; call addMaterialReference
				* with the shared Ids as many times as necessary for that material.
				*/
				class RepoMaterialBuilder
				{
				public:

					~RepoMaterialBuilder();

					// The current version of RepoMaterialBuilder holds all materials in
					// memory and expects them to be explicitly added. This can change in
					// the future once we know how much memory is typically dedicated to
					// material management.

					/*
					* Creates the MaterialNode, if necessary, and adds the parentId to its
					* parents member.
					*/
					void addMaterialReference(repo_material_t material, repo::lib::RepoUUID parentId);

					void extract(std::vector<repo::core::model::MaterialNode>& nodes);

				private:
					std::unordered_map<size_t, repo::core::model::MaterialNode> materials;
					std::unordered_map<size_t, repo::core::model::TextureNode> textures;
				};
			}
		}
	}
}