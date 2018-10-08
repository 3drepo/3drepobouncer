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
#include "repo_uuid.h"
#include "repo_vector.h"
#include <boost/crc.hpp>


typedef struct {
	std::unordered_map<std::string, std::vector<uint8_t>> geoFiles; //files where geometery are stored
	std::unordered_map<std::string, std::vector<uint8_t>> jsonFiles; //JSON mapping files
}repo_web_buffers_t;

//This is used to map info for multipart optimization
typedef struct{
	repo::lib::RepoVector3D min;
	repo::lib::RepoVector3D max;
	repo::lib::RepoUUID  mesh_id;
	repo::lib::RepoUUID  material_id;
	int32_t       vertFrom;
	int32_t       vertTo;
	int32_t       triFrom;
	int32_t       triTo;
}repo_mesh_mapping_t;

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

typedef struct{
	std::vector<float> ambient;
	std::vector<float> diffuse;
	std::vector<float> specular;
	std::vector<float> emissive;
	float opacity = 1;
	float shininess = 0;
	float shininessStrength = 0;
	bool isWireframe = false;
	bool isTwoSided = false;


	unsigned int checksum() const {
		std::stringstream ss;
		ss.precision(17);
		for (const auto &n : ambient) {
			ss << std::fixed <<  n;
		}
		for (const auto &n : diffuse) {
			ss << std::fixed << n;
		}
		for (const auto &n : specular) {
			ss << std::fixed << n;
		}
		for (const auto &n : emissive) {
			ss << std::fixed << n;
		}

		ss << opacity << shininess << shininessStrength << isWireframe << isTwoSided;
		auto stringified = ss.str();

		boost::crc_32_type crc32;
		crc32.process_bytes(stringified.c_str(), stringified.size());
		return crc32.checksum();
	}
}repo_material_t;

typedef struct{
	float r;
	float g;
	float b;
	float a;
}repo_color4d_t;

typedef std::vector<uint32_t> repo_face_t;

static std::string toString(const repo_face_t &f)
{
	std::string str;
	unsigned int mNumIndices = f.size();

	str += "[";
	for (unsigned int i = 0; i < mNumIndices; i++)
	{
		str += std::to_string(f[i]);
		if (i != mNumIndices - 1)
			str += ", ";
	}
	str += "]";
	return str;
}

static std::string toString(const repo_color4d_t &color)
{
	std::stringstream sstr;
	sstr << "[" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << "]";
	return sstr.str();
}
