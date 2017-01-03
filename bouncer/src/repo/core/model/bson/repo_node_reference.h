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
* Reference Node
*/

#pragma once
#include "repo_node.h"

//------------------------------------------------------------------------------
//
// Fields specific to reference only
//
//------------------------------------------------------------------------------
#define REPO_NODE_REFERENCE_LABEL_REVISION_ID            "_rid"
#define REPO_NODE_REFERENCE_LABEL_UNIQUE                  "unique"
#define REPO_NODE_REFERENCE_LABEL_PROJECT                 "project"
#define REPO_NODE_REFERENCE_LABEL_OWNER                   "owner"
//------------------------------------------------------------------------------

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT ReferenceNode :public RepoNode
			{
			public:

				/**
				* Default constructor
				*/
				ReferenceNode();

				/**
				* Construct a ReferenceNode from a RepoBSON object
				* @param RepoBSON object
				*/
				ReferenceNode(RepoBSON bson);

				/**
				* Default deconstructor
				*/
				~ReferenceNode();

				/**
				* Get the type of node
				* @return returns the type as a string
				*/
				virtual std::string getType() const
				{
					return REPO_NODE_TYPE_REFERENCE;
				}

				/**
				* Get the type of node as an enum
				* @return returns type as enum.
				*/
				virtual NodeType getTypeAsEnum() const
				{
					return NodeType::REFERENCE;
				}

				/**
				* Check if the node is semantically equal to another
				* Different node should have a different interpretation of what
				* this means.
				* @param other node to compare with
				* @param returns true if equal, false otherwise
				*/
				virtual bool sEqual(const RepoNode &other) const;

				/*
				*	------------- Convenience getters --------------
				*/

				/**
				* Retrieve the UUID of the revision this reference node is referring to
				* if it doesn't exist, return master branch id
				* @return returns the UUID for this reference
				*/
				repo::lib::RepoUUID getRevisionID() const{
					if (hasField(REPO_NODE_REFERENCE_LABEL_REVISION_ID))
						return getUUIDField(REPO_NODE_REFERENCE_LABEL_REVISION_ID);
					else
						return repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH);
				}

				/**
				* Retrieve the project this reference node is referring to
				* @return returns the project name for this reference
				*/
				std::string getDatabaseName() const{
					return getStringField(REPO_NODE_REFERENCE_LABEL_OWNER);
				}

				/**
				* Retrieve the project this reference node is referring to
				* @return returns the project name for this reference
				*/
				std::string getProjectName() const{
					return getStringField(REPO_NODE_REFERENCE_LABEL_PROJECT);
				}

				/**
				* Indicates if the Revision ID from getRevisionID()
				* is a branch ID (false) or a specific revision(true)
				* @return returns true if it is a uuid of a specific
				* revision, otherwise it's a branch uuid
				*/
				bool useSpecificRevision() const
				{
					bool isUnique = false; //defaults to false.
					if (hasField(REPO_NODE_REFERENCE_LABEL_UNIQUE))
						isUnique = getField(REPO_NODE_REFERENCE_LABEL_UNIQUE).boolean();

					return isUnique;
				}
			};
		} //namespace model
	} //namespace core
} //namespace repo
