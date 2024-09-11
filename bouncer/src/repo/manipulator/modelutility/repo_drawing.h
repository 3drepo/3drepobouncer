/**
*  Copyright (C) 2024 3D Repo Ltd
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
#include <vector>
#include "repo/lib/datastructure/repo_vector.h"

namespace repo {
	namespace manipulator {
		namespace modelutility {

			/**
			* Holds outcome of the autocalibration
			*/
			struct DrawingCalibration {
				std::vector<repo::lib::RepoVector3D> horizontalCalibration3d;
				std::vector<repo::lib::RepoVector2D> horizontalCalibration2d;
				std::vector<float> verticalRange;
				std::string units;
			};

			/**
			* Holds complete information about an imported drawing at runtime
			*/
			struct DrawingImageInfo
			{
				std::string name; // The name of the original file (e.g. "Floor1.DWG")
				std::vector<uint8_t> data; // The drawing in svg format
				DrawingCalibration calibration;
			};
		}
	}
}