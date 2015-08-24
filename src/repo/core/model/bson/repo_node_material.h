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
* Material Node
*/

#pragma once
#include "repo_node.h"

namespace repo {
	namespace core {
		namespace model {

				//------------------------------------------------------------------------------
				//
				// Fields specific to mesh only
				//
				//------------------------------------------------------------------------------
				#define REPO_NODE_MATERIAL_LABEL_AMBIENT					"ambient"
				#define REPO_NODE_MATERIAL_LABEL_DIFFUSE					"diffuse"
				#define REPO_NODE_MATERIAL_LABEL_SPECULAR				"specular"
				#define REPO_NODE_MATERIAL_LABEL_EMISSIVE				"emissive"
				#define REPO_NODE_MATERIAL_LABEL_WIREFRAME				"wireframe"
				#define REPO_NODE_MATERIAL_LABEL_TWO_SIDED				"two_sided"
				#define REPO_NODE_MATERIAL_LABEL_OPACITY					"opacity"
				#define REPO_NODE_MATERIAL_LABEL_SHININESS				"shininess"
				#define REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH		"shininess_strength"
				#define REPO_NODE_UUID_SUFFIX_MATERIAL			"07" //!< uuid suffix
				//------------------------------------------------------------------------------


				class REPO_API_EXPORT MaterialNode :public RepoNode
				{
				public:

					/**
					* Default constructor
					*/
					MaterialNode();

					/**
					* Construct a MaterialNode from a RepoBSON object
					* @param RepoBSON object
					*/
					MaterialNode(RepoBSON bson);


					/**
					* Default deconstructor
					*/
					~MaterialNode();


					/**
					* --------- Convenience functions -----------
					*/

					/**
					* Get material information from the node as a struct
					* @return returns a repo_material_t containing the information
					*/
					repo_material_t MaterialNode::getMaterialStruct() const;

				};
		} //namespace model
	} //namespace core
} //namespace repo


