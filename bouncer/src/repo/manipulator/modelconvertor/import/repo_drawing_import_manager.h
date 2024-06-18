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
#include <memory>
#include <vector>

#include "repo_model_import_config.h"
#include "../../../core/model/collection/repo_scene.h"
#include "repo_model_import_abstract.h"

namespace repo {
	namespace manipulator {
		namespace drawingconverter {

			/**
			* Holds complete information about an imported drawing at runtime
			*/
			struct DrawingImageInfo
			{
				std::string name; // The name of the original file (e.g. "Floor1.DWG")
				std::vector<uint8_t> data; // The drawing in svg format
			};

			class DrawingImportManager
			{
			public:

				DrawingImportManager() {}

				/**
				* Imports the drawing at file into the DrawingImageInfo object.
				* This method expects the source file to already be managed by
				* the FSHandler, so the type is provided explicitly. file should
				* already be fully qualified (i.e. it will not be combined with
				* filename).
				*/
				void importFromFile(
					DrawingImageInfo& drawing,
					const std::string &filename,
					const std::string &extension,
					uint8_t &error) const;
			};
		} //namespace drawingconverter
	} //namespace manipulator
} //namespace repo
