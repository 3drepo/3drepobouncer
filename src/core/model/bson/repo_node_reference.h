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
#define REPO_NODE_REFERENCE_TYPE_REFERENCE    			"ref"
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
				class ReferenceNode :public RepoNode
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

					/*
					*	------------- Convenience getters --------------
					*/

					/**
					* Retrieve the UUID of the revision this reference node is referring to
					* @return returns the UUID for this reference
					*/
					repo_uuid getRevisionID(){
						return getUUIDField(REPO_NODE_REFERENCE_LABEL_REVISION_ID);
					}

					/**
					* Retrieve the project this reference node is referring to
					* @return returns the project name for this reference
					*/
					std::string getProjectName(){
						return getField(REPO_NODE_REFERENCE_LABEL_PROJECT).String();
					}

				};
			}//namespace bson
		} //namespace model
	} //namespace core
} //namespace repo


