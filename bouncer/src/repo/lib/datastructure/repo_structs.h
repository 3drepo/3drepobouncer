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
#include "../../core/model/bson/repo_bson_assets.h"
#include "repo_uuid.h"
#include "repo_vector.h"
#include <boost/crc.hpp>

using repo_web_geo_files_t = std::unordered_map<std::string, std::vector<uint8_t>>;
using repo_web_json_files_t = std::unordered_map<std::string, std::vector<uint8_t>>;

struct repo_web_buffers_t{
	repo_web_geo_files_t geoFiles; //files where geometery are stored
	repo_web_json_files_t jsonFiles; //JSON mapping files
	repo::core::model::RepoAssets repoAssets; //RepoBundles assets list
};

//This is used to map info for multipart optimization
typedef struct {
	repo::lib::RepoVector3D min;
	repo::lib::RepoVector3D max;
	repo::lib::RepoUUID  mesh_id;
	repo::lib::RepoUUID  shared_id;
	repo::lib::RepoUUID  material_id; // MaterialNode Unique Id
	int32_t       vertFrom;
	int32_t       vertTo;
	int32_t       triFrom;
	int32_t       triTo;
}repo_mesh_mapping_t;

static bool operator== (repo_mesh_mapping_t a, repo_mesh_mapping_t b) 
{
	return
		a.min == b.min &&
		a.max == b.max &&
		a.mesh_id == b.mesh_id &&
		a.shared_id == b.shared_id &&
		a.material_id == b.material_id &&
		a.vertFrom == b.vertFrom &&
		a.vertTo == b.vertFrom &&
		a.triFrom == b.triFrom &&
		a.triTo == b.triTo;
}

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

namespace repo {
	enum class PartitioningTreeType { PARTITION_X, PARTITION_Y, PARTITION_Z, LEAF_NODE };
	enum class DiffMode { DIFF_BY_ID, DIFF_BY_NAME };
}

struct repo_partitioning_tree_t {
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
		right(right) {}

	//Construction of leaf node
	repo_partitioning_tree_t(
		const std::vector<repo_mesh_entry_t> &meshes)
		:
		type(repo::PartitioningTreeType::LEAF_NODE),
		meshes(meshes), pValue(0),
		left(std::shared_ptr<repo_partitioning_tree_t>(nullptr)),
		right(std::shared_ptr<repo_partitioning_tree_t>(nullptr)) {}
};

struct repo_diff_result_t {
	std::vector<repo::lib::RepoUUID> added; //nodes that does not exist on the other model
	std::vector<repo::lib::RepoUUID> modified; //nodes that exist on the other model but it is modified.
	std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher > correspondence;
};

typedef struct {
	std::vector<float> ambient;
	std::vector<float> diffuse;
	std::vector<float> specular;
	std::vector<float> emissive;
	float opacity = 1;
	float shininess = 0;
	float shininessStrength = 0;
	float lineWeight = 1;
	bool isWireframe = false;
	bool isTwoSided = false;

	std::string texturePath;

	unsigned int checksum() const {
		std::stringstream ss;
		ss.precision(17);
		for (const auto &n : ambient) {
			ss << std::fixed << n;
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
		for (const auto &n : texturePath) {
			ss << n;
		}

		ss << opacity << shininess << shininessStrength << lineWeight << isWireframe << isTwoSided;
		auto stringified = ss.str();

		boost::crc_32_type crc32;
		crc32.process_bytes(stringified.c_str(), stringified.size());
		return crc32.checksum();
	}
}repo_material_t;

struct repo_color4d_t {
	float r;
	float g;
	float b;
	float a;

	repo_color4d_t()
	{
		a = 1;
	}

	repo_color4d_t(float r, float g, float b, float a) : r(r), g(g), b(b), a(a)
	{
	}

	repo_color4d_t(float r, float g, float b) : r(r), g(g), b(b), a(1)
	{
	}

	repo_color4d_t(std::vector<float> v)
	{
		r = v[0];
		g = v[1];
		b = v[2];
		if (v.size() > 3) {
			a = v[3];
		}
		else
		{
			a = 1;
		}
	}

	repo_color4d_t(std::vector<float> v, float a)
	{
		r = v[0];
		g = v[1];
		b = v[2];
		this->a = a;
	}

	repo_color4d_t operator* (float scalar)
	{
		return {
			r * scalar,
			b * scalar,
			g * scalar,
			a * scalar
		};
	}
};

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
