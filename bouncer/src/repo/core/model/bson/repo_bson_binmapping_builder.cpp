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

#include "repo_bson_binmapping_builder.h"
#include "repo/core/model/repo_model_global.h"

using namespace repo::core::model;

void RepoBSONBinMappingBuilder::appendLargeArray(std::string name, const void* data, size_t size)
{
	auto obj = this->tempObj();
	if (!obj.hasField(REPO_NODE_LABEL_ID))
	{
		throw std::invalid_argument("appendLargeArray called before the builder is assigned a unique Id. Ensure appendDefaults has been called before appending large arrays.");
	}

	std::string bName = obj.getUUIDField(REPO_NODE_LABEL_ID).toString() + "_" + name;

	binMapping[name] =
		std::pair<std::string, std::vector<uint8_t>>(bName, std::vector<uint8_t>());
	binMapping[name].second.resize(size); //uint8_t will ensure it is a byte addrressing
	memcpy(binMapping[name].second.data(), data, size);
}
