/**
*  Copyright (C) 2014 3D Repo Ltd
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
* Repo Node - A node representation of RepoBSON
*/

#pragma once 

#include "repo_bson.h"
#include "../repo_node_properties.h"
#include "../repo_node_utils.h"

namespace repo{
	namespace core{
		namespace model{
			namespace bson{
				class RepoNode : public RepoBSON
				{
					public:
		
						/**
						* Constructor
						* Construct a RepoNode base on a RepoBSON object
						* 
						*/
						RepoNode(RepoBSON bson);

						/**
						* Empty Constructor
						*/
						RepoNode() : RepoBSON() {};

						/**
						* Default Deconstructor
						*/
						~RepoNode();
						/**
						* A static function (intended for use by RepoBSONFactory) to create
						* a repo node object.
						*
						* @param type repository object type such as 'material', 'mesh', etc.
						* @param api API level of this object, information used to decode it in
						*        clients
						* @param uuid unique identifier, randomly generated if not given
						* @param name optional name of this object, empty string if not specified,
						*             does not have to be unique
						*/
						static RepoNode createRepoNode(
							const std::string &type,
							const unsigned int api = REPO_NODE_API_LEVEL_0,
							const repo_uuid &sharedId = generateUUID(),
							const std::string &name = std::string(),
							const std::vector<repo_uuid> &parents = std::vector<repo_uuid>());

					protected:
					
						/*
						*	------------- Query operations --------------
						*/
						
						std::string type; //!< Compulsory type of this document.

						unsigned int api; //!< Compulsory API level of this document (used to decode).

						repo_uuid sharedID; //!< Shared unique graph document identifier.

						repo_uuid uniqueID; //!< Compulsory unique database document identifier.

						std::string name; //!< Optional name of this document.

						////! Parents of this node.
						///*!
						//* Parents are a std:set to make sure all entries are unique.
						//*/
						//std::set<const RepoNode *> parents;

						/*!
						* Shared IDs of the parents. Needs to be in sync with
						* parents set. This is only useful when retrieving data from the
						* repository as this set can otherwise be calculated on the fly.
						*/
						std::set<repo_uuid> parentSharedIDs;

						//! Children of this node.
						/*!
						* Children are a std:set to make sure all entries are unique.
						*/
						// TODO: remove const
						std::set<const repo_uuid *> children;
				};
			}//namespace bson
		} //namespace model
	} //namespace core
} //namespace repo

