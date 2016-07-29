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
#include "../../core/model/bson/repo_bson_factory.h"
#include "../../core/model/bson/repo_node_transformation.h"
#include <algorithm>

using namespace repo::manipulator::modeloptimizer;
const static std::string IFC_MAPPED_ITEM = "IFCMAPPEDITEM";
const static std::string IFC_RELVOID_ELE = "$RELVOIDSELEMENT";
const static std::string IFC_RELVOID_ELE2 = "IFCRELVOIDSELEMENT";
const static std::string IFC_RELAGGREGATES = "$RELAGGREGATES";

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

	std::vector<std::string> transToRemove = { IFC_MAPPED_ITEM, IFC_RELVOID_ELE, IFC_RELVOID_ELE2, IFC_RELAGGREGATES };
	success = removeTransformationsWithNames(scene, transByName, transToRemove);
	//TODO: group
	success &= sanitiseTransformationNames(scene);

	return success;
}

bool IFCOptimzer::removeTransformationsWithNames(
	repo::core::model::RepoScene *scene,
	const std::unordered_map<std::string, std::vector<repo::core::model::RepoNode*>>  &transByName,
	const std::vector<std::string> &names)
{
	bool success = true;
	for (const auto name : names)
	{
		auto it = transByName.find(name);
		if (it != transByName.end())
		{
			repoTrace << "Removing " << name << "s...";
			for (auto &node : it->second)
			{
				auto transNode = dynamic_cast<repo::core::model::TransformationNode *>(node);
				auto transSharedID = transNode->getSharedID();
				auto parents = transNode->getParentIDs();

				auto children = scene->getChildrenAsNodes(defaultG, transNode->getSharedID());
				auto trans = transNode->getTransMatrix(false);

				//Remove self from child
				for (auto &child : children)
				{
					if (child)
					{
						if (!transNode->isIdentity())
						{
							auto newChild = child->cloneAndApplyTransformation(trans);
							child->swap(newChild);
						}

						scene->abandonChild(defaultG, transSharedID, child, false, true);
					}
					else
					{
						repoError << "Child with nullpointer found";
						success = false;
					}
				}

				//Remove self from parent and  patch children to parents
				for (const auto &parent : parents)
				{
					auto parentNode = scene->getNodeBySharedID(defaultG, parent);

					if (parentNode)
					{
						//remove linkage to parent
						scene->abandonChild(defaultG, parent, transNode, true, false);
						for (auto &child : children)
						{
							scene->addInheritance(defaultG, parentNode, child);
						}
					}
					else
					{
						repoError << "Failed to find parent with id " << parent;
						success = false;
					}
				}

				scene->removeNode(defaultG, transSharedID);
			}
		}
	}
	return success;
}

bool IFCOptimzer::sanitiseTransformationNames(
	repo::core::model::RepoScene *scene)
{
	auto transformations = scene->getAllTransformations(defaultG);
	for (auto &trans : transformations)
	{
		//format of names: IFCTYPE_NAME_GUID
		auto name = trans->getName();
		auto posFirstUnderScore = name.find_first_of('_');
		auto posGuid = name.size() - 22;

		auto ifcType = name.substr(0, posFirstUnderScore);
		auto realName = name.substr(posFirstUnderScore + 1, posGuid - (posFirstUnderScore + 2));
		auto guid = name.substr(posGuid);

		auto newTrans = trans->cloneAndChangeName(realName, false);
		scene->modifyNode(defaultG, trans, &newTrans, true);

		repo::core::model::RepoBSON newMeta = BSON("IFC Type" << ifcType << "IFC GUID" << guid);

		//Check for metadata
		auto metas = scene->getChildrenNodesFiltered(defaultG, trans->getSharedID(), repo::core::model::NodeType::METADATA);
		if (metas.size())
		{
			//Metadata exists, append to them
			for (auto &meta : metas)
			{
				auto metaNode = dynamic_cast<repo::core::model::MetadataNode*>(meta);
				repo::core::model::RepoNode updatedMeta = metaNode->cloneAndAddMetadata(newMeta);
				scene->modifyNode(defaultG, meta, &updatedMeta, false);
			}
		}
		else
		{
			//no metadata, create one
			auto metaNode = new repo::core::model::MetadataNode(
				repo::core::model::RepoBSONFactory::makeMetaDataNode(newMeta, "", realName, { trans->getSharedID() }));
			std::vector<repo::core::model::RepoNode*> metaVec = { metaNode };
			scene->addNodes(metaVec);
		}
	}

	return true;
}