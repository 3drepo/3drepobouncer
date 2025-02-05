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

				SupermeshNode(RepoBSON bson);

			protected:
				std::vector<repo::lib::repo_mesh_mapping_t> mappings;
				std::vector<float> submeshIds;

			protected:
				virtual void deserialise(RepoBSON&);
				virtual void serialise(repo::core::model::RepoBSONBuilder&) const;

			public:

				const std::vector<repo::lib::repo_mesh_mapping_t>& getMeshMapping() const
				{
					return mappings;
				}

				void setMeshMapping(const std::vector<repo::lib::repo_mesh_mapping_t>& mapping)
				{
					this->mappings = mapping;
				}

				const std::vector<float>& getSubmeshIds() const
				{
					return submeshIds;
				}

				void setSubmeshIds(const std::vector<float>& ids)
				{
					this->submeshIds = ids;
				}
			};
		}
	}
}
