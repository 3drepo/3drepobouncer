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
#include <boost/filesystem.hpp>
#include <algorithm>

#include <ifcparse/IfcParse.h>
#include <ifcparse/IfcFile.h>

const static std::string IFC_ARGUMENT_GLOBAL_ID = "GlobalId";
const static std::string IFC_ARGUMENT_NAME = "Name";
const static std::string REPO_LABEL_IFC_TYPE = "IFC Type";
const static std::string REPO_LABEL_IFC_GUID = "IFC GUID";
const static std::string IFC_TYPE_SPACE_LABEL = "(IFC Space)";

using namespace repo::manipulator::modelconvertor;

IFCUtilsParser::IFCUtilsParser(const std::string &file) :
file(file)
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
	auto initialElements = ifcfile.entitiesByType(IfcSchema::Type::IfcSite);
	if (!initialElements->size())
	{
		repoTrace << "Could not find IFC Site tag... trying IfcBuilding";
		//If there is no site, get the buildings
		initialElements = ifcfile.entitiesByType(IfcSchema::Type::IfcBuilding);
	}

	repo::core::model::RepoNodeSet transNodes;
	if (initialElements->size())
	{
		repoTrace << "Initial elements: " << initialElements->size();
		repoUUID parentID = stringToUUID(REPO_HISTORY_MASTER_BRANCH);
		if (initialElements->size() > 1)
		{
			//More than one starting nodes. create a rootNode
			auto root = repo::core::model::RepoBSONFactory::makeTransformationNode();
			transNodes.insert(new repo::core::model::TransformationNode(root.cloneAndChangeName("<root>")));
		}

		repoTrace << "Looping through Elements";
		for (auto &element : *initialElements)
		{
			//auto element = *it;
			std::pair<std::vector<std::string>, std::vector<std::string>> metaValue({ std::vector<std::string>(), std::vector<std::string>() });
			auto childTransNodes = createTransformationsRecursive(ifcfile, element, meshes, materials, metaSet, metaValue, parentID);
			transNodes.insert(childTransNodes.begin(), childTransNodes.end());
		}
	}
	else //if (initialElements.size())
	{
		repoError << "Failed to generate IFC Tree: could not find a starting node (IFCSite, IFCBuilding)";
	}

	return transNodes;
}

repo::core::model::RepoNodeSet IFCUtilsParser::createTransformationsRecursive(
	IfcParse::IfcFile &ifcfile,
	const IfcUtil::IfcBaseClass *element,
	std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
	std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials,
	repo::core::model::RepoNodeSet											   &metaSet,
	std::pair<std::vector<std::string>, std::vector<std::string>>               &metaValue,
	const repoUUID															   &parentID,
	const std::set<int>													       &ancestorsID
	)
{
	bool createElement = true; //create a transformation for this element
	bool traverseChildren = true; //keep recursing its children
	bool isIFCSpace = false;
	std::vector<int> extraChildren; //children outside of reference
	std::pair<std::vector<std::string>, std::vector<std::string>> myMetaValues({ std::vector<std::string>(), std::vector<std::string>() });

	auto id = element->entity->id();
	std::string guid, name;
	std::string ifcType = element->entity->datatype();
	std::string ifcTypeUpper = ifcType;
	std::transform(ifcType.begin(), ifcType.end(), ifcTypeUpper.begin(), ::toupper);
	createElement = ifcTypeUpper.find("IFCREL") == std::string::npos;

	for (int i = 0; i < element->getArgumentCount(); ++i)
	{
		if (element->getArgumentName(i) == IFC_ARGUMENT_GLOBAL_ID)
		{
			//It comes with single quotes. remove those
			guid = element->getArgument(i)->toString().erase(0, 1);
			guid = guid.erase(guid.size() - 1, 1);
		}

		if (element->getArgumentName(i) == IFC_ARGUMENT_NAME)
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
		}
	}

	determineActionsByElementType(ifcfile, element, myMetaValues, createElement, traverseChildren, isIFCSpace, extraChildren);

	repoUUID transID = parentID;
	repo::core::model::RepoNodeSet transNodeSet;

	if (isIFCSpace) name += " " + IFC_TYPE_SPACE_LABEL;

	if (createElement)
	{
		std::vector<repoUUID> parents;
		if (UUIDtoString(parentID) != REPO_HISTORY_MASTER_BRANCH) parents.push_back(parentID);

		auto transNode = repo::core::model::RepoBSONFactory::makeTransformationNode(repo::core::model::TransformationNode::identityMat(), name, parents);
		transID = transNode.getSharedID();

		transNodeSet.insert(new repo::core::model::TransformationNode(transNode));

		if (meshes.find(guid) != meshes.end())
		{
			//has meshes associated with it. append parent
			for (auto &mesh : meshes[guid])
			{
				*mesh = mesh->cloneAndAddParent(transID);
				if (isIFCSpace)
					*mesh = mesh->cloneAndChangeName(name);
			}
		}
	}

	if (traverseChildren)
	{
		auto childrenElements = ifcfile.entitiesByReference(id);
		std::set<int> childrenAncestors(ancestorsID);
		childrenAncestors.insert(id);
		if (childrenElements)
		{
			for (auto &element : *childrenElements)
			{
				if (ancestorsID.find(element->entity->id()) == ancestorsID.end())
				{
					auto childTransNodes = createTransformationsRecursive(ifcfile, element, meshes, materials, metaSet, myMetaValues, transID, childrenAncestors);
					transNodeSet.insert(childTransNodes.begin(), childTransNodes.end());
				}
			}
		}

		for (const auto &childrenId : extraChildren)
		{
			if (ancestorsID.find(childrenId) == ancestorsID.end())
			{
				auto childTransNodes = createTransformationsRecursive(ifcfile, ifcfile.entityById(childrenId), meshes, materials, metaSet, myMetaValues, transID, childrenAncestors);
				transNodeSet.insert(childTransNodes.begin(), childTransNodes.end());
			}
		}
	}

	//If we created an element, add a metanode to it. if not, add them to our parent's meta info
	if (createElement)
	{
		myMetaValues.first.insert(myMetaValues.first.begin(), REPO_LABEL_IFC_GUID);
		myMetaValues.second.insert(myMetaValues.second.begin(), guid);
		myMetaValues.first.insert(myMetaValues.first.begin(), REPO_LABEL_IFC_TYPE);
		myMetaValues.second.insert(myMetaValues.second.begin(), ifcType);

		metaSet.insert(new repo::core::model::MetadataNode(repo::core::model::RepoBSONFactory::makeMetaDataNode(myMetaValues.first, myMetaValues.second, name, { transID })));
	}
	else
	{
		metaValue.first.insert(metaValue.first.end(), myMetaValues.first.begin(), myMetaValues.first.end());
		metaValue.second.insert(metaValue.second.end(), myMetaValues.second.begin(), myMetaValues.second.end());
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

	return scene;
}

void IFCUtilsParser::determineActionsByElementType(
	IfcParse::IfcFile &ifcfile,
	const IfcUtil::IfcBaseClass *element,
	std::pair<std::vector<std::string>, std::vector<std::string>> &metaValues,
	bool                                                          &createElement,
	bool                                                          &traverseChildren,
	bool                                                          &isIFCSpace,
	std::vector<int>                                              &extraChildren)
{
	switch (element->type())
	{
	case IfcSchema::Type::IfcRelAssignsToGroup: //This is group!
	case IfcSchema::Type::IfcRelSpaceBoundary: //This is group?
	case IfcSchema::Type::IfcElementQuantity:
	case IfcSchema::Type::IfcRelConnectsPathElements:
	case IfcSchema::Type::IfcAnnotation:
		createElement = false;
		traverseChildren = false;
		break;

	case IfcSchema::Type::IfcProject:
	{
		auto project = static_cast<const IfcSchema::IfcProject *>(element);
		if (project)
		{
			if (project->hasName())
			{
				metaValues.first.push_back("Project Name");
				metaValues.second.push_back(project->Name());
			}
			if (project->hasDescription())
			{
				metaValues.first.push_back("Project Description");
				metaValues.second.push_back(project->Description());
			}
		}

		createElement = false;
		traverseChildren = false;
		break;
	}
	case IfcSchema::Type::IfcPropertySingleValue:
	{
		auto propVal = static_cast<const IfcSchema::IfcPropertySingleValue *>(element);
		if (propVal)
		{
			metaValues.first.push_back(propVal->Name());

			std::string value = "n/a";
			if (propVal->hasNominalValue())
			{
				value = getValueAsString(propVal->NominalValue());
			}

			if (propVal->hasUnit())
			{
				repoTrace << propVal->entity->toString();
				repoTrace << "Property has units - We are currently not supporting this.";
				//FIXME: units
				auto units = propVal->Unit();
			}

			metaValues.second.push_back(value);
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
			extraChildren.push_back(defByProp->RelatingPropertyDefinition()->entity->id());
		}
		createElement = false;
		break;
	}
	case IfcSchema::Type::IfcPropertySet:
	{
		auto propSet = static_cast<const IfcSchema::IfcPropertySet *>(element);
		auto propDefs = propSet->HasProperties();

		for (auto &pd : *propDefs)
		{
			extraChildren.push_back(pd->entity->id());
		}
		createElement = false;
		break;
	}
	case IfcSchema::Type::IfcRelAssociatesClassification:
	{
		auto relCS = static_cast<const IfcSchema::IfcRelAssociatesClassification *>(element);
		createElement = false;
		traverseChildren = false;
		if (relCS)
		{
			metaValues.first.push_back(relCS->Name());
			std::string classValue;
			try
			{
				auto relatedClassification = relCS->RelatingClassification();
				if (relatedClassification)
				{
					//A classifcation can either be a classification notation or reference
					if (relatedClassification->type() == IfcSchema::Type::IfcClassificationNotation)
					{
						auto notation = static_cast<const IfcSchema::IfcClassificationNotation *>(relatedClassification);
						auto facets = notation->NotationFacets();
						for (IfcSchema::IfcClassificationNotationFacet *facet : *facets)
						{
							if (!classValue.empty()) classValue += ", ";
							classValue += facet->NotationValue();
						}
					}
					else
					{
						/*
						* A classification reference can hold classificationReferenceSelect again so this can be quite a bit more complicated
						* Not sure how far into it do we want to delve into
						*/
						auto reference = static_cast<const IfcSchema::IfcClassificationReference *>(relatedClassification);
						classValue = reference->Name();

						if (reference->hasLocation())
							classValue += " [ " + reference->Location() + " ]";
					}
				}
				else
				{
					repoError << "Nullptr to relatedClassification!!!";
				}
			}
			catch (...)
			{
				//This will throw an exception if there is no relating classification.
				classValue = "n/a";
			}
			metaValues.second.push_back(classValue);
		}
		else
		{
			repoError << "Failed to convert a IfcRelContainedInSpatialStructure element into a IfcRelContainedInSpatialStructureclass.";
		}
		break;
	}
	case IfcSchema::Type::IfcRelContainedInSpatialStructure:
	{
		auto relCS = static_cast<const IfcSchema::IfcRelContainedInSpatialStructure *>(element);
		createElement = false;
		if (relCS)
		{
			auto relatedObjects = relCS->RelatedElements();
			if (relatedObjects)
			{
				for (auto &e : *relatedObjects)
				{
					extraChildren.push_back(e->entity->id());
				}
			}
			else
			{
				repoError << "Nullptr to relatedObjects!!!";
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
				for (auto &e : *relatedObjects)
				{
					extraChildren.push_back(e->entity->id());
				}
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
	case IfcSchema::Type::IfcSpace:
		isIFCSpace = true;
		break;
	}
}

std::string IFCUtilsParser::getValueAsString(
	const IfcSchema::IfcValue    *ifcValue)
{
	std::string value = "n/a";
	//FIXME: should change this on IFCOpenShell to be an inheritance with a toString() function - but this is easier said than done due ot crazy inheritance (or lack of ) in the code
	if (ifcValue)
	{
		switch (ifcValue->type())
		{
		case IfcSchema::Type::IfcAbsorbedDoseMeasure:
			value = static_cast<const IfcSchema::IfcAbsorbedDoseMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcAccelerationMeasure:
			value = static_cast<const IfcSchema::IfcAccelerationMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcAmountOfSubstanceMeasure:
			value = static_cast<const IfcSchema::IfcAmountOfSubstanceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcAngularVelocityMeasure:
			value = static_cast<const IfcSchema::IfcAngularVelocityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcAreaMeasure:
			value = static_cast<const IfcSchema::IfcAreaMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcBoolean:
			value = static_cast<const IfcSchema::IfcBoolean *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcComplexNumber:
			value = static_cast<const IfcSchema::IfcComplexNumber *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcCompoundPlaneAngleMeasure:
			value = static_cast<const IfcSchema::IfcCompoundPlaneAngleMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcContextDependentMeasure:
			value = static_cast<const IfcSchema::IfcContextDependentMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcCountMeasure:
			value = static_cast<const IfcSchema::IfcCountMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcCurvatureMeasure:
			value = static_cast<const IfcSchema::IfcCurvatureMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcDescriptiveMeasure:
			value = static_cast<const IfcSchema::IfcDescriptiveMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcDoseEquivalentMeasure:
			value = static_cast<const IfcSchema::IfcDoseEquivalentMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcDynamicViscosityMeasure:
			value = static_cast<const IfcSchema::IfcDynamicViscosityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcElectricCapacitanceMeasure:
			value = static_cast<const IfcSchema::IfcElectricCapacitanceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcElectricChargeMeasure:
			value = static_cast<const IfcSchema::IfcElectricChargeMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcElectricConductanceMeasure:
			value = static_cast<const IfcSchema::IfcElectricConductanceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcElectricCurrentMeasure:
			value = static_cast<const IfcSchema::IfcElectricCurrentMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcElectricResistanceMeasure:
			value = static_cast<const IfcSchema::IfcElectricResistanceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcElectricVoltageMeasure:
			value = static_cast<const IfcSchema::IfcElectricVoltageMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcEnergyMeasure:
			value = static_cast<const IfcSchema::IfcEnergyMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcForceMeasure:
			value = static_cast<const IfcSchema::IfcForceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcFrequencyMeasure:
			value = static_cast<const IfcSchema::IfcFrequencyMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcHeatFluxDensityMeasure:
			value = static_cast<const IfcSchema::IfcHeatFluxDensityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcHeatingValueMeasure:
			value = static_cast<const IfcSchema::IfcHeatingValueMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcIdentifier:
			value = static_cast<const IfcSchema::IfcIdentifier *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcIlluminanceMeasure:
			value = static_cast<const IfcSchema::IfcIlluminanceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcInductanceMeasure:
			value = static_cast<const IfcSchema::IfcInductanceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcInteger:
			value = static_cast<const IfcSchema::IfcInteger *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcIntegerCountRateMeasure:
			value = static_cast<const IfcSchema::IfcIntegerCountRateMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcIonConcentrationMeasure:
			value = static_cast<const IfcSchema::IfcIonConcentrationMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcIsothermalMoistureCapacityMeasure:
			value = static_cast<const IfcSchema::IfcIsothermalMoistureCapacityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcKinematicViscosityMeasure:
			value = static_cast<const IfcSchema::IfcKinematicViscosityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLabel:
			value = static_cast<const IfcSchema::IfcLabel *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLengthMeasure:
			value = static_cast<const IfcSchema::IfcLengthMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLinearForceMeasure:
			value = static_cast<const IfcSchema::IfcLinearForceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLinearMomentMeasure:
			value = static_cast<const IfcSchema::IfcLinearMomentMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLinearStiffnessMeasure:
			value = static_cast<const IfcSchema::IfcLinearStiffnessMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLinearVelocityMeasure:
			value = static_cast<const IfcSchema::IfcLinearVelocityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLogical:
			value = static_cast<const IfcSchema::IfcLogical *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLuminousFluxMeasure:
			value = static_cast<const IfcSchema::IfcLuminousFluxMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLuminousIntensityDistributionMeasure:
			value = static_cast<const IfcSchema::IfcLuminousIntensityDistributionMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcLuminousIntensityMeasure:
			value = static_cast<const IfcSchema::IfcLuminousIntensityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMagneticFluxDensityMeasure:
			value = static_cast<const IfcSchema::IfcMagneticFluxDensityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMagneticFluxMeasure:
			value = static_cast<const IfcSchema::IfcMagneticFluxMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMassDensityMeasure:
			value = static_cast<const IfcSchema::IfcMassDensityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMassFlowRateMeasure:
			value = static_cast<const IfcSchema::IfcMassFlowRateMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMassMeasure:
			value = static_cast<const IfcSchema::IfcMassMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMassPerLengthMeasure:
			value = static_cast<const IfcSchema::IfcMassPerLengthMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcModulusOfElasticityMeasure:
			value = static_cast<const IfcSchema::IfcModulusOfElasticityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcModulusOfLinearSubgradeReactionMeasure:
			value = static_cast<const IfcSchema::IfcModulusOfLinearSubgradeReactionMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcModulusOfRotationalSubgradeReactionMeasure:
			value = static_cast<const IfcSchema::IfcModulusOfRotationalSubgradeReactionMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcModulusOfSubgradeReactionMeasure:
			value = static_cast<const IfcSchema::IfcModulusOfSubgradeReactionMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMoistureDiffusivityMeasure:
			value = static_cast<const IfcSchema::IfcMoistureDiffusivityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMolecularWeightMeasure:
			value = static_cast<const IfcSchema::IfcMolecularWeightMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMomentOfInertiaMeasure:
			value = static_cast<const IfcSchema::IfcMomentOfInertiaMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcMonetaryMeasure:
			value = static_cast<const IfcSchema::IfcMonetaryMeasure *>(ifcValue)->valueAsString();
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
			break;
		case IfcSchema::Type::IfcPlaneAngleMeasure:
			value = static_cast<const IfcSchema::IfcPlaneAngleMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcPositiveLengthMeasure:
			value = static_cast<const IfcSchema::IfcPositiveLengthMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcPowerMeasure:
			value = static_cast<const IfcSchema::IfcPowerMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcPressureMeasure:
			value = static_cast<const IfcSchema::IfcPressureMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcRadioActivityMeasure:
			value = static_cast<const IfcSchema::IfcRadioActivityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcRatioMeasure:
			value = static_cast<const IfcSchema::IfcRatioMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcReal:
			value = static_cast<const IfcSchema::IfcReal *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcRotationalFrequencyMeasure:
			value = static_cast<const IfcSchema::IfcRotationalFrequencyMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcRotationalMassMeasure:
			value = static_cast<const IfcSchema::IfcRotationalMassMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcRotationalStiffnessMeasure:
			value = static_cast<const IfcSchema::IfcRotationalStiffnessMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcSectionModulusMeasure:
			value = static_cast<const IfcSchema::IfcSectionModulusMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcSectionalAreaIntegralMeasure:
			value = static_cast<const IfcSchema::IfcSectionalAreaIntegralMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcShearModulusMeasure:
			value = static_cast<const IfcSchema::IfcShearModulusMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcSolidAngleMeasure:
			value = static_cast<const IfcSchema::IfcSolidAngleMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcSoundPowerMeasure:
			value = static_cast<const IfcSchema::IfcSoundPowerMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcSoundPressureMeasure:
			value = static_cast<const IfcSchema::IfcSoundPressureMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcSpecificHeatCapacityMeasure:
			value = static_cast<const IfcSchema::IfcSpecificHeatCapacityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcTemperatureGradientMeasure:
			value = static_cast<const IfcSchema::IfcTemperatureGradientMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcText:
			value = static_cast<const IfcSchema::IfcText *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcThermalAdmittanceMeasure:
			value = static_cast<const IfcSchema::IfcThermalAdmittanceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcThermalConductivityMeasure:
			value = static_cast<const IfcSchema::IfcThermalConductivityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcThermalExpansionCoefficientMeasure:
			value = static_cast<const IfcSchema::IfcThermalExpansionCoefficientMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcThermalResistanceMeasure:
			value = static_cast<const IfcSchema::IfcThermalResistanceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcThermalTransmittanceMeasure:
			value = static_cast<const IfcSchema::IfcThermalTransmittanceMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcThermodynamicTemperatureMeasure:
			value = static_cast<const IfcSchema::IfcThermodynamicTemperatureMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcTimeMeasure:
			value = static_cast<const IfcSchema::IfcTimeMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcTimeStamp:
			value = static_cast<const IfcSchema::IfcTimeStamp *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcTorqueMeasure:
			value = static_cast<const IfcSchema::IfcTorqueMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcVaporPermeabilityMeasure:
			value = static_cast<const IfcSchema::IfcVaporPermeabilityMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcVolumeMeasure:
			value = static_cast<const IfcSchema::IfcVolumeMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcVolumetricFlowRateMeasure:
			value = static_cast<const IfcSchema::IfcVolumetricFlowRateMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcWarpingConstantMeasure:
			value = static_cast<const IfcSchema::IfcWarpingConstantMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcWarpingMomentMeasure:
			value = static_cast<const IfcSchema::IfcWarpingMomentMeasure *>(ifcValue)->valueAsString();
			break;
		case IfcSchema::Type::IfcPositivePlaneAngleMeasure:
			value = static_cast<const IfcSchema::IfcPositivePlaneAngleMeasure *>(ifcValue)->valueAsString();
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