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

#include <repo/manipulator/modelconvertor/import/repo_model_import_3drepo.h>
#include <repo/lib/repo_log.h>

using namespace repo::manipulator::modelconvertor;

TEST(RepoModelImport, ImportModel)
{
	ModelImportConfig config;

	auto modelConvertor = std::shared_ptr<AbstractModelImport>(new RepoModelImport(config));

	uint8_t errCode = 0;

	std::string filePath = R"(C:\Users\haroo\Desktop\BIMTextureFiles\cube_bim3.bim)";

	modelConvertor->importModel(filePath, errCode);

	repoInfo << "Error code from importModel(): " << errCode <<std::endl;

	auto repoScene = modelConvertor->generateRepoScene(errCode);

	repoInfo << "Error code from generateRepoScene(): " << errCode << std::endl;


	EXPECT_EQ(0,errCode);
}