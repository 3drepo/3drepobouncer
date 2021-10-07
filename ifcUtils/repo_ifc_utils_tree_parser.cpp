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

#include "repo_ifc_utils_globals.h"
#define _INCLUDE_HEADER(x) TO_STRING(repo_ifc_utils_ ## x)
#define INCLUDE_HEADER(x) _INCLUDE_HEADER(x.h)
#include INCLUDE_HEADER(IfcSchema)
#undef INCLUDE_HEADER
#undef _INCLUDE_HEADER

#include <boost/algorithm/string/predicate.hpp>

#include "repo_ifc_utils_constants.h"
#include <repo/lib/repo_utils.h>

TransNode  repo::ifcUtility::SCHEMA_NS::TreeParser::createTransformations(const std::string &filePath, bool &missingEntities)
{
	IfcParse::IfcFile ifcFile = filePath;
	auto initialElements = ifcFile.instances_by_type<IfcSchema::IfcProject>();

	TransNode node;
	if (initialElements->size())
	{
		repoTrace << "Looping through Elements";
		std::unordered_map<std::string, std::string> metaValue, locationInfo;
		std::unordered_map<std::string, std::string> projectUnits;
		std::unordered_map<int, std::unordered_map<std::string, std::string>> metadataGroup;
		node = createTransformationsRecursive(ifcFile, missingEntities, *initialElements->begin(), metaValue, locationInfo, projectUnits, metadataGroup);
	}
	else
	{
		repoError << "Failed to generate IFC Tree: could not find a starting node (IFCProject)";
	}

	return node;
}

TransNode repo::ifcUtility::SCHEMA_NS::TreeParser::createTransformationsRecursive(
	IfcParse::IfcFile &ifcFile,
	bool &missingEntities,
	const IfcUtil::IfcBaseClass *element,
	std::unordered_map<std::string, std::string>                               &metaValue,
	std::unordered_map<std::string, std::string>                               &locationValue,
	std::unordered_map<std::string, std::string>                               &projectUnits,
	std::unordered_map<int, std::unordered_map<std::string, std::string>>      &metadataGroup,
	const std::set<int>													       &ancestorsID,
	const std::string														   &metaPrefix
)
{
	bool createElement = true; //create a transformation for this element
	bool traverseChildren = true; //keep recursing its children
	std::vector<IfcUtil::IfcBaseClass *> extraChildren; //children outside of reference
	std::unordered_map<std::string, std::string> myMetaValues, elementInfo, locationInfo(locationValue);

	auto id = element->data().id();
	std::string guid, childrenMetaPrefix;
	std::string name = "(" + element->data().type()->name() + ")";
	std::string ifcType = element->data().type()->name_lc();
	createElement = ifcType.find(IFC_TYPE_IFCREL_PREFIX) == std::string::npos;
	bool isIFCSpace = IFC_TYPE_SPACE == ifcType;
	bool cacheMetadata = false;
	determineActionsByElementType(element, missingEntities, myMetaValues, locationInfo, projectUnits, createElement, traverseChildren, cacheMetadata, extraChildren, metaPrefix, childrenMetaPrefix);
	std::vector<TransNode> nodesCollected;

	if (cacheMetadata && metadataGroup.find(id) != metadataGroup.end()) {
		metaValue.insert(metadataGroup[id].begin(), metadataGroup[id].end());
		return TransNode();
	}

	auto eleEntity = element->declaration().as_entity();
	for (int i = 0; i < element->data().getArgumentCount(); ++i)
	{
		if (element->data().getArgument(i)->isNull()) continue;
		auto argumentName = eleEntity->attribute_by_index(i)->name();
		if (argumentName == IFC_ARGUMENT_GLOBAL_ID)
		{
			//It comes with single quotes. remove those
			guid = element->data().getArgument(i)->toString().erase(0, 1);
			guid = guid.erase(guid.size() - 1, 1);
			elementInfo[REPO_LABEL_IFC_GUID] = guid;
		}
		else if (argumentName == IFC_ARGUMENT_NAME)
		{
			auto elementName = element->data().getArgument(i)->toString();
			auto typeName = element->data().type()->name();
			if (elementName != "$")
			{
				elementName = elementName.erase(0, 1);;
				elementName = elementName.erase(elementName.size() - 1, 1);
				if (!elementName.empty())
				{
					name = elementName;
				}
			}

			if (isIFCSpace) {
				name += " " + IFC_TYPE_SPACE_LABEL;
			}

			elementInfo[argumentName] = name;
		}
		else if (createElement) {
			std::string value;
			auto type = element->data().getArgument(i)->type();

			switch (type) {
			case IfcUtil::ArgumentType::Argument_ENTITY_INSTANCE:
			case IfcUtil::ArgumentType::Argument_NULL:
			case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_ENTITY_INSTANCE:
			case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE:
				//do nothing. ignore these as they are either empty or it's linkage
				break;
			case IfcUtil::ArgumentType::Argument_STRING:
			case IfcUtil::ArgumentType::Argument_ENUMERATION:
				value = element->data().getArgument(i)->toString();
				//It comes with single quotes or . for enumeration. remove these
				value = element->data().getArgument(i)->toString().erase(0, 1);
				value = value.erase(value.size() - 1, 1);
				break;
			default:
				value = element->data().getArgument(i)->toString();
			}

			if (!value.empty()) {
				elementInfo[argumentName] = value;
			}
		}
	}

	//Do not create element if there's no guid
	createElement &= !guid.empty();

	TransNode transNode;
	if (createElement)
	{
		transNode.createNode = true;
		transNode.name = name;
		transNode.guid = guid;
	}

	bool hasTransChildren = false;

	if (traverseChildren)
	{
		auto childrenElements = ifcFile.instances_by_reference(id);

		std::set<int> childrenAncestors(ancestorsID);
		childrenAncestors.insert(id);
		if (childrenElements)
		{
			for (auto &child : *childrenElements)
			{
				try {
					if (ancestorsID.find(child->data().id()) == ancestorsID.end())
					{
						auto childNode = createTransformationsRecursive(ifcFile, missingEntities, child, myMetaValues, locationValue, projectUnits, metadataGroup, childrenAncestors, childrenMetaPrefix);
						if (childNode.children.size() || childNode.createNode) {
							transNode.children.push_back(childNode);
							hasTransChildren = true;
						}
					}
				}
				catch (IfcParse::IfcException &e)
				{
					repoError << "Failed to process child entity " << child->data().id() << " (" << e.what() << ")" << " element: " << element->data().id();
					missingEntities = true;
				}
			}
		}
		for (const auto &child : extraChildren)
		{
			if (ancestorsID.find(child->data().id()) == ancestorsID.end())
			{
				try {
					auto childNode = createTransformationsRecursive(ifcFile, missingEntities, child, myMetaValues, locationValue, projectUnits, metadataGroup, childrenAncestors, childrenMetaPrefix);
					if (childNode.children.size() || childNode.createNode) {
						transNode.children.push_back(childNode);
						hasTransChildren = true;
					}
				}
				catch (IfcParse::IfcException &e)
				{
					repoError << "Failed to process child entity " << child->data().id() << " (" << e.what() << ")" << " element: " << element->data().id();
					missingEntities = true;
				}
			}
		}
	}
	//If we created an element, add a metanode to it. if not, add them to our parent's meta info
	if (createElement)
	{
		transNode.meta = myMetaValues;
		//NOTE: if we ever recursive after creating the element, we'd have to take a copy of the metadata or it'll leak into the children.
		transNode.meta[REPO_LABEL_IFC_TYPE] = element->data().type()->name();
		transNode.meta.insert(elementInfo.begin(), elementInfo.end());
		transNode.meta.insert(locationInfo.begin(), locationInfo.end());
		transNode.meshTakeName = hasTransChildren;
		transNode.isIfcSpace = hasTransChildren;
	}
	else
	{
		if (cacheMetadata) {
			metadataGroup[id] = std::unordered_map<std::string, std::string>(myMetaValues);
		}
		metaValue.insert(myMetaValues.begin(), myMetaValues.end());
	}

	return transNode;
}

std::string repo::ifcUtility::SCHEMA_NS::TreeParser::constructMetadataLabel(
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

void repo::ifcUtility::SCHEMA_NS::TreeParser::determineActionsByElementType(
	const IfcUtil::IfcBaseClass *element,
	bool & missingEntities,
	std::unordered_map<std::string, std::string>                  &metaValues,
	std::unordered_map<std::string, std::string>                  &locationData,
	std::unordered_map<std::string, std::string>                  &projectUnits,
	bool                                                          &createElement,
	bool                                                          &traverseChildren,
	bool                                                          &cacheMetadata,
	std::vector<IfcUtil::IfcBaseClass *>                          &extraChildren,
	const std::string											  &metaPrefix,
	std::string													  &childrenMetaPrefix)
{
	childrenMetaPrefix = metaPrefix;

	auto typeName = element->data().type()->name_lc();

	if (typesToIgnore.find(typeName) != typesToIgnore.end()) {
		createElement = false;
		traverseChildren = false;
	}
	else if (typeName == IFC_TYPE_REL_NESTS) {
		createElement = false;
		traverseChildren = true;
		auto nest = static_cast<const IfcSchema::IfcRelNests *>(element);
		auto objs = nest->RelatedObjects();
		extraChildren.insert(extraChildren.end(), objs->begin(), objs->end());
	}
	else if (typeName == IFC_TYPE_REL_DEFINES_BY_TYPE) {
		auto def = static_cast<const IfcSchema::IfcRelDefinesByType *>(element);
		auto typeObj = static_cast<const IfcSchema::IfcTypeObject *>(def->RelatingType());
		if (typeObj->hasHasPropertySets()) {
			auto propSets = typeObj->HasPropertySets();
			extraChildren.insert(extraChildren.end(), propSets->begin(), propSets->end());
		}
		cacheMetadata = true;
		createElement = false;
	}
	else if (typeName == IFC_TYPE_PROJECT) {
		auto project = static_cast<const IfcSchema::IfcProject *>(element);
		setProjectUnits(project->UnitsInContext(), projectUnits);

		locationData[constructMetadataLabel(PROJECT_LABEL, LOCATION_LABEL)] = project->hasName() ? project->Name() : "(" + element->data().type()->name() + ")";

		createElement = true;
		traverseChildren = true;
	}
	else if (typeName == IFC_TYPE_BUILDING) {
		auto building = static_cast<const IfcSchema::IfcBuilding *>(element);
		locationData[constructMetadataLabel(BUILDING_LABEL, LOCATION_LABEL)] = building->hasName() ? building->Name() : "(" + element->data().type()->name() + ")";

		createElement = true;
		traverseChildren = true;
	}
	else if (typeName == IFC_TYPE_BUILDING_STOREY) {
		auto storey = static_cast<const IfcSchema::IfcBuildingStorey *>(element);
		locationData[constructMetadataLabel(STOREY_LABEL, LOCATION_LABEL)] = storey->hasName() ? storey->Name() : "(" + element->data().type()->name() + ")";

		createElement = true;
		traverseChildren = true;
	}
	else if (typeName == IFC_TYPE_PROPERTY_SINGLE_VALUE) {
		auto propVal = static_cast<const IfcSchema::IfcPropertySingleValue *>(element);
		if (propVal)
		{
			std::string value = "n/a";
			std::string units = "";
			if (propVal->hasNominalValue())
			{
				value = getValueAsString(propVal->NominalValue(), units, projectUnits);
			}

			if (propVal->hasUnit())
			{
				units = processUnits(propVal->Unit()).second;
			}

			metaValues[constructMetadataLabel(propVal->Name(), metaPrefix, units)] = value;
		}
		createElement = false;
		traverseChildren = false;
	}
	else if (typeName == IFC_TYPE_REL_DEFINES_BY_PROPERTIES) {
		auto defByProp = static_cast<const IfcSchema::IfcRelDefinesByProperties *>(element);
		if (defByProp)
		{
			extraChildren.push_back(defByProp->RelatingPropertyDefinition());
		}

		createElement = false;
		cacheMetadata = true;
	}
	else if (typeName == IFC_TYPE_PROPERTY_SET) {
		auto propSet = static_cast<const IfcSchema::IfcPropertySet *>(element);
		auto propDefs = propSet->HasProperties();

		extraChildren.insert(extraChildren.end(), propDefs->begin(), propDefs->end());
		createElement = false;
		cacheMetadata = true;
		childrenMetaPrefix = constructMetadataLabel(propSet->Name(), metaPrefix);
	}
	else if (typeName == IFC_TYPE_PROPERTY_BOUNDED_VALUE) {
		auto propSet = static_cast<const IfcSchema::IfcPropertyBoundedValue *>(element);
		auto unitsOverride = propSet->hasUnit() ? processUnits(propSet->Unit()).second : "";
		std::string upperBound, lowerBound;
		if (propSet->hasUpperBoundValue()) {
			std::string units;
			upperBound = getValueAsString(propSet->UpperBoundValue(), units, projectUnits);
			if (unitsOverride.empty()) unitsOverride = units;
		}
		if (propSet->hasLowerBoundValue()) {
			std::string units;
			lowerBound = getValueAsString(propSet->UpperBoundValue(), units, projectUnits);
			if (unitsOverride.empty()) unitsOverride = units;
		}

		metaValues[constructMetadataLabel(propSet->Name(), metaPrefix, unitsOverride)] = "[" + lowerBound + ", " + upperBound + "]";

		createElement = false;
		traverseChildren = false;
	}
	else if (typeName == IFC_TYPE_ELEMENT_QUANTITY) {
		auto eleQuan = static_cast<const IfcSchema::IfcElementQuantity *>(element);
		auto quantities = eleQuan->Quantities();
		cacheMetadata = true;
		extraChildren.insert(extraChildren.end(), quantities->begin(), quantities->end());
		childrenMetaPrefix = constructMetadataLabel(eleQuan->Name(), metaPrefix);
		createElement = false;
	}
	else if (typeName == IFC_TYPE_QUANTITY_LENGTH) {
		auto quantity = static_cast<const IfcSchema::IfcQuantityLength *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second :
				getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LENGTHUNIT, projectUnits);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->LengthValue());
		}
		createElement = false;
		traverseChildren = false;
	}
	else if (typeName == IFC_TYPE_QUANTITY_AREA) {
		auto quantity = static_cast<const IfcSchema::IfcQuantityArea *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_AREAUNIT, projectUnits);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->AreaValue());
		}
		createElement = false;
		traverseChildren = false;
	}
	else if (typeName == IFC_TYPE_QUANTITY_COUNT) {
		auto quantity = static_cast<const IfcSchema::IfcQuantityCount *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : "";
			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->CountValue());
		}
		createElement = false;
		traverseChildren = false;
	}
	else if (typeName == IFC_TYPE_QUANTITY_TIME) {
		auto quantity = static_cast<const IfcSchema::IfcQuantityTime *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_TIMEUNIT, projectUnits);
			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->TimeValue());
		}
		createElement = false;
		traverseChildren = false;
	}
	else if (typeName == IFC_TYPE_QUANTITY_VOLUME) {
		auto quantity = static_cast<const IfcSchema::IfcQuantityVolume *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_VOLUMEUNIT, projectUnits);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->VolumeValue());
		}
		createElement = false;
		traverseChildren = false;
	}
	else if (typeName == IFC_TYPE_QUANTITY_WEIGHT) {
		auto quantity = static_cast<const IfcSchema::IfcQuantityWeight *>(element);
		if (quantity)
		{
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MASSUNIT, projectUnits);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = std::to_string(quantity->WeightValue());
		}
		createElement = false;
		traverseChildren = false;
	}
	else if (typeName == IFC_TYPE_REL_ASSOCIATES_CLASSIFICATION) {
		auto relCS = static_cast<const IfcSchema::IfcRelAssociatesClassification *>(element);
		createElement = false;
		traverseChildren = false;
		generateClassificationInformation(relCS, metaValues);
	}
	else if (typeName == IFC_TYPE_REL_CONTAINED_IN_SPATIAL_STRUCTURE) {
		auto relCS = static_cast<const IfcSchema::IfcRelContainedInSpatialStructure *>(element);

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
			repoError << "Failed to retrieve related elements from " << relCS->data().id() << ": " << e.what();
			missingEntities = true;
		}
		createElement = false;
	}
	else if (typeName == IFC_TYPE_REL_AGGREGATES) {
		auto relAgg = static_cast<const IfcSchema::IfcRelAggregates *>(element);
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
	else if (dynamic_cast<const IfcSchema::IfcElementType *>(element)) {
		createElement = false;
		traverseChildren = true;
		cacheMetadata = true;
	}
	else if (
		//Ignore anything that is to do with styles (e.g. IfcWindowStyle)
		boost::algorithm::ends_with(element->data().type()->name_lc(), "style") ||

		boost::algorithm::ends_with(element->data().type()->name_lc(), "material")
		) {
		createElement = false;
		traverseChildren = false;
	}
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
	const IfcSchema::IfcSIUnitName::Value &unitName) {
	switch (unitName) {
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_AMPERE:
		return "A";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_BECQUEREL:
		return "Bq";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_CANDELA:
		return "cd";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_COULOMB:
		return "C";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_CUBIC_METRE:
		return u8"m³";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_DEGREE_CELSIUS:
		return u8"°C";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_FARAD:
		return "F";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_GRAM:
		return "g";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_GRAY:
		return "gy";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_HENRY:
		return "H";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_HERTZ:
		return "Hz";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_JOULE:
		return "J";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_KELVIN:
		return "K";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_LUMEN:
		return "lm";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_LUX:
		return "lx";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_METRE:
		return "m";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_MOLE:
		return "mol";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_NEWTON:
		return "N";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_OHM:
		return u8"Ω";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_PASCAL:
		return "Pa";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_RADIAN:
		return "rad";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_SECOND:
		return "s";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_SIEMENS:
		return "S";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_SIEVERT:
		return "Sv";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_SQUARE_METRE:
		return u8"m²";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_STERADIAN:
		return "sr";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_TESLA:
		return "T";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_VOLT:
		return "V";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_WATT:
		return "W";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_WEBER:
		return "Wb";
	}

	return "";
}

std::string determinePrefix(
	const IfcSchema::IfcSIPrefix::Value &prefixType) {
	switch (prefixType) {
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_EXA:
		return "E";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_PETA:
		return "P";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_TERA:
		return "T";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_GIGA:
		return "G";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_MEGA:
		return "M";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_KILO:
		return "k";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_HECTO:
		return "h";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_DECA:
		return "da";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_DECI:
		return "d";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_CENTI:
		return "c";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_MILLI:
		return "m";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_MICRO:
		return u8"µ";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_NANO:
		return "n";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_PICO:
		return "p";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_FEMTO:
		return "f";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_ATTO:
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

std::pair<std::string, std::string> repo::ifcUtility::SCHEMA_NS::TreeParser::processUnits(
	const IfcUtil::IfcBaseClass *element) {
	std::string unitType, unitsLabel;
	auto typeName = element->data().type()->name_lc();
	if (typeName == IFC_TYPE_SI_UNIT)
	{
		auto units = static_cast<const IfcSchema::IfcSIUnit *>(element);
		auto baseUnits = determineUnitsLabel(units->Name());
		auto prefix = units->hasPrefix() ? determinePrefix(units->Prefix()) : "";
		unitType = IfcSchema::IfcUnitEnum::ToString(units->UnitType());
		unitsLabel = prefix + baseUnits;
	}
	else if (typeName == IFC_TYPE_CONTEXT_DEPENDENT_UNIT)
	{
		auto units = static_cast<const IfcSchema::IfcContextDependentUnit *>(element);
		auto baseUnits = determineUnitsLabel(units->Name());
		unitType = IfcSchema::IfcUnitEnum::ToString(units->UnitType());
		unitsLabel = baseUnits;
	}
	else if (typeName == IFC_TYPE_CONVERSION_BASED_UNIT)
	{
		auto units = static_cast<const IfcSchema::IfcConversionBasedUnit *>(element);
		unitType = IfcSchema::IfcUnitEnum::ToString(units->UnitType());
		unitsLabel = determineUnitsLabel(units->Name());;
	}
	else if (typeName == IFC_TYPE_MONETARY_UNIT)
	{
		auto units = static_cast<const IfcSchema::IfcMonetaryUnit *>(element);
		unitType = MONETARY_UNIT;
#if DEFINED_Ifc2x3
		unitsLabel = IfcSchema::IfcCurrencyEnum::ToString(units->Currency());
#else
		unitsLabel = units->Currency();
#endif
	}
	else if (typeName == IFC_TYPE_DERIVED_UNIT)
	{
		auto units = static_cast<const IfcSchema::IfcDerivedUnit *>(element);
		auto elementList = units->Elements();
		std::stringstream ss;
		for (const auto & ele : *elementList) {
			auto baseUnit = processUnits(ele).second;
			if (!baseUnit.empty()) {
				ss << baseUnit << (ele->Exponent() == 1 ? "" : getSuperScriptAsString(ele->Exponent()));
			}
			else {
				repoError << "Unrecognised sub unit type: " << ele->Unit()->data().toString();
			}
	}
		unitType = IfcSchema::IfcDerivedUnitEnum::ToString(units->UnitType());
		unitsLabel = ss.str();
}
	else if (typeName == IFC_TYPE_DERIVED_UNIT_ELEMENT)
	{
		auto units = static_cast<const IfcSchema::IfcDerivedUnitElement *>(element);

		auto baseUnit = processUnits(units->Unit()).second;
		if (!baseUnit.empty()) {
			unitsLabel = baseUnit + (units->Exponent() == 1 ? "" : getSuperScriptAsString(units->Exponent()));
		}
		else {
			repoError << "Unrecognised sub unit type: " << units->Unit()->data().toString();
		}
	}
	else {
		repoWarning << "Unrecognised units entry: " << element->data().toString();
	}

	return { unitType, unitsLabel };
}

void repo::ifcUtility::SCHEMA_NS::TreeParser::setProjectUnits(
	const IfcSchema::IfcUnitAssignment* unitsAssignment,
	std::unordered_map<std::string, std::string>                  &projectUnits
) {
	const auto unitsList = unitsAssignment->Units();
	for (const auto &element : *unitsList) {
		auto unitData = processUnits(element);
		if (!unitData.first.empty()) {
			projectUnits[unitData.first] = unitData.second;
		}
	}
}

std::string repo::ifcUtility::SCHEMA_NS::TreeParser::getUnits(
	const IfcSchema::IfcDerivedUnitEnum::Value &unitType,
	const std::unordered_map<std::string, std::string> &projectUnits
) {
	return getUnits(IfcSchema::IfcDerivedUnitEnum::ToString(unitType), projectUnits);
}

std::string repo::ifcUtility::SCHEMA_NS::TreeParser::getUnits(
	const IfcSchema::IfcUnitEnum::Value &unitType,
	const std::unordered_map<std::string, std::string> &projectUnits
) {
	return getUnits(IfcSchema::IfcUnitEnum::ToString(unitType), projectUnits);
}

std::string repo::ifcUtility::SCHEMA_NS::TreeParser::getUnits(
	const std::string &unitType,
	const std::unordered_map<std::string, std::string> &projectUnits
) {
	if (projectUnits.find(unitType) != projectUnits.end())
		return projectUnits.at(unitType);
	return "";
}

std::string repo::ifcUtility::SCHEMA_NS::TreeParser::getValueAsString(
	const IfcSchema::IfcValue    *ifcValue,
	std::string &units,
	const std::unordered_map<std::string, std::string> &projectUnits)
{
	std::string value = "n/a";
	if (ifcValue)
	{
		auto typeName = ifcValue->data().type()->name_lc();
		if (typeName == IFC_TYPE_ABSORBED_DOSE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcAbsorbedDoseMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ABSORBEDDOSEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ACCELERATION_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcAccelerationMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ACCELERATIONUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_AMOUNT_OF_SUBSTANCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcAmountOfSubstanceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_AMOUNTOFSUBSTANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ANGULAR_VELOCITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcAngularVelocityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ANGULARVELOCITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_AREA_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcAreaMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_AREAUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_BOOLEAN) {
			auto element = static_cast<const IfcSchema::IfcBoolean *>(ifcValue);
			value = element ? std::to_string((bool)*element) : value;
		}
		else if (typeName == IFC_TYPE_COMPLEX_NUMBER) {
			auto element = static_cast<const IfcSchema::IfcComplexNumber *>(ifcValue);
			if (element) {
				std::vector<double> num = *element;
				value = std::to_string(num[0]) + "+" + std::to_string(num[1]) + "i";
			}
		}
		else if (typeName == IFC_TYPE_COMPOUND_PLANE_ANGLE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcCompoundPlaneAngleMeasure *>(ifcValue);
			if (element) {
				std::vector<int> num = *element;
				std::stringstream ss;
				ss << "[";
				bool first = true;
				for (const auto &i : num) {
					if (!first) {
						ss << ", ";
					}
					ss << i;
					first = false;
				}
				ss << "]";
				value = ss.str();
			}
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_PLANEANGLEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_CONTEXT_DEPENDENT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcContextDependentMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_COUNT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcCountMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_CURVATURE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcCurvatureMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_CURVATUREUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_DESCRIPTIVE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcDescriptiveMeasure *>(ifcValue);
			value = element ? (std::string)*element : value;
		}
		else if (typeName == IFC_TYPE_DOSE_EQUIVALENT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcDoseEquivalentMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_DOSEEQUIVALENTUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_DYNAMIC_VISCOSITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcDynamicViscosityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_DYNAMICVISCOSITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ELECTRIC_CAPACITANCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcElectricCapacitanceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICCAPACITANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ELECTRIC_CHARGE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcElectricChargeMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICCHARGEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ELECTRIC_CONDUCTANCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcElectricConductanceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICCONDUCTANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ELECTRIC_CURRENT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcElectricCurrentMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICCURRENTUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ELECTRIC_RESISTANCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcElectricResistanceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICRESISTANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ELECTRIC_VOLTAGE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcElectricVoltageMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ELECTRICVOLTAGEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ENERGY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcEnergyMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ENERGYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_FORCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcForceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_FORCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_FREQUENCY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcFrequencyMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_FREQUENCYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_HEAT_FLUX_DENSITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcHeatFluxDensityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_HEATFLUXDENSITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_HEATING_VALUE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcHeatingValueMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_HEATINGVALUEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_IDENTIFIER) {
			auto element = static_cast<const IfcSchema::IfcIdentifier *>(ifcValue);
			if (element) {
				value = *element;
			}
		}
		else if (typeName == IFC_TYPE_ILLUMINANCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcIlluminanceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_ILLUMINANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_INDUCTANCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcInductanceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_INDUCTANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_INTEGER) {
			auto element = static_cast<const IfcSchema::IfcInteger *>(ifcValue);
			value = element ? std::to_string((int)*element) : value;
		}
		else if (typeName == IFC_TYPE_INTEGER_COUNT_RATE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcIntegerCountRateMeasure *>(ifcValue);
			value = element ? std::to_string((int)*element) : value;
		}
		else if (typeName == IFC_TYPE_ION_CONCENTRATION_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcIonConcentrationMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_ISOTHERMAL_MOISTURE_CAPACITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcIsothermalMoistureCapacityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ISOTHERMALMOISTURECAPACITYUNIT, projectUnits);
		}

		else if (typeName == IFC_TYPE_ISOTHERMAL_MOISTURE_CAPACITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcIsothermalMoistureCapacityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ISOTHERMALMOISTURECAPACITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_KINEMATIC_VISCOSITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcKinematicViscosityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_KINEMATICVISCOSITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_LABEL) {
			auto element = static_cast<const IfcSchema::IfcLabel *>(ifcValue);
			value = element ? (std::string)*element : value;
		}
		else if (typeName == IFC_TYPE_LENGTH_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcLengthMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LENGTHUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_LINEAR_FORCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcLinearForceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LINEARFORCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_LINEAR_MOMENT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcLinearMomentMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LINEARMOMENTUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_LINEAR_STIFFNESS_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcLinearStiffnessMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LINEARSTIFFNESSUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_LINEAR_VELOCITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcLinearVelocityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LINEARVELOCITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_LOGICAL) {
			auto element = static_cast<const IfcSchema::IfcLogical *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_LUMINOUS_FLUX_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcLuminousFluxMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LUMINOUSFLUXUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_LUMINOUS_INTENSITY_DISTRIBUTION_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcLuminousIntensityDistributionMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_LUMINOUSINTENSITYDISTRIBUTIONUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_LUMINOUS_INTENSITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcLuminousIntensityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LUMINOUSINTENSITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MAGNETIC_FLUX_DENSITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMagneticFluxDensityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MAGNETICFLUXDENSITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MAGNETIC_FLUX_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMagneticFluxMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MAGNETICFLUXUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MASS_DENSITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMassDensityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_MASS_FLOW_RATE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMassFlowRateMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_MASS_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMassMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MASSUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MASS_PER_LENGTH_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMassPerLengthMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MASSPERLENGTHUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MODULUS_OF_ELASTICITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcModulusOfElasticityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MODULUSOFELASTICITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MODULUS_OF_LINEAR_SUBGRADE_REACTION_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcModulusOfLinearSubgradeReactionMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MODULUSOFLINEARSUBGRADEREACTIONUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MODULUS_OF_ROTATIONAL_SUBGRADE_REACTION_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcModulusOfRotationalSubgradeReactionMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MODULUSOFROTATIONALSUBGRADEREACTIONUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MODULUS_OF_SUBGRADE_REACTION_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcModulusOfSubgradeReactionMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MODULUSOFSUBGRADEREACTIONUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MOISTURE_DIFFUSIVITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMoistureDiffusivityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MOISTUREDIFFUSIVITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MOLECULAR_WEIGHT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMolecularWeightMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MOLECULARWEIGHTUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MOMENT_OF_INERTIA_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMomentOfInertiaMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_MOMENTOFINERTIAUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_MONETARY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcMonetaryMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(MONETARY_UNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_NUMERIC_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcNumericMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_PH_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcPHMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_PARAMETER_VALUE) {
			auto element = static_cast<const IfcSchema::IfcParameterValue *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_PLANAR_FORCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcPlanarForceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_PLANARFORCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_PLANE_ANGLE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcPlaneAngleMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_PLANEANGLEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_POSITIVE_LENGTH_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcPositiveLengthMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LENGTHUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_POWER_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcPowerMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_POWERUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_PRESSURE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcPressureMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_PRESSUREUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_RADIO_ACTIVITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcRadioActivityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_RADIOACTIVITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_RATIO_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcRatioMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_REAL) {
			auto element = static_cast<const IfcSchema::IfcReal *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_ROTATIONAL_FREQUENCY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcRotationalFrequencyMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ROTATIONALFREQUENCYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ROTATIONAL_MASS_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcRotationalMassMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ROTATIONALMASSUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_ROTATIONAL_STIFFNESS_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcRotationalStiffnessMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_ROTATIONALSTIFFNESSUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_SECTION_MODULUS_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcSectionModulusMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SECTIONMODULUSUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_SECTIONAL_AREA_INTEGRAL_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcSectionalAreaIntegralMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SECTIONAREAINTEGRALUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_SHEAR_MODULUS_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcShearModulusMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SHEARMODULUSUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_SOLID_ANGLE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcSolidAngleMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_SOLIDANGLEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_SOUND_POWER_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcSoundPowerMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SOUNDPOWERUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_SOUND_PRESSURE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcSoundPressureMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SOUNDPRESSUREUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_SPECIFIC_HEAT_CAPACITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcSpecificHeatCapacityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_SPECIFICHEATCAPACITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_TEMPERATURE_GRADIENT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcTemperatureGradientMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_TEMPERATUREGRADIENTUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_TEXT) {
			auto element = static_cast<const IfcSchema::IfcText *>(ifcValue);
			value = element ? (std::string)*element : value;
		}
		else if (typeName == IFC_TYPE_THERMAL_ADMITTANCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcThermalAdmittanceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALADMITTANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_THERMAL_CONDUCTIVITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcThermalConductivityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALCONDUCTANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_THERMAL_EXPANSION_COEFFICIENT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcThermalExpansionCoefficientMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALEXPANSIONCOEFFICIENTUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_THERMAL_RESISTANCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcThermalResistanceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALRESISTANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_THERMAL_TRANSMITTANCE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcThermalTransmittanceMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_THERMALTRANSMITTANCEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_THERMODYNAMIC_TEMPERATURE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcThermodynamicTemperatureMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_THERMODYNAMICTEMPERATUREUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_TIME_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcTimeMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_TIMEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_TIME_STAMP) {
			auto element = static_cast<const IfcSchema::IfcTimeStamp *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_TORQUE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcTorqueMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_TORQUEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_VAPOR_PERMEABILITY_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcVaporPermeabilityMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_VAPORPERMEABILITYUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_VOLUME_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcVolumeMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_VOLUMEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_VOLUMETRIC_FLOW_RATE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcVolumetricFlowRateMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_VOLUMETRICFLOWRATEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_WARPING_CONSTANT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcWarpingConstantMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_WARPINGCONSTANTUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_WARPING_MOMENT_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcWarpingMomentMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcDerivedUnitEnum::IfcDerivedUnitEnum::IfcDerivedUnit_WARPINGMOMENTUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_POSITIVE_PLANE_ANGLE_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcPositivePlaneAngleMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
			units = getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_PLANEANGLEUNIT, projectUnits);
		}
		else if (typeName == IFC_TYPE_NORMALISED_RATIO_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcNormalisedRatioMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
		else if (typeName == IFC_TYPE_POSITIVE_RATIO_MEASURE) {
			auto element = static_cast<const IfcSchema::IfcPositiveRatioMeasure *>(ifcValue);
			value = element ? std::to_string((double)*element) : value;
		}
	}

	return value;
}

#if DEFINED_Ifc2x3
std::string ifcDateToString(const IfcSchema::IfcCalendarDate *date) {
	std::stringstream ss;
	ss << date->YearComponent() << "-" << date->MonthComponent() << "-" << date->DayComponent();
	return ss.str();
}
#endif

void  repo::ifcUtility::SCHEMA_NS::TreeParser::generateClassificationInformation(
	const IfcSchema::IfcRelAssociatesClassification * &relCS,
	std::unordered_map<std::string, std::string>         &metaValues

) {
	//TODO: add implementation for Ifc4 once we have a sample to work with
#if DEFINED_Ifc2x3
	auto relatedClassification = relCS->RelatingClassification();
	if (relatedClassification)
	{
		//A classifcation can either be a classification notation or reference
		if (relatedClassification->data().type()->name_lc() == IFC_TYPE_CLASSIFICATION_NOTATION)
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
				int editionIdx = -1;
				auto eleEntity = refSource->declaration().as_entity();
				for (int i = 0; i < refSource->data().getArgumentCount(); ++i) {
					if (eleEntity->attribute_by_index(i)->name() == "Edition") {
						editionIdx = i;
					}
				}

				if (editionIdx >= 0 && !refSource->data().getArgument(editionIdx)->isNull()) {
					metaValues[constructMetadataLabel("Edition", classificationName)] = refSource->Edition();
				}

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
#endif
}