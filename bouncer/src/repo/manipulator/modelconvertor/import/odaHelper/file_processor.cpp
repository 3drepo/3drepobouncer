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

FileProcessor::FileProcessor(const std::string& inputFile, repo::manipulator::modelutility::DrawingImageInfo* collector)
	: file(inputFile),
	importConfig(repo::manipulator::modelconvertor::ModelImportConfig()),
	drawingCollector(collector),
	repoSceneBuilder(nullptr)
{
}

FileProcessor::FileProcessor(const std::string& inputFile, repo::manipulator::modelutility::RepoSceneBuilder* builder, const ModelImportConfig& config)
	: file(inputFile),
	importConfig(config),
	drawingCollector(nullptr),
	repoSceneBuilder(builder)
{
}

repo::manipulator::modelconvertor::odaHelper::FileProcessor::~FileProcessor()
{
}

template<typename T, typename... Args>
std::unique_ptr<T> makeUnique(Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

std::unique_ptr<FileProcessor> FileProcessor::getFileProcessor(const std::string &inputFile, repo::manipulator::modelutility::RepoSceneBuilder* builder, const ModelImportConfig& config) {
	boost::filesystem::path filePathP(inputFile);
	std::string fileExt = filePathP.extension().string();
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);

	if (fileExt == ".DGN")
		return makeUnique<FileProcessorDgn>(inputFile, builder, config);
	else if (fileExt == ".DWG" || fileExt == ".DXF")
		return makeUnique<FileProcessorDwg>(inputFile, builder, config);
	else if (fileExt == ".RVT" || fileExt == ".RFA")
		return makeUnique<FileProcessorRvt>(inputFile, builder, config);
	else if (fileExt == ".NWD" || fileExt == ".NWC")
		return makeUnique<FileProcessorNwd>(inputFile, builder, config);

	return nullptr;
}

void FileProcessor::updateDrawingHorizontalCalibration(const OdGsView* pGsView, repo::manipulator::modelutility::DrawingCalibration& calibration)
{
	auto worldToDeviceMatrix = pGsView->worldToDeviceMatrix();

	// Define the vectors in the drawing world coordinate system to resolve to
	// SVG points.

	OdGePoint3d a3d(0, 0, 0);
	OdGePoint3d b3d(1, 0, 0);

	// Calculate points in SVG space

	OdGePoint3d a2d = worldToDeviceMatrix * a3d;
	OdGePoint3d b2d = worldToDeviceMatrix * b3d;

	// The SVG primitive coordinate system is the same one used by the frontend
	// when interacting with the SVG.

	calibration.horizontalCalibration2d.push_back(repo::lib::RepoVector2D(a2d.x, a2d.y));
	calibration.horizontalCalibration2d.push_back(repo::lib::RepoVector2D(b2d.x, b2d.y));

	// Transform the 3d vectors to the Repo coordinate system - the same
	// transforms that would be applied to the 3D model, and which the Unity API
	// operates in, which includes a conversion to a left-handed coordinate
	// system.

	calibration.horizontalCalibration3d.push_back(repo::lib::RepoVector3D(a3d.x, a3d.z, -a3d.y));
	calibration.horizontalCalibration3d.push_back(repo::lib::RepoVector3D(b3d.x, b3d.z, -b3d.y));
}