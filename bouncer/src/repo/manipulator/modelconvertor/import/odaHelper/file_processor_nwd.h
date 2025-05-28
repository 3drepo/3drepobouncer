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

#include "repo/core/model/bson/repo_node_mesh.h"

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

					FileProcessorNwd(const std::string& inputFile, modelutility::RepoSceneBuilder* builder, const ModelImportConfig& config) : FileProcessor(inputFile, builder, config) {
						shouldApplyReduction = true;
					};
					~FileProcessorNwd() override;

					uint8_t readFile() override;

					/*
					* If the FileProcessorNwd will be used multiple times within a single
					* process - such as in the unit tests - then a shared system services
					* must be used, due to a possible ODA bug on Linux:
					* https://forum.opendesign.com/showthread.php?24792-Fonts-stop-working-after-odrxInitialize-odrxUninitialize
					*
					* To do so, call createSharedSystemServices() before readFile(). If
					* this is done, the responsibility for cleaning up the shared services
					* will fall to the caller. Otherwise, readFile will create and destroy
					* the services automatically, but must not be called again.
					*/
					static bool createSharedSystemServices();
					static void destorySharedSystemServices();
				};
			}
		}
	}
}
