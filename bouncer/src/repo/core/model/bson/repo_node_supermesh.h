/**
*  Copyright (C) 2024 3D Repo Ltd
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

#pragma once
#include "repo_node_mesh.h"

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT SupermeshNode : public MeshNode
			{
			public:
				SupermeshNode();

				SupermeshNode(RepoBSON bson,
					const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>& binMapping);

			protected:
				std::vector<repo_mesh_mapping_t> mappings;
				std::vector<float> submeshIds;

			protected:
				virtual void deserialise(RepoBSON&);
				virtual void serialise(repo::core::model::RepoBSONBuilder&) const;

			public:

				std::vector<repo_mesh_mapping_t> getMeshMapping() const
				{
					return mappings;
				}

				std::vector<float> getSubmeshIds() const
				{
					return submeshIds;
				}

				/**
				* Given a mesh mapping, convert it into a bson object
				* @param mapping the mapping to convert
				* @return return a bson object containing the mapping
				*/
				static RepoBSON meshMappingAsBSON(const repo_mesh_mapping_t& mapping);
			};
		}
	}
}
