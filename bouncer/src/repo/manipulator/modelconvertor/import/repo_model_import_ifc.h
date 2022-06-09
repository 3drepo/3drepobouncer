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
* IFC Geometry convertor(Import)
*/

#pragma once

#include <string>
#include "repo_model_import_abstract.h"
#include "../../../core/model/bson/repo_node_material.h"
#include "../../../core/model/bson/repo_node_mesh.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class IFCModelImport : public AbstractModelImport
			{
			public:

				/**
				* Create IFCModelImport with specific settings
				* NOTE: The destructor will destroy the settings object referenced
				* in this object!
				* @param settings
				*/
				IFCModelImport(const ModelImportConfig & settings);

				/**
				* Default Deconstructor
				* NOTE: The destructor will destroy the settings object referenced
				* in this object!
				*/
				virtual ~IFCModelImport();

				/**
				* Generates a repo scene graph
				* an internal representation needs to have
				* been created before this call (e.g. by means of importModel())
				* @return returns a populated RepoScene upon success.
				*/
				virtual repo::core::model::RepoScene* generateRepoScene(uint8_t &errMsg);

				/**
				* Import model from a given file
				* This does not generate the Repo Scene Graph
				* Use getRepoScene() to generate a Repo Scene Graph.
				* @param path to the file
				* @param error message if failed
				* @return returns true upon success
				*/
				virtual bool importModel(std::string filePath, uint8_t &err);

				virtual bool requireReorientation() const { return true; }

			protected:

				std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> meshes;
				std::unordered_map<std::string, repo::core::model::MaterialNode*> materials;
				std::vector<double> offset;
				std::string ifcFile;
				bool partialFailure;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
