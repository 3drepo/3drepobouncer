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
* Repo Node - A node representation of RepoBSON
*/

#pragma once 

#include "repo_bson.h"
#include "repo_bson_builder.h"
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
						static RepoNode* createRepoNode(
							const std::string &type,
							const unsigned int api = REPO_NODE_API_LEVEL_0,
							const repo_uuid &sharedId = generateUUID(),
							const std::string &name = std::string(),
							const std::vector<repo_uuid> &parents = std::vector<repo_uuid>());

						/**
						* Append default information onto the a RepoBSONBuilder
						* This is used for children nodes to create their BSONs.
						*/
						static void appendDefaults(
							RepoBSONBuilder &builder,
							const std::string &type,
							const unsigned int api = REPO_NODE_API_LEVEL_0,
							const repo_uuid &sharedId = generateUUID(),
							const std::string &name = std::string(),
							const std::vector<repo_uuid> &parents = std::vector<repo_uuid>())
						{
							//--------------------------------------------------------------------------
							// ID field (UUID)
							builder.append(REPO_NODE_LABEL_ID, generateUUID());

							//--------------------------------------------------------------------------
							// Shared ID (UUID)
							builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId);

							//--------------------------------------------------------------------------
							// Type
							if (!type.empty())
								builder << REPO_NODE_LABEL_TYPE << type;

							//--------------------------------------------------------------------------
							// API level
							builder << REPO_NODE_LABEL_API << api;

							//--------------------------------------------------------------------------
							// Parents
							if (parents.size() > 0)
								builder.appendArray(REPO_NODE_LABEL_PARENTS, builder.createArrayBSON(parents));

							//--------------------------------------------------------------------------
							// Name
							if (!name.empty())
								builder << REPO_NODE_LABEL_NAME << name;
						}


						/*
						*	------------- Delusional modifiers --------------
						*   These are like "setters" but not. We are actually
						*   creating a new bson object with the changed field
						*/

						/**
						* Create a new object with this object's values,
						* and add another parent into this new object
						* NOTE: this object is unchanged!
						* @returns new object with the field updated
						*/
						RepoNode cloneAndAddParent(repo_uuid parentID);



						/*
						*	------------- Convenience getters --------------
						*/

						//FIXME: if we don't want convenince fields we need to use getFields()

						/**
						* Get the shared ID from the object
						* @return returns the shared ID of the object
						*/
						repo_uuid getSharedID(){ return sharedID; }

						/**
						* Get the unique ID from the object
						* @return returns the unique ID of the object
						*/
						repo_uuid getUniqueID(){ return uniqueID; }

						/**
						* Get the list of parent IDs 
						* @return returns a set of parent IDs
						*/
						std::vector<repo_uuid> getParentIDs();

						/*
						*	------------- Compare operations --------------
						*/
						//! Returns true if the node is the same, false otherwise.
						bool operator==(const RepoNode& other) const
						{
							return uniqueID == other.uniqueID && sharedID == other.sharedID;
						}

						//! Returns true if the other node is greater than this one, false otherwise.
						bool operator<(const RepoNode& other) const
						{
							if (sharedID == other.sharedID){
								return sharedID < other.sharedID;
							}
							else{
								return uniqueID < other.uniqueID;
							}
						}


					protected:



						/*
						*	------------- node fields --------------
						*/

						//FIXME: Convenience fields, should these really exist?
						
						std::string type; //!< Compulsory type of this document.

						repo_uuid sharedID; //!< Shared unique graph document identifier.

						repo_uuid uniqueID; //!< Compulsory unique database document identifier.

				};
				/*!
				* Comparator definition to enable std::set to store pointers to abstract nodes
				* so that they are compared based on their value rather than their integer
				* pointers.
				*
				* This is a general Repo Node comparator where you would expect a different
				* shared ID and unique ID. Should a different comparator is needed it should be
				* implemented on that node's class level
				*/
				struct RepoNodeComparator
				{
					bool operator()(const RepoNode* a, const RepoNode* b) const
					{
						return *a < *b;
					}
				};

				//! Set definition for pointers to abstract nodes (sorted by value rather than pointer)
				typedef std::set<RepoNode *, RepoNodeComparator> RepoNodeSet;

			}//namespace bson
		} //namespace model
	} //namespace core
} //namespace repo

