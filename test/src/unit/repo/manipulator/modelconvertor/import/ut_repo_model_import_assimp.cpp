/**
*  Copyright (C) 2020 3D Repo Ltd
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
#include <repo/manipulator/modelconvertor/import/repo_model_import_assimp.h>

// #include <repo/lib/repo_log.h>
// #include "../../../../repo_test_utils.h"
// #include "../../../../repo_test_database_info.h"
// #include "boost/filesystem.hpp"
// #include "../../bouncer/src/repo/error_codes.h"

using namespace repo::manipulator::modelconvertor;

TEST(AssimpModelImport, MainTest)
{
	std::string bimFilePath = "";
	uint8_t impModelErrCode = 0;
	ModelImportConfig config;
	auto modelConvertor = std::unique_ptr<AbstractModelImport>(new AssimpModelImport(config));
	bool importedSuccesfully = modelConvertor->importModel(bimFilePath, impModelErrCode);
	EXPECT_TRUE(importedSuccesfully);
}


