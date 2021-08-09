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
// #include "../../../../repo_test_utils.h"
// #include "../../../../repo_test_database_info.h"
// #include "boost/filesystem.hpp"
// #include "../../bouncer/src/repo/error_codes.h"

using namespace repo::manipulator::modelconvertor;

namespace RepoModelImportUtils
{
	static std::unique_ptr<AbstractModelImport> ImportFBXFile(
		std::string bimFilePath,
		uint8_t& impModelErrCode)
	{
		ModelImportConfig config;
		auto modelConvertor = std::unique_ptr<AbstractModelImport>(new AssimpModelImport(config));
		modelConvertor->importModel(bimFilePath, impModelErrCode);
		return modelConvertor;
	}
}

TEST(AssimpModelImport, MainTest)
{
	uint8_t errCode = 0;
	RepoModelImportUtils::ImportFBXFile(R"(C:\Users\haroo\Downloads\XYZ.arrows\XYZ arrows.FBX)", errCode);
	RepoModelImportUtils::ImportFBXFile(R"(C:\Users\haroo\Downloads\XYZ.arrows\XYZ arrows blender.FBX)", errCode);
	//RepoModelImportUtils::ImportFBXFile(R"(C:\DevProjects\tests\cplusplus\bouncer\data\models\unsupported.FBX)", errCode);
	RepoModelImportUtils::ImportFBXFile(R"(C:\Users\haroo\Downloads\apartment block.fbx)", errCode);
	RepoModelImportUtils::ImportFBXFile(R"(C:\Users\haroo\Downloads\AIM4G.fbx)", errCode);
	RepoModelImportUtils::ImportFBXFile(R"(C:\Users\haroo\Downloads\Ch44_nonPBR.fbx)", errCode);
	RepoModelImportUtils::ImportFBXFile(R"(C:\Users\haroo\Downloads\character.fbx)", errCode);
	
}




