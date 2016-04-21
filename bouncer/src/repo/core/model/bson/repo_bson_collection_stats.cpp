#include "repo_bson_collection_stats.h"

using namespace repo::core::model;

CollectionStats::CollectionStats() : RepoBSON()
{
}

CollectionStats::~CollectionStats()
{
}

uint64_t repo::core::model::CollectionStats::getActualSizeOnDisk() const
{
	return getSize() + 16 * getCount() + getTotalIndexSize();
}

std::string repo::core::model::CollectionStats::getDatabase() const
{
	return getDatabase(getNs());
}

std::string repo::core::model::CollectionStats::getDatabase(const std::string& ns)
{
	std::string database = ns;
	const char *str = ns.c_str();
	const char *p;
	if (ns.find('.') != std::string::npos && (p = strchr(str, '.')))
		database = std::string(str, p - str);
	return database;
}

uint64_t repo::core::model::CollectionStats::getCount() const
{
	return getSize("count");
}

std::string repo::core::model::CollectionStats::getCollection() const
{
	return getCollection(getNs());
}

std::string repo::core::model::CollectionStats::getCollection(const std::string& ns)
{
	std::string collection = ns;
	const char *p;
	if (ns.find('.') != std::string::npos && (p = strchr(ns.c_str(), '.')))
		collection = p + 1;
	return collection;
}

std::string repo::core::model::CollectionStats::getNs() const
{
	std::string ns;
	if (hasField("ns"))
		ns = getField("ns").String();
	return ns;
}

uint64_t repo::core::model::CollectionStats::getSize(const std::string& name) const
{
	uint64_t size = 0;
	if (hasField(name))
		size = getField(name).safeNumberLong();
	return size;
}

uint64_t repo::core::model::CollectionStats::getSize() const
{
	return getSize("size");
}

uint64_t repo::core::model::CollectionStats::getStorageSize() const
{
	return getSize("storageSize");
}

uint64_t repo::core::model::CollectionStats::getTotalIndexSize() const
{
	return getSize("totalIndexSize");
}