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

#include "../repo_model_import_config.h"
#include "../../../../core/model/bson/repo_node_material.h"
#include "../../../../core/model/bson/repo_node_mesh.h"
namespace modelConverter = repo::manipulator::modelconvertor;

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace ifcHelper {
				class IFCUtilsGeometry
				{
				public:

					/**
					* Create IFCUtilsGeometry
					* in this object!
					* @param file IFC file location
					*/
					IFCUtilsGeometry(const std::string &file, const modelConverter::ModelImportConfig &settings);

					/**
					* Default Deconstructor
					*/
					virtual ~IFCUtilsGeometry();

					/**
					* Generate geometry with the file given.
					* also populates the meshes and offset fields within this object.
					* @param errMsg error message shown should the function fail
					* @return returns true upon success
					*/
					bool generateGeometry(std::string &errMsg, bool &partialFailure);

					/**
					* Return a map of ifc guid -> repo mesh nodes that was generated by
					* this object
					* @return returns a map of mesh nodes, empty if process failed
					*/
					std::unordered_map < std::string, std::vector<repo::core::model::MeshNode*> >
						getGeneratedMeshes()
					{
						return meshes;
					}

					/**
					* Return a map of material name -> repo material nodes that was generated by
					* this object
					* @return returns a map of mesh nodes, empty if process failed
					*/
					std::vector <repo::core::model::MaterialNode* >
						getGeneratedMaterials()
					{
						return materials;
					}

					/**
					* Return the (x,y,z) offset from world coordinates
					* @return returns (x,y,z) offset
					*/
					std::vector<double> getGeometryOffset()
					{
						return offset;
					}

				protected:
					const std::string file;
					std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> meshes;
					std::vector<repo::core::model::MaterialNode*> materials;
					std::vector<double> offset;
				};
			}
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
