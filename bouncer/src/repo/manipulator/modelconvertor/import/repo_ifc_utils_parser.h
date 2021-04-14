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

#include <ifcgeom/IfcGeom.h>
#include <ifcgeom/IfcGeomIterator.h>

#include "../../../core/model/bson/repo_node_material.h"
#include "../../../core/model/bson/repo_node_mesh.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
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
				repo::core::model::RepoNodeSet createTransformations(
					IfcParse::IfcFile &ifcfile,
					std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
					std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials,
					repo::core::model::RepoNodeSet											   &metaSet
				);

				repo::core::model::RepoNodeSet createTransformationsRecursive(
					IfcParse::IfcFile &ifcfile,
					const IfcUtil::IfcBaseClass *element,
					std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
					std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials,
					repo::core::model::RepoNodeSet											   &metaSet,
					std::unordered_map<std::string, std::string>                               &metaValue,
					const repo::lib::RepoUUID												   &parentID,
					const std::set<int>													       &ancestorsID = std::set<int>(),
					const std::string														   &metaPrefix = std::string()
				);

				void determineActionsByElementType(
					IfcParse::IfcFile &ifcfile,
					const IfcUtil::IfcBaseClass *element,
					std::unordered_map<std::string, std::string>                  &metaValues,
					bool                                                          &createElement,
					bool                                                          &traverseChildren,
					bool                                                          &isIFCSpace,
					std::vector<int>                                              &extraChildren,
					const std::string											  &metaPrefix,
					std::string											          &childrenMetaPrefix);

				std::string getValueAsString(
					const IfcSchema::IfcValue    *ifcValue);

				const std::string file;
				bool missingEntities;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
