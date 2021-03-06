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
* General Model Convertor Config
* used to track all settings required for Model Convertor
*/

#pragma once

#include <string>
#include <map>
#include <vector>
#include <stdint.h>

#include "../../../repo_bouncer_global.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class REPO_API_EXPORT ModelImportConfig
			{
			private:
				bool applyReductions;
				bool rotateModel;
				bool importAnimations;
			public:
				ModelImportConfig(
					const bool applyReductions = true,
					const bool rotateModel = false,
					const bool importAnimations = true
				) {
					this->applyReductions = applyReductions;
					this->rotateModel = rotateModel;
					this->importAnimations = importAnimations;
				}
				~ModelImportConfig() {}

				bool shouldApplyReductions() const { return applyReductions; }
				bool shouldRotateModel() const { return rotateModel; }
				bool shouldImportAnimations() const { return importAnimations; }
			};
		}//namespace modelconvertor
	}//namespace manipulator
}//namespace repo
