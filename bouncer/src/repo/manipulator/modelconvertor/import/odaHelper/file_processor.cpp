/**
*  Copyright (C) 2018 3D Repo Ltd
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

#include "boost/filesystem.hpp"
#include <memory>
#include "file_processor.h"
#include "file_processor_dgn.h"
#include "file_processor_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

FileProcessor::FileProcessor(const std::string &inputFile, GeometryCollector *geoCollector)
	: file(inputFile),
	  collector(geoCollector)
{
}

repo::manipulator::modelconvertor::odaHelper::FileProcessor::~FileProcessor()
{
}
std::unique_ptr<FileProcessor> FileProcessor::getFileProcessor(const std::string &inputFile, GeometryCollector * geoCollector) {

	boost::filesystem::path filePathP(inputFile);
	std::string fileExt = filePathP.extension().string();
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);

	if (fileExt == ".DGN")
		return std::make_unique<FileProcessorDgn>(inputFile, geoCollector);
	else if (fileExt == ".RVT" || fileExt == ".RFA")
		return std::make_unique<FileProcessorRvt>(inputFile, geoCollector);

	return nullptr;
}

