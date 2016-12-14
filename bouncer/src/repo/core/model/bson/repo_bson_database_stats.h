/**
*  Copyright (C) 2016 3D Repo Ltd
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
* Repo Database stats. A RepoBSON that stores the statistiscal information
* of a d
*/

#pragma once
#include "repo_bson.h"

#include "../../../repo_bouncer_global.h"

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT DatabaseStats : public RepoBSON
			{
			public:
				/**
				* Default Constructor
				*/
				DatabaseStats();

				DatabaseStats(RepoBSON bson) : RepoBSON(bson) {}

				/**
				* Destructor
				*/
				~DatabaseStats();

				//--------------------------------------------------------------------------

				/*
				* Get the database name of this collection stats
				* @returns the name of the database
				*/
				std::string getDatabase() const;

				/*
				* Get the number of collections in the database.
				* @returns the number of collections in this database
				*/
				uint64_t getCollectionsCount() const;

				/**
				 * Get the number of objects
				 * (i.e. documents) in the database across all
				 * collections.
				 * @returns returns the number of documents within the database
				 */
				uint64_t getObjectsCount() const;

				/**
				 * This is the dataSize divided by the number
				 * of documents.
				 * @returns The average size of each document in bytes.
				 *
				 */
				uint64_t getAvgObjSize() const;

				/**
				 * Get the total size in bytes of the uncompressed
				 * data held in this database. The scale argument
				 * affects this value. The dataSize will
				 * decrease when you remove documents.
				 * For databases using the MMAPv1 storage engine,
				 * dataSize includes preallocated space and the
				 * padding factor. The dataSize will not
				 * decrease when documents shrink.
				 * For databases using the WiredTiger storage
				 * engine, dataSize may be larger than
				 * storageSize if compression is enabled.
				 * The dataSize will decrease when documents
				 * shrink.
				 * @return data size in bytes
				 */
				uint64_t getDataSize() const;

				/**
				 * the total amount of space in bytes
				 * allocated to collections in this database for
				 * document storage. The scale argument affects
				 * this value. The storageSize does not decrease
				 * as you remove or shrink documents. This value
				 * may be smaller than dataSize for databases
				 * using the WiredTiger storage engine with
				 * compression enabled.
				 * @returns the amount of storage allocated in bytes
				 */
				uint64_t getStorageSize() const;

				/**
				 * Get a count of the number of extents in
				 * the database across all collections.
				 * @returns the number of extents
				 */
				uint64_t getNumExtents() const;

				/**
				* get the total count of all indices.
				* @return count of all indices
				*/
				uint64_t getIndexesCount() const;

				/**
				* get the total size of all indices.
				* @return size of all indices
				*/
				uint64_t getIndexSize() const;

				/**
				 * Get the total size in bytes of the data
				 * files that hold the database. This value
				 * includes preallocated space and the padding
				 * factor. The value of fileSize only reflects
				 * the size of the data files for the database
				 * and not the namespace file.
				 * The scale argument affects this value. Only
				 * present when using the mmapv1 storage engine.
				 * @returns the total file size
				 */
				uint64_t getFileSize() const;

				/**
				 * Get  the total size of the namespace files
				 * (i.e. that end with .ns) for this database.
				 * You cannot change the size of the namespace
				 * file after creating a database, but you can
				 * change the default size for all new namespace
				 * files with the nsSize runtime option.
				 * Only present when using the mmapv1 storage engine.
				 * @returns the namespace size
				 */
				uint64_t getNsSizeMB() const;

				/**
				 * GetThe major version number for the on-disk format
				 * of the data files for the database. Only
				 * present when using the mmapv1 storage engine.
				 * @returns the major number, 0 if not mmapv1
				 */
				uint64_t getDataFileVersionMajor() const;

				/**
				 * Get the minor version number for the on-disk
				 * format of the data files for the database.
				 * Only present when using the mmapv1 storage
				 * engine.
				 * @returns the minor number, 0 if not mmapv1
				 */
				uint64_t getDataFileVersionMinor() const;

				/**
				 * Number of extents in the freelist. Only
				 * present when using the mmapv1 storage engine.
				 * @returns number of extentFreeList
				 */
				uint64_t getExtentFreeListNum() const;

				/**
				 * Total size of the extents on the freelist.
				 * The scale argument affects this value. Only
				 * present when using the mmapv1 storage engine.
				 * @returns the total size of extents on freelist in bytes
				 */
				uint64_t getExtentFreeListSize() const;

			private:
				/*
				* Get the size of the field given
				* @params name name of the field
				* @returns the size of the field given, 0 if not found
				*/
				uint64_t getSize(const std::string& name) const;

				/**
				* Get a number(long) from an embedded bson field
				* @params embeddedObj name of the embedded obj
				* @params name name of the field
				* @returns the value of the embedded bson number, 0 if not found
				*/
				uint64_t getEmbeddedNumber(
					const std::string &embeddedObj,
					const std::string& name) const;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
