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
	const repo::core::model::RepoScene *base,
	const repo::core::model::RepoScene *compare) : AbstractDiff(base, compare)
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
	if (baseScene && compareScene && baseScene->hasRoot() && compareScene->hasRoot())
	{

		std::set<repoUUID> baseIDs = baseScene->getAllSharedIDs();
		std::set<repoUUID> compIDs = compareScene->getAllSharedIDs();
		for (const repoUUID &sharedID : baseIDs)
		{
			bool existInComp = compIDs.find(sharedID) != compIDs.end();

			if (existInComp)
			{
				//either equal or modified
				repo::core::model::RepoNode *baseNode = baseScene->getNodeBySharedID(sharedID);
				repo::core::model::RepoNode *compNode = compareScene->getNodeBySharedID(sharedID);

				if (baseNode->getUniqueID() != compNode->getUniqueID())
				{
					//modified
					modified.push_back(sharedID);
				}

				compIDs.erase(sharedID);
			}
			else
			{
				//doesn't exist - add to deleted
				deleted.push_back(sharedID);
			}			
		}

		//any remaining items within compIDs doesn't exist in base.
		added.insert(added.end(), compIDs.begin(), compIDs.end());
		repoInfo << "Comparison completed: #nodes added: " << added.size() << " deleted: " 
			<< deleted.size() << " modified: " << modified.size();
		res = true;
	}
	else
	{
		msg += "Cannot compare the scenes, null pointer to scene/graphs!";
	}

	return res;

}
