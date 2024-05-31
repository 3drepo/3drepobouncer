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

using namespace repo::core::model;

RepoBSON SupermeshNode::meshMappingAsBSON(const repo_mesh_mapping_t& mapping)
{
	RepoBSONBuilder builder;
	builder.append(REPO_NODE_MESH_LABEL_MAP_ID, mapping.mesh_id);
	builder.append(REPO_NODE_MESH_LABEL_MAP_SHARED_ID, mapping.shared_id);
	builder.append(REPO_NODE_MESH_LABEL_MATERIAL_ID, mapping.material_id);
	builder.append(REPO_NODE_MESH_LABEL_VERTEX_FROM, mapping.vertFrom);
	builder.append(REPO_NODE_MESH_LABEL_VERTEX_TO, mapping.vertTo);
	builder.append(REPO_NODE_MESH_LABEL_TRIANGLE_FROM, mapping.triFrom);
	builder.append(REPO_NODE_MESH_LABEL_TRIANGLE_TO, mapping.triTo);

	RepoBSONBuilder bbBuilder;
	bbBuilder.append("0", mapping.min);
	bbBuilder.append("1", mapping.max);

	builder.appendArray(REPO_NODE_MESH_LABEL_BOUNDING_BOX, bbBuilder.obj());

	return builder.obj();
}

SupermeshNode SupermeshNode::cloneAndUpdateIds(
	repo::lib::RepoUUID uniqueID,
	repo::lib::RepoUUID sharedID
)
{
	RepoBSONBuilder builder;
	builder.append(REPO_NODE_LABEL_ID, uniqueID);
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedID);
	builder.appendElementsUnique(*this);
	return SupermeshNode(builder.obj(), bigFiles);
}

SupermeshNode SupermeshNode::cloneAndUpdateMeshMapping(
	const std::vector<repo_mesh_mapping_t>& vec,
	const bool& overwrite)
{
	RepoBSONBuilder builder, mapbuilder;
	uint32_t index = 0;
	std::vector<repo_mesh_mapping_t> mappings;
	RepoBSON mapArray = getObjectField(REPO_NODE_MESH_LABEL_MERGE_MAP);
	if (!overwrite && !mapArray.isEmpty())
	{
		//if map array isn't empty, find the next index it needs to slot in
		std::set<std::string> fields = mapArray.getFieldNames();
		index = fields.size();
	}

	for (uint32_t i = 0; i < vec.size(); ++i)
	{
		mapbuilder.append(std::to_string(index + i), meshMappingAsBSON(vec[i]));
	}
	//append the rest of the array onto this new map bson
	if (!overwrite) mapbuilder.appendElementsUnique(mapArray);

	builder.appendArray(REPO_NODE_MESH_LABEL_MERGE_MAP, mapbuilder.obj());

	//append the rest of the mesh onto this new bson
	builder.appendElementsUnique(*this);

	return SupermeshNode(builder.obj(), bigFiles);
}


std::vector<float> SupermeshNode::getSubmeshIds() const
{
	std::vector<float> submeshIds = std::vector<float>();
	if (hasBinField(REPO_NODE_MESH_LABEL_SUBMESH_IDS))
	{
		getBinaryFieldAsVector(REPO_NODE_MESH_LABEL_SUBMESH_IDS, submeshIds);
	}

	return submeshIds;
}

std::vector<repo_mesh_mapping_t> SupermeshNode::getMeshMapping() const
{
	std::vector<repo_mesh_mapping_t> mappings;
	RepoBSON mapArray = getObjectField(REPO_NODE_MESH_LABEL_MERGE_MAP);
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
	return mappings;
}