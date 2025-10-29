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

#include "repo/repo_bouncer_global.h"

namespace repo {
	namespace lib {
		enum class REPO_API_EXPORT ModelUnits { 
			METRES,
			MILLIMETRES,
			CENTIMETRES,
			DECIMETRES,
			FEET,
			INCHES,
			UNKNOWN 
		};

		namespace units {
			REPO_API_EXPORT std::string toUnitsString(const ModelUnits& units);

			REPO_API_EXPORT ModelUnits fromString(const std::string& unitsStr);

			REPO_API_EXPORT double scaleFactorToMetres(const ModelUnits& units);

			REPO_API_EXPORT double scaleFactorFromMetres(const ModelUnits& units);

			REPO_API_EXPORT double determineScaleFactor(const ModelUnits& base, const ModelUnits& target);
		}
	}
}
