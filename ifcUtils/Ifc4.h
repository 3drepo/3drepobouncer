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
#include "globals.h"
#include <ifcparse/Ifc4.h>
#include <ifcparse/IfcParse.h>
#include <ifcparse/IfcFile.h>

//FIXME
#include "../bouncer/src/repo/lib/datastructure/repo_structs.h"
#include "../bouncer/src/repo/core/model/bson/repo_bson_factory.h"

namespace IfcUtils {
	namespace Schema_Ifc4 {
		class IFC_UTILS_API_EXPORT GeometryHandler {
		public:
			static bool retrieveGeometry(
				const std::string &file,
				std::vector < std::vector<double>> &allVertices,
				std::vector<std::vector<repo_face_t>> &allFaces,
				std::vector < std::vector<double>> &allNormals,
				std::vector < std::vector<double>> &allUVs,
				std::vector<std::string> &allIds,
				std::vector<std::string> &allNames,
				std::vector<std::string> &allMaterials,
				std::unordered_map<std::string, repo_material_t> &matNameToMaterials,
				std::vector<double>		&offset,
				std::string              &errMsg);
		};

		class IFC_UTILS_API_EXPORT TreeParser
		{
		public:
			bool missingEntities = false;

			TreeParser(const std::string &filePath) : ifcFile(filePath) {}
			~TreeParser() {}

			TransNode createTransformations();

		protected:

			std::pair<std::string, std::string> processUnits(
				const IfcUtil::IfcBaseClass *element);

			std::string getUnits(
				const Ifc4::IfcDerivedUnitEnum::Value &unitType
			);

			std::string getUnits(
				const Ifc4::IfcUnitEnum::Value &unitType
			);

			std::string getUnits(
				const std::string &unitType
			);

			void generateClassificationInformation(
				const Ifc4::IfcRelAssociatesClassification * &relCS,
				std::unordered_map<std::string, std::string>    &metaValues

			);

			TransNode createTransformationsRecursive(
				const IfcUtil::IfcBaseClass *element,
				std::unordered_map<std::string, std::string>                               &metaValue,
				std::unordered_map<std::string, std::string>                               &locationValue,
				const std::set<int>													       &ancestorsID = std::set<int>(),
				const std::string														   &metaPrefix = std::string()
			);

			void determineActionsByElementType(
				const IfcUtil::IfcBaseClass *element,
				std::unordered_map<std::string, std::string>                  &metaValues,
				std::unordered_map<std::string, std::string>                  &locationData,
				bool                                                          &createElement,
				bool                                                          &traverseChildren,
				std::vector<IfcUtil::IfcBaseClass *>                          &extraChildren,
				const std::string											  &metaPrefix,
				std::string											          &childrenMetaPrefix);

			void setProjectUnits(const Ifc4::IfcUnitAssignment* unitsAssignment);

			std::string constructMetadataLabel(
				const std::string &label,
				const std::string &prefix = "",
				const std::string &units = ""
			);

			std::string getValueAsString(
				const Ifc4::IfcValue    *ifcValue,
				std::string &unitType);

			std::unordered_map<std::string, std::string> projectUnits;
			IfcParse::IfcFile ifcFile;
		};
	}
}