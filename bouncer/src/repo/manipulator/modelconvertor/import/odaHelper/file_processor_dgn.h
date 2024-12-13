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

#pragma once

#include "repo/core/model/bson/repo_node_mesh.h"
#include "repo/manipulator/modelutility/repo_drawing.h"

#include <OdaCommon.h>
#include <DgDatabase.h>
#include <Gs/GsBaseInclude.h>
#include <RxObjectImpl.h>
#include <ExSystemServices.h>
#include <ExDgnServices.h>
#include <ExDgnHostAppServices.h>

#include <vector>
#include <string>

#include "geometry_collector.h"
#include "file_processor.h"
#include "repo_oda_system_services.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class RepoDgnServices : public RepoSystemServices, public OdExDgnHostAppServices
				{
				protected:
					ODRX_USING_HEAP_OPERATORS(RepoSystemServices);
				};

				class FileProcessorDgn : public FileProcessor
				{
				public:
					FileProcessorDgn(const std::string &inputFile, GeometryCollector * geoCollector, const ModelImportConfig& config) : FileProcessor(inputFile, geoCollector, config) {};
					FileProcessorDgn(const std::string& inputFile, modelutility::DrawingImageInfo* collector) : FileProcessor(inputFile, collector) {};
					~FileProcessorDgn() override;

					uint8_t readFile() override;

				protected:
					virtual OdDgDatabasePtr initialiseOdDatabase();
					OdStaticRxObject<RepoDgnServices> svcs;

				private:
					void importModel(OdDbBaseDatabase *pDb,
						const ODCOLORREF* pPallete,
						int numColors,
						const OdGeExtents3d &extModel,
						const OdGiDrawable* pEntity = nullptr,
						const OdGeMatrix3d& matTransform = OdGeMatrix3d::kIdentity,
						const std::map<OdDbStub*, double>* pMapDeviations = nullptr);

					void importDrawing(OdDgDatabasePtr pDb,
						const ODCOLORREF* pPallete,
						int numColors,
						OdDgElementId view);


					ModelUnits determineModelUnits(const OdDgModel::UnitMeasure &units);
				};
			}
		}
	}
}
