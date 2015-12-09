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
* Simple Diff Comparison - only compare names
*/

#pragma once

#include "repo_diff_abstract.h"

namespace repo{
	namespace manipulator{
		namespace diff{
			class DiffByName : public AbstractDiff
			{
			public:
				/**
				* Construct a diff comparator given the 2 scenes supplied
				* @param base base scene to compare from
				* @param compare scene to compare against
				*/
				DiffByName(
					const repo::core::model::RepoScene *base,
					const repo::core::model::RepoScene *compare);
				virtual ~DiffByName();

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

				/**
				* Compare the 2 given node sets and update the diff results
				* @param baseIDs (compared IDs will be removed)
				* @param compIDs (compared IDs will be removed)
				* @param baseNodes nodes to compare as base revision
				* @param compNodes nodes to compare as compare-to revision
				*/
				void compareNodes(
					std::set<repoUUID> &baseIDs,
					std::set<repoUUID> &compIDs,
					const repo::core::model::RepoNodeSet &baseNodes,
					const repo::core::model::RepoNodeSet &compNodes);

				/**
				* Given a RepoNodeSet, return a map of {name, RepoNode*}
				* @param nodes repoNodeSet to convert from
				* @return returns an undered map containing key value of name and RepoNode*
				*/
				std::unordered_map<std::string, repo::core::model::RepoNode*> 
					createNodeMap(
						const repo::core::model::RepoNodeSet &nodes);


				bool ok; //Check if comparator status is ok
				std::string msg; //error message if comaprator statis is false
			};
		}
	}
}



