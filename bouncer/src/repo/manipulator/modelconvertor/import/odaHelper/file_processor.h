/**
*  Copyright (C) 2018 3D Repo Ltd
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
#include "geometry_collector.h"
#include "../../../modelutility/repo_drawing.h"
#include "../repo_model_import_config.h"
#include "../../../../error_codes.h"
#include <string>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class FileProcessor
				{
				protected:
					FileProcessor(const std::string& inputFile, GeometryCollector* geoCollector, const ModelImportConfig& config);
					FileProcessor(const std::string& inputFile, modelutility::DrawingImageInfo* collector);

				public:
					static std::unique_ptr<FileProcessor> getFileProcessor(const std::string& inputFile, GeometryCollector* geoCollector, const ModelImportConfig& config);
					virtual ~FileProcessor();
					virtual uint8_t readFile() = 0;
					bool shouldApplyReduction = false;

				protected:
					const std::string file;
					GeometryCollector *collector;
					modelutility::DrawingImageInfo* drawingCollector;
					const ModelImportConfig& importConfig;
				};
			}
		}
	}
}
