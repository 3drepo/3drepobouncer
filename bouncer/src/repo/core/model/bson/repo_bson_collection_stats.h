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
* Repo Collection stats. A RepoBSON that stores the statistiscal information
* of a collection
*/

#pragma once
#include "repo_bson.h"

#include "../../../repo_bouncer_global.h"

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT CollectionStats :
				public RepoBSON
			{
			public:
				/**
				* Default Constructor
				*/
				CollectionStats();

				CollectionStats(RepoBSON bson) : RepoBSON(bson) {};

				/**
				* Destructor
				*/
				~CollectionStats();

				//--------------------------------------------------------------------------

				/**
				* Get the actual size on disk.
				* @returns the size of disk in bytes
				*/
				uint64_t getActualSizeOnDisk() const;

				/*
				* Get the database name of this collection stats
				* @returns the name of the database
				*/
				std::string getDatabase() const;

				/**
				* Get the number of objects or documents in this collection.
				* @returns number of documents
				*/
				uint64_t getCount() const;

				/*
				* Get the name of the collection
				* @returns collection name
				*/
				std::string getCollection() const;

				/*
				* Get the namespace from the stats (database.collection)
				* @returns namespace string.
				*/
				std::string getNs() const;

				/**
				* get the total size of all records in a collection. This value does
				* not include the record header, which is 16 bytes per record, but does
				* include the record’s padding. Additionally, size does not include
				* the size of any indexes associated with the collection, which the
				* totalIndexSize field reports.
				*
				* See http://docs.mongodb.org/manual/reference/command/collStats/#collStats.size
				* @returns the size of all records in collection in bytes
				*/
				uint64_t getSize() const;

				/**
				* get the total amount of storage allocated to this collection for
				* document storage.
				*
				* See: http://docs.mongodb.org/manual/reference/command/collStats/#collStats.storageSize
				* @returns the amount of storage allocated in bytes
				*/
				uint64_t getStorageSize() const;

				/**
				* get the total size of all indices.
				* @return size of all indices
				*/
				uint64_t getTotalIndexSize() const;

			private:

				/*
				* Get the size of the field given
				* @params name name of the field
				* @returns the size of the field given, 0 if not found
				*/
				uint64_t getSize(const std::string& name) const;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
