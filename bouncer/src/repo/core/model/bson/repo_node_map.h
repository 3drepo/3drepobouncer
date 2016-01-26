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
					#define REPO_NODE_MAP_LABEL_APIKEY              "apiKey" //FIXME: Temporary measure until we change the way x3dom reads google map tiles
					#define REPO_NODE_MAP_LABEL_WIDTH               "width"
					#define REPO_NODE_MAP_LABEL_YROT                "yrot"
					#define REPO_NODE_MAP_LABEL_TILESIZE            "worldTileSize"
					#define REPO_NODE_MAP_LABEL_LONG                "long"
					#define REPO_NODE_MAP_LABEL_LAT                 "lat"
					#define REPO_NODE_MAP_LABEL_MAP_TYPE            "maptype"
					#define REPO_NODE_MAP_LABEL_ZOOM                "zoom"
					#define REPO_NODE_MAP_LABEL_TRANS               "trans"
					#define REPO_NODE_MAP_LABEL_TWO_SIDED           "twosided"
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

					/**
					* --------- Convenience functions -----------
					*/

					std::string getAPIKey() const
					{
						return getStringField(REPO_NODE_MAP_LABEL_APIKEY);
					}

					/**
					* Check if the map tile is suppose to be 2 sided
					* Default is false
					* @return returns true if the map tile is 2 sided
					*/
					bool getIsTwoSided() const
					{
						bool twoSided = false;
						if (hasField(REPO_NODE_MAP_LABEL_TWO_SIDED))
						{
							twoSided = getField(REPO_NODE_MAP_LABEL_TWO_SIDED).Bool();
						}
						return twoSided;
					}

					/**
					* Get the type of map
					* @return returns type of map as a string, empty string if not found
					*/
					std::string getMapType() const
					{
						return getStringField(REPO_NODE_MAP_LABEL_MAP_TYPE);
					}

					/**
					* Get transformation matrix within the Map node
					* @return returns the transformation matrix within this node
					*/
					std::vector<float> getTransformationMatrix(const bool &rowMajor = true) const;

					/**
					* Get the tile width 
					* @return returns the tile width
					*/
					uint32_t getWidth() const;


					/**
					* Get Y Rotational(tilt) from the map
					* @retun returns the rotational tilt
					*/
					float getYRot() const;
					
					/**
					* Check if the node is semantically equal to another
					* Different node should have a different interpretation of what
					* this means.
					* @param other node to compare with
					* @param returns true if equal, false otherwise
					*/
					virtual bool sEqual(const RepoNode &other) const;

				};
		} //namespace model
	} //namespace core
} //namespace repo


