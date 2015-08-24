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
				#define REPO_NODE_UUID_SUFFIX_METADATA			"14" //!< uuid suffix
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
					MetadataNode(RepoBSON bson);


					/**
					* Default deconstructor
					*/
					~MetadataNode();



					/**
					* Static builder for factory use to create a Metadata Node
					* @param metadata Metadata itself in RepoBSON format
					* @param mimtype Mime type, the media type of the metadata (optional)
					* @param name Name of Metadata (optional)
					* @param parents shared ID of parents for this node (optional)
					* @param apiLevel Repo Node API level (optional)
					* @return returns a pointer metadata node
					*/
					static MetadataNode createMetadataNode(
						RepoBSON			  &metadata,
						const std::string     &mimeType = std::string(),
						const std::string     &name = std::string(),
						const std::vector<repoUUID> &parents = std::vector<repoUUID>(),
						const int             &apiLevel = REPO_NODE_API_LEVEL_1);

				};
		} //namespace model
	} //namespace core
} //namespace repo


