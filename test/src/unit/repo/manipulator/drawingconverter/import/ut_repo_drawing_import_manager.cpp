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

#include <gtest/gtest.h>
#include <repo/manipulator/drawingconverter/import/repo_drawing_import_manager.h>
#include <repo/lib/repo_log.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_database_info.h"
#include "boost/filesystem.hpp"
#include "../../bouncer/src/repo/error_codes.h"

using namespace repo::manipulator::drawingconverter;

TEST(DrawingImportManager, DGN)
{
	// Tests that the importer can handle a DGN

	DrawingImportManager manager;
	DrawingImageInfo drawing;
	uint8_t error;
	manager.importFromFile(drawing, getDataPath(dgnDrawing), "DGN", error);

	EXPECT_EQ(error, REPOERR_OK);
}

TEST(DrawingImportManager, DWG)
{
	// Tests that the importer can handle a DWG

	DrawingImportManager manager;
	DrawingImageInfo drawing;
	uint8_t error;
	manager.importFromFile(drawing, getDataPath(dwgDrawing), "DWG", error);

	EXPECT_EQ(error, REPOERR_OK);
}