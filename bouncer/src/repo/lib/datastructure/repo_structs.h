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
#include <memory>
#include "repo/repo_bouncer_global.h"
#include "repo/core/model/bson/repo_bson_assets.h"
#include "repo_uuid.h"
#include "repo_vector.h"
#include <boost/crc.hpp>

namespace repo {
	namespace lib {

		using repo_web_geo_files_t = std::unordered_map<std::string, std::vector<uint8_t>>;
		using repo_web_json_files_t = std::unordered_map<std::string, std::vector<uint8_t>>;

		struct repo_web_buffers_t {
			repo_web_geo_files_t geoFiles; //files where geometery are stored
			repo_web_json_files_t jsonFiles; //JSON mapping files
			repo::core::model::RepoAssets repoAssets; //RepoBundles assets list
		};

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

		enum class PartitioningTreeType { PARTITION_X, PARTITION_Y, PARTITION_Z, LEAF_NODE };
		enum class DiffMode { DIFF_BY_ID, DIFF_BY_NAME };

		struct repo_partitioning_tree_t {
			PartitioningTreeType              type;
			std::vector<repo_mesh_entry_t>            meshes; //mesh ids if it is a leaf node
			float                             pValue; //partitioning value if not
			std::shared_ptr<repo_partitioning_tree_t> left;
			std::shared_ptr<repo_partitioning_tree_t> right;

			//Construction of branch node
			repo_partitioning_tree_t(
				const PartitioningTreeType& type,
				const float& pValue,
				std::shared_ptr<repo_partitioning_tree_t> left,
				std::shared_ptr<repo_partitioning_tree_t> right)
				: type(type), pValue(pValue),
				left(left),
				right(right) {}

			//Construction of leaf node
			repo_partitioning_tree_t(
				const std::vector<repo_mesh_entry_t>& meshes)
				:
				type(PartitioningTreeType::LEAF_NODE),
				meshes(meshes), pValue(0),
				left(std::shared_ptr<repo_partitioning_tree_t>(nullptr)),
				right(std::shared_ptr<repo_partitioning_tree_t>(nullptr)) {}
		};

		struct repo_diff_result_t {
			std::vector<repo::lib::RepoUUID> added; //nodes that does not exist on the other model
			std::vector<repo::lib::RepoUUID> modified; //nodes that exist on the other model but it is modified.
			std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher > correspondence;
		};

		struct repo_color3d_t {
			float r;
			float g;
			float b;

			repo_color3d_t() :
				r(0),
				g(0),
				b(0)
			{
			}

			repo_color3d_t(float r, float g, float b) : r(r), g(g), b(b)
			{
			}

			repo_color3d_t(std::vector<float> v)
			{
				r = v[0];
				g = v[1];
				b = v[2];
			}

			repo_color3d_t operator* (float scalar)
			{
				return {
					r * scalar,
					b * scalar,
					g * scalar
				};
			}

			size_t checksum() const
			{
				std::hash<float> hasher;
				size_t seed = 0;
				seed ^= hasher(r) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= hasher(g) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= hasher(b) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				return seed;
			}

			/*
			* Returns whether the current state differs from the default constructor
			*/
			operator bool() const
			{
				return r != 0 || g != 0 || b != 0;
			}

			bool operator==(const repo_color3d_t& other) const
			{
				return r == other.r && g == other.g && b == other.b;
			}

			operator std::vector<float>() const
			{
				return { r, g, b };
			}
		};

		struct repo_color4d_t {
			float r;
			float g;
			float b;
			float a;

			repo_color4d_t() :
				r(0),
				g(0),
				b(0),
				a(0)
			{
			}

			repo_color4d_t(float r, float g, float b, float a) : r(r), g(g), b(b), a(a)
			{
			}

			repo_color4d_t(float r, float g, float b) : r(r), g(g), b(b), a(1)
			{
			}

			repo_color4d_t(const repo_color3d_t& color, float alpha) :
				r(color.r),
				g(color.g),
				b(color.b),
				a(alpha)
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

			size_t checksum() const
			{
				std::hash<float> hasher;
				size_t seed = 0;
				seed ^= hasher(r) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= hasher(g) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= hasher(b) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				seed ^= hasher(a) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				return seed;
			}

			operator bool() const
			{
				return r != 0 || g != 0 || b != 0 || a != 0;
			}

			bool operator==(const repo_color4d_t& other) const
			{
				return r == other.r && g == other.g && b == other.b && a == other.a;
			}
		};

		struct repo_material_t {
			repo_color3d_t ambient;
			repo_color3d_t diffuse;
			repo_color3d_t specular;
			repo_color3d_t emissive;
			float opacity = 1;
			float shininess = 0;
			float shininessStrength = 0;
			float lineWeight = 1;
			bool isWireframe = false;
			bool isTwoSided = false;

			std::string texturePath;

			size_t checksum() const {
				size_t seed = 0;
				checksum(seed, ambient.checksum());
				checksum(seed, diffuse.checksum());
				checksum(seed, specular.checksum());
				checksum(seed, emissive.checksum());
				checksum(seed, opacity);
				checksum(seed, shininess);
				checksum(seed, shininessStrength);
				checksum(seed, lineWeight);
				checksum(seed, isWireframe);
				checksum(seed, isTwoSided);
				checksum(seed, texturePath);
				return seed;
			}

			bool hasTexture() const {
				return !texturePath.empty();
			}

			static repo_material_t DefaultMaterial()
			{
				repo_material_t material;
				material.shininess = 0.0;
				material.shininessStrength = 0.0;
				material.opacity = 1;
				material.specular = { 0, 0, 0 };
				material.diffuse = { 0.5f, 0.5f, 0.5f };
				material.emissive = material.diffuse;
				return material;
			}

			bool operator==(const repo_material_t& other) const = default;

		private:
			template<typename T>
			static void checksum(size_t& seed, const T& v)
			{
				std::hash<T> hasher;
				seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}
		};

		//This is used to map info for multipart optimization		
		typedef struct {
			repo::lib::RepoVector3D min;
			repo::lib::RepoVector3D max;
			repo::lib::RepoUUID  mesh_id;
			repo::lib::RepoUUID  shared_id;
			repo::lib::RepoUUID  material_id; // MaterialNode Unique Id
			repo::lib::repo_material_t material;
			repo::lib::RepoUUID  texture_id;
			int32_t       vertFrom;
			int32_t       vertTo;
			int32_t       triFrom;
			int32_t       triTo;
		}repo_mesh_mapping_t;

		/*
		* A single object intended to be passed by value type that expresses a face with
		* up to three indices. In the future we could consider templating this type for
		* the number of indicies, but it is very unlikely we'd need to work with, e.g.
		* quads, and n-gons would have their own type anyway.
		*/
		struct repo_face_t {
			uint32_t indices[3];
			uint8_t sides;

			repo_face_t() :
				sides(0)
			{
				indices[0] = 0;
				indices[1] = 1;
				indices[2] = 2;
			}

			repo_face_t(std::initializer_list<size_t> indices) :
				sides(0)
			{
				for (auto i : indices) {
					push_back(i);
				}
			}

			repo_face_t(uint32_t* begin, uint32_t* end) :
				sides(0)
			{
				while (begin != end) {
					push_back(*begin);
					begin++;
				}
			}

			size_t size() const
			{
				return sides;
			}

			void resize(size_t sides)
			{
				this->sides = sides;
			}

			uint32_t& operator[](size_t i)
			{
				return indices[i];
			}

			uint32_t operator[](size_t i) const
			{
				return indices[i];
			}

			void push_back(uint32_t i)
			{
				indices[sides] = i;
				sides++;
			}

			bool operator==(const repo_face_t& other) const
			{
				if (sides != other.sides) {
					return false;
				}
				for (auto i = 0; i < sides; i++) {
					if (indices[i] != other[i]) {
						return false;
					}
				}
				return true;
			}

			class iterator : public std::iterator<
				std::forward_iterator_tag,
				uint32_t,
				uint8_t,
				uint32_t*,
				uint32_t> {

				uint8_t index;
				const repo_face_t& face;
			public:
				iterator(const repo_face_t& face, uint8_t index = 0) :face(face), index(index)
				{
				}

				iterator& operator++()
				{
					index++;
					return *this;
				}

				bool operator==(iterator other) const
				{
					return index == other.index;
				}

				bool operator!=(iterator other) const
				{
					return index != other.index;
				}

				reference operator*() const
				{
					return face[index];
				}
			};

			iterator begin() const
			{
				return iterator(*this, 0);
			}

			iterator end() const
			{
				return iterator(*this, size());
			}
		};
	}
}