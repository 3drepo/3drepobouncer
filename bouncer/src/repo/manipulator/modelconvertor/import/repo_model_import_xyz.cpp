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

#include "repo_model_import_xyz.h"

#include <filesystem>


using namespace repo::manipulator::modelconvertor;

XYZModelImport::XYZModelImport(const ModelImportConfig& settings) : 
	AbstractPointCloudImport(settings)
{
	// XYZ files do not have metadata to indicate the unit.
	// For ease of use, we default to millimetres.
	modelUnits = repo::lib::ModelUnits::MILLIMETRES;
}

repo::manipulator::modelconvertor::XYZModelImport::~XYZModelImport()
{
	if (fileStream.is_open())
		fileStream.close();
}

bool repo::manipulator::modelconvertor::XYZModelImport::getNextPoint(repo::core::model::PointData& point)
{
	if (!fileStream.is_open())
		throw repo::lib::RepoException("XYZ file stream closed unexpectedly!");
	if (fileStream.eof())
		return false;

	// Get the next line
	std::string line;
	std::getline(fileStream, line);

	// Split the line by delimiter " "
	std::stringstream ss(line);
	std::string item;
	std::vector<std::string> results;
	char delim = ' ';
	while (std::getline(ss, item, delim))
	{
		results.push_back(item);
	}

	// Default the alpha channel of the point to 1.0f so that
	// files that have no alpha column don't produce invisible
	// point clouds
	point.colour.a = 1.f;

	// Sort values into point struct
	for (int i = 0; i < results.size(); i++)
	{
		auto column = schema[i];
		auto value = results[i];

		switch (column)
		{
		case ColumnType::PosX:
			point.position.x = std::stod(value);
			break;
		case ColumnType::PosY:
			point.position.y = std::stod(value);
			break;
		case ColumnType::PosZ:
			point.position.z = std::stod(value);
			break;
		case ColumnType::ColR:
			point.colour.r = std::stof(value) / 255;
			break;
		case ColumnType::ColG:
			point.colour.g = std::stof(value) / 255;
			break;
		case ColumnType::ColB:
			point.colour.b = std::stof(value) / 255;
			break;
		case ColumnType::ColA:
			point.colour.a = std::stof(value) / 255;
			break;
		case ColumnType::UNKNOWN:
			// Results of an unknown scheme are skipped
			break;
		}
	}

	return true;
}

void XYZModelImport::resetReader()
{
	// Clear flags and reset seeker head
	fileStream.clear();
	fileStream.seekg(0, std::ios::beg);
	
	// Skip first line again because we already have the schema
	fileStream.ignore(std::numeric_limits<std::streamsize>::max(), fileStream.widen('\n'));
}

uint8_t XYZModelImport::loadFile(std::string filePath)
{
	repoInfo << "IMPORT [" << filePath << "]";

	//check if a file exist first
	std::ifstream fs(std::filesystem::u8path(filePath));
	if (!fs.is_open() || !fs.good())
	{
		repoError << "Failed to find file";
		return REPOERR_MODEL_FILE_READ;
	}

	repoInfo << "=== IMPORTING MODEL WITH XYZ IMPORTER ===";

	try {
		fileStream = std::ifstream(filePath, std::ios_base::in);
	}
	catch (std::exception ex)
	{
		repoError << "Error creating file stream: " << ex.what();
		return REPOERR_MODEL_FILE_READ;
	}

	// Get first line for identification of the categories
	std::string line;
	std::getline(fileStream, line);

	// Remove "//" from the first line (if present)
	std::size_t found = line.find("//");
	if (found != std::string::npos)
		line.erase(found, 2);

	// Split the line by delimiter " " to get schema
	std::stringstream ss(line);
	std::string item;
	schema.clear();
	char delim = ' ';

	// We use a bitmask to keep track of the required columns
	// Bit 0: X
	// Bit 1: Y
	// Bit 2: Z
	// Bit 3: R
	// Bit 4: G
	// Bit 5: B
	uint8_t mask = 0;

	while (std::getline(ss, item, delim))
	{
		if (item == "X")
		{
			schema.push_back(ColumnType::PosX);
			mask |= 1;
		}
		else if (item == "Y")
		{
			schema.push_back(ColumnType::PosY);
			mask |= (1 << 1);
		}
		else if (item == "Z")
		{
			schema.push_back(ColumnType::PosZ);
			mask |= (1 << 2);
		}
		else if (item == "R")
		{
			schema.push_back(ColumnType::ColR);
			mask |= (1 << 3);
		}
		else if (item == "G")
		{
			schema.push_back(ColumnType::ColG);
			mask |= (1 << 4);
		}
		else if (item == "B")
		{
			schema.push_back(ColumnType::ColB);
			mask |= (1 << 5);
		}
		else if (item == "A")
		{
			schema.push_back(ColumnType::ColA);
		}
		else
			schema.push_back(ColumnType::UNKNOWN);
	}

	if (schema.size() == 1)
	{
		repoError << "Failed to split column description. Wrong delimiter?";
		return REPOERR_XYZ_WRONG_DELIMITER;
	}
	else if (mask == 0)
	{
		repoError << "Could not read point cloud schema. Column descriptions missing?";
		return REPOERR_XYZ_COLUMN_DESCRIPTION_MISSING;
	}
	// We allow position only (value 7) or position with RGB colour (value 63).
	// Anything under, above, or in between would mean incomplete information
	else if (mask != 63 && mask != 7)
	{
		repoError << "Required Columns missing. Incomplete position or column information";
		return REPOERR_XYZ_REQUIRED_COLUMNS_MISSING;
	}


	return 0;
}

repo::core::model::RepoScene* XYZModelImport::importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& errMsg)
{
	// We are just calling the implementation in the abstract point cloud importer
	// since that takes care of the actual loading.
	return AbstractPointCloudImport::importModel(filePath, handler, errMsg);
}
