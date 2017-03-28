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
* Allows Export functionality from 3D Repo World to SRC
*/

#pragma once

#include <string>

#include "repo_model_export_abstract.h"
#include "../../../lib/repo_property_tree.h"
#include "../../../lib/datastructure/repo_structs.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			enum class WebExportType { GLTF, SRC, UNITY };

			class WebModelExport : public AbstractModelExport
			{
			public:
				/**
				* Default Constructor, export model with default settings
				* @param scene repo scene to convert
				*/
				WebModelExport(const repo::core::model::RepoScene *scene);

				/**
				* Default Destructor
				*/
				virtual ~WebModelExport();

				/**
				* Export the repo scene graph to file
				* @param filePath path to destination file
				* @return returns true upon success
				*/
				virtual bool exportToFile(
					const std::string &filePath);

				/**
				* Export all necessary files as buffers
				* @return returns a repo_src_export_t containing all files needed for this
				*          model to be rendered
				*/
				virtual repo_web_buffers_t getAllFilesExportedAsBuffer() const = 0;

				/**
				* Return the JSON file as raw bytes buffer
				* returns an empty map if the export has failed
				*/
				virtual std::unordered_map<std::string, std::vector<uint8_t>> getJSONFilesAsBuffer() const;

				/**
				* Get supported file formats for this exporter
				*/
				static std::string getSupportedFormats();

				/**
				* Returns the status of the converter,
				* whether it has successfully converted the model
				* @return returns true if success
				*/
				bool isOk() const
				{
					return convertSuccess;
				}

			protected:
				bool convertSuccess;
				repo::core::model::RepoScene::GraphType gType;
				std::unordered_map<std::string, repo::lib::PropertyTree> trees;
				std::unordered_map<std::string, repo::lib::PropertyTree> jsonTrees;

			private:
				std::string sanitizeFileName(
					const std::string &name) const;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
