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

#define _NS_TO_USE(prefix) _CREATE_SCHEMA_NS(prefix, SCHEMA_IN_FILE)
#define NS_TO_USE _NS_TO_USE(Schema_)

#include "repo_ifc_utils_globals.h"
#include <ifcparse/IfcParse.h>
#include <ifcparse/IfcFile.h>

#define CONCAT(a, b) TO_STRING(a/b)
#define _INCLUDE_SCHEMA_HEADER(x) CONCAT(ifcparse, x)
#define INCLUDE_SCHEMA_HEADER(x) _INCLUDE_SCHEMA_HEADER(x.h)
#include INCLUDE_SCHEMA_HEADER(SCHEMA_IN_FILE)
#undef INCLUDE_SCHEMA_HEADER
#undef _INCLUDE_SCHEMA_HEADER
#undef CONCAT

#include <repo/lib/datastructure/repo_structs.h>

namespace repo {
	namespace ifcUtility {
		namespace NS_TO_USE {
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

				TreeParser() {}
				~TreeParser() {}

				static TransNode createTransformations(const std::string &filePath, bool &missingEntities);

			protected:

				static std::pair<std::string, std::string> processUnits(
					const IfcUtil::IfcBaseClass *element);

				static std::string getUnits(
					const SCHEMA_IN_FILE::IfcDerivedUnitEnum::Value &unitType,
					const std::unordered_map<std::string, std::string> &projectUnits
				);

				static std::string getUnits(
					const SCHEMA_IN_FILE::IfcUnitEnum::Value &unitType,
					const std::unordered_map<std::string, std::string> &projectUnits
				);

				static std::string getUnits(
					const std::string &unitType,
					const std::unordered_map<std::string, std::string> &projectUnits
				);

				static void generateClassificationInformation(
					const SCHEMA_IN_FILE::IfcRelAssociatesClassification * &relCS,
					std::unordered_map<std::string, std::string>    &metaValues

				);

				static TransNode createTransformationsRecursive(
					IfcParse::IfcFile &ifcFile,
					bool &missingEntities,
					const IfcUtil::IfcBaseClass *element,
					std::unordered_map<std::string, std::string>                               &metaValue,
					std::unordered_map<std::string, std::string>                               &locationValue,
					std::unordered_map<std::string, std::string>                               &projectUnits,
					std::unordered_map<int, std::unordered_map<std::string, std::string>>      &metadataGroup,
					const std::set<int>													       &ancestorsID = std::set<int>(),
					const std::string														   &metaPrefix = std::string()
				);

				static void determineActionsByElementType(
					const IfcUtil::IfcBaseClass *element,
					bool &missingEntities,
					std::unordered_map<std::string, std::string>                  &metaValues,
					std::unordered_map<std::string, std::string>                  &locationData,
					std::unordered_map<std::string, std::string>                  &projectUnits,
					bool                                                          &createElement,
					bool                                                          &traverseChildren,
					bool                                                          &cacheMetadata,
					std::vector<IfcUtil::IfcBaseClass *>                          &extraChildren,
					const std::string											  &metaPrefix,
					std::string											          &childrenMetaPrefix);

				static void setProjectUnits(
					const SCHEMA_IN_FILE::IfcUnitAssignment* unitsAssignment,
					std::unordered_map<std::string, std::string> &projectUnits);

				static std::string constructMetadataLabel(
					const std::string &label,
					const std::string &prefix = "",
					const std::string &units = ""
				);

				static std::string getValueAsString(
					const SCHEMA_IN_FILE::IfcValue    *ifcValue,
					std::string &unitType,
					const std::unordered_map<std::string, std::string> &projectUnits);
			};
		}
	}
}

#undef _NS_TO_USE
#undef NS_TO_USE