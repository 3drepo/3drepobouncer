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
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_oda.h>
#include <repo/lib/repo_log.h>
#include <repo/error_codes.h>
#include "../../../../../repo_test_utils.h"
#include "../../../../../repo_test_database_info.h"
#include "../../../../../repo_test_scenecomparer.h"
#include "boost/filesystem.hpp"

#include <repo/manipulator/modelutility/repo_scene_builder.h>

using namespace repo::manipulator::modelconvertor;
using namespace repo::core::model;
using namespace testing;

#define DBNAME "ODAModelImport"

#pragma optimize("",off)

/* ODA is predominantly tested via the system tests using the client. These tests
 * cover specific features or regressions. */

 /*
 * To get the reference data for these tests, import models into the
 * ODAModelImport database using a known-good bouncer.
 * Some tests assume that the scenes have not had any optimisations applied.
 * Depending on the age of the known-good version, this may require a custom
 * build since not all versions of bouncer exposed this option to the config.
 */

namespace ODAModelImportUtils
{
	std::unique_ptr<AbstractModelImport> ImportFile(
		std::string filePath,
		uint8_t& impModelErrCode)
	{
		ModelImportConfig config;
		auto modelConvertor = std::unique_ptr<AbstractModelImport>(new OdaModelImport(config));
		modelConvertor->importModel(filePath, impModelErrCode);
		return modelConvertor;
	}

	void ImportFileWithoutOptimisations(std::string filePath, std::string database, std::string project)
	{
		uint8_t errCode;
		auto importer = ODAModelImportUtils::ImportFile(filePath, errCode);
		auto scene = importer->generateRepoScene(errCode);
		scene->setDatabaseAndProjectName(database, project);
		auto handler = getHandler();
		std::string errMsg;
		scene->commit(handler.get(), handler->getFileManager().get(), errMsg, "unit tests", "", "");
	}
}

MATCHER(IsSuccess, "")
{
	*result_listener << arg.message;
	return (bool)arg;
}

TEST(ODAModelImport, Sample2025NWD)
{
	auto handler = getHandler();

	auto db = "ODAModelImportTest";
	auto col = "Test";

	repo::manipulator::modelutility::RepoSceneBuilder builder(
		handler,
		db,
		col
	);

	ModelImportConfig config;
	auto modelConvertor = std::unique_ptr<OdaModelImport>(new OdaModelImport(config));

	
//	modelConvertor->importModel("D:/3drepo/QA/IFC NWD Federation.nwd", &builder);
//	modelConvertor->importModel("D:/3drepo/tests/cplusplus/bouncer/data/models/sample2025.nwd", &builder);	
	modelConvertor->importModel("D:/3drepo/tests/cplusplus/bouncer/data/models/sampleHouse.nwd", &builder);

	builder.finalise();
	
	repo::test::utils::SceneComparer comparer;
	comparer.ignoreVertices = true;
	comparer.ignoreTextures = true;
	comparer.ignoreMeshNodes = true;

	EXPECT_THAT(comparer.compare(DBNAME, "SampleHouse", db, col), IsSuccess());
}

