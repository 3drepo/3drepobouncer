/**
*  Copyright (C) 2015 3D Repo Ltd
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

/**
* 3DRepo Geometry convertor(Import)
*/

#pragma once

#include <string>
#include "repo_model_import_abstract.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include "repo_model_import_abstract.h"
#include "../../../core/model/collection/repo_scene.h"
#include "../../../core/model/bson/repo_node_camera.h"
#include "../../../core/model/bson/repo_node_material.h"
#include "../../../core/model/bson/repo_node_mesh.h"
#include "../../../core/model/bson/repo_node_metadata.h"
#include "../../../core/model/bson/repo_node_transformation.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{

			const char REPO_IMPORT_TYPE_STRING   = 'S';
			const char REPO_IMPORT_TYPE_DOUBLE   = 'D';
			const char REPO_IMPORT_TYPE_INT      = 'I';
			const char REPO_IMPORT_TYPE_BOOL     = 'B';
			const char REPO_IMPORT_TYPE_DATETIME = 'T';

			const std::string REPO_IMPORT_METADATA = "metadata";
			const std::string REPO_IMPORT_GEOMETRY = "geometry";
			const std::string REPO_IMPORT_MATERIAL = "material";

			const std::string REPO_IMPORT_VERTICES = "vertices";
			const std::string REPO_IMPORT_NORMALS = "normals";
			const std::string REPO_IMPORT_INDICES = "indices";

			const std::string REPO_IMPORT_BBOX = "bbox";

			const char supportedFileVersion[7] = "BIM001";

			// taken from http://stackoverflow.com/questions/6899392/generic-hash-function-for-all-stl-containers			
			template <class T>
			inline void hash_combine(std::size_t & seed, const T & v)
			{
				std::hash<T> hasher;
				seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			}

			class MaterialHasher
			{
				static const int NUM_DP_POW = 1000.0f;

				private:
					static void hashColor(size_t &h, const std::vector<float> color)
					{
						hash_combine(h, (int)(color[0] * NUM_DP_POW));
						hash_combine(h, (int)(color[1] * NUM_DP_POW));
						hash_combine(h, (int)(color[2] * NUM_DP_POW));
					}
				public:
					size_t operator() (repo_material_t const& key) const
					{
						std::hash<float> floatHasher;

						size_t h = 0;

						hashColor(h, key.ambient);
						hashColor(h, key.specular);
						hashColor(h, key.diffuse);
						hashColor(h, key.emissive);

						hash_combine(h, (int)(key.opacity * NUM_DP_POW));
						hash_combine(h, (int)(key.shininess * NUM_DP_POW));
						hash_combine(h, (int)(key.shininessStrength * NUM_DP_POW));
						hash_combine(h, key.isWireframe);
						hash_combine(h, key.isTwoSided);
						
						return h;
					}
			};

			class MaterialHashEqual
			{
				public:
					bool operator() (repo_material_t const& t1, repo_material_t const& t2) const
					{
						MaterialHasher hasher;
						return hasher(t1) == hasher(t2);
					}
			};

			class RepoModelImport : public AbstractModelImport
			{
				private:
					std::vector<repo::core::model::RepoNode *> node_map;
					std::vector<repo::lib::RepoMatrix> trans_map;

					void createObject(const boost::property_tree::ptree& tree);

					char *geomBuf;
					std::ifstream *finCompressed;
					boost::iostreams::filtering_streambuf<boost::iostreams::input> *inbuf;
					std::istream *fin;

					int64_t sizesStart;
					int64_t sizesSize;
					int64_t numChildren;

					std::vector<long> sizes;

					repo::core::model::MetadataNode* createMetadataNode(const boost::property_tree::ptree &metadata, const std::string &parentName, const repo::lib::RepoUUID &parentID);
					repo::core::model::MeshNode* createMeshNode(const boost::property_tree::ptree &geometry, const std::string &parentName, const repo::lib::RepoUUID &parentID, const repo::lib::RepoMatrix &trans);
					void createMaterialNode(const boost::property_tree::ptree& material, const std::string &parentName, const repo::lib::RepoUUID &parentID);
					boost::property_tree::ptree getNextJSON(long jsonSize);
					void skipAheadInFile(long amount);

					std::unordered_map<repo_material_t, repo::core::model::MaterialNode *, MaterialHasher, MaterialHashEqual> materialMap;

					repo::core::model::RepoNodeSet cameras; //!< Cameras
					repo::core::model::RepoNodeSet meshes; //!< Meshes
					repo::core::model::RepoNodeSet materials; //!< Materials
					repo::core::model::RepoNodeSet metadata; //!< Metadata
					repo::core::model::RepoNodeSet transformations; //!< Transformations
					repo::core::model::RepoNodeSet textures;

					std::string orgFile;

					std::vector<double> offset;

				public:

					/**
					* Create IFCModelImport with specific settings
					* NOTE: The destructor will destroy the settings object referenced
					* in this object!
					* @param settings
					*/
					RepoModelImport(const ModelImportConfig *settings = nullptr);

					/**
					* Default Deconstructor
					* NOTE: The destructor will destroy the settings object referenced
					* in this object!
					*/
					virtual ~RepoModelImport();

					/**
					* Generates a repo scene graph
					* an internal representation needs to have
					* been created before this call (e.g. by means of importModel())
					* @return returns a populated RepoScene upon success.
					*/
					virtual repo::core::model::RepoScene* generateRepoScene();

					/**
					* Import model from a given file
					* This does not generate the Repo Scene Graph
					* Use getRepoScene() to generate a Repo Scene Graph.
					* @param path to the file
					* @param error message if failed
					* @return returns true upon success
					*/
					virtual bool importModel(std::string filePath, std::string &errMsg);
			};

			//http://stackoverflow.com/questions/23481262/using-boost-property-tree-to-read-int-array
			template <typename T>
			inline std::vector<T> as_vector(const boost::property_tree::ptree &pt, const boost::property_tree::ptree::key_type &key)
			{
					std::vector<T> r;
					for (const auto& item : pt.get_child(key))
							r.push_back(item.second.get_value<T>());
					return r;
			}
		}
	}
}
