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

	std::vector<uint32_t> getMaterialMappings() const {
		return matVector;
	}

	std::unordered_map < uint32_t, repo::core::model::MaterialNode > getMaterialNodes() const {
		return codeToMat;
	}

	std::vector<repo::core::model::MeshNode> getMeshes() const {
		return meshVector;
	}

	void addMesh(const repo::core::model::MeshNode &meshNode) {
		meshVector.push_back(meshNode);
	}

	void addMaterialWithColor(const uint32_t &r, const uint32_t &g, const uint32_t &b, const uint32_t &a) {
		uint32_t code = r | (g << 8) | (b << 16) | (a << 24);
		if (codeToMat.find(code) == codeToMat.end())
		{
			repo_material_t mat;
			mat.diffuse = {r/255.f, g/255.f, b/255.f};
			mat.opacity = 1;//a / 255.f; alpha doesn't seem to be used
			codeToMat[code] = repo::core::model::RepoBSONFactory::makeMaterialNode(mat);
		}

		matVector.push_back(code);
	}

private:
	std::vector<repo::core::model::MeshNode> meshVector;
	std::vector<uint32_t> matVector;
	std::unordered_map < uint32_t, repo::core::model::MaterialNode > codeToMat;
};

