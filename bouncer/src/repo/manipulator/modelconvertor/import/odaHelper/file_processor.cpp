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
#include "file_processor_dwg.h"
#include "file_processor_rvt.h"
#include "file_processor_nwd.h"

using namespace repo::manipulator::modelconvertor;
using namespace repo::manipulator::modelconvertor::odaHelper;

FileProcessor::FileProcessor(const std::string &inputFile, GeometryCollector *geoCollector, const ModelImportConfig& config)
	: file(inputFile),
	collector(geoCollector),
	importConfig(config)
{
}

repo::manipulator::modelconvertor::odaHelper::FileProcessor::~FileProcessor()
{
}

template<typename T, typename... Args>
std::unique_ptr<T> makeUnique(Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

std::unique_ptr<FileProcessor> FileProcessor::getFileProcessor(const std::string &inputFile, GeometryCollector * geoCollector, const ModelImportConfig& config) {
	boost::filesystem::path filePathP(inputFile);
	std::string fileExt = filePathP.extension().string();
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);

	if (fileExt == ".DGN")
		return makeUnique<FileProcessorDgn>(inputFile, geoCollector, config);
	else if (fileExt == ".DWG" || fileExt == ".DXF")
		return makeUnique<FileProcessorDwg>(inputFile, geoCollector, config);
	else if (fileExt == ".RVT" || fileExt == ".RFA")
		return makeUnique<FileProcessorRvt>(inputFile, geoCollector, config);
	else if (fileExt == ".NWD" || fileExt == ".NWC")
		return makeUnique<FileProcessorNwd>(inputFile, geoCollector, config);

	return nullptr;
}