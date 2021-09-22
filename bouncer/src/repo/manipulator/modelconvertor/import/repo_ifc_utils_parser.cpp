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
* Allows geometry creation using ifcopenshell
*/

#include "repo_ifc_utils_parser.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../core/model/collection/repo_scene.h"
#include "../../../lib/repo_utils.h"
#include <boost/filesystem.hpp>
#include <algorithm>

const static std::string IFC_ARGUMENT_GLOBAL_ID = "GlobalId";
const static std::string IFC_ARGUMENT_NAME = "Name";
const static std::string REPO_LABEL_IFC_TYPE = "IFC Type";
const static std::string REPO_LABEL_IFC_GUID = "IFC GUID";
const static std::string IFC_TYPE_SPACE_LABEL = "(IFC Space)";
const static std::string MONETARY_UNIT = "MONETARYUNIT";
const static std::string LOCATION_LABEL = "Location";
const static std::string PROJECT_LABEL = "Project";
const static std::string BUILDING_LABEL = "Building";
const static std::string STOREY_LABEL = "Storey";

using namespace repo::manipulator::modelconvertor;

IFCUtilsParser::IFCUtilsParser(const std::string &file) :
	file(file),
	missingEntities(false)
{
}

IFCUtilsParser::~IFCUtilsParser()
{
}

repo::core::model::RepoNodeSet IFCUtilsParser::createTransformations(
	IfcParse::IfcFile &ifcfile,
	std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
	std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials,
	repo::core::model::RepoNodeSet											   &metaSet
)
{
	//The ifc file shoudl always have a IFC Site as the starting tag
	auto initialElements = ifcfile.entitiesByType(IfcSchema::Type::IfcProject);

	repo::core::model::RepoNodeSet transNodes;
	if (initialElements->size())
	{
		repoTrace << "Initial elements: " << initialElements->size();
		repo::lib::RepoUUID parentID = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH);
		if (initialElements->size() > 1)
		{
			//More than one starting nodes. create a rootNode
			auto root = repo::core::model::RepoBSONFactory::makeTransformationNode();
			transNodes.insert(new repo::core::model::TransformationNode(root.cloneAndChangeName("<root>")));
		}

		repoTrace << "Looping through Elements";
		for (auto &element : *initialElements)
		{
			std::unordered_map<std::string, std::string> metaValue, locationInfo;
			auto childTransNodes = createTransformationsRecursive(ifcfile, element, meshes, materials, metaSet, metaValue, locationInfo, parentID);
			transNodes.insert(childTransNodes.begin(), childTransNodes.end());
		}
	}
	else //if (initialElements.size())
	{
		repoError << "Failed to generate IFC Tree: could not find a starting node (IFCProject)";
	}

	return transNodes;
}

repo::core::model::RepoNodeSet IFCUtilsParser::createTransformationsRecursive(
	IfcParse::IfcFile &ifcfile,
	const IfcUtil::IfcBaseClass *element,
	std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
	std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials,
	repo::core::model::RepoNodeSet											   &metaSet,
	std::unordered_map<std::string, std::string>                               &metaValue,
	std::unordered_map<std::string, std::string>                               &locationValue,
	const repo::lib::RepoUUID												   &parentID,
	const std::set<int>													       &ancestorsID,
	const std::string														   &metaPrefix
)
{
	bool createElement = true; //create a transformation for this element
	bool traverseChildren = true; //keep recursing its children
	std::vector<IfcUtil::IfcBaseClass *> extraChildren; //children outside of reference
	std::unordered_map<std::string, std::string> myMetaValues, elementInfo, locationInfo(locationValue);

	auto id = element->entity->id();
	std::string guid, name, childrenMetaPrefix;
	std::string ifcType = element->entity->datatype();
	std::string ifcTypeUpper = ifcType;
	std::transform(ifcType.begin(), ifcType.end(), ifcTypeUpper.begin(), ::toupper);
	createElement = ifcTypeUpper.find("IFCREL") == std::string::npos;
	bool isIFCSpace = IfcSchema::Type::IfcSpace == element->type();

	determineActionsByElementType(ifcfile, element, myMetaValues, locationInfo, createElement, traverseChildren, extraChildren, metaPrefix, childrenMetaPrefix);
	repo::lib::RepoUUID transID = parentID;
	repo::core::model::RepoNodeSet transNodeSet;
	for (int i = 0; i < element->getArgumentCount(); ++i)
	{
		if (element->getArgument(i)->isNull()) continue;
		auto argumentName = element->getArgumentName(i);
		if (argumentName == IFC_ARGUMENT_GLOBAL_ID)
		{
			//It comes with single quotes. remove those
			guid = element->getArgument(i)->toString().erase(0, 1);
			guid = guid.erase(guid.size() - 1, 1);
			elementInfo[REPO_LABEL_IFC_GUID] = guid;
		}
		else if (argumentName == IFC_ARGUMENT_NAME)
		{
			name = element->getArgument(i)->toString();
			if (name != "$")
			{
				name = name.erase(0, 1);;
				name = name.erase(name.size() - 1, 1);
				if (name.empty())
				{
					name = "(" + ifcType + ")";
				}
			}
			else
			{
				name = "(" + ifcType + ")";;
			}
			elementInfo[argumentName] = name;
		}
		else if (createElement) {
			std::string value;
			auto type = element->getArgument(i)->type();

			switch (type) {
			case IfcUtil::ArgumentType::Argument_ENTITY_INSTANCE:
			case IfcUtil::ArgumentType::Argument_NULL:
			case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_ENTITY_INSTANCE:
			case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE:
				//do nothing. ignore these as they are either empty or it's linkage
				break;
			case IfcUtil::ArgumentType::Argument_STRING:
			case IfcUtil::ArgumentType::Argument_ENUMERATION:
				value = element->getArgument(i)->toString();
				//It comes with single quotes or . for enumeration. remove these
				value = element->getArgument(i)->toString().erase(0, 1);
				value = value.erase(value.size() - 1, 1);
				break;
			default:
				value = element->getArgument(i)->toString();
			}

			if (!value.empty()) {
				elementInfo[argumentName] = value;
			}
		}
	}

	if (isIFCSpace) {
		name += " " + IFC_TYPE_SPACE_LABEL;
	}

	if (createElement)
	{
		std::vector<repo::lib::RepoUUID> parents;
		if (parentID.toString() != REPO_HISTORY_MASTER_BRANCH) parents.push_back(parentID);

		auto transNode = repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), name, parents);

		transID = transNode.getSharedID();

		transNodeSet.insert(new repo::core::model::TransformationNode(transNode));
	}
	bool hasTransChildren = false;
	if (traverseChildren)
	{
		auto childrenElements = ifcfile.entitiesByReference(id);

		std::set<int> childrenAncestors(ancestorsID);
		childrenAncestors.insert(id);
		if (childrenElements)
		{
			for (auto &child : *childrenElements)
			{
				if (ancestorsID.find(child->entity->id()) == ancestorsID.end())
				{
					try {
						auto childTransNodes = createTransformationsRecursive(ifcfile, child, meshes, materials, metaSet, myMetaValues, locationInfo, transID, childrenAncestors, childrenMetaPrefix);
						transNodeSet.insert(childTransNodes.begin(), childTransNodes.end());
						hasTransChildren |= childTransNodes.size();
					}
					catch (IfcParse::IfcException &e)
					{
						repoError << "Failed to process child entity " << child->entity->id() << " (" << e.what() << ")" << " element: " << element->entity->id();
						missingEntities = true;
					}
				}
			}
		}
		for (const auto &child : extraChildren)
		{
			if (ancestorsID.find(child->entity->id()) == ancestorsID.end())
			{
				try {
					auto childTransNodes = createTransformationsRecursive(ifcfile, child, meshes, materials, metaSet, myMetaValues, locationInfo, transID, childrenAncestors, childrenMetaPrefix);
					transNodeSet.insert(childTransNodes.begin(), childTransNodes.end());
					hasTransChildren |= childTransNodes.size();
				}
				catch (IfcParse::IfcException &e)
				{
					repoError << "Failed to process child entity " << child->entity->id() << " (" << e.what() << ")" << " element: " << element->entity->id();
					missingEntities = true;
				}
			}
		}
	}
	//If we created an element, add a metanode to it. if not, add them to our parent's meta info
	if (createElement)
	{
		//NOTE: if we ever recursive after creating the element, we'd have to take a copy of the metadata or it'll leak into the children.
		myMetaValues[REPO_LABEL_IFC_TYPE] = ifcType;

		myMetaValues.insert(elementInfo.begin(), elementInfo.end());
		myMetaValues.insert(locationInfo.begin(), locationInfo.end());

		std::vector<repo::lib::RepoUUID> metaParents = { transID };
		if (meshes.find(guid) != meshes.end())
		{
			//has meshes associated with it. append parent
			for (auto &mesh : meshes[guid])
			{
				*mesh = mesh->cloneAndAddParent(transID);
				if (isIFCSpace || mesh->getName().empty() && hasTransChildren) {
					*mesh = mesh->cloneAndChangeName(name);
					metaParents.push_back(mesh->getSharedID());
				}
			}
		}

		metaSet.insert(new repo::core::model::MetadataNode(repo::core::model::RepoBSONFactory::makeMetaDataNode(myMetaValues, name, metaParents)));
	}
	else
	{
		metaValue.insert(myMetaValues.begin(), myMetaValues.end());
	}

	return transNodeSet;
}

repo::core::model::RepoScene* IFCUtilsParser::generateRepoScene(
	std::string                                                                &errMsg,
	std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
	std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials,
	const std::vector<double>                                                  &offset
)
{
	IfcParse::IfcFile ifcfile;
	if (!ifcfile.Init(file))
	{
		errMsg = "Failed initialising " + file;
		return nullptr;
	}

	repoInfo << "IFC Parser initialised.";

	repoInfo << "Creating Transformations...";
	repo::core::model::RepoNodeSet dummy, meshSet, matSet, metaSet;
	repo::core::model::RepoNodeSet transNodes = createTransformations(ifcfile, meshes, materials, metaSet);

	if (!transNodes.size())
	{
		repoError << "Failed to generate a Tree from the IFC file.";
		return nullptr;
	}
	for (auto &m : meshes)
	{
		for (auto &mesh : m.second)
			meshSet.insert(mesh);
	}

	for (auto &m : materials)
	{
		matSet.insert(m.second);
	}

	std::vector<std::string> files = { file };
	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene(files, dummy, meshSet, matSet, metaSet, dummy, transNodes);
	scene->setWorldOffset(offset);

	if (missingEntities)
		scene->setMissingNodes();

	return scene;
}

std::string IFCUtilsParser::getUnits(
	const IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum &unitType
) {
	return getUnits(IfcSchema::IfcDerivedUnitEnum::ToString(unitType));
}

std::string IFCUtilsParser::getUnits(
	const IfcSchema::IfcUnitEnum::IfcUnitEnum &unitType
) {
	return getUnits(IfcSchema::IfcUnitEnum::ToString(unitType));
}

std::string IFCUtilsParser::getUnits(
	const std::string &unitType
) {
	if (projectUnits.find(unitType) != projectUnits.end())
		return projectUnits[unitType];
	return "";
}

std::string IFCUtilsParser::constructMetadataLabel(
	const std::string &label,
	const std::string &prefix,
	const std::string &units
) {
	std::stringstream ss;

	if (!prefix.empty())
		ss << prefix << "::";

	ss << label;

	if (!units.empty()) {
		ss << " (" << units << ")";
	}
	return ss.str();
}

std::string determineUnitsLabel(
	const std::string &unitName) {
	std::string data = unitName;
	repo::lib::toLower(data);

	if (unitName == "inch") return "in";
	if (unitName == "foot") return "ft";
	if (unitName == "yard") return "yd";
	if (unitName == "mile") return "mi";
	if (unitName == "acre") return "ac";
	if (unitName == "square inch") return u8"in²";
	if (unitName == "square foot") return u8"ft²";
	if (unitName == "square yard") return u8"yd²";
	if (unitName == "square mile") return u8"mi²";
	if (unitName == "cubie inch") return u8"in³";
	if (unitName == "cubic foot") return u8"ft³";
	if (unitName == "cubic yard") return u8"yd³";
	if (unitName == "litre") return "l";
	if (unitName == "fluid ounce uk") return "fl oz(UK)";
	if (unitName == "fluid ounce us") return "fl oz(US)";
	if (unitName == "pint uk") return "pint(UK)";
	if (unitName == "pint us") return "pint(UK)";
	if (unitName == "gallon uk") return "gal(UK)";
	if (unitName == "gallon us") return "gal(UK)";
	if (unitName == "degree") return u8"°";
	if (unitName == "ounce") return "oz";
	if (unitName == "pound") return "lb";
	if (unitName == "ton uk") return "t(UK)";
	if (unitName == "ton us") return "t(US)";

	return data;
}

std::string determineUnitsLabel(
	const IfcSchema::IfcSIUnitName::IfcSIUnitName &unitName) {
	switch (unitName) {
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_AMPERE:
		return "A";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_BECQUEREL:
		return "Bq";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_CANDELA:
		return "cd";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_COULOMB:
		return "C";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_CUBIC_METRE:
		return u8"m³";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_DEGREE_CELSIUS:
		return u8"°C";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_FARAD:
		return "F";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_GRAM:
		return "g";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_GRAY:
		return "gy";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_HENRY:
		return "H";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_HERTZ:
		return "Hz";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_JOULE:
		return "J";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_KELVIN:
		return "K";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_LUMEN:
		return "lm";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_LUX:
		return "lx";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_METRE:
		return "m";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_MOLE:
		return "mol";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_NEWTON:
		return "N";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_OHM:
		return u8"Ω";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_PASCAL:
		return "Pa";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_RADIAN:
		return "rad";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_SECOND:
		return "s";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_SIEMENS:
		return "S";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_SIEVERT:
		return "Sv";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_SQUARE_METRE:
		return u8"m²";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_STERADIAN:
		return "sr";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_TESLA:
		return "T";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_VOLT:
		return "V";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_WATT:
		return "W";
	case IfcSchema::IfcSIUnitName::IfcSIUnitName::IfcSIUnitName_WEBER:
		return "Wb";
	}

	return "";
}

std::string determinePrefix(
	const IfcSchema::IfcSIPrefix::IfcSIPrefix &prefixType) {
	switch (prefixType) {
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_EXA:
		return "E";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_PETA:
		return "P";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_TERA:
		return "T";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_GIGA:
		return "G";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_MEGA:
		return "M";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_KILO:
		return "k";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_HECTO:
		return "h";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_DECA:
		return "da";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_DECI:
		return "d";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_CENTI:
		return "c";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_MILLI:
		return "m";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_MICRO:
		return u8"µ";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_NANO:
		return "n";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_PICO:
		return "p";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_FEMTO:
		return "f";
	case IfcSchema::IfcSIPrefix::IfcSIPrefix::IfcSIPrefix_ATTO:
		return "a";
	}

	return "";
}

std::string getSuperScriptAsString(int value) {
	auto valueStr = std::to_string(value);
	std::stringstream ss;
	std::string symbolMapping[] = {
		u8"⁰",
		u8"¹",
		u8"²",
		u8"³",
		u8"⁴",
		u8"⁵",
		u8"⁶",
		u8"⁷",
		u8"⁸",
		u8"⁹"
	};
	for (const auto &chr : valueStr) {
		if (chr == '-') {
			ss << u8"⁻";
		}
		else {
			int digit = atoi(&chr);
			ss << symbolMapping[digit];
		}
	}

	return ss.str();
}

std::pair<std::string, std::string> processUnits(
	const IfcUtil::IfcBaseClass *element) {
	std::string unitType, unitsLabel;
	switch (element->type()) {
	case IfcSchema::Type::IfcSIUnit:
	{
		auto units = static_cast<const IfcSchema::IfcSIUnit *>(element);
		auto baseUnits = determineUnitsLabel(units->Name());
		auto prefix = units->hasPrefix() ? determinePrefix(units->Prefix()) : "";
		unitType = IfcSchema::IfcUnitEnum::ToString(units->UnitType());
		unitsLabel = prefix + baseUnits;
		break;
	}
	case IfcSchema::Type::IfcContextDependentUnit:
	{
		auto units = static_cast<const IfcSchema::IfcContextDependentUnit *>(element);
		auto baseUnits = determineUnitsLabel(units->Name());
		unitType = IfcSchema::IfcUnitEnum::ToString(units->UnitType());
		unitsLabel = baseUnits;
		break;
	}
	case IfcSchema::Type::IfcConversionBasedUnit:
	{
		auto units = static_cast<const IfcSchema::IfcConversionBasedUnit *>(element);
		unitType = IfcSchema::IfcUnitEnum::ToString(units->UnitType());
		unitsLabel = determineUnitsLabel(units->Name());;
		break;
	}
	case IfcSchema::Type::IfcMonetaryUnit:
	{
		auto units = static_cast<const IfcSchema::IfcMonetaryUnit *>(element);
		unitType = MONETARY_UNIT;
		unitsLabel = IfcSchema::IfcCurrencyEnum::ToString(units->Currency());
		break;
	}
	case IfcSchema::Type::IfcDerivedUnit:
	{
		auto units = static_cast<const IfcSchema::IfcDerivedUnit *>(element);
		auto elementList = units->Elements();
		std::stringstream ss;
		for (const auto & ele : *elementList) {
			auto parentUnits = processUnits(ele->Unit());
			auto baseUnits = parentUnits.second;
			if (!baseUnits.empty()) {
				ss << baseUnits << (ele->Exponent() == 1 ? "" : getSuperScriptAsString(ele->Exponent()));
			}
			else {
				repoError << "Unrecognised sub unit type: " << ele->Unit()->entity->toString();
			}
		}
		unitType = IfcSchema::IfcDerivedUnitEnum::ToString(units->UnitType());
		unitsLabel = ss.str();
		break;
	}
	default:
		repoWarning << "Unrecognised units entry: " << element->entity->toString();
	}

	return { unitType, unitsLabel };
}

void IFCUtilsParser::setProjectUnits(const IfcSchema::IfcUnitAssignment* unitsAssignment) {
	const auto unitsList = unitsAssignment->Units();
	for (const auto &element : *unitsList) {
		auto unitData = processUnits(element);
		if (!unitData.first.empty()) {
			projectUnits[unitData.first] = unitData.second;
		}
	}
}

void IFCUtilsParser::determineActionsByElementType(
	IfcParse::IfcFile &ifcfile,
	const IfcUtil::IfcBaseClass *element,
	std::unordered_map<std::string, std::string>                  &metaValues,
	std::unordered_map<std::string, std::string>                  &locationData,
	bool                                                          &createElement,
	bool                                                          &traverseChildren,
	std::vector<IfcUtil::IfcBaseClass *>                          &extraChildren,
	const std::string											  &metaPrefix,
	std::string													  &childrenMetaPrefix)
{
	childrenMetaPrefix = metaPrefix;

	switch (element->type())
	{
	case IfcSchema::Type::IfcRelAssignsToGroup: //This is group!
	case IfcSchema::Type::IfcRelSpaceBoundary: //This is group?
	case IfcSchema::Type::IfcRelConnectsPathElements:
	case IfcSchema::Type::IfcAnnotation:
	case IfcSchema::Type::IfcComplexProperty: //We don't support this atm.
		createElement = false;
		traverseChildren = false;
		break;
	case IfcSchema::Type::IfcRelDefinesByType:
	{
		auto def = static_cast<const IfcSchema::IfcRelDefinesByType *>(element);
		auto typeObj = static_cast<const IfcSchema::IfcTypeObject *>(def->RelatingType());
		if (typeObj->hasHasPropertySets()) {
			auto propSets = typeObj->HasPropertySets();
			extraChildren.insert(extraChildren.end(), propSets->begin(), propSets->end());
		}
		break;
	}
	case IfcSchema::Type::IfcProject:
	{
		auto project = static_cast<const IfcSchema::IfcProject *>(element);
		setProjectUnits(project->UnitsInContext());

		locationData[constructMetadataLabel(PROJECT_LABEL, LOCATION_LABEL)] = project->hasName() ? project->Name() : "(" + IfcSchema::Type::ToString(element->type()) + ")";

		createElement = true;
		traverseChildren = true;
		break;
	}
	case IfcSchema::Type::IfcBuilding:
	{
		auto building = static_cast<const IfcSchema::IfcBuilding *>(element);
		locationData[constructMetadataLabel(BUILDING_LABEL, LOCATION_LABEL)] = building->hasName() ? building->Name() : "(" + IfcSchema::Type::ToString(element->type()) + ")";

		createElement = true;
		traverseChildren = true;
		break;
	}
	case IfcSchema::Type::IfcBuildingStorey:
	{
		auto storey = static_cast<const IfcSchema::IfcBuildingStorey *>(element);
		locationData[constructMetadataLabel(STOREY_LABEL, LOCATION_LABEL)] = storey->hasName() ? storey->Name() : "(" + IfcSchema::Type::ToString(element->type()) + ")";

		createElement = true;
		traverseChildren = true;
		break;
	}
	case IfcSchema::Type::IfcPropertySingleValue:
	{
		auto propVal = static_cast<const IfcSchema::IfcPropertySingleValue *>(element);
		if (propVal)
		{
			std::string value = "n/a";
			std::string units = "";
			if (propVal->hasNominalValue())
			{
				value = getValueAsString(propVal->NominalValue(), units);
			}

			if (propVal->hasUnit())
			{
				units = processUnits(propVal->Unit()).second;
			}

			metaValues[constructMetadataLabel(propVal->Name(), metaPrefix, units)] = value;
		}
		createElement = false;
		traverseChildren = false;
		break;
	}
	case IfcSchema::Type::IfcRelDefinesByProperties:
	{
		auto defByProp = static_cast<const IfcSchema::IfcRelDefinesByProperties *>(element);
		if (defByProp)
		{
			extraChildren.push_back(defByProp->RelatingPropertyDefinition());
		}

		createElement = false;
		break;
	}
	case IfcSchema::Type::IfcPropertySet:
	{
		auto propSet = static_cast<const IfcSchema::IfcPropertySet *>(element);
		auto propDefs = propSet->HasProperties();

		extraChildren.insert(extraChildren.end(), propDefs->begin(), propDefs->end());
		createElement = false;
		childrenMetaPrefix = constructMetadataLabel(propSet->Name(), metaPrefix);
		break;
	}
	case IfcSchema::Type::IfcElementQuantity:
	{
		auto eleQuan = static_cast<const IfcSchema::IfcElementQuantity *>(element);
		auto quantities = eleQuan->Quantities();
		extraChildren.insert(extraChildren.end(), quantities->begin(), quantities->end());
		childrenMetaPrefix = constructMetadataLabel(eleQuan->Name(), metaPrefix);
		createElement = false;
		break;
	}
	case IfcSchema::Type::IfcQuantityLength:
	{
		auto quantity = static_cast<const IfcSchema::IfcQuantityLength *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second :
				getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LENGTHUNIT);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->LengthValue());
		}
		createElement = false;
		traverseChildren = false;
		break;
	}
	case IfcSchema::Type::IfcQuantityArea:
	{
		auto quantity = static_cast<const IfcSchema::IfcQuantityArea *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_AREAUNIT);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->AreaValue());
		}
		createElement = false;
		traverseChildren = false;
		break;
	}
	case IfcSchema::Type::IfcQuantityCount:
	{
		auto quantity = static_cast<const IfcSchema::IfcQuantityCount *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : "";
			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->CountValue());
		}
		createElement = false;
		traverseChildren = false;
		break;
	}
	case IfcSchema::Type::IfcQuantityTime:
	{
		auto quantity = static_cast<const IfcSchema::IfcQuantityTime *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_TIMEUNIT);
			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->TimeValue());
		}
		createElement = false;
		traverseChildren = false;
		break;
	}
	case IfcSchema::Type::IfcQuantityVolume:
	{
		auto quantity = static_cast<const IfcSchema::IfcQuantityVolume *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_VOLUMEUNIT);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->VolumeValue());
		}
		createElement = false;
		traverseChildren = false;
		break;
	}
	case IfcSchema::Type::IfcQuantityWeight:
	{
		auto quantity = static_cast<const IfcSchema::IfcQuantityWeight *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MASSUNIT);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->WeightValue());
		}
		createElement = false;
		traverseChildren = false;
		break;
	}
	case IfcSchema::Type::IfcRelAssociatesClassification:
	{
		auto relCS = static_cast<const IfcSchema::IfcRelAssociatesClassification *>(element);
		createElement = false;
		traverseChildren = false;
		generateClassificationInformation(relCS, metaValues);
	}
	case IfcSchema::Type::IfcRelContainedInSpatialStructure:
	{
		auto relCS = static_cast<const IfcSchema::IfcRelContainedInSpatialStructure *>(element);
		createElement = false;
		if (relCS)
		{
			try {
				auto relatedObjects = relCS->RelatedElements();
				if (relatedObjects)
				{
					extraChildren.insert(extraChildren.end(), relatedObjects->begin(), relatedObjects->end());
				}
				else
				{
					repoError << "Nullptr to relatedObjects!!!";
				}
			}
			catch (const IfcParse::IfcException &e)
			{
				repoError << "Failed to retrieve related elements from " << relCS->entity->id() << ": " << e.what();
				missingEntities = true;
			}
		}
		else
		{
			repoError << "Failed to convert a IfcRelContainedInSpatialStructure element into a IfcRelContainedInSpatialStructureclass.";
		}
		break;
	}
	case IfcSchema::Type::IfcRelAggregates:
	{
		auto relAgg = static_cast<const IfcSchema::IfcRelAggregates *>(element);
		createElement = false;
		if (relAgg)
		{
			auto relatedObjects = relAgg->RelatedObjects();
			if (relatedObjects)
			{
				extraChildren.insert(extraChildren.end(), relatedObjects->begin(), relatedObjects->end());
			}
			else
			{
				repoError << "Nullptr to relatedObjects!!!";
			}
		}
		else
		{
			repoError << "Failed to convert a Rel Aggregate element into a Rel Aggregate class.";
		}
		break;
	}
	}
}

std::string IFCUtilsParser::getValueAsString(
	const IfcSchema::IfcValue    *ifcValue,
	std::string &units)
{
	std::string value = "n/a";
	//FIXME: should change this on IFCOpenShell to be an inheritance with a toString() function - but this is easier said than done due ot crazy inheritance (or lack of ) in the code
	if (ifcValue)
	{
		switch (ifcValue->type())
		{
		case IfcSchema::Type::IfcAbsorbedDoseMeasure:
			value = static_cast<const IfcSchema::IfcAbsorbedDoseMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ABSORBEDDOSEUNIT);
			break;
		case IfcSchema::Type::IfcAccelerationMeasure:
			value = static_cast<const IfcSchema::IfcAccelerationMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ACCELERATIONUNIT);
			break;
		case IfcSchema::Type::IfcAmountOfSubstanceMeasure:
			value = static_cast<const IfcSchema::IfcAmountOfSubstanceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_AMOUNTOFSUBSTANCEUNIT);
			break;
		case IfcSchema::Type::IfcAngularVelocityMeasure:
			value = static_cast<const IfcSchema::IfcAngularVelocityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ANGULARVELOCITYUNIT);
			break;
		case IfcSchema::Type::IfcAreaMeasure:
			value = static_cast<const IfcSchema::IfcAreaMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_AREAUNIT);
			break;
		case IfcSchema::Type::IfcBoolean:
			value = static_cast<const IfcSchema::IfcBoolean *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcComplexNumber:
			value = static_cast<const IfcSchema::IfcComplexNumber *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcCompoundPlaneAngleMeasure:
			value = static_cast<const IfcSchema::IfcCompoundPlaneAngleMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_PLANEANGLEUNIT);
			break;
		case IfcSchema::Type::IfcContextDependentMeasure:
			value = static_cast<const IfcSchema::IfcContextDependentMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcCountMeasure:
			value = static_cast<const IfcSchema::IfcCountMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcCurvatureMeasure:
			value = static_cast<const IfcSchema::IfcCurvatureMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_CURVATUREUNIT);
			break;
		case IfcSchema::Type::IfcDescriptiveMeasure:
			value = static_cast<const IfcSchema::IfcDescriptiveMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcDoseEquivalentMeasure:
			value = static_cast<const IfcSchema::IfcDoseEquivalentMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_DOSEEQUIVALENTUNIT);
			break;
		case IfcSchema::Type::IfcDynamicViscosityMeasure:
			value = static_cast<const IfcSchema::IfcDynamicViscosityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_DYNAMICVISCOSITYUNIT);
			break;
		case IfcSchema::Type::IfcElectricCapacitanceMeasure:
			value = static_cast<const IfcSchema::IfcElectricCapacitanceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICCAPACITANCEUNIT);
			break;
		case IfcSchema::Type::IfcElectricChargeMeasure:
			value = static_cast<const IfcSchema::IfcElectricChargeMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICCHARGEUNIT);
			break;
		case IfcSchema::Type::IfcElectricConductanceMeasure:
			value = static_cast<const IfcSchema::IfcElectricConductanceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICCONDUCTANCEUNIT);
			break;
		case IfcSchema::Type::IfcElectricCurrentMeasure:
			value = static_cast<const IfcSchema::IfcElectricCurrentMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICCURRENTUNIT);
			break;
		case IfcSchema::Type::IfcElectricResistanceMeasure:
			value = static_cast<const IfcSchema::IfcElectricResistanceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICRESISTANCEUNIT);
			break;
		case IfcSchema::Type::IfcElectricVoltageMeasure:
			value = static_cast<const IfcSchema::IfcElectricVoltageMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICVOLTAGEUNIT);
			break;
		case IfcSchema::Type::IfcEnergyMeasure:
			value = static_cast<const IfcSchema::IfcEnergyMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ENERGYUNIT);
			break;
		case IfcSchema::Type::IfcForceMeasure:
			value = static_cast<const IfcSchema::IfcForceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_FORCEUNIT);
			break;
		case IfcSchema::Type::IfcFrequencyMeasure:
			value = static_cast<const IfcSchema::IfcFrequencyMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_FREQUENCYUNIT);
			break;
		case IfcSchema::Type::IfcHeatFluxDensityMeasure:
			value = static_cast<const IfcSchema::IfcHeatFluxDensityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_HEATFLUXDENSITYUNIT);
			break;
		case IfcSchema::Type::IfcHeatingValueMeasure:
			value = static_cast<const IfcSchema::IfcHeatingValueMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_HEATINGVALUEUNIT);
			break;
		case IfcSchema::Type::IfcIdentifier:
			value = static_cast<const IfcSchema::IfcIdentifier *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcIlluminanceMeasure:
			value = static_cast<const IfcSchema::IfcIlluminanceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ILLUMINANCEUNIT);
			break;
		case IfcSchema::Type::IfcInductanceMeasure:
			value = static_cast<const IfcSchema::IfcInductanceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_INDUCTANCEUNIT);
			break;
		case IfcSchema::Type::IfcInteger:
			value = static_cast<const IfcSchema::IfcInteger *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcIntegerCountRateMeasure:
			value = static_cast<const IfcSchema::IfcIntegerCountRateMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcIonConcentrationMeasure:
			value = static_cast<const IfcSchema::IfcIonConcentrationMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_IONCONCENTRATIONUNIT);
			break;
		case IfcSchema::Type::IfcIsothermalMoistureCapacityMeasure:
			value = static_cast<const IfcSchema::IfcIsothermalMoistureCapacityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ISOTHERMALMOISTURECAPACITYUNIT);
			break;
		case IfcSchema::Type::IfcKinematicViscosityMeasure:
			value = static_cast<const IfcSchema::IfcKinematicViscosityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_KINEMATICVISCOSITYUNIT);
			break;
		case IfcSchema::Type::IfcLabel:
			value = static_cast<const IfcSchema::IfcLabel *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLengthMeasure:
			value = static_cast<const IfcSchema::IfcLengthMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LENGTHUNIT);
			break;
		case IfcSchema::Type::IfcLinearForceMeasure:
			value = static_cast<const IfcSchema::IfcLinearForceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LINEARFORCEUNIT);
			break;
		case IfcSchema::Type::IfcLinearMomentMeasure:
			value = static_cast<const IfcSchema::IfcLinearMomentMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LINEARMOMENTUNIT);
			break;
		case IfcSchema::Type::IfcLinearStiffnessMeasure:
			value = static_cast<const IfcSchema::IfcLinearStiffnessMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LINEARSTIFFNESSUNIT);
			break;
		case IfcSchema::Type::IfcLinearVelocityMeasure:
			value = static_cast<const IfcSchema::IfcLinearVelocityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LINEARVELOCITYUNIT);
			break;
		case IfcSchema::Type::IfcLogical:
			value = static_cast<const IfcSchema::IfcLogical *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLuminousFluxMeasure:
			value = static_cast<const IfcSchema::IfcLuminousFluxMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LUMINOUSFLUXUNIT);
			break;
		case IfcSchema::Type::IfcLuminousIntensityDistributionMeasure:
			value = static_cast<const IfcSchema::IfcLuminousIntensityDistributionMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LUMINOUSINTENSITYDISTRIBUTIONUNIT);
			break;
		case IfcSchema::Type::IfcLuminousIntensityMeasure:
			value = static_cast<const IfcSchema::IfcLuminousIntensityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LUMINOUSINTENSITYUNIT);
			break;
		case IfcSchema::Type::IfcMagneticFluxDensityMeasure:
			value = static_cast<const IfcSchema::IfcMagneticFluxDensityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MAGNETICFLUXDENSITYUNIT);
			break;
		case IfcSchema::Type::IfcMagneticFluxMeasure:
			value = static_cast<const IfcSchema::IfcMagneticFluxMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MAGNETICFLUXUNIT);
			break;
		case IfcSchema::Type::IfcMassDensityMeasure:
			value = static_cast<const IfcSchema::IfcMassDensityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMassFlowRateMeasure:
			value = static_cast<const IfcSchema::IfcMassFlowRateMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMassMeasure:
			value = static_cast<const IfcSchema::IfcMassMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MASSUNIT);
			break;
		case IfcSchema::Type::IfcMassPerLengthMeasure:
			value = static_cast<const IfcSchema::IfcMassPerLengthMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MASSPERLENGTHUNIT);
			break;
		case IfcSchema::Type::IfcModulusOfElasticityMeasure:
			value = static_cast<const IfcSchema::IfcModulusOfElasticityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MODULUSOFELASTICITYUNIT);
			break;
		case IfcSchema::Type::IfcModulusOfLinearSubgradeReactionMeasure:
			value = static_cast<const IfcSchema::IfcModulusOfLinearSubgradeReactionMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MODULUSOFLINEARSUBGRADEREACTIONUNIT);
			break;
		case IfcSchema::Type::IfcModulusOfRotationalSubgradeReactionMeasure:
			value = static_cast<const IfcSchema::IfcModulusOfRotationalSubgradeReactionMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MODULUSOFROTATIONALSUBGRADEREACTIONUNIT);
			break;
		case IfcSchema::Type::IfcModulusOfSubgradeReactionMeasure:
			value = static_cast<const IfcSchema::IfcModulusOfSubgradeReactionMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MODULUSOFSUBGRADEREACTIONUNIT);
			break;
		case IfcSchema::Type::IfcMoistureDiffusivityMeasure:
			value = static_cast<const IfcSchema::IfcMoistureDiffusivityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MOISTUREDIFFUSIVITYUNIT);
			break;
		case IfcSchema::Type::IfcMolecularWeightMeasure:
			value = static_cast<const IfcSchema::IfcMolecularWeightMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MOLECULARWEIGHTUNIT);
			break;
		case IfcSchema::Type::IfcMomentOfInertiaMeasure:
			value = static_cast<const IfcSchema::IfcMomentOfInertiaMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MOMENTOFINERTIAUNIT);
			break;
		case IfcSchema::Type::IfcMonetaryMeasure:
			value = static_cast<const IfcSchema::IfcMonetaryMeasure *>(ifcValue)->valueAsString();
			units = getUnits(MONETARY_UNIT);
			break;
		case IfcSchema::Type::IfcNumericMeasure:
			value = static_cast<const IfcSchema::IfcNumericMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcPHMeasure:
			value = static_cast<const IfcSchema::IfcPHMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcParameterValue:
			value = static_cast<const IfcSchema::IfcParameterValue *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcPlanarForceMeasure:
			value = static_cast<const IfcSchema::IfcPlanarForceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_PLANARFORCEUNIT);
			break;
		case IfcSchema::Type::IfcPlaneAngleMeasure:
			value = static_cast<const IfcSchema::IfcPlaneAngleMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_PLANEANGLEUNIT);
			break;
		case IfcSchema::Type::IfcPositiveLengthMeasure:
			value = static_cast<const IfcSchema::IfcPositiveLengthMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LENGTHUNIT);
			break;
		case IfcSchema::Type::IfcPowerMeasure:
			value = static_cast<const IfcSchema::IfcPowerMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_POWERUNIT);
			break;
		case IfcSchema::Type::IfcPressureMeasure:
			value = static_cast<const IfcSchema::IfcPressureMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_PRESSUREUNIT);
			break;
		case IfcSchema::Type::IfcRadioActivityMeasure:
			value = static_cast<const IfcSchema::IfcRadioActivityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_RADIOACTIVITYUNIT);
			break;
		case IfcSchema::Type::IfcRatioMeasure:
			value = static_cast<const IfcSchema::IfcRatioMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcReal:
			value = static_cast<const IfcSchema::IfcReal *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcRotationalFrequencyMeasure:
			value = static_cast<const IfcSchema::IfcRotationalFrequencyMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ROTATIONALFREQUENCYUNIT);
			break;
		case IfcSchema::Type::IfcRotationalMassMeasure:
			value = static_cast<const IfcSchema::IfcRotationalMassMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ROTATIONALMASSUNIT);
			break;
		case IfcSchema::Type::IfcRotationalStiffnessMeasure:
			value = static_cast<const IfcSchema::IfcRotationalStiffnessMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ROTATIONALSTIFFNESSUNIT);
			break;
		case IfcSchema::Type::IfcSectionModulusMeasure:
			value = static_cast<const IfcSchema::IfcSectionModulusMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SECTIONMODULUSUNIT);
			break;
		case IfcSchema::Type::IfcSectionalAreaIntegralMeasure:
			value = static_cast<const IfcSchema::IfcSectionalAreaIntegralMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SECTIONAREAINTEGRALUNIT);
			break;
		case IfcSchema::Type::IfcShearModulusMeasure:
			value = static_cast<const IfcSchema::IfcShearModulusMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SHEARMODULUSUNIT);
			break;
		case IfcSchema::Type::IfcSolidAngleMeasure:
			value = static_cast<const IfcSchema::IfcSolidAngleMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_SOLIDANGLEUNIT);
			break;
		case IfcSchema::Type::IfcSoundPowerMeasure:
			value = static_cast<const IfcSchema::IfcSoundPowerMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SOUNDPOWERUNIT);
			break;
		case IfcSchema::Type::IfcSoundPressureMeasure:
			value = static_cast<const IfcSchema::IfcSoundPressureMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SOUNDPRESSUREUNIT);
			break;
		case IfcSchema::Type::IfcSpecificHeatCapacityMeasure:
			value = static_cast<const IfcSchema::IfcSpecificHeatCapacityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SPECIFICHEATCAPACITYUNIT);
			break;
		case IfcSchema::Type::IfcTemperatureGradientMeasure:
			value = static_cast<const IfcSchema::IfcTemperatureGradientMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_TEMPERATUREGRADIENTUNIT);
			break;
		case IfcSchema::Type::IfcText:
			value = static_cast<const IfcSchema::IfcText *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcThermalAdmittanceMeasure:
			value = static_cast<const IfcSchema::IfcThermalAdmittanceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALADMITTANCEUNIT);
			break;
		case IfcSchema::Type::IfcThermalConductivityMeasure:
			value = static_cast<const IfcSchema::IfcThermalConductivityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALCONDUCTANCEUNIT);
			break;
		case IfcSchema::Type::IfcThermalExpansionCoefficientMeasure:
			value = static_cast<const IfcSchema::IfcThermalExpansionCoefficientMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALEXPANSIONCOEFFICIENTUNIT);
			break;
		case IfcSchema::Type::IfcThermalResistanceMeasure:
			value = static_cast<const IfcSchema::IfcThermalResistanceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALRESISTANCEUNIT);
			break;
		case IfcSchema::Type::IfcThermalTransmittanceMeasure:
			value = static_cast<const IfcSchema::IfcThermalTransmittanceMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALTRANSMITTANCEUNIT);
			break;
		case IfcSchema::Type::IfcThermodynamicTemperatureMeasure:
			value = static_cast<const IfcSchema::IfcThermodynamicTemperatureMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_THERMODYNAMICTEMPERATUREUNIT);
			break;
		case IfcSchema::Type::IfcTimeMeasure:
			value = static_cast<const IfcSchema::IfcTimeMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_TIMEUNIT);
			break;
		case IfcSchema::Type::IfcTimeStamp:
			value = static_cast<const IfcSchema::IfcTimeStamp *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcTorqueMeasure:
			value = static_cast<const IfcSchema::IfcTorqueMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_TORQUEUNIT);
			break;
		case IfcSchema::Type::IfcVaporPermeabilityMeasure:
			value = static_cast<const IfcSchema::IfcVaporPermeabilityMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_VAPORPERMEABILITYUNIT);
			break;
		case IfcSchema::Type::IfcVolumeMeasure:
			value = static_cast<const IfcSchema::IfcVolumeMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_VOLUMEUNIT);
			break;
		case IfcSchema::Type::IfcVolumetricFlowRateMeasure:
			value = static_cast<const IfcSchema::IfcVolumetricFlowRateMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_VOLUMETRICFLOWRATEUNIT);
			break;
		case IfcSchema::Type::IfcWarpingConstantMeasure:
			value = static_cast<const IfcSchema::IfcWarpingConstantMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_WARPINGCONSTANTUNIT);
			break;
		case IfcSchema::Type::IfcWarpingMomentMeasure:
			value = static_cast<const IfcSchema::IfcWarpingMomentMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_WARPINGMOMENTUNIT);
			break;
		case IfcSchema::Type::IfcPositivePlaneAngleMeasure:
			value = static_cast<const IfcSchema::IfcPositivePlaneAngleMeasure *>(ifcValue)->valueAsString();
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_PLANEANGLEUNIT);
			break;
		case IfcSchema::Type::IfcNormalisedRatioMeasure:
			value = static_cast<const IfcSchema::IfcNormalisedRatioMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcPositiveRatioMeasure:
			value = static_cast<const IfcSchema::IfcPositiveRatioMeasure *>(ifcValue)->valueAsString();
			break;

		default:
			repoWarning << "Unrecognised IFC Property value type : " << ifcValue->type();
		}
	}

	return value;
}

std::string ifcDateToString(const IfcSchema::IfcCalendarDate *date) {
	std::stringstream ss;
	ss << date->YearComponent() << "-" << date->MonthComponent() << "-" << date->DayComponent();
	return ss.str();
}

void IFCUtilsParser::generateClassificationInformation(
	const IfcSchema::IfcRelAssociatesClassification * &relCS,
	std::unordered_map<std::string, std::string>         &metaValues

) {
	auto relatedClassification = relCS->RelatingClassification();
	if (relatedClassification)
	{
		//A classifcation can either be a classification notation or reference
		if (relatedClassification->type() == IfcSchema::Type::IfcClassificationNotation)
		{
			auto notation = static_cast<const IfcSchema::IfcClassificationNotation *>(relatedClassification);
			std::string facetList;
			auto facets = notation->NotationFacets();
			for (IfcSchema::IfcClassificationNotationFacet *facet : *facets)
			{
				if (!facetList.empty()) facetList += ", ";
				facetList += facet->NotationValue();
			}
		}
		else {
			auto reference = static_cast<const IfcSchema::IfcClassificationReference *>(relatedClassification);
			std::string classificationName = "Unknown Classification";
			if (reference->hasReferencedSource()) {
				auto refSource = reference->ReferencedSource();
				classificationName = refSource->Name();
				metaValues[constructMetadataLabel("Name", classificationName)] = classificationName;
				metaValues[constructMetadataLabel("Source", classificationName)] = refSource->Source();
				// Ad-hoc check to make sure Edition is not null, as we've found some schema defying IFC files which put $ which breaks the API.

				int editionIdx = -1;
				for (int i = 0; i < refSource->getArgumentCount(); ++i) {
					if (refSource->getArgumentName(i) == "Edition") {
						editionIdx = i;
					}
				}

				if (editionIdx >= 0 && !refSource->getArgument(editionIdx)->isNull())
					metaValues[constructMetadataLabel("Edition", classificationName)] = refSource->Edition();

				if (refSource->hasEditionDate()) {
					metaValues[constructMetadataLabel("Edition date", classificationName)] = ifcDateToString(refSource->EditionDate());
				}
			}
			const auto refPrefix = constructMetadataLabel("Reference", classificationName);
			if (reference->hasName())
				metaValues[constructMetadataLabel("Name", refPrefix)] = reference->Name();
			if (reference->hasItemReference())
				metaValues[constructMetadataLabel("Identification", refPrefix)] = reference->ItemReference();
			if (reference->hasLocation())
				metaValues[constructMetadataLabel("Location", refPrefix)] = reference->Location();
		}
	}
}