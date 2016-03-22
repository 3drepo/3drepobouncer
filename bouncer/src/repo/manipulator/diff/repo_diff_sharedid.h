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

/*
* Simple Diff Comparison - only compare sharedIDs and unique IDs
* base scene - base scene to compare from, compare scene - compare scene to compare to
* if shared ID from base scene exists in compare scene, check if unique ID is the same
* if same - same, if different - modified
* if shared ID doesn't exist, the node is deleted.
*/

#pragma once

#include "repo_diff_abstract.h"

namespace repo{
	namespace manipulator{
		namespace diff{
			class DiffBySharedID : public AbstractDiff
			{
			public:
				/**
				* Construct a diff comparator given the 2 scenes supplied
				* @param base base scene to compare from
				* @param compare scene to compare against
				* @param gType graph type to diff (default: unoptimised)
				*/
				DiffBySharedID(
					const repo::core::model::RepoScene *base,
					const repo::core::model::RepoScene *compare,
					const repo::core::model::RepoScene::GraphType &gType
					= repo::core::model::RepoScene::GraphType::DEFAULT);
				virtual ~DiffBySharedID();

				/**
				* Check if comparator functioned fine
				* @param msg error message should boolean returns false
				* @return returns true if comparator operated successfully
				*/
				virtual bool isOk(std::string &msg) const 
				{
					msg = this->msg;
					return ok; 
				};

			private:
				/**
				* Get the difference of the 2 graphs as a scene
				*/
				bool compare(
					std::string &msg);


				bool ok; //Check if comparator status is ok
				std::string msg; //error message if comaprator statis is false
			};
		}
	}
}



