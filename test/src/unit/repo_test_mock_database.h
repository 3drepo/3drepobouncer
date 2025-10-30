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

#pragma once

#include "repo/core/handler/repo_database_handler_abstract.h"
#include "repo/core/model/bson/repo_bson.h"

namespace testing {

	struct MockDatabase : public repo::core::handler::AbstractDatabaseHandler
	{
		class MockDatabaseMethodNotImplemented : public std::logic_error
		{
		public:
			// Every stub inside the mock database handler or its dependencies, such as
			// cursors or the file manager should throw this.
			// Place a breakpoint on this constructor to immediately see which 
			// unimplemented methods are being called.
			MockDatabaseMethodNotImplemented() : std::logic_error("Mock database method not yet implemented") {};
		};

		// A mock database that can be used to test the clash pipeline without actually
		// going to Mongo, which would be prohibitive for test with many permutations.

		MockDatabase();

		std::vector<repo::core::model::RepoBSON> documents;

		struct Index;
		std::unordered_map<std::string, Index*> indexes;

		repo::core::model::RepoBSON projectSettings;

		void setDocuments(const std::vector<repo::core::model::RepoBSON>& documents);

		virtual std::vector<repo::core::model::RepoBSON>
			getAllFromCollectionTailable(
				const std::string& database,
				const std::string& collection,
				const uint64_t& skip = 0,
				const uint32_t& limit = 0,
				const std::list<std::string>& fields = std::list<std::string>(),
				const std::string& sortField = std::string(),
				const int& sortOrder = -1) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual std::list<std::string> getCollections(
			const std::string& database) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual void createIndex(
			const std::string& database,
			const std::string& collection,
			const repo::core::handler::database::index::RepoIndex&) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual void createIndex(
			const std::string& database,
			const std::string& collection,
			const repo::core::handler::database::index::RepoIndex& index,
			bool sparse) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual void insertDocument(
			const std::string& database,
			const std::string& collection,
			const repo::core::model::RepoBSON& obj) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual void insertManyDocuments(
			const std::string& database,
			const std::string& collection,
			const std::vector<repo::core::model::RepoBSON>& obj,
			const Metadata& metadata = {}) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual void upsertDocument(
			const std::string& database,
			const std::string& collection,
			const repo::core::model::RepoBSON& obj,
			const bool& overwrite) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual void dropCollection(
			const std::string& database,
			const std::string& collection) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual void dropDocument(
			const repo::core::model::RepoBSON bson,
			const std::string& database,
			const std::string& collection) override;

		virtual std::vector<repo::core::model::RepoBSON> findAllByCriteria(
			const std::string& database,
			const std::string& collection,
			const repo::core::handler::database::query::RepoQuery& criteria,
			const bool loadBinaries = false) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual std::vector<repo::core::model::RepoBSON> findAllByCriteria(
			const std::string& database,
			const std::string& collection,
			const repo::core::handler::database::query::RepoQuery& filter,
			const repo::core::handler::database::query::RepoQuery& projection,
			const bool loadBinaries = false) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual std::unique_ptr<repo::core::handler::database::Cursor> findCursorByCriteria(
			const std::string& database,
			const std::string& collection,
			const repo::core::handler::database::query::RepoQuery& criteria) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual std::unique_ptr<repo::core::handler::database::Cursor> findCursorByCriteria(
			const std::string& database,
			const std::string& collection,
			const repo::core::handler::database::query::RepoQuery& filter,
			const repo::core::handler::database::query::RepoQuery& projection) override;

		virtual repo::core::model::RepoBSON findOneByCriteria(
			const std::string& database,
			const std::string& collection,
			const repo::core::handler::database::query::RepoQuery& criteria,
			const std::string& sortField = "") override;

		virtual repo::core::model::RepoBSON findOneBySharedID(
			const std::string& database,
			const std::string& collection,
			const repo::lib::RepoUUID& uuid,
			const std::string& sortField) override;

		virtual repo::core::model::RepoBSON findOneByUniqueID(
			const std::string& database,
			const std::string& collection,
			const repo::lib::RepoUUID& id) override;

		virtual repo::core::model::RepoBSON findOneByUniqueID(
			const std::string& database,
			const std::string& collection,
			const std::string& id) override;

		virtual size_t count(
			const std::string& database,
			const std::string& collection,
			const repo::core::handler::database::query::RepoQuery& criteria) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual std::unique_ptr<repo::core::handler::database::BulkWriteContext> getBulkWriteContext(
			const std::string& database,
			const std::string& collection) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual void setFileManager(std::shared_ptr<repo::core::handler::fileservice::FileManager> manager) {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual std::shared_ptr<repo::core::handler::fileservice::FileManager> getFileManager() {
			throw MockDatabaseMethodNotImplemented();
		}

		virtual void loadBinaryBuffers(
			const std::string& database,
			const std::string& collection,
			repo::core::model::RepoBSON& bson) {
			// The mock database never unloads the binaries of its internal bsons.
			// Todo: see if we want to implement this to exercise the loading/unloading paths...
		}
	};
}