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
* Abstract Model convertor(Import)
*/

#pragma once

#include <string>
#include <unordered_map>

#include <ifcgeom/IfcGeom.h>
#include <ifcgeom/IfcGeomIterator.h>

#include "../../../core/model/bson/repo_node_material.h"
#include "../../../core/model/bson/repo_node_mesh.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			class IFCUtilsGeometry
			{
			public:

				/**
				* Create IFCUtilsGeometry
				* in this object!
				* @param file IFC file location
				*/
				IFCUtilsGeometry(const std::string &file);

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
				bool generateGeometry(std::string &errMsg);

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
				std::unordered_map < std::string, repo::core::model::MaterialNode* >
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
				/**
				* construct a repo_material_t based on the IfcGeom Material
				* @param material IfcGeom material
				* @return returns a repo_material_t with properties from the provided ifcgeom material
				*/
				repo_material_t createMaterial(
					const IfcGeom::Material &material);

				/**
				* Create an IFCGeom Iterator settings
				* @return returns a Iterator settings with default settings
				*/
				IfcGeom::IteratorSettings createSettings();

				/**
				* Retrieve geometry from the iterator
				* @param contextIterator iterator with ifc file loaded
				* @param allVertices where vertices of all meshes will be stored as a result
				* @param allFaces where faces of all meshes will be stored as a result
				* @param allNormals where normals of all meshes will be stored as a result
				* @param allUVs where uvs of all meshes will be stored as a result
				* @param allIds where ifcIDs associated to all meshes will be stored as a result
				* @param allNames where names associated to all meshes will be stored as a result
				* @param allMaterials where materials associated to all meshes will be stored as a result
				*/
				void retrieveGeometryFromIterator(
					IfcGeom::Iterator<double> &contextIterator,
					const bool useMaterialNames,
					std::vector < std::vector<double>> &allVertices,
					std::vector<std::vector<repo_face_t>> &allFaces,
					std::vector < std::vector<double>> &allNormals,
					std::vector < std::vector<double>> &allUVs,
					std::vector<std::string> &allIds,
					std::vector<std::string> &allNames,
					std::vector<std::string> &allMaterials);

				const std::string file;
				std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> meshes;
				std::unordered_map<std::string, repo::core::model::MaterialNode*> materials;
				std::vector<double> offset;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
