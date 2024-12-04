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
#include <gmock/gmock.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_ifc.h>
#include <repo/lib/repo_log.h>
#include "../../../../repo_test_utils.h"

using namespace repo::manipulator::modelconvertor;
using namespace testing;

namespace RepoModelImportUtils
{
	static std::unique_ptr<IFCModelImport> ImportIFC(
		std::string filePath,
		uint8_t& impModelErrCode)
	{
		ModelImportConfig config;
		auto modelConvertor = std::make_unique<IFCModelImport>(config);
		modelConvertor->importModel(filePath, impModelErrCode);
		return modelConvertor;
	}
}

TEST(IfcModelImport, Unicode)
{
	uint8_t errMsg;

	auto importer = RepoModelImportUtils::ImportIFC(getDataPath("duct.ifc"), errMsg);
	auto scene = importer->generateRepoScene(errMsg);

	EXPECT_THAT(scene, IsTrue());

	// Check the metadata entries to ensure special units use proper unicode
	// encoding
	// (Note if this test breaks it is most likely a compiler flags issue)

	auto metadata = scene->getAllMetadata(repo::core::model::RepoScene::GraphType::DEFAULT);
	for (auto n : metadata)
	{
		auto mn = dynamic_cast<repo::core::model::MetadataNode*>(n);
		if (mn->getName() == "Oval Duct:Standard:329435")
		{
			auto md = mn->getAllMetadata();
			for (auto& m : md)
			{
				if (m.first.starts_with("Mechanical::Area"))
				{
					// The full key is Mechanical::Area (m²), and the last bytes
					// should be {0x28, 0x6d, 0xc2, 0xb2, 0x29} (the superscript
					// is encoded as 0xc2, 0xb2 in unicode.

					const uint8_t s[] = { 0x28, 0x6d, 0xc2, 0xb2, 0x29 };

					EXPECT_THAT(m.first.length(), Eq(22));
					EXPECT_THAT(memcmp(m.first.c_str() + m.first.size() - 5, s, 5), Eq(0));
				}
			}
		}
	}








}
