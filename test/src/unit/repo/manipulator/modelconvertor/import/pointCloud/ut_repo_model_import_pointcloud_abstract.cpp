/**
*  Copyright (C) 2026 3D Repo Ltd
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
#include <repo/manipulator/modelconvertor/import/repo_model_import_point_cloud_abstract.h>
#include <test/src/unit/repo_test_utils.h>
#include <test/src/unit/repo_test_matchers.h>
#include <test/src/unit/repo_test_point_utils.h>

using namespace repo::manipulator::modelconvertor;
using namespace repo::test::utils::point;
using namespace testing;

#define TESTDB "PCImportTestAbstract"

TEST(PointCloudImportAbstract, MainTest)
{
	// Create settings for importer
	auto revId = repo::lib::RepoUUID::createUUID();
	std::string project = "ChunkingTest";
	ModelImportConfig config(revId, TESTDB, project);
	
	// Create mock importer
	auto mockImporter = std::make_unique<TestPCImport>(config);

	// Parameters for mock importer
	int numPoints = REPO_PC_CHUNKING_MAXPOINTS * 10;
	auto bounds = repo::lib::RepoBounds(makeRandomRepoVector(), makeRandomRepoVector());
	
	// Create test data in mock importer
	mockImporter->createTestData(numPoints, bounds);

	// "Import" the model
	uint8_t errMsg;
	auto handler = getHandler();
	mockImporter->importModel("", handler, errMsg);
	EXPECT_THAT(errMsg, Eq(0));

	// Compare the point clouds as it was imported with the test
	// data the mock importer created.
	bool compResult = comparePointClouds(
		TESTDB,
		project,
		revId,
		mockImporter.get());
	EXPECT_TRUE(compResult);

	// Now check whether the chunks are correct
	// This means the following:
	// - point limit is adhered to
	// - number of attribute values and points is equivalent
	// - all points in a chunk fit into its bounding box
	// - the bounding boxes are cubic
	// - the length of the sides of the bounding box is correct for their tree level
	// - the bounding box is correctly placed in relation to its tree level and the overall bounds

	// First, calculate expected bounds
	auto dims = bounds.max() - bounds.min();
	double maxDim = std::max(dims.x, std::max(dims.y, dims.z));
	auto expectedBounds = repo::lib::RepoBounds(repo::lib::RepoVector3D64(), repo::lib::RepoVector3D64(maxDim, maxDim, maxDim));

	// Now test the chunks
	checkChunkCorrectness(
		TESTDB,
		project,
		revId,
		expectedBounds);
}