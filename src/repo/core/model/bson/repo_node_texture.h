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
* Texture Node
*/

#pragma once
#include "repo_node.h"

namespace repo {
	namespace core {
		namespace model {
			namespace bson {

				//------------------------------------------------------------------------------
				//
				// Fields specific to texture only
				//
				//------------------------------------------------------------------------------
				#define REPO_NODE_TYPE_TEXTURE				"texture"
				#define REPO_NODE_LABEL_BIT_DEPTH			"bit_depth"
				#define REPO_NODE_LABEL_EXTENSION			"extension"
				#define REPO_NODE_LABEL_DATA_BYTE_COUNT		"data_byte_count"
				#define REPO_NODE_UUID_SUFFIX_TEXTURE		"11" //!< uuid suffix
				//------------------------------------------------------------------------------

				class REPO_API_EXPORT TextureNode :public RepoNode
				{
				public:

					/**
					* Default constructor
					*/
					TextureNode();

					/**
					* Construct a TextureNode from a RepoBSON object
					* @param RepoBSON object
					*/
					TextureNode(RepoBSON bson);


					/**
					* Default deconstructor
					*/
					~TextureNode();

					/**
					* Static builder for factory use to create a Texture Node
					* @param name of the texture node
					* @param binary data to store
					* @param size of the binary data in bytes
					* @param width of the texture
					* @param height of the texture
					* @return returns a texture node
					*/
					static TextureNode createTextureNode(
						const std::string &name,
						const char        *data,
						const uint32_t    &size,
						const uint32_t    &width,
						const uint32_t    &height,
						const int &apiLevel = REPO_NODE_API_LEVEL_1);


					/**
					* --------- Convenience functions -----------
					*/

					/**
					* Retrieve texture image as raw data
					* @return returns a pointer to the image (represented as char)
					*/
					std::vector<char>* getRawData() const;

					std::string getFileExtension() const;

				};
			}//namespace bson
		} //namespace model
	} //namespace core
} //namespace repo


