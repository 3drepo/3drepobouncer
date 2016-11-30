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

#include "repo_bson_database_stats.h"

using namespace repo::core::model;

DatabaseStats::DatabaseStats() : RepoBSON()
{
}

DatabaseStats::~DatabaseStats()
{
}

std::string repo::core::model::DatabaseStats::getDatabase() const
{
    std::string db;
    if (hasField("db"))
            db = getField("db").String();
    return db;
}

uint64_t repo::core::model::DatabaseStats::getCollectionsCount() const
{
    return getSize("collections");
}

uint64_t repo::core::model::DatabaseStats::getObjectsCount() const
{
    return getSize("objects");
}

uint64_t repo::core::model::DatabaseStats::getAvgObjSize() const
{
    return getSize("avgObjSize");
}

uint64_t repo::core::model::DatabaseStats::getDataSize() const
{
    return getSize("dataSize");
}

uint64_t repo::core::model::DatabaseStats::getStorageSize() const
{
    return getSize("storageSize");
}

uint64_t repo::core::model::DatabaseStats::getNumExtents() const
{
    return getSize("numExtents");
}

uint64_t repo::core::model::DatabaseStats::getIndexesCount() const
{
    return getSize("indexes");
}

uint64_t repo::core::model::DatabaseStats::getIndexSize() const
{
    return getSize("indexSize");
}

uint64_t repo::core::model::DatabaseStats::getFileSize() const
{
    return getSize("fileSize");
}

uint64_t repo::core::model::DatabaseStats::getNsSizeMB() const
{
    return getSize("nsSizeMB");
}

std::string repo::core::model::DatabaseStats::getDataFileVersion() const
{
        std::string dfv;
        if (hasField("dataFileVersion"))
                dfv = getField("dataFileVersion").String();
        return dfv;
}

uint64_t repo::core::model::DatabaseStats::getDataFileVersionMajor() const
{
    return getSize("dataFileVersion.major");
}

uint64_t repo::core::model::DatabaseStats::getDataFileVersionMinor() const
{
    return getSize("dataFileVersion.minor");
}

std::string repo::core::model::DatabaseStats::getExtentFreeList() const
{
        std::string efl;
        if (hasField("extentFreeList"))
                efl = getField("extentFreeList").String();
        return efl;
}

uint64_t repo::core::model::DatabaseStats::getExtentFreeListNum() const
{
    return getSize("extentFreeList.num");
}

uint64_t repo::core::model::DatabaseStats::getExtentFreeListSize() const
{
    return getSize("extentFreeList.size");
}

uint64_t repo::core::model::DatabaseStats::getSize(const std::string& name) const
{
        uint64_t size = 0;
        if (hasField(name))
                size = getField(name).safeNumberLong();
        return size;
}
