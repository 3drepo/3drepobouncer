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
	std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials
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
		repoUUID parentID;
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
			auto childTransNodes = createTransformationsRecursive(ifcfile, element, meshes, materials, parentID);
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
	const repoUUID															   &parentID
	)
{
	switch (element->type())
	{
		//TODO: Need to filter out unwanted IFC nodes
	}

	//create a transformation for the entry, and loop through it's children
	auto name = element->entity->toString();
	auto id = element->entity->id();
	std::string guid;
	for (int i = 0; i < element->getArgumentCount(); ++i)
	{
		if (element->getArgumentName(i) == IFC_ARGUMENT_GLOBAL_ID)
			guid = element->getArgument(i)->toString();
	}

	std::vector<repoUUID> parents;
	if (!parentID.is_nil()) parents.push_back(parentID);

	auto transNode = repo::core::model::RepoBSONFactory::makeTransformationNode(repo::core::model::TransformationNode::identityMat(), name, parents);
	auto transID = transNode.getSharedID();

	repo::core::model::RepoNodeSet transNodeSet;
	transNodeSet.insert(new repo::core::model::TransformationNode(transNode));

	if (meshes.find(guid) != meshes.end())
	{
		//has meshes associated with it. append parent
		for (auto &mesh : meshes[guid])
		{
			*mesh = mesh->cloneAndAddParent(transID);
		}
	}

	auto childrenElements = ifcfile.entitiesByReference(id);
	repoTrace << "My id is: " << id << " name: " << name;
	repoTrace << "Getting childnre. children size: " << childrenElements->size();
	for (auto &element : *childrenElements)
	{
		repoTrace << "Looping children...";
		repoTrace << "Recursing";
		auto childTransNodes = createTransformationsRecursive(ifcfile, element, meshes, materials, transID);
		transNodeSet.insert(childTransNodes.begin(), childTransNodes.end());
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
	repo::core::model::RepoNodeSet dummy, meshSet, matSet;
	repo::core::model::RepoNodeSet transNodes = createTransformations(ifcfile, meshes, materials);
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
	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene(files, dummy, meshSet, matSet, dummy, dummy, transNodes);
	scene->setWorldOffset(offset);

	return scene;
}