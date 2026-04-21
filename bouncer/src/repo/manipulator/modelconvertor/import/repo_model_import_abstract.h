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
				* Import model from a given file
				* @param path to the file
				* @param database handler
				* @param error message if failed
				* @return returns a populated RepoScene upon success
				*/
				virtual repo::core::model::RepoScene* importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& errMsg) = 0;

				virtual bool applyReduction() const { return true; }
				virtual bool requireReorientation() const { return false; }

				repo::lib::ModelUnits getUnits() const { return modelUnits; }

			protected:
				const ModelImportConfig settings; /*! Stores related settings for model import */
				repo::lib::ModelUnits modelUnits = repo::lib::ModelUnits::UNKNOWN;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
