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
#include "repo_node.h"


//------------------------------------------------------------------------------
//
// Fields specific to revision only
//
//------------------------------------------------------------------------------
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

				class REPO_API_EXPORT RevisionNode : public RepoNode
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

					/**
					* Static builder for factory use to create a Revision Node
					* @param user name of the user who is commiting this project
					* @param branch UUID of the branch
					* @param currentNodes vector of current nodes (unique IDs)
					* @param added    vector of what nodes are added    (shared IDs)
					* @param removed  vector of what nodes are deleted  (shared IDs)
					* @param modified vector of what nodes are modified (shared IDs)
					* @param parent   UUID of parent (in a vector)
					* @param message  A message to describe what this commit is for (optional)
					* @param tag      A tag for this specific revision (optional)
					* @param apiLevel API Level(optional)
					* @return returns a texture node
					*/
					static RevisionNode createRevisionNode(
						const std::string			 &user,
						const repoUUID              &branch,
						const std::vector<repoUUID> &currentNodes,
						const std::vector<repoUUID> &added,
						const std::vector<repoUUID> &removed,
						const std::vector<repoUUID> &modified,
						const std::vector<repoUUID> &parent,
						const std::string            &message = std::string(),
						const std::string            &tag = std::string(),
						const int                    &apiLevel = REPO_NODE_API_LEVEL_1
						);


					/**
					* --------- Convenience functions -----------
					*/

					/**
					* Get the author commited the revision
					* @return returns a string for message. empty string if none.
					*/
					std::string getAuthor() const;

					/**
					* Get a list of current IDs for this revision
					* @return returns a vector of unique IDs.
					*/
					std::vector<repoUUID> getCurrentIDs() const;

					/**
					* Get a list of IDs of nodes which were Added for this revision
					* @return returns a vector of shared IDs.
					*/
					std::vector<repoUUID> getAddedIDs() const;

					/**
					* Get a list of IDs of nodes which were deleted for this revision
					* @return returns a vector of shared IDs.
					*/
					std::vector<repoUUID> getDeletedIDs() const;

					/**
					* Get a list of IDs of nodes which were modified for this revision
					* @return returns a vector of shared IDs.
					*/
					std::vector<repoUUID> getModifiedIDs() const;

					/**
					* Get the message commited with the revision
					* @return returns a string for message. empty string if none.
					*/
					std::string getMessage() const;

					/**
					* Get the timestamp as int when this revision was commited
					* @return returns a timestamp
					*/
					int64_t getTimestampInt64() const;




				};
		}// end namespace model
	} // end namespace core
} // end namespace repo