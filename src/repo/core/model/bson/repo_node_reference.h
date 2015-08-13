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
#define REPO_NODE_REFERENCE_UUID_SUFFIX_REFERENCE			"13" //!< uuid suffix
//------------------------------------------------------------------------------



namespace repo {
	namespace core {
		namespace model {
			namespace bson {
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
					* Create a Reference Node
					* If revision ID is unique, it will be referencing a specific revision
					* If it isn't unique, it will represent a branch ID and reference its head
					* @param database name of the database to reference
					* @param project name of the project to reference
					* @param revisionID uuid of the revision (default: master branch)
					* @param isUniqueID is revisionID a specific revision ID (default: false - i.e it is a branch ID)
					* @return returns a reference node
					*/
					static ReferenceNode createReferenceNode(
						const std::string &database,
						const std::string &project,
						const repoUUID    &revisionID = stringToUUID(REPO_HISTORY_MASTER_BRANCH),
						const bool        &isUniqueID = false,
						const std::string &name = std::string(),
						const int         &apiLevel = REPO_NODE_API_LEVEL_1);


					/*
					*	------------- Convenience getters --------------
					*/

					/**
					* Retrieve the UUID of the revision this reference node is referring to
					* @return returns the UUID for this reference
					*/
					repoUUID getRevisionID(){
						return getUUIDField(REPO_NODE_REFERENCE_LABEL_REVISION_ID);
					}

					/**
					* Retrieve the project this reference node is referring to
					* @return returns the project name for this reference
					*/
					std::string getProjectName(){
						return getStringField(REPO_NODE_REFERENCE_LABEL_PROJECT);
					}

					/**
					* Indicates if the Revision ID from getRevisionID() 
					* is a branch ID (false) or a specific revision(true)
					* @return returns true if it is a uuid of a specific 
					* revision, otherwise it's a branch uuid
					*/
					bool useSpecificRevision()
					{
						bool isUnique = false; //defaults to false.
						if (hasField(REPO_NODE_REFERENCE_LABEL_UNIQUE))
							isUnique =  getField(REPO_NODE_REFERENCE_LABEL_UNIQUE).boolean();

						return isUnique;
					}

				};
			}//namespace bson
		} //namespace model
	} //namespace core
} //namespace repo


