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
#include "../../../../core/model/bson/repo_bson_factory.h"
#include "../../../../lib/datastructure/repo_structs.h"


#include <vector>
#include <string>


class OdaGeometryCollector
{
public:
	OdaGeometryCollector();
	~OdaGeometryCollector();

	std::vector<const repo_material_t> getMaterials() const {
		return matVector;
	}

	std::vector<const repo::core::model::MeshNode> getMeshes() const {
		return meshVector;
	}

	void addMesh(const repo::core::model::MeshNode &meshNode) {
		meshVector.push_back(meshNode);
	}

	int addMaterialWithColor(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) {
		auto code = r | (g << 8) | (b << 16) | (a << 24);
		if (codeToMat.find(code) == codeToMat.end())
		{
			repo_material_t mat;
			mat.diffuse = {r/255.f, g/255.f, b/255.f};
			mat.opacity = a / 255.f;
			codeToMat[code] = mat;
		}
		

				//matVector.push_back(mat);

	}

private:
	std::vector<const repo::core::model::MeshNode> meshVector;
	std::vector<> matVector;
	std::unordered_map<uint32_t, const repo::core::model:MaterialNode> codeToMat;
};

