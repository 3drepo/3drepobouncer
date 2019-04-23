/**
*  Copyright (C) 2019 3D Repo Ltd
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
* Repo File Ref - A reference object for file linkage
*/

#pragma once

#include "repo_bson.h"

namespace repo{
	namespace core{
		namespace model{
			
			class REPO_API_EXPORT RepoFileRef : public RepoBSON
			{
			public:

				/**
				* Constructor
				* Construct a RepoFileRef base on a RepoBSON object
				*
				*/
				RepoFileRef(RepoBSON bson,
					const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping =
					std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>());

				/**
				* Empty Constructor
				*/
				RepoFileRef() : RepoBSON() {};

				/**
				* Default Deconstructor
				*/
				virtual ~RepoFileRef();				

				/*
				*	------------- Convenience getters --------------
				*/

				/**
				* Get the name of the file
				* @return name
				*/
				std::string getFileName() const
				{
					return std::string(getStringField(REPO_LABEL_ID));
				}

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
				repo::lib::RepoUUID getUniqueID() const{ return getUUIDField(REPO_NODE_LABEL_ID); }

				/**
				* Get the list of parent IDs
				* @return returns a set of parent IDs
				*/
				std::vector<repo::lib::RepoUUID> getParentIDs() const;

				/*
				*	------------- Compare operations --------------
				*/
				//! Returns true if the node is the same, false otherwise.
				bool operator==(const RepoFileRef& other) const
				{
					return getUniqueID() == other.getUniqueID() && getSharedID() == other.getSharedID();
				}

				//! Returns true if the other node is greater than this one, false otherwise.
				bool operator<(const RepoFileRef& other) const
				{
					repo::lib::RepoUUID sharedID = getSharedID();

					if (sharedID == other.getSharedID()){
						return getUniqueID() < other.getUniqueID();
					}
					else{
						return sharedID < other.getSharedID();
					}
				}

				/**
				* Check if the node is semantically equal to another
				* Different node should have a different interpretation of what
				* this means.
				* @param other node to compare with
				* @param returns true if equal, false otherwise
				*/
				virtual bool sEqual(const RepoFileRef &other) const
				{
					//On a node level, it is impossible to tell if
					//one node is semantically the same as other.
					//One does not expect this to be ever called
					repoWarning << "sEqual() is called for RepoFileRef* this is not expected!";
					return false; //returns false just incase.
				}

				//! Returns true if the other node is greater than this one, false otherwise.
				bool operator>(const RepoFileRef& other) const
				{
					repo::lib::RepoUUID sharedID = getSharedID();

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
			struct RepoFileRefComparator
			{
				bool operator()(const RepoFileRef* a, const RepoFileRef* b) const
				{
					return *a < *b;
				}
			};

			//! Set definition for pointers to abstract nodes (sorted by value rather than pointer)
			typedef std::set<RepoFileRef *, RepoFileRefComparator> RepoFileRefSet;
		} //namespace model
	} //namespace core
} //namespace repo
