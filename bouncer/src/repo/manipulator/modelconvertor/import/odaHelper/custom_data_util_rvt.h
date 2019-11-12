/**
*  Copyright (C) 2019 3D Repo Ltd
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

#include <BimCommon.h>
#include <Database/BmElement.h>
#include <unordered_map>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {				
				class CustomDataProcessorRVT
				{
				public:
					CustomDataProcessorRVT(OdBmElementPtr _element) : element(_element) {}

					void fillCustomMetadata(std::unordered_map<std::string, std::string>& metadata);

				private:
					const OdBmElementPtr element;

					void fetchStringData(std::unordered_map<std::string, std::string>& dataStore);
					void fetchIntData(std::unordered_map<std::string, std::string>& dataStore);
					void fetchDoubleData(std::unordered_map<std::string, std::string>& dataStore);

				};
			}
		}
	}
}
