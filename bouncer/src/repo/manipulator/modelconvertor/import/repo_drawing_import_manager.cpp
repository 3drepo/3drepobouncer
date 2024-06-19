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

#include "repo_drawing_import_manager.h"
#include "../../../lib/repo_utils.h"
#include "../../../error_codes.h"
#include "../../modelconvertor/import/odaHelper/file_processor.h"

using namespace repo::manipulator::modelconvertor;
using namespace repo::manipulator::modelutility;

void DrawingImportManager::importFromFile(
	DrawingImageInfo& drawing,
	const std::string& filename,
	const std::string& extension,
	uint8_t& error
) const {
	repoTrace << "Importing drawing...";

	if (!repo::lib::doesFileExist(filename)) {
		error = REPOERR_MODEL_FILE_READ;
		repoError << "Cannot find file: " << filename;
		return;
	}

	auto processor = odaHelper::FileProcessor::getFileProcessor(filename, extension, &drawing);
	if (processor) {
		error = processor->readFile();
	}
	else {
		repoError << "Unable to identify suitable importer for " << extension;
		error = REPOERR_FILE_TYPE_NOT_SUPPORTED;
	}
}