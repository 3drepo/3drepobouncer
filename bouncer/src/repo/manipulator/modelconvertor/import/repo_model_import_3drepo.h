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

			const std::string REPO_V1 = "BIM001";
			const std::string REPO_V2 = "BIM002";

			const std::set<std::string> supportedFileVersions = { REPO_V1, REPO_V2 };
			const static int REPO_VERSION_LENGTH = 6;

			class RepoModelImport : public AbstractModelImport
			{
				private:
					std::vector<repo::core::model::RepoNode *> node_map;
					std::vector<repo::lib::RepoMatrix> trans_map;
					bool is32Bit = false;

					void createObject(const boost::property_tree::ptree& tree);

					char *geomBuf;
					std::ifstream *finCompressed;
					boost::iostreams::filtering_streambuf<boost::iostreams::input> *inbuf;
					std::istream *fin;

					typedef struct
					{
						int64_t headerSize;
						int64_t geometrySize;
						int64_t sizesStart;
						int64_t sizesSize;
						int64_t matStart;
						int64_t matSize;
						int64_t numChildren;
					} fileMeta; 

					struct mesh_data_t {
						std::vector<repo::lib::RepoVector3D64> rawVertices;
						std::vector<repo::lib::RepoVector3D> normals;
						std::vector<repo_face_t> faces;
						std::vector<std::vector<double>> boundingBox;
						repo::lib::RepoUUID parent;
						repo::lib::RepoUUID sharedID;
					};

					fileMeta file_meta;

					std::vector<long> sizes;

					repo::core::model::MaterialNode* parseMaterial(const boost::property_tree::ptree &pt);

					repo::core::model::MetadataNode*  createMetadataNode(const boost::property_tree::ptree &metadata, const std::string &parentName, const repo::lib::RepoUUID &parentID);
					mesh_data_t createMeshRecord(const boost::property_tree::ptree &geometry, const std::string &parentName, const repo::lib::RepoUUID &parentID, const repo::lib::RepoMatrix &trans);
					boost::property_tree::ptree getNextJSON(long jsonSize);
					void skipAheadInFile(long amount);

					std::vector<repo::core::model::MaterialNode *> matNodeList;
					std::vector<std::vector<repo::lib::RepoUUID>> matParents;

					std::vector<mesh_data_t> meshEntries;
					repo::core::model::RepoNodeSet cameras; //!< Cameras
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
					virtual bool importModel(std::string filePath, uint8_t &errMsg);
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

			template <typename T>
			inline std::vector<T> as_vector(const boost::property_tree::ptree &pt)
			{
					std::vector<T> r;
					for (const auto& item : pt)
							r.push_back(item.second.get_value<T>());
					return r;
			}			
		}
	}
}
