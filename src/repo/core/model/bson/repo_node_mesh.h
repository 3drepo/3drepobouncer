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

#include "../../../repo_bouncer_global.h"

namespace repo {
	namespace core {
		namespace model {

				//------------------------------------------------------------------------------
				//
				// Fields specific only to mesh
				//
				//------------------------------------------------------------------------------
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

				class REPO_API_EXPORT MeshNode :public RepoNode
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
					* --------- Convenience functions -----------
					*/

					/**
					* Retrieve a vector of Colors from the bson object
					*/
					std::vector<repo_color4d_t>* getColors() const;

					/**
					* Retrieve a vector of faces from the bson object
					*/
					std::vector<repo_face_t>* getFaces() const;


					/**
					* Retrieve a vector of vertices from the bson object
					*/
					std::vector<repo_vector_t>* getNormals() const;

					/**
					* Retrieve a vector of UV Channels from the bson object
					*/
					std::vector<repo_vector2d_t>* getUVChannels() const;

					/**
					* Retrieve a vector of UV Channels, separated by channels
					*/
					std::vector<std::vector<repo_vector2d_t>>* getUVChannelsSeparated() const;


					/**
					* Retrieve a vector of vertices from the bson object
					*/
					std::vector<repo_vector_t>* getVertices() const;


				};
		} //namespace model
	} //namespace core
} //namespace repo


