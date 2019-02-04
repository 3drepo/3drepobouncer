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

#include "../../../../core/model/bson/repo_node_mesh.h"

#include <OdaCommon.h>
#include <Gs/GsBaseInclude.h>
#include <RxObjectImpl.h>

#include <vector>
#include <string>

#include "geometry_collector.h"
#include "file_processor.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class FileProcessorRvt : public FileProcessor
				{
				public:
					FileProcessorRvt(const std::string& inputFile, GeometryCollector* geoCollector) : FileProcessor(inputFile, geoCollector) {};
					~FileProcessorRvt() override;

					int readFile() override;

				private:
					//.. adjust triangulation options
					void setMaxEdgeLength(double edgeLength);
				};
			}
		}
	}
}

