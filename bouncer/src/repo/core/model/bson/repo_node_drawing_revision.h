/**
*  Copyright (C) 2024 3D Repo Ltd
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
* A Revision Node - storing information about a specific drawing revision
*/

#pragma once
#include "repo_node_revision.h"

//------------------------------------------------------------------------------
//
// Fields specific to revision only
//
//------------------------------------------------------------------------------
#define REPO_NODE_REVISION_LABEL_PROJECT			"project"
#define REPO_NODE_REVISION_LABEL_MODEL				"model"
#define REPO_NODE_REVISION_LABEL_STATUS				"statusCode"
#define REPO_NODE_REVISION_LABEL_REVCODE			"revCode"
#define REPO_NODE_REVISION_LABEL_FORMAT				"format"
#define REPO_NODE_REVISION_LABEL_IMAGE				"image"

//------------------------------------------------------------------------------

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT DrawingRevisionNode : public RevisionNode
			{
			public:
				/**
				* Constructor
				* Construct a RepoNode base on a RepoBSON object
				* @param replicate this bson object
				*/
				DrawingRevisionNode(RepoBSON bson);

				DrawingRevisionNode();
				~DrawingRevisionNode();

				/**
				* Get the type of node
				* @return returns the type as a string
				*/
				virtual std::string getType() const
				{
					return REPO_NODE_TYPE_REVISION;
				}

				/**
				* Get the type of node as an enum
				* @return returns type as enum.
				*/
				virtual NodeType getTypeAsEnum() const
				{
					return NodeType::REVISION;
				}

				/**
				* --------- Convenience functions -----------
				*/

				/**
				* Returns the list of files uploaded for this revision
				*/
				std::vector<lib::RepoUUID> getFiles() const;

				lib::RepoUUID getProject() const;

				lib::RepoUUID getModel() const;

				DrawingRevisionNode cloneAndAddImage(lib::RepoUUID imageRefNodeId) const;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo