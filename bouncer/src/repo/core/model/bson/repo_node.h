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

				enum class NodeType{ CAMERA, MAP, MATERIAL, MESH, METADATA, REFERENCE, 
					REVISION, TEXTURE, TRANSFORMATION, UNKNOWN};

				class REPO_API_EXPORT RepoNode : public RepoBSON
				{
					public:
		
						/**
						* Constructor
						* Construct a RepoNode base on a RepoBSON object
						* 
						*/
						RepoNode(RepoBSON bson,
							const std::unordered_map<std::string, std::vector<uint8_t>> &binMapping =
								std::unordered_map<std::string, std::vector<uint8_t>>());

						/**
						* Empty Constructor
						*/
						RepoNode() : RepoBSON() {};

						/**
						* Default Deconstructor
						*/
						virtual ~RepoNode();
	


						/*
						*	------------- Delusional modifiers --------------
						*   These are like "setters" but not. We are actually
						*   creating a new bson object with the changed field
						*/

						/**
						* Create a new object with this object's values,
						* and add another parent into this new object
						* NOTE: this object is unchanged!
						* @param parentID the shared uuid of the parent
						* @returns new object with the field updated
						*/
						RepoNode cloneAndAddParent(
							const repoUUID &parent) const;

						/**
						* Create a new object with this object's values,
						* and add other parents into this new object
						* NOTE: this object is unchanged!
						* @param parentID the shared uuid of the parent
						* @returns new object with the field updated
						*/
						RepoNode cloneAndAddParent(
							const std::vector<repoUUID> &parents) const;

						/**
						* Create a new object with fields within the 
						* change node (excluding parentID, unique ID and shared ID)
						* NOTE this object is unchanged!
						* @param changes a repobson containing the fields to change
						* @param newUniqueID generate a new unique ID if set to true
						* @return returns a new object with fields updated
						*/
						RepoNode cloneAndAddFields(
							const RepoBSON *changes, 
							const bool     &newUniqueID = true) const;

						/**
						* Clone the object with a new added merge nodes field
						* @param mergeMap A vector of nodes merged
						* @return returns a new object with field updated
						*/
						RepoNode cloneAndAddMergedNodes(
							const std::vector<repoUUID> &mergeMap) const;

						/*
						*	------------- Convenience getters --------------
						*/

						

						/**
						* Get the name of the node
						* @return returns name or "" if no name
						*/
						std::string getName() const
						{
							return std::string(getStringField(REPO_NODE_LABEL_NAME));
						}

						/**
						* Get the shared ID from the object
						* @return returns the shared ID of the object
						*/
						repoUUID getSharedID() const { return sharedID; }


						/**
						* Get the type of node
						* @return returns the type as a string
						*/
						virtual std::string getType() const
						{ 
							return std::string(getStringField(REPO_NODE_LABEL_TYPE)); 
						}

						/**
						* Get the type of node as an enum
						* @return returns type as enum.
						*/
						virtual NodeType getTypeAsEnum() const;

						/**
						* Get the unique ID from the object
						* @return returns the unique ID of the object
						*/
						repoUUID getUniqueID() const{ return uniqueID; }

						/**
						* Get the list of parent IDs 
						* @return returns a set of parent IDs
						*/
						std::vector<repoUUID> getParentIDs() const;


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

						repoUUID sharedID; //!< Shared unique graph document identifier.

						repoUUID uniqueID; //!< Compulsory unique database document identifier.

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

		} //namespace model
	} //namespace core
} //namespace repo

