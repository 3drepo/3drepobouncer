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
			//------------------------------------------------------------------------------
			//
			// Fields specific to texture only
			//
			//------------------------------------------------------------------------------
#define REPO_NODE_TYPE_TEXTURE				"texture"
#define REPO_NODE_LABEL_BIT_DEPTH			"bit_depth"
#define REPO_NODE_LABEL_DATA_BYTE_COUNT		"data_byte_count"
			//------------------------------------------------------------------------------

			class REPO_API_EXPORT TextureNode : public RepoNode
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
				TextureNode(RepoBSON bson,
					const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>&binMapping = std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>());

				/**
				* Default deconstructor
				*/
				~TextureNode();

				/**
				* Get the type of node
				* @return returns the type as a string
				*/
				virtual std::string getType() const
				{
					return REPO_NODE_TYPE_TEXTURE;
				}

				/**
				* Get the type of node as an enum
				* @return returns type as enum.
				*/
				virtual NodeType getTypeAsEnum() const
				{
					return NodeType::TEXTURE;
				}

				/**
				* Check if the node is semantically equal to another
				* Different node should have a different interpretation of what
				* this means.
				* @param other node to compare with
				* @param returns true if equal, false otherwise
				*/
				virtual bool sEqual(const RepoNode &other) const;

				/**
				* --------- Convenience functions -----------
				*/

				/**
				* Retrieve texture image as raw data
				* @return returns a pointer to the image (represented as char)
				*/
				std::vector<char> getRawData() const;

				std::string getFileExtension() const;
			};
		} //namespace model
	} //namespace core
} //namespace repo
