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

#include <vector>
#include <string>

#include "geometry_collector.h"
#include "file_processor.h"

/*
 * FileProcessorNwd handles Navisworks NWD files; these are the files that contain
 * geometry of federations. Navisworks also outputs NWF (which are indices) and
 * NWCs (which are caches). These are not currently supported.
 * https://knowledge.autodesk.com/support/navisworks-products/troubleshooting/caas/sfdcarticles/sfdcarticles/NavisWorks-JetStream-file-formats-NWC-NWF-NWD-and-NWP.html
 */

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class FileProcessorNwd : public FileProcessor
				{
				public:

					//Environment variable name for Autodesk textures
					const char* RVT_TEXTURES_ENV_VARIABLE = "REPO_RVT_TEXTURES";

					FileProcessorNwd(const std::string& inputFile, GeometryCollector* geoCollector) : FileProcessor(inputFile, geoCollector) {
						shouldApplyReduction = true;
					};
					~FileProcessorNwd() override;

					uint8_t readFile() override;
				};
			}
		}
	}
}
