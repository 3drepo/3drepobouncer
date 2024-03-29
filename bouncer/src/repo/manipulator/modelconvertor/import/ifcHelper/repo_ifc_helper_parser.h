/**
*  Copyright (C) 2015 3D Repo Ltd
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

/**
* IFC Geometry convertor(Import)
*/

#pragma once

#include <string>
#include <unordered_map>

#include "../../../../core/model/bson/repo_node_material.h"
#include "../../../../core/model/bson/repo_node_mesh.h"
#include "../../../../core/model/collection/repo_scene.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace ifcHelper {
				class IFCUtilsParser
				{
				public:

					/**
					* Create IFCUtilsParser
					* in this object!
					* @param file IFC file location
					*/
					IFCUtilsParser(const std::string &file);

					/**
					* Default Deconstructor
					*/
					virtual ~IFCUtilsParser();

					/**
					* Generate tree based on the file given.
					* @param errMsg error message shown should the function fail
					* @param meshes meshes generated by IFCUtilsGeometry
					* @param materials materials generated by IFCUtilsGeometry
					* @param offset world offset of the geometry
					* @return returns the geenrated repoScene upon success, nullptr otherwise
					*/
					repo::core::model::RepoScene* generateRepoScene(
						std::string                                                                &errMsg,
						std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
						std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials,
						const std::vector<double>                                                  &offset
					);

				protected:

					const std::string file;
				};
			}
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
