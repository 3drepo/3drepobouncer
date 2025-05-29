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
* Abstract Model convertor(Import)
*/

#pragma once

#include <string>

#include "repo_model_import_config.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class AbstractModelImport
			{
			public:
				/**
				* Create AbstractModelImport with specific settings
				* NOTE: The destructor will destroy the settings object referenced
				* in this object!
				* @param settings
				*/
				AbstractModelImport(const ModelImportConfig &settings = ModelImportConfig());

				/**
				* Default Deconstructor
				* NOTE: The destructor will destroy the settings object referenced
				* in this object!
				*/
				virtual ~AbstractModelImport() {};

				/**
				* Generates a repo scene graph
				* an internal representation needs to have
				* been created before this call (e.g. by means of importModel())
				* @return returns a populated RepoScene upon success.
				*/
				virtual repo::core::model::RepoScene* generateRepoScene(uint8_t &errMsg) = 0;

				/**
				* Import model from a given file
				* This does not generate the Repo Scene Graph
				* Use getRepoScene() to generate a Repo Scene Graph.
				* @param path to the file
				* @param error message if failed
				* @return returns true upon success
				*/
				virtual bool importModel(std::string filePath, uint8_t& errMsg) = 0;

				virtual bool importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& errMsg)
				{
					return importModel(filePath, errMsg);
				}

				virtual bool applyReduction() const { return true; }
				virtual bool requireReorientation() const { return false; }

				ModelUnits getUnits() const { return modelUnits; }

			protected:
				const ModelImportConfig settings; /*! Stores related settings for model import */
				ModelUnits modelUnits = ModelUnits::UNKNOWN;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
