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
#include "../../../lib/repo_log.h"

#include "../../../repo_bouncer_global.h"

#include "repo_model_units.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			class REPO_API_EXPORT ModelImportConfig
			{
			private:
				bool applyReductions;
				bool importAnimations;
				int lod;
				std::string timeZone;
				ModelUnits targetUnits;
			public:
				ModelImportConfig(
					const bool applyReductions = true,
					const bool importAnimations = true,
					const ModelUnits targetUnits = ModelUnits::UNKNOWN,
					const std::string timeZone = "",
					const int lod = 0) :
					applyReductions(applyReductions),
					importAnimations(importAnimations),
					targetUnits(targetUnits),
					timeZone(timeZone),
					lod(lod) {}

				~ModelImportConfig() {}

				bool shouldApplyReductions() const { return applyReductions; }
				bool shouldImportAnimations() const { return importAnimations; }
				std::string getTimeZone() const { return timeZone; }
				ModelUnits getTargetUnits() const { return targetUnits; }
				int getLevelOfDetail() const { return lod; }
			};
		}//namespace modelconvertor
	}//namespace manipulator
}//namespace repo
