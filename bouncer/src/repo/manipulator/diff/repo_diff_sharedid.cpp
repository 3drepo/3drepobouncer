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

#include "repo_diff_sharedid.h"

using namespace repo::manipulator::diff;

DiffBySharedID::DiffBySharedID(
	const repo::core::model::RepoScene            *base,
	const repo::core::model::RepoScene            *compare,
	const repo::core::model::RepoScene::GraphType &gType) : AbstractDiff(base, compare, gType)
{
	ok = this->compare(msg);
}

DiffBySharedID::~DiffBySharedID()
{
}

bool DiffBySharedID::compare(
	std::string &msg)
{
	bool res = false;
	if (baseScene && compareScene && baseScene->hasRoot(gType) && compareScene->hasRoot(gType))
	{
		std::set<repo::lib::RepoUUID> baseIDs = baseScene->getAllSharedIDs(gType);
		std::set<repo::lib::RepoUUID> compIDs = compareScene->getAllSharedIDs(gType);
		for (const repo::lib::RepoUUID &sharedID : baseIDs)
		{
			bool existInComp = compIDs.find(sharedID) != compIDs.end();

			if (existInComp)
			{
				//either equal or modified
				repo::core::model::RepoNode *baseNode = baseScene->getNodeBySharedID(gType, sharedID);
				repo::core::model::RepoNode *compNode = compareScene->getNodeBySharedID(gType, sharedID);

				auto corrIt = baseRes.correspondence.find(sharedID);

				if (corrIt == baseRes.correspondence.end())
				{
					baseRes.correspondence[sharedID] = sharedID;
					compRes.correspondence[sharedID] = sharedID;
				}
				else
				{
					repoLogError("Found multiple potential correspondence for " + sharedID.toString());
				}

				if (baseNode->getUniqueID() != compNode->getUniqueID())
				{
					//modified
					compRes.modified.push_back(sharedID);
					baseRes.modified.push_back(sharedID);
				}

				compIDs.erase(sharedID);
			}
			else
			{
				//doesn't exist - add to deleted
				baseRes.added.push_back(sharedID);
			}
		}

		//any remaining items within compIDs doesn't exist in base.
		compRes.added.insert(compRes.added.end(), compIDs.begin(), compIDs.end());
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