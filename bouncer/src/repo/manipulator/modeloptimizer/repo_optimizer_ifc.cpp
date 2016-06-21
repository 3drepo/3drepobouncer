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
* Abstract optimizer class
*/

#include "repo_optimizer_ifc.h"
#include <algorithm>

using namespace repo::manipulator::modeloptimizer;
const static std::string IFC_MAPPED_ITEM = "IFCMAPPEDITEM";

IFCOptimzer::IFCOptimzer()
{
}

IFCOptimzer::~IFCOptimzer()
{
}

bool IFCOptimzer::apply(repo::core::model::RepoScene *scene)
{
	bool success = false;
	std::unordered_map<std::string, std::vector<repo::core::model::RepoNode*>> transByName;
	//sort transformations by name

	auto tranNodes = scene->getAllTransformations(repo::core::model::RepoScene::GraphType::DEFAULT);
	for (auto &trans : tranNodes)
	{
		auto name = trans->getName();
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);
		if (transByName.find(name) == transByName.end())
		{
			transByName[name] = std::vector<repo::core::model::RepoNode*>();
		}

		transByName[name].push_back(trans);
	}

	success = optimizeIFCMappedItems(scene, transByName);
	return success;
}

bool IFCOptimzer::optimizeIFCMappedItems(
	repo::core::model::RepoScene *scene,
	const std::unordered_map<std::string, std::vector<repo::core::model::RepoNode*>>  &transByName)
{
	bool success = true;
	auto it = transByName.find(IFC_MAPPED_ITEM);
	repoTrace << "Optimising " << IFC_MAPPED_ITEM << "s...";
	auto defaultG = repo::core::model::RepoScene::GraphType::DEFAULT;
	if (it != transByName.end())
	{
		//There must be a sibling that is a metadata, with nodes as child
		auto nodes = it->second;
		for (auto &mappedItemNode : nodes)
		{
			auto parents = mappedItemNode->getParentIDs();
			auto children = scene->getChildrenAsNodes(defaultG, mappedItemNode->getSharedID());

			if (!(parents.size() && children.size()))
			{
				repoError << "Found a " << IFC_MAPPED_ITEM << " with no parents or children!";
				success = false;
				continue;
			}
			for (auto parent : parents)
			{
				auto parentNode = scene->getNodeBySharedID(defaultG, parent);
				if (parentNode)
				{
					auto metaVector = scene->getChildrenNodesFiltered(defaultG,
						parent, repo::core::model::NodeType::METADATA);

					//reparent its children to its parent
					//assign metadata to children
					scene->abandonChild(defaultG, parent, mappedItemNode);
					////FIXME: to remove
					//for (auto &meta : metaVector)
					//{
					//	scene->abandonChild(defaultG, parent, meta->getSharedID());
					//}

					for (auto &child : children)
					{
						auto childSharedID = child->getSharedID();
						scene->addInheritance(defaultG, parentNode, child);
						for (auto &meta : metaVector)
						{
							scene->addInheritance(defaultG, child, meta);
						}
					}
				}
				else
				{
					repoError << "Failed to find parent node with ID " << parent;
					success = false;
				}
			}

			//All nodes should be patched by now. delete the node
			scene->removeNode(defaultG, mappedItemNode->getSharedID());
		}
	}
	else
	{
		//No mapped items not an error.
		repoTrace << "No " << IFC_MAPPED_ITEM << " found. skipping...";
	}

	return success;
}