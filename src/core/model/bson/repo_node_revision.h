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
* A Revision Node - storing information about a specific revision
*/


#pragma once
#include "repo_bson.h"
#include "repo_node.h"


//------------------------------------------------------------------------------
//
// Fields specific to revision only
//
//------------------------------------------------------------------------------
#define REPO_NODE_TYPE_REVISION					"revision" //!< Type
#define REPO_NODE_REVISION_LABEL_AUTHOR					"author" //!< Author
#define REPO_NODE_REVISION_LABEL_MESSAGE					"message" //!< Message
#define REPO_NODE_REVISION_LABEL_TAG						"tag" //!< Tag
#define REPO_NODE_REVISION_LABEL_TIMESTAMP				"timestamp" //!< Timestamp
#define REPO_NODE_REVISION_LABEL_CURRENT_UNIQUE_IDS		"current" //!< Current UIDs
#define REPO_NODE_REVISION_LABEL_ADDED_SHARED_IDS		"added" //!< Added SIDs
#define REPO_NODE_REVISION_LABEL_DELETED_SHARED_IDS		"deleted" //!< Deleted SIDs
#define REPO_NODE_REVISION_LABEL_MODIFIED_SHARED_IDS		"modified" //!< Modified SIDs
#define REPO_NODE_REVISION_LABEL_UNMODIFIED_SHARED_IDS	"unmodified" //!< Unmodified SIDs
#define REPO_NODE_REVISION_LABEL_BRANCH_MASTER			"master" //!< Master branch
#define REPO_NODE_UUID_SUFFIX_REVISION			"10" //!< uuid suffix
//------------------------------------------------------------------------------

namespace repo {
	namespace core {
		namespace model {
			namespace bson {

				class RevisionNode : public RepoNode
				{
				public:

					/**
					* Constructor
					* Construct a RepoNode base on a RepoBSON object
					* @param replicate this bson object
					*/
					RevisionNode(RepoBSON bson);


					RevisionNode();
					~RevisionNode();
				};

			}// end namespace bson
		}// end namespace model
	} // end namespace core
} // end namespace repo