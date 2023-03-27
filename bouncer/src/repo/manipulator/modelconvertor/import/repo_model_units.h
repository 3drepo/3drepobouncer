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
			enum class REPO_API_EXPORT ModelUnits { METRES, MILLIMETRES, CENTIMETRES, DECIMETRES, FEET, INCHES, UNKNOWN };

			static std::string toUnitsString(const ModelUnits &units) {
				const std::string strlookUp[] = { "m", "mm", "cm", "dm", "ft", "in", "unknown" };
				int index = (int)units;
				return strlookUp[index];
			}

			static float scaleFactorToMetres(const ModelUnits &units) {
				const float toMetreLookUp[] = { 1.0, 0.001, 0.01, 0.1, 0.3048 , 0.0254 , 1.0 };
				int index = (int)units;
				return toMetreLookUp[index];
			}

			static float scaleFactorFromMetres(const ModelUnits &units) {
				const float fromMetreLookUp[] = { 1.0, 1000, 100, 10, 3.28084, 39.3701, 1.0 };
				int index = (int)units;
				return fromMetreLookUp[index];
			}
		}
	}
}
