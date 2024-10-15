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
#include "repo/core/model/repo_model_global.h"

namespace repo {
	namespace core {
		namespace model {
			//------------------------------------------------------------------------------
			//
			// Fields specific to texture only
			//
			//------------------------------------------------------------------------------
#define REPO_NODE_TYPE_TEXTURE				"texture"
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
				TextureNode(RepoBSON bson);

				/**
				* Default deconstructor
				*/
				~TextureNode();

			protected:
				virtual void deserialise(RepoBSON&);
				virtual void serialise(class RepoBSONBuilder&) const;

			public:

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

			private:
				std::vector<uint8_t> data;
				std::string extension;
				uint32_t width;
				uint32_t height;

			public:
				/**
				* Retrieve texture image as raw data
				* @return returns a pointer to the image (represented as char)
				*/
				const std::vector<uint8_t>& getRawData() const
				{
					return data;
				}

				std::string getFileExtension() const
				{
					return extension;
				}

				uint32_t getWidth() const 
				{
					return width;
				}

				uint32_t getHeight() const 
				{
					return height;
				}

				bool isEmpty() const;

				void setData(const std::vector<uint8_t>& data, size_t width, size_t height, std::string extension = "") 
				{
					this->data = data;
					this->width = width;
					this->height = height;
					this->extension = extension;
				}
			};
		} //namespace model
	} //namespace core
} //namespace repo
