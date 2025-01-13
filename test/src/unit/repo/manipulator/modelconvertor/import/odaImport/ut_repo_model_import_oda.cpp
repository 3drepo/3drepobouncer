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
#include "../../../../../repo_test_utils.h"
#include "../../../../../repo_test_database_info.h"
#include "../../../../../repo_test_scenecomparer.h"
#include "boost/filesystem.hpp"
#include "../../bouncer/src/repo/error_codes.h"

#include <repo/manipulator/modelutility/repo_scene_builder.h>

using namespace repo::manipulator::modelconvertor;
using namespace repo::core::model;
using namespace testing;

#define DBNAME "ODAModelImport"

#pragma optimize("",off)

/* ODA is predominantly tested via the system tests using the client. These tests
 * cover specific features or regressions. */


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

TEST(ODAModelImport, CheckReferenceDatabases)
{
	// Checks for the existence of the SampleNWDTree reference database.
	bool haveImported = false;

	if (!projectExists(DBNAME, "Sample2025NWDODA"))
	{
		uint8_t errCode = 0;
		auto importer = ODAModelImportUtils::ImportFile(getDataPath("sample2025.nwd"), errCode);
		auto scene = importer->generateRepoScene(errCode);
		auto handler = getHandler();
		std::string errMsg;
		scene->setDatabaseAndProjectName(DBNAME, "Sample2025NWDODA");
		scene->commit(handler.get(), handler->getFileManager().get(), errMsg, "unit tests", "", "");
	}

	if (!projectExists(DBNAME, "Sample2025NWD")) {
		runProcess(produceUploadArgs(DBNAME, "Sample2025NWD", getDataPath("sample2025.nwd")));
		haveImported = true;
	}

	if (!projectExists(DBNAME, "Sample2025NWD2")) {
		runProcess(produceUploadArgs(DBNAME, "Sample2025NWD2", getDataPath("sample2025.nwd")));
		haveImported = true;
	}

	if(haveImported)
	{
		FAIL() << "One or more reference databases were not found. These have been reinitialised but must be checked manually and committed.";
	}
}

TEST(ODAModelImport, Sample2025NWD)
{
	auto handler = getHandler();

	repo::manipulator::modelutility::RepoSceneBuilder builder(
		handler,
		"ODAModelImportTest",
		"Sample2025NWDODARSB"
	);

	ModelImportConfig config;
	auto modelConvertor = std::unique_ptr<OdaModelImport>(new OdaModelImport(config));
	modelConvertor->importModel(getDataPath("sample2025.nwd"), &builder);

	builder.finalise();
	
	repo::test::utils::SceneComparer comparer;
	comparer.ignoreMetadataContent = false;
	comparer.ignoreVertices = true;

	EXPECT_THAT(comparer.compare(DBNAME, "Sample2025NWDODA", "ODAModelImportTest", "Sample2025NWDODARSB"), IsSuccess());
}

// todo::

// Variations in metadata value
// Variations in geometry