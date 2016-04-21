/**
*  Copyright (C) 2016 3D Repo Ltd
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

#include <unordered_map>
#include <cstdint>
#include "../../repo_bouncer_global.h"
#include "../../core/model/repo_node_utils.h"

typedef struct {
	std::unordered_map<std::string, std::vector<uint8_t>> geoFiles; //files where geometery are stored
	std::unordered_map<std::string, std::vector<uint8_t>> x3dFiles; //back bone x3dom files
	std::unordered_map<std::string, std::vector<uint8_t>> jsonFiles; //JSON mapping files
}repo_web_buffers_t;

struct MeshEntry
{
	std::vector<float> min;
	std::vector<float> max;
	std::vector<float> mid;// midpoint
	repoUUID      id;

	MeshEntry() :mid({ 0, 0, 0 })
	{
		min = { 0, 0, 0 };
		max = { 0, 0, 0 };
	}
};

enum class PartitioningTreeType{ PARTITION_X, PARTITION_Y, PARTITION_Z, LEAF_NODE };

struct PartitioningTree{
	PartitioningTreeType              type;
	std::vector<MeshEntry>            meshes; //mesh ids if it is a leaf node
	float                             pValue; //partitioning value if not
	std::shared_ptr<PartitioningTree> left;
	std::shared_ptr<PartitioningTree> right;

	//Construction of branch node
	PartitioningTree(
		const PartitioningTreeType &type,
		const float &pValue,
		std::shared_ptr<PartitioningTree> left,
		std::shared_ptr<PartitioningTree> right)
		: type(type), pValue(pValue),
		left(left),
		right(right){}

	//Construction of leaf node
	PartitioningTree(
		const std::vector<MeshEntry> &meshes)
		:
		type(PartitioningTreeType::LEAF_NODE),
		meshes(meshes), pValue(0),
		left(std::shared_ptr<PartitioningTree>(nullptr)),
		right(std::shared_ptr<PartitioningTree>(nullptr)){}
};

struct DiffResult{
	std::vector<repoUUID> added; //nodes that does not exist on the other model
	std::vector<repoUUID> modified; //nodes that exist on the other model but it is modified.
	std::unordered_map<repoUUID, repoUUID, RepoUUIDHasher > correspondence;
};

enum class DiffMode{ DIFF_BY_ID, DIFF_BY_NAME };