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
	std::unordered_map<std::string, std::vector<uint8_t>> jsonFiles; //JSON mapping files
}repo_web_buffers_t;

struct repo_mesh_entry_t
{
	std::vector<float> min;
	std::vector<float> max;
	std::vector<float> mid;// midpoint
	repo::lib::RepoUUID      id;

	repo_mesh_entry_t() :mid({ 0, 0, 0 })
	{
		min = { 0, 0, 0 };
		max = { 0, 0, 0 };
	}
};

namespace repo{
	enum class PartitioningTreeType{ PARTITION_X, PARTITION_Y, PARTITION_Z, LEAF_NODE };
	enum class DiffMode{ DIFF_BY_ID, DIFF_BY_NAME };
}

struct repo_partitioning_tree_t{
	repo::PartitioningTreeType              type;
	std::vector<repo_mesh_entry_t>            meshes; //mesh ids if it is a leaf node
	float                             pValue; //partitioning value if not
	std::shared_ptr<repo_partitioning_tree_t> left;
	std::shared_ptr<repo_partitioning_tree_t> right;

	//Construction of branch node
	repo_partitioning_tree_t(
		const repo::PartitioningTreeType &type,
		const float &pValue,
		std::shared_ptr<repo_partitioning_tree_t> left,
		std::shared_ptr<repo_partitioning_tree_t> right)
		: type(type), pValue(pValue),
		left(left),
		right(right){}

	//Construction of leaf node
	repo_partitioning_tree_t(
		const std::vector<repo_mesh_entry_t> &meshes)
		:
		type(repo::PartitioningTreeType::LEAF_NODE),
		meshes(meshes), pValue(0),
		left(std::shared_ptr<repo_partitioning_tree_t>(nullptr)),
		right(std::shared_ptr<repo_partitioning_tree_t>(nullptr)){}
};

struct repo_diff_result_t{
	std::vector<repo::lib::RepoUUID> added; //nodes that does not exist on the other model
	std::vector<repo::lib::RepoUUID> modified; //nodes that exist on the other model but it is modified.
	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher > correspondence;
};
