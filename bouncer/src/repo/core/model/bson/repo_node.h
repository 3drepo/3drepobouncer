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

#pragma once

#include "repo/repo_bouncer_global.h"
#include "repo/lib/datastructure/repo_matrix.h"
#include "repo/lib/datastructure/repo_uuid.h"
#include <vector>
#include <set>

namespace repo {
	namespace core {
		namespace model {

			class RepoBSON;

			enum class NodeType {
				MATERIAL,
				MESH,
				METADATA,
				REFERENCE,
				REVISION,
				TEXTURE,
				TRANSFORMATION,
				UNKNOWN,
			};

			/**
			* RepoNode is the base class for Repo database Nodes - classes
			* corresponding to specific types of document that are written
			* to the database and filesystem. These types are dynamic at
			* runtime and used to build representations of scenes and other
			* assets. They are then commited to the database by turning them
			* into immutable RepoBSON objects.
			*/
			class REPO_API_EXPORT RepoNode
			{
			public:

				/**
				* Constructor
				* Construct a RepoNode base on a RepoBSON object. This should
				* not be called directly, because RepoNodes should always be
				* initialised to their most specific subtype from RepoBSONs.
				*/
				RepoNode(const RepoBSON& bson);

				/**
				* Empty Constructor
				*/
				RepoNode();

				/**
				* Default Deconstructor
				*/
				virtual ~RepoNode();

				RepoBSON getBSON() const;

				operator RepoBSON() const;

			protected:
				virtual void deserialise(const RepoBSON&);
				virtual void serialise(class RepoBSONBuilder&) const;

			public:

				/**
				* Check if the node is position dependant.
				* i.e. if parent transformation is merged onto the node,
				* does the node requre to a transformation applied to it
				* e.g. meshes and cameras are position dependant, metadata isn't
				* Default behaviour is false. Position dependant child requires
				* override this function.
				* @return true if node is positionDependant.
				*/
				virtual bool positionDependant()
				{
					return false;
				}

				void addParent(
					const repo::lib::RepoUUID& parent
				)
				{
					parentIds.insert(parent);
				}

				void addParents(
					const std::vector<repo::lib::RepoUUID>& parents)
				{
					for (auto id : parents) {
						parentIds.insert(id);
					}
				}

				void setParents(const std::vector<repo::lib::RepoUUID>& parents)
				{
					parentIds.clear();
					addParents(parents);
				}

				void removeParent(
					const repo::lib::RepoUUID& parent
				)
				{
					parentIds.erase(parent);
				}

				void changeName(
					const std::string& newName,
					const bool& newUniqueID = false
				)
				{
					name = newName;
					if (newUniqueID) {
						uniqueId = repo::lib::RepoUUID::createUUID();
					}
				}

				virtual void applyTransformation(
					const repo::lib::RepoMatrix& matrix)
				{
					// This is a no-op for most types
				}

				/*
				*	------------- Convenience getters --------------
				*/

			protected:
				std::string name;
				repo::lib::RepoUUID uniqueId;
				repo::lib::RepoUUID sharedId;
				std::set<repo::lib::RepoUUID> parentIds;
				repo::lib::RepoUUID revId;

			public:
				/**
				* Get the name of the node
				* @return returns name or "" if no name
				*/
				std::string getName() const
				{
					return name;
				}

				/**
				* Get the shared ID from the object
				* @return returns the shared ID of the object
				*/
				repo::lib::RepoUUID getSharedID() const
				{
					return sharedId;
				}

				void setSharedID(const repo::lib::RepoUUID& sharedId)
				{
					this->sharedId = sharedId;
				}

				/**
				* Get the type of node
				* @return returns the type as a string
				*/
				virtual std::string getType() const
				{
					return "";
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
				repo::lib::RepoUUID getUniqueID() const
				{
					return uniqueId;
				}

				void setUniqueID(const repo::lib::RepoUUID& uniqueId)
				{
					this->uniqueId = uniqueId;
				}

				/**
				* Get the list of parent IDs
				* @return returns a set of parent IDs
				*/
				std::vector<repo::lib::RepoUUID> getParentIDs() const
				{
					return std::vector<repo::lib::RepoUUID>(parentIds.begin(), parentIds.end());
				}

				const repo::lib::RepoUUID& getRevision() const
				{
					return revId;
				}

				void setRevision(const repo::lib::RepoUUID& id)
				{
					revId = id;
				}

				/*
				*	------------- Compare operations --------------
				*/
				//! Returns true if the node is the same, false otherwise.
				bool operator==(const RepoNode& other) const
				{
					return getUniqueID() == other.getUniqueID() && getSharedID() == other.getSharedID();
				}

				bool operator!=(const RepoNode& other) const
				{
					return getUniqueID() != other.getUniqueID() || getSharedID() != other.getSharedID() || parentIds != other.parentIds;
				}

				//! Returns true if the other node is greater than this one, false otherwise.
				bool operator<(const RepoNode& other) const
				{
					repo::lib::RepoUUID sharedID = getSharedID();

					if (sharedID == other.getSharedID()) {
						return getUniqueID() < other.getUniqueID();
					}
					else {
						return sharedID < other.getSharedID();
					}
				}

				/**
				* Checks if two nodes are semantically equivalent. Semantically equivalent
				* means they would have say the same geometry for MeshNodes, or the same
				* metadata for MetadataNodes, or the same matrix for TransformationNodes.
				* The meaning will depend on the subclass. This method does not consider
				* 'meta' properties such as the shared or unique Ids, or Revision.
				* The default implementation will throw an exception.
				* @param other node to compare with
				* @param returns true if equal, false otherwise
				*/
				virtual bool sEqual(const RepoNode& other) const;

				//! Returns true if the other node is greater than this one, false otherwise.
				bool operator>(const RepoNode& other) const
				{
					repo::lib::RepoUUID sharedID = getSharedID();

					if (sharedID == other.getSharedID()) {
						return getUniqueID() > other.getUniqueID();
					}
					else {
						return sharedID > other.getSharedID();
					}
				}

				/*
				* Returns the estimated memory consumption, in bytes, of this node and all
				* its members. This must be overridden by interested subclasses. The estimated
				* values will be approximate.
				*/
				virtual size_t getSize() const
				{
					return sizeof(*this);
				}
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


			/* Concept can be used as the typename in templates to constrain the type to a
			* subclass of RepoNode.
			*/
			template<class T>
			concept RepoNodeClass = std::is_base_of<repo::core::model::RepoNode, T>::value;

			//! Set definition for pointers to abstract nodes (sorted by value rather than pointer)
			typedef std::set<RepoNode*, RepoNodeComparator> RepoNodeSet;

		} //namespace model
	} //namespace core
} //namespace repo
