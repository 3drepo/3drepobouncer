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
* Map Node
*/

#pragma once
#include "repo_node.h"

namespace repo {
	namespace core {
		namespace model {
				class REPO_API_EXPORT MapNode :public RepoNode
				{
					#define REPO_NODE_MAP_LABEL_WIDTH               "width"
					#define REPO_NODE_MAP_LABEL_YROT                "yrot"
					#define REPO_NODE_MAP_LABEL_TILESIZE            "worldTileSize"
					#define REPO_NODE_MAP_LABEL_LONG                "long"
					#define REPO_NODE_MAP_LABEL_LAT                 "lat"
					#define REPO_NODE_MAP_LABEL_ZOOM                "zoom"
					#define REPO_NODE_MAP_LABEL_TRANS               "trans"
					#define REPO_NODE_MAP_DEFAULTNAME               "<map>"

				public:

					/**
					* Default constructor
					*/
					MapNode();

					/**
					* Construct a MapNode from a RepoBSON object
					* @param RepoBSON object
					*/
					MapNode(RepoBSON bson);


					/**
					* Default deconstructor
					*/
					~MapNode();

				};
		} //namespace model
	} //namespace core
} //namespace repo


