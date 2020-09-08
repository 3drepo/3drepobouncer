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
#include <repo/manipulator/modelconvertor/import/repo_model_import_synchro.h>
#include "../../../../repo_test_database_info.h"

using namespace repo::manipulator::modelconvertor;

TEST(SynchroModelImport, ConstructorTest)
{
	SynchroModelImport(ModelImportConfig());
}

TEST(SynchroModelImport, DeconstructorTest)
{
	auto ptr = new SynchroModelImport(ModelImportConfig());
	delete ptr;
}

TEST(SynchroModelImport, ImportModel)
{
	auto import = SynchroModelImport(ModelImportConfig());
	uint8_t errCode;
	EXPECT_TRUE(import.importModel(getDataPath(synchroWithTransform), errCode));
	EXPECT_EQ(0, errCode);

	auto scene = import.generateRepoScene();

	ASSERT_TRUE(scene);
}