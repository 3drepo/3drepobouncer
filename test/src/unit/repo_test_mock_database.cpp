/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo_test_mock_database.h"
#include "repo_test_utils.h"

#include "repo/core/model/bson/repo_bson.h"
#include "repo/core/handler/database/repo_query.h"
#include "repo/lib//datastructure/repo_variant.h"
#include "repo/lib/datastructure/repo_variant_utils.h"

#include <vector>

using namespace repo::core::handler::database;
using namespace testing;

struct MockDatabase::Index
{
	std::unordered_map<repo::lib::RepoUUID, std::vector<const repo::core::model::RepoBSON*>, repo::lib::RepoUUIDHasher> indexed;

	Index(const std::vector<repo::core::model::RepoBSON>& documents, std::string field){
		for (auto& doc : documents) {
			auto u = doc.getUUIDField(field);
			indexed[u].push_back(&doc);
		}
	}

	void find(repo::lib::RepoVariant v, std::vector<repo::core::model::RepoBSON>& results) {	
		auto& docs = indexed[boost::get<repo::lib::RepoUUID>(v)];
		for(auto& d : docs) {
			results.push_back(*d);
		}
	}
};

struct MockQueryFilterVisitor
{
	std::unordered_map<std::string, MockDatabase::Index*>* indexes = nullptr;
	std::vector<repo::core::model::RepoBSON> results;

	void operator() (const query::Eq& n)
	{
		auto index = indexes->operator[](n.field);
		if(!index) {
			throw std::runtime_error("MockDatabase does not have an index for field. Ensure the indexes are built for this field. Indexes are definde statically in setDocuments.");
		}
		for (auto& v : n.values) {
			index->find(v, results);
		}
	}

	void operator() (const query::Exists& n)
	{
		throw MockDatabase::MockDatabaseMethodNotImplemented();
	}

	void operator() (const query::Or& n)
	{
		throw MockDatabase::MockDatabaseMethodNotImplemented();
	}

	void operator() (const query::RepoQueryBuilder& n)
	{
		throw MockDatabase::MockDatabaseMethodNotImplemented();
	}

	void operator() (const query::RepoProjectionBuilder& n)
	{
		throw MockDatabase::MockDatabaseMethodNotImplemented();
	}
};

MockDatabase::MockDatabase()
	: repo::core::handler::AbstractDatabaseHandler(16000) 
{
	projectSettings = testing::makeProjectSettings("Mock Database Project");
}

repo::core::model::RepoBSON MockDatabase::findOneByCriteria(
	const std::string& database,
	const std::string& collection,
	const repo::core::handler::database::query::RepoQuery& criteria,
	const std::string& sortField)
{
	if (collection == std::string("settings")) {
		return projectSettings;
	}
	else {
		MockQueryFilterVisitor visitor;
		visitor.indexes = &indexes;
		std::visit(visitor, criteria);
		return visitor.results.size() ? visitor.results[0] : repo::core::model::RepoBSON();
	}
}

repo::core::model::RepoBSON MockDatabase::findOneBySharedID(
	const std::string& database,
	const std::string& collection,
	const repo::lib::RepoUUID& uuid,
	const std::string& sortField)
{
	throw MockDatabaseMethodNotImplemented();
}

repo::core::model::RepoBSON MockDatabase::findOneByUniqueID(
	const std::string& database,
	const std::string& collection,
	const repo::lib::RepoUUID& id)
{
	throw MockDatabaseMethodNotImplemented();
}

repo::core::model::RepoBSON MockDatabase::findOneByUniqueID(
	const std::string& database,
	const std::string& collection,
	const std::string& id)
{
	throw MockDatabaseMethodNotImplemented();
}

void MockDatabase::dropDocument(
	const repo::core::model::RepoBSON bson,
	const std::string& database,
	const std::string& collection)
{
	throw MockDatabaseMethodNotImplemented();
}

struct VectorCursorIterator : public Cursor::Iterator::Impl
{
	std::vector<repo::core::model::RepoBSON> data;
	size_t index;
	
	VectorCursorIterator(std::vector<repo::core::model::RepoBSON> data) : 
		data(data),
		index(0)
	{
	}

	VectorCursorIterator(size_t end) :
		index(end)
	{
	}

	const repo::core::model::RepoBSON operator*()
	{
		return data[index];
	}

	void operator++()
	{
		index++;
	}

	bool operator!=(const Cursor::Iterator::Impl* other)
	{
		return index != static_cast<const VectorCursorIterator*>(other)->index;
	}
};

struct VectorCursor : public repo::core::handler::database::Cursor
{
	std::vector<repo::core::model::RepoBSON> data;

	VectorCursorIterator _begin;
	VectorCursorIterator _end;

	VectorCursor(std::vector<repo::core::model::RepoBSON> results) :
		_begin(VectorCursorIterator(results)),
		_end(VectorCursorIterator(results.size()))
	{
	}

	virtual Cursor::Iterator begin()
	{
		return Cursor::Iterator(&_begin);
	}

	virtual Cursor::Iterator end()
	{
		return Cursor::Iterator(&_end); // Passing a nullptr signifies that this is the end iterator
	}
};

std::unique_ptr<repo::core::handler::database::Cursor> MockDatabase::findCursorByCriteria(
	const std::string& database,
	const std::string& collection,
	const repo::core::handler::database::query::RepoQuery& filter,
	const repo::core::handler::database::query::RepoQuery& projection)
{
	// The mock ignores the projection for now.

	MockQueryFilterVisitor visitor;
	visitor.indexes = &indexes;
	std::visit(visitor, filter);

	return std::make_unique<VectorCursor>(visitor.results);
}

void MockDatabase::setDocuments(
	const std::vector<repo::core::model::RepoBSON>& documents)
{
	this->documents = documents;

	// The MockDatabase is expected to handle many millions of documents, so we
	// do need an indexing system.
	// Only a subset of fields are indexed, and these are defined at compile time
	// based on what we know will be accessed by the unit tests, below.

	for (auto i : indexes) {
		delete i.second;
	}
	indexes.clear();
	indexes[REPO_NODE_LABEL_ID] = new MockDatabase::Index(this->documents, REPO_NODE_LABEL_ID);
	indexes[REPO_NODE_LABEL_SHARED_ID] = new MockDatabase::Index(this->documents, REPO_NODE_LABEL_SHARED_ID);
}