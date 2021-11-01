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
* Metadata Node
*/

#pragma once
#include "repo_node.h"

namespace repo {
	namespace core {
		namespace model {
			//------------------------------------------------------------------------------
			//
			// Fields specific to metadata only
			//
			//------------------------------------------------------------------------------
#define REPO_NODE_LABEL_METADATA     			"metadata"
#define REPO_NODE_LABEL_META_KEY     			"key"
#define REPO_NODE_LABEL_META_VALUE     			"value"
#define REPO_NODE_META_MAX_VALUE_LENGTH         1024
			//------------------------------------------------------------------------------

			class REPO_API_EXPORT MetadataNode :public RepoNode
			{
			public:

				/**
				* Default constructor
				*/
				MetadataNode();

				/**
				* Construct a MetadataNode from a RepoBSON object
				* @param RepoBSON object
				*/
				MetadataNode(RepoBSON bson,
					const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping =
					std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>());

				/**
				* Default deconstructor
				*/
				~MetadataNode();

				/**
				* Get the type of node
				* @return returns the type as a string
				*/
				virtual std::string getType() const
				{
					return REPO_NODE_TYPE_METADATA;
				}

				/**
				* Get the type of node as an enum
				* @return returns type as enum.
				*/
				virtual NodeType getTypeAsEnum() const
				{
					return NodeType::METADATA;
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
				* Append the given metadata to the list of currently existing metadata
				* @param metadata the metadata to add
				* @return returns a cloned version with updated metadata
				*/
				MetadataNode cloneAndAddMetadata(
					const RepoBSON &metadata) const;
			};
		} //namespace model
	} //namespace core
} //namespace repo
