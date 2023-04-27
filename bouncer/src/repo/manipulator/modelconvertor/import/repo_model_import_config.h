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
				bool rotateModel;
				bool importAnimations;
				double simplificationQuality;
				int simplificationMinVertexCount;
				std::string timeZone;
				ModelUnits targetUnits;
			public:
				ModelImportConfig(
					const bool applyReductions = true,
					const bool rotateModel = false,
					const bool importAnimations = true,
					const ModelUnits targetUnits = ModelUnits::UNKNOWN,
					const double quality = 0.0,
					const int minVertexCount = 0.0,
					const std::string timeZone = "") :
					applyReductions(applyReductions), rotateModel(rotateModel), importAnimations(importAnimations), targetUnits(targetUnits), simplificationQuality(quality), simplificationMinVertexCount(minVertexCount), timeZone(timeZone)
				{
				}

				~ModelImportConfig() {}

				bool shouldApplyReductions() const { return applyReductions; }
				bool shouldRotateModel() const { return rotateModel; }
				bool shouldImportAnimations() const { return importAnimations; }
				bool shouldSimplifyMeshes() const { return simplificationQuality > 0; }
				double getSimplificationQuality() const { return simplificationQuality; }
				int getSimplificationMinVertexCount() const { return simplificationMinVertexCount; }
				std::string getTimeZone() const { return timeZone; }
				ModelUnits getTargetUnits() const { return targetUnits; }
			};
		}//namespace modelconvertor
	}//namespace manipulator
}//namespace repo
