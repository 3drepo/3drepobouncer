/**
*  Copyright (C) 2014 3D Repo Ltd
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
* A Scene graph representation of a collection
*/

#pragma once

#include "repo_graph_revision.h"

namespace repo{
	namespace manipulator{
		namespace graph{
			class SceneGraph
			{
				public:
					/**
					* Default Constructor
					*/
					SceneGraph();

					/**
					* Instantiates a Scene graph with a reference to a revision graph
					* @param A revision graph representing the history of this scene
					*/
					SceneGraph(RevisionGraph *history);

					/**
					* Default Deconstructor
					*/
					~SceneGraph();

					/**
					* Set the revision graph of this scene
					* @param the revision graph
					*/
					void setRevisionGraph(RevisionGraph *history) { this->history = history; };
				private:
					RevisionGraph *history;
			};
		}//namespace graph
	}//namespace manipulator
}//namespace repo


