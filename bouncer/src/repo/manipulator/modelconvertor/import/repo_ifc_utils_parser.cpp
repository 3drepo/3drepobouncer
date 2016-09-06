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

#include <ifcparse/IfcParse.h>
#include <ifcparse/IfcFile.h>

const static std::string IFC_ARGUMENT_GLOBAL_ID = "GlobalId";
const static std::string IFC_ARGUMENT_NAME = "Name";
const static std::string REPO_LABEL_IFC_TYPE = "IFC Type";
const static std::string REPO_LABEL_IFC_GUID = "IFC GUID";

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
		for (auto &it = initialElements->begin(); initialElements->end() != it; ++it)
		{
			auto element = *it;
			std::pair<std::vector<std::string>, std::vector<std::string>>               metaValue({ std::vector<std::string>(), std::vector<std::string>() });
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
			if (name == "$")
			{
				createElement = false;
			}
			else {
				name = name.erase(0, 1);;
				name = name.erase(name.size() - 1, 1);
				if (name.empty())
				{
					name = "<no name>";
				}
			}
		}
	}
	std::string ifcType = element->entity->datatype();

	//repoTrace << "My id is: " << id << " name: " << name << " type:" << element->entity->datatype();
	switch (element->type())
	{
		//FIXME: need to relay these information
	case Ifc2x3::Type::IfcProject:
	case Ifc2x3::Type::IfcRelDefinesByProperties:
	case Ifc2x3::Type::IfcRelAssignsToGroup: //This is group!
	case Ifc2x3::Type::IfcRelSpaceBoundary: //This is group?
		createElement = false;
		traverseChildren = false;
		break;
	case Ifc2x3::Type::IfcRelAssociatesClassification:
	{
		auto relCS = static_cast<const IfcSchema::IfcRelAssociatesClassification *>(element);
		createElement = false;
		traverseChildren = false;
		if (relCS)
		{
			metaValue.first.push_back(name);
			std::string classValue;
			try
			{
				auto relatedClassification = relCS->RelatingClassification();
				if (relatedClassification)
				{
					//A classifcation can either be a classification notation or reference
					if (relatedClassification->type() == Ifc2x3::Type::IfcClassificationNotation)
					{
						auto notation = static_cast<const IfcSchema::IfcClassificationNotation *>(relatedClassification);
						auto facets = notation->NotationFacets();
						for (Ifc2x3::IfcClassificationNotationFacet *facet : *facets)
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
		}
		else
		{
			repoError << "Failed to convert a IfcRelContainedInSpatialStructure element into a IfcRelContainedInSpatialStructureclass.";
		}
		break;
	}
	case Ifc2x3::Type::IfcRelContainedInSpatialStructure:
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
	case Ifc2x3::Type::IfcRelAggregates:
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
	case Ifc2x3::Type::IfcSpace:
		isIFCSpace = true;
		break;
	}

	repoUUID transID = parentID;
	repo::core::model::RepoNodeSet transNodeSet;
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

	if (createElement)
	{
		myMetaValues.first.insert(myMetaValues.first.begin(), REPO_LABEL_IFC_TYPE);
		myMetaValues.second.insert(myMetaValues.second.begin(), ifcType);
		myMetaValues.first.insert(myMetaValues.first.begin(), REPO_LABEL_IFC_GUID);
		myMetaValues.second.insert(myMetaValues.second.begin(), guid);

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