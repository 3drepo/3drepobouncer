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
* A Revision Node - storing information about a specific revision
*/

#pragma once
#include "repo_node_revision.h"

//------------------------------------------------------------------------------
//
// Fields specific to revision only
//
//------------------------------------------------------------------------------
#define REPO_NODE_REVISION_LABEL_MESSAGE					"desc" //!< Message
#define REPO_NODE_REVISION_LABEL_TAG						"tag" //!< Tag
#define REPO_NODE_REVISION_LABEL_BRANCH_MASTER			"master" //!< Master branch
#define REPO_NODE_REVISION_LABEL_WORLD_COORD_SHIFT      "coordOffset"
//------------------------------------------------------------------------------

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT ModelRevisionNode : public RevisionNode
			{
			public:
				// Some of these statuses will no longer be set by bouncer, but
				// may still exist in the database.
				enum class UploadStatus { COMPLETE = 0, GEN_DEFAULT = 1, GEN_REPO_STASH = 2, GEN_WEB_STASH = 3, GEN_SEL_TREE = 4, MISSING_BUNDLES = 5, UNKNOWN = 6 };

				/**
				* Constructor
				* Construct a RepoNode base on a RepoBSON object
				* @param replicate this bson object
				*/
				ModelRevisionNode(RepoBSON bson);

				ModelRevisionNode();
				~ModelRevisionNode();

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
				* Update the status flag with the given status
				* NOTE: the status flags denotes what it is currently doing
				*       not what has been done. e.g. GEN_DEFAULT denotes the
				*       revision does not have a fully commited default scene graph
				* @param status the status to set to
				* @return returns a clone of this node with updated status flag
				*/
				ModelRevisionNode cloneAndUpdateStatus(
					const UploadStatus &status) const;

				/**
				* --------- Convenience functions -----------
				*/

				/**
				* Get the offset coordinates to translate the model
				* @return return a vector of double (size of 3)
				*/
				std::vector<double> getCoordOffset() const;
				/**
				* Get the message commited with the revision
				* @return returns a string for message. empty string if none.
				*/
				std::string getMessage() const;

				/**
				* Get the tag commited with the revision
				* @return returns a string for tag. empty string if none.
				*/
				std::string getTag() const;

				/**
				* Get the original file(s) the scene original created from
				* @return returns a vector of string of files
				*/
				std::vector<std::string> getOrgFiles() const;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo