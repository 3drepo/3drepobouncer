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
#include <repo/lib/repo_log.h>
#include "../../../../repo_test_utils.h"

using namespace repo::manipulator::modelconvertor;
using namespace testing;

namespace RepoModelImportUtils
{
	static std::unique_ptr<AssimpModelImport> ImportFBXFile(
		std::string filePath,
		uint8_t& impModelErrCode)
	{
		ModelImportConfig config;
		auto modelConvertor = std::unique_ptr<AssimpModelImport>(new AssimpModelImport(config));
		modelConvertor->importModel(filePath, impModelErrCode);
		return modelConvertor;
	}
}

TEST(AssimpModelImport, MainTest)
{
	uint8_t errCode = 0;
	// non fbx file that does not have metadata
	auto importer = RepoModelImportUtils::ImportFBXFile(getDataPath("3DrepoBIM.obj"), errCode);
	EXPECT_FALSE(importer->requireReorientation());
	EXPECT_EQ(0, errCode);

	// file with enough metadata to reorientate
	importer = RepoModelImportUtils::ImportFBXFile(getDataPath("AssimpModelImport/AIM4G.fbx"), errCode);
	EXPECT_FALSE(importer->requireReorientation());
	EXPECT_EQ(0, errCode);
}

TEST(AssimpModelImport, SupportExtenions)
{
	EXPECT_TRUE(AssimpModelImport::isSupportedExts(".fbx"));
	EXPECT_TRUE(AssimpModelImport::isSupportedExts(".ifc"));
	EXPECT_TRUE(AssimpModelImport::isSupportedExts(".obj"));
}


