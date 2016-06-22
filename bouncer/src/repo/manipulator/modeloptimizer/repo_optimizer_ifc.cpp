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
#include "../../core/model/bson/repo_node_transformation.h"
#include <algorithm>

using namespace repo::manipulator::modeloptimizer;
const static std::string IFC_MAPPED_ITEM = "IFCMAPPEDITEM";
const static std::string IFC_RELVOID_ELE = "$RELVOIDSELEMENT";
const static std::string IFC_RELVOID_ELE2 = "IFCRELVOIDSELEMENT";

auto defaultG = repo::core::model::RepoScene::GraphType::DEFAULT;

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

	auto tranNodes = scene->getAllTransformations(defaultG);
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

	success = optimizeRelVoidElements(scene, transByName);
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

	if (it != transByName.end())
	{
		//There must be a sibling that is a metadata, with nodes as child
		auto nodes = it->second;
		for (auto &mappedItemNode : nodes)
		{
			auto parents = mappedItemNode->getParentIDs();
			auto children = scene->getChildrenAsNodes(defaultG, mappedItemNode->getSharedID());

			repo::core::model::TransformationNode *transNode = dynamic_cast<repo::core::model::TransformationNode*>(mappedItemNode);

			if (transNode)
			{
				if (!transNode->isIdentity())
				{
					repoError << IFC_MAPPED_ITEM << " "
						<< transNode->getUniqueID() << " is not an identity transformation - this is not expected!";
					success = false;
					continue;
				}
			}
			else
			{
				repoError << IFC_MAPPED_ITEM << " "
					<< transNode->getUniqueID() << " is not a transformation node - this is not expected!";
				success = false;
				continue;
			}

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
#ifdef DEBUG_ME
					for (auto &meta : metaVector)
					{
						scene->abandonChild(defaultG, parent, meta->getSharedID());
					}
#endif

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

bool IFCOptimzer::optimizeRelVoidElements(
	repo::core::model::RepoScene *scene,
	const std::unordered_map<std::string, std::vector<repo::core::model::RepoNode*>>  &transByName)
{
	bool success = true;

	auto it = transByName.find(IFC_RELVOID_ELE);
	auto it2 = transByName.find(IFC_RELVOID_ELE2);

	std::vector<repo::core::model::RepoNode*> relVoidArray;
	if (it != transByName.end())
	{
		relVoidArray = it->second;
	}

	if (it2 != transByName.end())
	{
		//Shouldn't exist, but just incase.
		relVoidArray.insert(relVoidArray.end(), it2->second.begin(), it2->second.end());
	}

	repoTrace << "Optimising " << IFC_RELVOID_ELE2 << "s...";

	for (auto &voidNode : relVoidArray)
	{
		/*
		* If Identity:
		*		- If there is metadata within that subbranch, shrink up the RelVoids and maintain the sub branch
		*		- If all there is is transformations within the subbranch, remove the whole branch
		* If not identity:
		*		- not expected to happen, but leave it as it is.
		*/

		auto transNode = dynamic_cast<repo::core::model::TransformationNode *>(voidNode);
		if (transNode->isIdentity())
		{
			std::vector<repoUUID> ids;
			auto parents = transNode->getParentIDs();

			//Check if all of its parents have a meta node as child, if it does, append the metadata to the meshes
			for (const auto &parent : parents)
			{
				auto metaVector = scene->getChildrenNodesFiltered(defaultG,
					parent, repo::core::model::NodeType::METADATA);
				auto meshVector = scene->getChildrenNodesFiltered(defaultG,
					parent, repo::core::model::NodeType::MESH);

				for (auto &mesh : meshVector)
				{
					for (auto &meta : metaVector)
					{
						scene->addInheritance(defaultG, mesh, meta);
#ifdef DEBUG_ME
						scene->abandonChild(defaultG, parent, meta);
#endif
					}
				}

				//remove linkage to parent
				scene->abandonChild(defaultG, parent, transNode, true, false);
			}

			if (isBranchMeaningless(scene, transNode, ids))
			{
				//Branch is meaningless, deleting the whole thing
				repoTrace << "Found meaningless " << IFC_RELVOID_ELE2 << " branch, removing...";
				for (const auto &id : ids)
					scene->removeNode(defaultG, id);
			}
			else
			{
				repoTrace << "Found " << IFC_RELVOID_ELE2 << " branch contains info, preserving...";
				auto children = scene->getChildrenAsNodes(defaultG, transNode->getSharedID());

				for (auto &child : children)
				{
					scene->abandonChild(defaultG, transNode->getSharedID(), child, false, true);
				}

				//remove itself and append child to parent
				for (const auto &parent : parents)
				{
					auto parentNode = scene->getNodeBySharedID(defaultG, parent);
					if (parentNode)
					{
						for (auto &child : children)
						{
							scene->addInheritance(defaultG, parentNode, child);
						}
					}
				}

				scene->removeNode(defaultG, transNode->getSharedID());
			}
		}
	}

	return success;
}

bool IFCOptimzer::isBranchMeaningless(
	const repo::core::model::RepoScene *scene,
	const repo::core::model::RepoNode  *node,
	std::vector<repoUUID>              &ids,
	const uint32_t                           &currentDepth,
	const uint32_t                           &maxDepth)
{
	if (currentDepth == maxDepth)
	{
		repoTrace << "Hit max depth";
		return false;
	}

	auto sharedID = node->getSharedID();
	auto children = scene->getChildrenAsNodes(defaultG, sharedID);

	ids.push_back(sharedID);
	for (const auto &child : children)
	{
		if (child->getTypeAsEnum() == repo::core::model::NodeType::TRANSFORMATION)
		{
			auto transChild = dynamic_cast<repo::core::model::TransformationNode*>(child);
			if (!isBranchMeaningless(scene, child, ids, currentDepth + 1, maxDepth))
			{
				repoTrace << "Child is meaningful";
				return false;
			}

			//ensure this transformation has no other parent
			if (transChild->getParentIDs().size() > 1)
			{
				repoTrace << "Child has more than one parent";
				return false;
			}
		}
		else
		{
			repoTrace << "Child[" << child->getUniqueID() << "] is not transformation";
			return false;
		}
	}

	return true;
}