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
#include "repo/core/model/collection/repo_scene.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class RepoModelImport : public AbstractModelImport
			{
			private:
				std::unordered_map<uint8_t, uint8_t> FILE_META_BYTE_LEN_BY_VERSION =
				{
					{2, 56},
					{3, 72},
					{4, 72}
				};

				struct fileMeta
				{
					int64_t jsonSize	 = -1;	//!< Size of the entire JSON segment
					int64_t dataSize	 = -1;	//!< Size of the entire binary footer segment
					int64_t sizesStart	 = -1;	//!< Starting location of the JSON sizes array from the top of file in bytes
					int64_t sizesSize	 = -1;	//!< Length of the JSON sizes array in bytes
					int64_t matStart	 = -1;	//!< Starting location of the JSON materials array from the top of the file in bytes
					int64_t matSize		 = -1;	//!< Size of the JSON materials array in bytes
					int64_t numChildren	 = -1;	//!< Number of children of the root node
					int64_t textureStart = -1;	//!< Starting location of the JSON textures array from the top of the file in bytes
					int64_t textureSize  = -1;  //!< Size of the JSON textures array in bytes
				};

				// Source file meta data storage
				fileMeta file_meta;

				repo::core::model::RepoScene* scene;

			public:

				/**
				* Create RepoModelImport with specific settings
				* NOTE: The destructor will destroy the settings object referenced
				* in this object!
				* @param settings
				*/
				RepoModelImport(const ModelImportConfig &settings);

				/**
				* Default Deconstructor
				* NOTE: The destructor will destroy the settings object referenced
				* in this object!
				*/
				virtual ~RepoModelImport();

				/**
				* Import model from a given file.
				* Loads material nodes.
				* This does not generate the Repo Scene Graph
				* Use generateRepoScene() to generate a Repo Scene Graph.
				* @param path to the file
				* @param database handler
				* @param error message if failed
				* @return returns a populated RepoScene upon success
				*/
				virtual repo::core::model::RepoScene* importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t &errMsg);
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
