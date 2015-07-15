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
* Mesh Node
*/

#pragma once
#include "repo_node.h"

namespace repo {
	namespace core {
		namespace model {
			namespace bson {

				//------------------------------------------------------------------------------
				//
				// Fields specific only to mesh
				//
				//------------------------------------------------------------------------------
				#define REPO_NODE_TYPE_MESH						"mesh"
				#define REPO_NODE_LABEL_VERTICES				"vertices" //<! vertices array
				#define REPO_NODE_LABEL_VERTICES_COUNT			"vertices_count" //<! vertices size
				#define REPO_NODE_LABEL_VERTICES_BYTE_COUNT		"vertices_byte_count"
				//------------------------------------------------------------------------------
				#define REPO_NODE_LABEL_FACES					"faces" //<! faces array label
				#define REPO_NODE_LABEL_FACES_COUNT				"faces_count" //<! number of faces
				#define REPO_NODE_LABEL_FACES_BYTE_COUNT		"faces_byte_count"
				//------------------------------------------------------------------------------
				#define REPO_NODE_LABEL_NORMALS					"normals" //!< normals array label
				//------------------------------------------------------------------------------
				#define REPO_NODE_LABEL_OUTLINE					"outline" //!< outline array label
				#define REPO_NODE_LABEL_BOUNDING_BOX			"bounding_box" //!< bounding box
				//------------------------------------------------------------------------------
				#define REPO_NODE_LABEL_UV_CHANNELS				"uv_channels" //!< uv channels array
				#define REPO_NODE_LABEL_UV_CHANNELS_COUNT		"uv_channels_count"
				#define REPO_NODE_LABEL_UV_CHANNELS_BYTE_COUNT	"uv_channels_byte_count"
				#define REPO_NODE_LABEL_SHA256                  "sha256"
				#define REPO_NODE_LABEL_COLORS                  "colors"
				//------------------------------------------------------------------------------
				#define REPO_NODE_UUID_SUFFIX_MESH				"08" //!< uuid suffix
				//------------------------------------------------------------------------------

				class MeshNode :public RepoNode
				{
				public:

					/**
					* Default constructor
					*/
					MeshNode();

					/**
					* Construct a MeshNode from a RepoBSON object
					* @param RepoBSON object
					*/
					MeshNode(RepoBSON bson);


					/**
					* Default deconstructor
					*/
					~MeshNode();

					/**
					* Static builder for factory use to create a Mesh Node
					* @param vector of vertices
					* @param vector of faces
					* @param vecotr of normals
					* @param vector of 2 vertex indicating the bounding box
					* @param vector of UV Channels
					* @param vector of colours
					* @param outline, generated from bounding box
					* @param API level of the node (optional, default REPO_NODE_API_LEVEL_1)
					* @param name of the node (optional, default empty string)
					* @return returns a pointer mesh node
					*/
					static MeshNode* MeshNode::createMeshNode(
						std::vector<repo_vector_t>                  &vertices,
						std::vector<repo_face_t>                    &faces,
						std::vector<repo_vector_t>                  &normals,
						std::vector<repo_vector_t>                  &boundingBox,
						std::vector<std::vector<repo_vector2d_t>>   &uvChannels,
						std::vector<repo_color4d_t>                 &colors,
						std::vector<repo_vector2d_t>                &outline,
						const int                                   &apiLevel = REPO_NODE_API_LEVEL_1,
						const std::string                           &name = std::string());

				};
			}//namespace bson
		} //namespace model
	} //namespace core
} //namespace repo


