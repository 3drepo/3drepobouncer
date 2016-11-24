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
                        class REPO_API_EXPORT DatabaseStats :
				public RepoBSON
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

                                //! Returns database.
                                std::string getDatabase() const;

                                //! Returns a count of the number of collections in the database.
                                uint64_t getCollectionsCount() const;

                                /**
                                 * Returns a count of the number of objects
                                 * (i.e. documents) in the database across all
                                 * collections.
                                 */
                                uint64_t getObjectsCount() const;

                                /**
                                 * The average size of each document in bytes.
                                 * This is the dataSize divided by the number
                                 * of documents.
                                 */
                                uint64_t getAvgObjSize() const;

                                /**
                                 * Returns the total size in bytes of the uncompressed
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
                                 */
                                uint64_t getDataSize() const;

                                /**
                                 * Returns the total amount of space in bytes
                                 * allocated to collections in this database for
                                 * document storage. The scale argument affects
                                 * this value. The storageSize does not decrease
                                 * as you remove or shrink documents. This value
                                 * may be smaller than dataSize for databases
                                 * using the WiredTiger storage engine with
                                 * compression enabled.
                                 * @return
                                 */
                                uint64_t getStorageSize() const;

                                /**
                                 * Returns a count of the number of extents in
                                 * the database across all collections.
                                 */
                                uint64_t getNumExtents() const;

                                /**
                                 * Returns a count of the total number of
                                 * indexes across all collections in the database.
                                 */
                                uint64_t getIndexesCount() const;

                                /**
                                 * Returns the total size in bytes of all
                                 * indexes created on this database. The scale
                                 * arguments affects this value.
                                 */
                                uint64_t getIndexSize() const;

                                /**
                                 * Returns the total size in bytes of the data
                                 * files that hold the database. This value
                                 * includes preallocated space and the padding
                                 * factor. The value of fileSize only reflects
                                 * the size of the data files for the database
                                 * and not the namespace file.
                                 * The scale argument affects this value. Only
                                 * present when using the mmapv1 storage engine.
                                 */
                                uint64_t getFileSize() const;

                                /**
                                 * Returns the total size of the namespace files
                                 * (i.e. that end with .ns) for this database.
                                 * You cannot change the size of the namespace
                                 * file after creating a database, but you can
                                 * change the default size for all new namespace
                                 * files with the nsSize runtime option.
                                 * Only present when using the mmapv1 storage engine.
                                 */
                                uint64_t getNsSizeMB() const;

                                /**
                                 * Returns document that contains information
                                 * about the on-disk format of the data files
                                 * for the database. Only present when using the
                                 * mmapv1 storage engine.
                                 */
                                std::string getDataFileVersion() const;

                                /**
                                 * The major version number for the on-disk format
                                 * of the data files for the database. Only
                                 * present when using the mmapv1 storage engine.
                                 */
                                uint64_t getDataFileVersionMajor() const;

                                /**
                                 * The minor version number for the on-disk
                                 * format of the data files for the database.
                                 * Only present when using the mmapv1 storage
                                 * engine.
                                 */
                                uint64_t getDataFileVersionMinor() const;

                                std::string getExtentFreeList() const;

                                /**
                                 * Number of extents in the freelist. Only
                                 * present when using the mmapv1 storage engine.
                                 */
                                uint64_t getExtentFreeListNum() const;

                                /**
                                 * Total size of the extents on the freelist.
                                 * The scale argument affects this value. Only
                                 * present when using the mmapv1 storage engine.
                                 */
                                uint64_t getExtentFreeListSize() const;

                                uint64_t getSize(const std::string& name) const;

			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
