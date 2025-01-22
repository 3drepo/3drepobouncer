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
#include <repo/manipulator/modelconvertor/import/repo_model_import_manager.h>
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

#define REFDB "ODAModelImport"
#define TESTDB "ODAModelImportTest"

#pragma optimize("", off)

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
	void ModelImportManagerImport(std::string collection, std::string filename)
	{
		ModelImportConfig config(
			true,
			ModelUnits::MILLIMETRES,
			"",
			0,
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			collection);

		auto handler = getHandler();

		uint8_t err;
		std::string msg;

		ModelImportManager manager;
		auto scene = manager.ImportFromFile(filename, config, handler, err);
		scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());
	}
}

MATCHER(IsSuccess, "")
{
	*result_listener << arg.message;
	return (bool)arg;
}

// The set of 'tree' tests aims to compare the trees & metadata (for transformation nodes)
// between the scenes.

TEST(ODAModelImport, Sample2025NWDTree)
{
	auto collection = "Sample2025NWD";

	ODAModelImportUtils::ModelImportManagerImport(collection, getDataPath("sample2025.nwd"));

	repo::test::utils::SceneComparer comparer;
	comparer.ignoreMeshNodes = true;
	comparer.ignoreMetadataKeys.insert("Item::File Name");

	EXPECT_THAT(comparer.compare(REFDB, collection, TESTDB, collection), IsSuccess());
}

TEST(ODAModelImport, SampleHouseNWDTree)
{
	auto collection = "SampleHouseNWD";

	ODAModelImportUtils::ModelImportManagerImport(collection, getDataPath("sampleHouse.nwd"));

	repo::test::utils::SceneComparer comparer;
	comparer.ignoreMeshNodes = true;
	comparer.ignoreMetadataKeys.insert("Item::File Name");

	EXPECT_THAT(comparer.compare(REFDB, collection, TESTDB, collection), IsSuccess());
}

TEST(ODAModelImport, IFCNWDFederationTree)
{
	auto collection = "IfcNwdFederationNWD";

	ODAModelImportUtils::ModelImportManagerImport(collection, getDataPath("IfcNwdFederation.nwd"));

	repo::test::utils::SceneComparer comparer;
	comparer.ignoreMeshNodes = true;
	comparer.ignoreMetadataKeys.insert("Item::File Name");

	EXPECT_THAT(comparer.compare(REFDB, collection, TESTDB, collection), IsSuccess());
}

TEST(ODAModelImport, TestNWCTree)
{
	auto collection = "TestNWC";

	ODAModelImportUtils::ModelImportManagerImport(collection, getDataPath("test.nwc"));

	repo::test::utils::SceneComparer comparer;
	comparer.ignoreMeshNodes = true;
	comparer.ignoreMetadataKeys.insert("Item::File Name");

	EXPECT_THAT(comparer.compare(REFDB, collection, TESTDB, collection), IsSuccess());
}