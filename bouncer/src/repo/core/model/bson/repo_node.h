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
							const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping =
								std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>());

						/**
						* Empty Constructor
						*/
						RepoNode() : RepoBSON() {};

						/**
						* Default Deconstructor
						*/
						virtual ~RepoNode();
	
						/**
						* Check if the node is position dependant.
						* i.e. if parent transformation is merged onto the node,
						* does the node requre to a transformation applied to it
						* e.g. meshes and cameras are position dependant, metadata isn't
						* Default behaviour is false. Position dependant child requires 
						* override this function.
						* @return true if node is positionDependant.
						*/
						virtual bool positionDependant() { return false; }


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
						* @return new object with the field updated
						*/
						RepoNode cloneAndAddParent(
							const repoUUID &parent) const;

						/**
						* Create a new object with this object's values,
						* and add other parents into this new object
						* NOTE: this object is unchanged!
						* @param parentID the shared uuid of the parent
						* @return new object with the field updated
						*/
						RepoNode cloneAndAddParent(
							const std::vector<repoUUID> &parents) const;

						/**
						*  Create a new object with transformation applied to the node
						* default behaviour is do nothing. Children object
						* needs to override this function to perform their own specific behaviour.
						* @param matrix transformation matrix to apply.
						* @return returns a new object with transformation applied.
						*/
						virtual RepoNode cloneAndApplyTransformation(
							const std::vector<float> &matrix) const
						{
							return RepoNode(copy(), bigFiles);
						}

						/**
						* Create a new object with this object's values,
						* but a different name
						* @param newName new name
						* @return returns an object with the change
						*/
						RepoNode cloneAndChangeName(
							const std::string &newName,
							const bool &newUniqueID = true
						) const
						{
							return cloneAndAddFields(new RepoBSON(BSON(REPO_NODE_LABEL_NAME << newName)), newUniqueID);
						}

						/**
						* Create a new object with this object's values,
						* and remove a parent into this new object
						* NOTE: this object is unchanged!
						* @param parentID the shared uuid of the parent
						* @param newUniqueID generate a new unique ID if set to true
						* @returns new object with the field updated
						*/
						RepoNode cloneAndRemoveParent(
							const repoUUID &parent,
							const bool     &newUniqueID = true) const;

						/**
						* Create a new object with fields within the 
						* change node (excluding parentID, unique ID and shared ID)
						* NOTE this object is unchanged!
						* @param changes a repobson containing the fields to change
						* @param newUniqueID generate a new unique ID if set to true
						* @return returns a new object with fields updated
						*/
						virtual RepoNode cloneAndAddFields(
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
						repoUUID getSharedID() const { return getUUIDField(REPO_NODE_LABEL_SHARED_ID); }


						/**
						* Get the type of node
						* @return returns the type as a string
						*/
						virtual std::string getType() const
						{ 
							return getStringField(REPO_NODE_LABEL_TYPE); 
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
						repoUUID getUniqueID() const{ return getUUIDField(REPO_NODE_LABEL_ID); }

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

							return getUniqueID() == other.getUniqueID() && getSharedID() == other.getSharedID();
						}

						//! Returns true if the other node is greater than this one, false otherwise.
						bool operator<(const RepoNode& other) const
						{
							repoUUID sharedID = getSharedID();
							
							if (sharedID == other.getSharedID()){
								return getUniqueID() < other.getUniqueID();								
							}
							else{
								return sharedID < other.getSharedID();
							}
						}

						//! Returns true if the other node is greater than this one, false otherwise.
						bool operator>(const RepoNode& other) const
						{
							repoUUID sharedID = getSharedID();

							
							if (sharedID == other.getSharedID()){
								return getUniqueID() > other.getUniqueID();
							}
							else{
								return sharedID > other.getSharedID();
							}
						}


					protected:



						/*
						*	------------- node fields --------------
						*/

						//FIXME: Convenience fields, should these really exist?
						
						std::string type; //!< Compulsory type of this document.


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

