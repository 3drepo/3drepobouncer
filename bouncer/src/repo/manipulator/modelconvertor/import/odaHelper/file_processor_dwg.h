/**
*  Copyright (C) 2021 3D Repo Ltd
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

#include <OdaCommon.h>
#include <DbDatabase.h>
#include <RxObjectImpl.h>
#include <ExSystemServices.h>
#include <ExHostAppServices.h>

#include "file_processor.h"
#include "geometry_collector.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {

				class RepoDwgServices : public ExSystemServices, public ExHostAppServices
				{
				protected:
					ODRX_USING_HEAP_OPERATORS(ExSystemServices);
				};

				class FileProcessorDwg : public FileProcessor
				{
				public:
					FileProcessorDwg(const std::string& inputFile, GeometryCollector* geoCollector, const ModelImportConfig& config) : FileProcessor(inputFile, geoCollector, config) {}
					FileProcessorDwg(const std::string& inputFile, repo::manipulator::modelutility::DrawingImageInfo* collector) : FileProcessor(inputFile, collector) {}
					~FileProcessorDwg() override {}
					uint8_t readFile();

				protected:
					OdStaticRxObject<RepoDwgServices> svcs;
				};
			}
		}
	}
}