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

#include "repo_node_supermesh.h"
#include "repo_bson_builder.h"
#include "../../../lib/repo_exception.h"

using namespace repo::core::model;

SupermeshNode::SupermeshNode(RepoBSON bson,
	const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>& binMapping) 
	: MeshNode(bson, binMapping)
{
	deserialise(bson);
}

SupermeshNode::SupermeshNode() 
	: MeshNode()
{
}

void SupermeshNode::deserialise(RepoBSON& bson)
{
	RepoBSON mapArray = bson.getObjectField(REPO_NODE_MESH_LABEL_MERGE_MAP);
	if (!mapArray.isEmpty())
	{
		std::set<std::string> fields = mapArray.getFieldNames();
		mappings.resize(fields.size());
		for (const auto& name : fields)
		{
			repo_mesh_mapping_t mapping;
			RepoBSON mappingObj = mapArray.getObjectField(name);

			mapping.mesh_id = mappingObj.getUUIDField(REPO_NODE_MESH_LABEL_MAP_ID);
			mapping.shared_id = mappingObj.getUUIDField(REPO_NODE_MESH_LABEL_MAP_SHARED_ID);
			mapping.material_id = mappingObj.getUUIDField(REPO_NODE_MESH_LABEL_MATERIAL_ID);
			mapping.vertFrom = mappingObj.getIntField(REPO_NODE_MESH_LABEL_VERTEX_FROM);
			mapping.vertTo = mappingObj.getIntField(REPO_NODE_MESH_LABEL_VERTEX_TO);
			mapping.triFrom = mappingObj.getIntField(REPO_NODE_MESH_LABEL_TRIANGLE_FROM);
			mapping.triTo = mappingObj.getIntField(REPO_NODE_MESH_LABEL_TRIANGLE_TO);

			RepoBSON boundingBox = mappingObj.getObjectField(REPO_NODE_MESH_LABEL_BOUNDING_BOX);

			std::vector<repo::lib::RepoVector3D> bboxVec = MeshNode::getBoundingBox(boundingBox);
			mapping.min.x = bboxVec[0].x;
			mapping.min.y = bboxVec[0].y;
			mapping.min.z = bboxVec[0].z;

			mapping.max.x = bboxVec[1].x;
			mapping.max.y = bboxVec[1].y;
			mapping.max.z = bboxVec[1].z;

			mappings[std::stoi(name)] = mapping;
		}
	}

	if (bson.hasBinField(REPO_NODE_MESH_LABEL_SUBMESH_IDS))
	{
		bson.getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_SUBMESH_IDS, submeshIds);
	}
}

void SupermeshNode::serialise(repo::core::model::RepoBSONBuilder& builder) const
{
	throw repo::lib::RepoException("Supermesh Nodes cannot not be serialised");
}