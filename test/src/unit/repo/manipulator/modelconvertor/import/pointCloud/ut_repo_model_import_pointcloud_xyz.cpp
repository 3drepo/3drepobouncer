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

#include <filesystem>
#include <gtest/gtest.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_xyz.h>
#include <test/src/unit/repo_test_utils.h>
#include <test/src/unit/repo_test_matchers.h>
#include <test/src/unit/repo_test_point_utils.h>

using namespace repo::manipulator::modelconvertor;
using namespace repo::test::utils::point;
using namespace testing;

#define TESTDB "PCImportTestXYZ"
#define FILE_TESTDATA "/PointClouds/XYZ/TestData.csv"
#define FILE_POS_RGBA "/PointClouds/XYZ/Pos_RGBA.xyz"
#define FILE_POS_RGB "/PointClouds/XYZ/Pos_RGB.xyz"
#define FILE_POS "/PointClouds/XYZ/Pos.xyz"
#define FILE_NOCOLUMNDESC "/PointClouds/XYZ/NoColumnDesc.xyz"
#define FILE_POS_INCOMPLETE "/PointClouds/XYZ/Pos_incomplete.xyz"
#define FILE_POS_RGB_INCOMPLETE "/PointClouds/XYZ/Pos_RGB_incomplete.xyz"
#define FILE_POS_RGBA_UNORDERED "/PointClouds/XYZ/Pos_RGBA_unordered.xyz"
#define FILE_NOPOINTS "/PointClouds/XYZ/NoPoints.xyz"
#define FILE_WRONGDELIMITER "/PointClouds/XYZ/WrongDelimiter.xyz"

// Class inheriting from the XYZ Model import for testing purposes.
// It allows us to call the protected methods the abstract importer
// uses from the outside.
class XYZModelImportTestExtension : public XYZModelImport
{
public:
	XYZModelImportTestExtension(const ModelImportConfig& settings)
		: XYZModelImport(settings)
	{

	}

	~XYZModelImportTestExtension()
	{

	}

	bool getNextPointPublic(repo::core::model::PointData& point) 
	{
		return getNextPoint(point);
	}

	void resetReaderPublic()
	{
		resetReader();
	}

	uint8_t loadFilePublic(std::string filePath)
	{
		return loadFile(filePath);
	}
};

std::vector<repo::core::model::PointData> getTestPoints()
{
	auto points = std::vector<repo::core::model::PointData>();

	std::ifstream stream(getDataPath(FILE_TESTDATA));
	EXPECT_TRUE(stream.is_open());
	EXPECT_TRUE(stream.good());

	while (!stream.eof())
	{
		// Get the next line
		std::string line;
		std::getline(stream, line);

		// Split the line by delimiter ","
		std::stringstream ss(line);
		std::string item;
		std::vector<std::string> results;
		char delim = ',';
		while (std::getline(ss, item, delim))
		{
			results.push_back(item);
		}
	
		EXPECT_TRUE(results.size() == 7);

		double x = std::stod(results[0]);
		double y = std::stod(results[1]);
		double z = std::stod(results[2]);
		float r = std::stof(results[3]) / 255.f;
		float g = std::stof(results[4]) / 255.f;
		float b = std::stof(results[5]) / 255.f;
		float a = std::stof(results[6]) / 255.f;

		repo::lib::RepoVector3D64 pos(x, y, z);
		repo::lib::repo_color4d_t col(r, g, b, a);

		repo::core::model::PointData point(pos, col);
		points.push_back(point);
	}

	EXPECT_TRUE(points.size() > 0);

	return points;
}

TEST(PointCloudImportXYZ, NoFile)
{
	// Create importer
	ModelImportConfig config = ModelImportConfig();
	auto mockImporter = std::make_unique<XYZModelImportTestExtension>(config);

	// Load file
	auto err = mockImporter->loadFilePublic(getDataPath("thisFileDoesNotExist.xyz"));

	EXPECT_THAT(err, Eq(REPOERR_MODEL_FILE_READ));
}

TEST(PointCloudImportXYZ, PositionAndColourRGBA)
{
	// Get test data
	auto testData = getTestPoints();

	// Create importer
	ModelImportConfig config = ModelImportConfig();
	auto mockImporter = std::make_unique<XYZModelImportTestExtension>(config);

	// Load file
	mockImporter->loadFilePublic(getDataPath(FILE_POS_RGBA));

	// Check loaded data against test data
	for (int i = 0; i < testData.size(); i++)
	{
		auto expectedPoint = testData[i];

		auto actualPoint = repo::core::model::PointData();
		EXPECT_TRUE(mockImporter->getNextPointPublic(actualPoint));

		EXPECT_TRUE(expectedPoint == actualPoint);
	}

	// Trying to get another point should return false
	auto anotherPoint = repo::core::model::PointData();
	EXPECT_FALSE(mockImporter->getNextPointPublic(anotherPoint));

	// After resetting the reader, we should be able to check the loaded data
	// against the test data again.
	mockImporter->resetReaderPublic();
	for (int i = 0; i < testData.size(); i++)
	{
		auto expectedPoint = testData[i];

		auto actualPoint = repo::core::model::PointData();
		EXPECT_TRUE(mockImporter->getNextPointPublic(actualPoint));

		EXPECT_TRUE(expectedPoint == actualPoint);
	}
}

TEST(PointCloudImportXYZ, PositionAndColourRGB)
{
	// Get test data
	auto testData = getTestPoints();

	// Alter the test data so that the A channel is set to 1.0 as it would
	// be if the data is missing that colour
	for (auto& point : testData)
	{
		point.colour.a = 1.0f;
	}

	// Create importer
	ModelImportConfig config = ModelImportConfig();
	auto mockImporter = std::make_unique<XYZModelImportTestExtension>(config);

	// Load file
	mockImporter->loadFilePublic(getDataPath(FILE_POS_RGB));

	// Check loaded data against test data
	for (int i = 0; i < testData.size(); i++)
	{
		auto expectedPoint = testData[i];

		auto actualPoint = repo::core::model::PointData();
		EXPECT_TRUE(mockImporter->getNextPointPublic(actualPoint));

		EXPECT_TRUE(expectedPoint == actualPoint);
	}
}

TEST(PointCloudImportXYZ, PositionOnly)
{
	// Get test data
	auto testData = getTestPoints();

	// Alter the test data so that the colour is set to 0, 0, 0, 1 
	// as it would be if the data is missing the colour
	for (auto& point : testData)
	{
		point.colour = repo::lib::repo_color4d_t(0, 0, 0, 1);
	}

	// Create importer
	ModelImportConfig config = ModelImportConfig();
	auto mockImporter = std::make_unique<XYZModelImportTestExtension>(config);

	// Load file
	mockImporter->loadFilePublic(getDataPath(FILE_POS));

	// Check loaded data against test data
	for (int i = 0; i < testData.size(); i++)
	{
		auto expectedPoint = testData[i];

		auto actualPoint = repo::core::model::PointData();
		EXPECT_TRUE(mockImporter->getNextPointPublic(actualPoint));

		EXPECT_TRUE(expectedPoint == actualPoint);
	}
}

TEST(PointCloudImportXYZ, NoColumnDescriptors)
{
	// Create importer
	ModelImportConfig config = ModelImportConfig();
	auto mockImporter = std::make_unique<XYZModelImportTestExtension>(config);

	// Load file
	auto err = mockImporter->loadFilePublic(getDataPath(FILE_NOCOLUMNDESC));

	EXPECT_THAT(err, Eq(REPOERR_XYZ_COLUMN_DESCRIPTION_MISSING));
}

TEST(PointCloudImportXYZ, MissingColumns)
{
	// Create importer
	ModelImportConfig config = ModelImportConfig();
	auto mockImporter = std::make_unique<XYZModelImportTestExtension>(config);

	// Load file (incomplete position)
	auto err = mockImporter->loadFilePublic(getDataPath(FILE_POS_INCOMPLETE));
	EXPECT_THAT(err, Eq(REPOERR_XYZ_REQUIRED_COLUMNS_MISSING));

	// Load file (incomplete colour)
	err = mockImporter->loadFilePublic(getDataPath(FILE_POS_RGB_INCOMPLETE));
	EXPECT_THAT(err, Eq(REPOERR_XYZ_REQUIRED_COLUMNS_MISSING));
}

TEST(PointCloudImportXYZ, ColumnsUnordered)
{
	// Get test data
	auto testData = getTestPoints();

	// Create importer
	ModelImportConfig config = ModelImportConfig();
	auto mockImporter = std::make_unique<XYZModelImportTestExtension>(config);

	// Load file
	mockImporter->loadFilePublic(getDataPath(FILE_POS_RGBA_UNORDERED));

	// Check loaded data against test data
	for (int i = 0; i < testData.size(); i++)
	{
		auto expectedPoint = testData[i];

		auto actualPoint = repo::core::model::PointData();
		EXPECT_TRUE(mockImporter->getNextPointPublic(actualPoint));

		EXPECT_TRUE(expectedPoint == actualPoint);
	}
}

TEST(PointCloudImportXYZ, NoPoints)
{
	// Get test data
	auto testData = getTestPoints();

	// Create importer
	ModelImportConfig config = ModelImportConfig();
	auto mockImporter = std::make_unique<XYZModelImportTestExtension>(config);

	// Load file
	mockImporter->loadFilePublic(getDataPath(FILE_NOPOINTS));

	// Trying to get a point should return false
	auto point = repo::core::model::PointData();
	EXPECT_FALSE(mockImporter->getNextPointPublic(point));
}

TEST(PointCloudImportXYZ, WrongDelimiter)
{
	// Create importer
	ModelImportConfig config = ModelImportConfig();
	auto mockImporter = std::make_unique<XYZModelImportTestExtension>(config);

	// Load file
	auto err = mockImporter->loadFilePublic(getDataPath(FILE_WRONGDELIMITER));

	EXPECT_THAT(err, Eq(REPOERR_XYZ_WRONG_DELIMITER));
}