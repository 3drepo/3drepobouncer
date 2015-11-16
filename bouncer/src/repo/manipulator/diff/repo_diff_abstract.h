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


#pragma once

#include "../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace diff{
			class AbstractDiff
			{
			public:
				/**
				* Construct a diff comparator given the 2 scenes supplied
				* @param base base scene to compare from
				* @param compare scene to compare against
				*/
				AbstractDiff(
					const repo::core::model::RepoScene *base,
					const repo::core::model::RepoScene *compare);
				virtual ~AbstractDiff();

				/**
				* Return the sharedID of nodes which are added into the scene
				* @return return a vector of sharedIDs of nodes added
				*/
				std::vector<repoUUID> getNodesAdded()
				{
					return added;
				}

				/**
				* Return the sharedID of nodes which are deleted from the scene
				* @return return a vector of sharedIDs of nodes deleted
				*/
				std::vector<repoUUID> getNodesDeleted()
				{
					return deleted;
				}

				/**
				* Return the sharedID of nodes which are modified in scene
				* @return return a vector of sharedIDs of nodes modified
				*/
				std::vector<repoUUID> getNodesModified()
				{
					return modified;
				}

				/**
				* Check if comparator functioned fine
				* @param msg error message should boolean returns false
				* @return returns true if comparator operated successfully
				*/
				virtual bool isOk(std::string &msg) const = 0;


			protected:
				const repo::core::model::RepoScene *baseScene;
				const repo::core::model::RepoScene *compareScene;
				std::vector<repoUUID> deleted, modified, added;

			};
		}
	}
}



