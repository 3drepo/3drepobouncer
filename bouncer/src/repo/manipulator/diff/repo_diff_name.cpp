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


#include "repo_diff_name.h"

using namespace repo::manipulator::diff;

DiffByName::DiffByName(
	const repo::core::model::RepoScene *base,
	const repo::core::model::RepoScene *compare) 
	: AbstractDiff(base, compare)
	, errorReported(false)
{

	ok = this->compare(msg);
}


DiffByName::~DiffByName()
{
}

bool DiffByName::compare(
	std::string &msg)
{
	bool res = false;
	if (baseScene && compareScene && baseScene->hasRoot() && compareScene->hasRoot())
	{

		std::set<repoUUID> baseIDs = baseScene->getAllSharedIDs();
		std::set<repoUUID> compIDs = compareScene->getAllSharedIDs();
	
		//Compare meshes
		compareNodes(baseIDs, compIDs, baseScene->getAllMeshes(), compareScene->getAllMeshes());

		//Compare transformations
		compareNodes(baseIDs, compIDs, baseScene->getAllTransformations(), compareScene->getAllTransformations());

		//Compare cameras
		compareNodes(baseIDs, compIDs, baseScene->getAllCameras(), compareScene->getAllCameras());

		//Compare materials
		compareNodes(baseIDs, compIDs, baseScene->getAllMaterials(), compareScene->getAllMaterials());

		//Compare textures
		compareNodes(baseIDs, compIDs, baseScene->getAllTextures(), compareScene->getAllTextures());

		//Compare metadata
		compareNodes(baseIDs, compIDs, baseScene->getAllMetadata(), compareScene->getAllMetadata());

		//Compare referebces
		compareNodes(baseIDs, compIDs, baseScene->getAllReferences(), compareScene->getAllReferences());

		//Compare maps
		compareNodes(baseIDs, compIDs, baseScene->getAllMaps(), compareScene->getAllMaps());

		//NOTE: can't semantically compare unknowns, leave them for now.

		//any remaining items within compIDs doesn't exist in base.
		compRes.added.insert(compRes.added.end(), compIDs.begin(), compIDs.end());
		baseRes.added.insert(baseRes.added.end(), baseIDs.begin(), baseIDs.end());
		repoInfo << "Comparison completed: #nodes added: " << compRes.added.size() << " deleted: "
			<< baseRes.added.size() << " modified: " << baseRes.modified.size();
		res = true;
	}
	else
	{
		msg += "Cannot compare the scenes, null pointer to scene/graphs!";
	}

	return res;

}

void DiffByName::compareNodes(
	std::set<repoUUID> &baseIDs,
	std::set<repoUUID> &compIDs,
	const repo::core::model::RepoNodeSet &baseNodes,
	const repo::core::model::RepoNodeSet &compNodes)
{
	const std::unordered_map<std::string, repo::core::model::RepoNode*> baseNodeMap =
		createNodeMap(baseNodes);

	const std::unordered_map<std::string, repo::core::model::RepoNode*> compNodeMap =
		createNodeMap(compNodes);

	for (const auto pair: baseNodeMap)
	{
		//Try to find the same name in compNodeMap
		auto mapIt = compNodeMap.find(pair.first);
		repoUUID baseId = pair.second->getSharedID();
		if (mapIt != compNodeMap.end())
		{
			//found a name match
			repoUUID compId = mapIt->second->getSharedID();
			auto corrIt = baseRes.correspondence.find(baseId);

			if (corrIt == baseRes.correspondence.end())
			{
				baseRes.correspondence[baseId] = compId;
				compRes.correspondence[compId] = baseId;
			}
			else
			{
				repoLogError("Found multiple potential correspondence for " + UUIDtoString(baseId));
			}


			//Compare to see if it is modified


			if (!pair.second->sEqual(*mapIt->second))
			{
				//unmatch, implies modified
				baseRes.modified.push_back(baseId);
				compRes.modified.push_back(compId);
			}
			else

			compIDs.erase(compId);
		}
		else
		{

			baseRes.added.push_back(baseId);
		}
		baseIDs.erase(baseId);
	}
}

std::unordered_map<std::string, repo::core::model::RepoNode*> DiffByName::createNodeMap(
	const repo::core::model::RepoNodeSet &nodes)
{
	std::unordered_map<std::string, repo::core::model::RepoNode*> map;

	for (repo::core::model::RepoNode* node : nodes)
	{
		const std::string name = node->getName();
		auto mapIt = map.find(name);

		if (mapIt == map.end())
		{
			map[name] = node;
		}
		else if (!errorReported)
		{
			repoLogError("More than one node is named the same. Some nodes which may have correspondence might be presented as a mismatch.");
			errorReported = true;
		}
	}

	return map;
}