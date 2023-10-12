/**
*  Copyright (C) 2021 3D Repo Ltd
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

#include "file_processor_dgn.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class FileProcessorDwg : public FileProcessorDgn
				{
				public:
					FileProcessorDwg(const std::string &inputFile, GeometryCollector * geoCollector, const ModelImportConfig& config) : FileProcessorDgn(inputFile, geoCollector, config) {}
					~FileProcessorDwg() override {}
				protected:
					virtual OdDgDatabasePtr initialiseOdDatabase();
				};
			}
		}
	}
}