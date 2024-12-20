/*
*  Copyright(C) 2024 3D Repo Ltd
*
*  This program is free software : you can redistribute it and / or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or(at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "repo_bson_builder.h"

namespace repo {
	namespace core {
		namespace model {
			/**
			* Specialisation of the BSON Builder dedicated to building nodes that
			* come with a binary mapping for large blobs, such as Meshes and
			* Supermeshes.
			*/
			class REPO_API_EXPORT RepoBSONBinMappingBuilder : public RepoBSONBuilder
			{
				using BinMapping = std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>;
			public:
				BinMapping& mapping()
				{
					return binMapping;
				}

				// (Keep the templated definition in the header so the compiler
				// can create concrete classes with the templated types when they
				// are used)

				template<typename T>
				void appendLargeArray(std::string name, const std::vector<T>& data)
				{
					appendLargeArray(name, &data[0], data.size() * sizeof(data[0]));
				}

				void appendLargeArray(std::string name, const void* data, size_t size);

			private:
				BinMapping binMapping;
			};
		}
	}
}