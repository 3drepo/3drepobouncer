/**
*  Copyright (C) 2019 3D Repo Ltd
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

#include <string>
#include <memory>

#include "repo_model_import_config.h"
#include "../../../core/model/collection/repo_scene.h"
#include "repo_model_import_abstract.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class ModelImportManager
			{
			public:
				ModelImportManager() {}

				repo::core::model::RepoScene* ImportFromFile(
					const std::string &file,
					const repo::manipulator::modelconvertor::ModelImportConfig &config,
					uint8_t &error) const;

			private:
				std::shared_ptr<AbstractModelImport> chooseModelConvertor(
					const std::string &file,
					const repo::manipulator::modelconvertor::ModelImportConfig &config
				) const;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
