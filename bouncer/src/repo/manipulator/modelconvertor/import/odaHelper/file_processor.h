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
#include "../../../../core/model/bson/repo_node_mesh.h"
#include "geometry_collector.h"
#include <OdaCommon.h>
#include <Gs/GsBaseInclude.h>
#include <RxObjectImpl.h>
#include <vector>
#include <string>


namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class FileProcessor
				{
				public:
					FileProcessor(const std::string &inputFile, GeometryCollector * geoCollector);
					~FileProcessor();

					int readFile();

				private:
					const std::string file;
					GeometryCollector *collector;
					int importDgn(OdDbBaseDatabase *pDb,
						const ODCOLORREF* pPallete,
						int numColors,
						const OdGiDrawable* pEntity = nullptr,
						const OdGeMatrix3d& matTransform = OdGeMatrix3d::kIdentity,
						const std::map<OdDbStub*, double>* pMapDeviations = nullptr);

				};
			}
		}
	}
}

