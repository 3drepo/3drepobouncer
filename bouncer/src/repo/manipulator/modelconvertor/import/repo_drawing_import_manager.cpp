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
#include <repo_log.h>
#include "repo/lib/repo_utils.h"
#include "repo/error_codes.h"

#ifdef ODA_SUPPORT
#include "../../modelconvertor/import/odaHelper/file_processor_dgn.h"
#include "../../modelconvertor/import/odaHelper/file_processor_dwg.h"
#endif

using namespace repo::manipulator::modelconvertor;
using namespace repo::manipulator::modelutility;

void DrawingImportManager::importFromFile(
	DrawingImageInfo& drawing,
	const std::string& filename,
	const std::string& extension,
	uint8_t &error
) const {
	repoTrace << "Importing drawing...";

	if (!repo::lib::doesFileExist(filename)) {
		error = REPOERR_MODEL_FILE_READ;
		repoError << "Cannot find file: " << filename;
		return;
	}

	// Choose the file processor based on the format
#ifdef ODA_SUPPORT
	if (extension == ".dwg")
	{
		repo::manipulator::modelconvertor::odaHelper::FileProcessorDwg dwg(filename, &drawing);
		error = dwg.readFile();
	}
	else if (extension == ".dgn")
	{
		repo::manipulator::modelconvertor::odaHelper::FileProcessorDgn dgn(filename, &drawing);
		error = dgn.readFile();
	}
	else
	{
		repoError << "Unable to identify suitable importer for " << extension;
		error = REPOERR_FILE_TYPE_NOT_SUPPORTED;
	}
#else
	error = REPOERR_ODA_UNAVAILABLE;
#endif
}