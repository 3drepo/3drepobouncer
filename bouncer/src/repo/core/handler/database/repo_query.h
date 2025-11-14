/**
*  Copyright (C) 2024 3D Repo Ltd
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
#include "repo/lib/datastructure/repo_variant.h"
#include "repo_query_fwd.h"

#include <vector>
#include <set>
#include <string>
#include <memory>
#include <variant>

namespace repo {
	namespace core {
		namespace model {
			class RepoBSON;
			class RepoBSONBuilder;
		}
		namespace handler {
			class MongoDatabaseHandler;
			namespace database {
				namespace query {

					/*
					 * Defines a set of composable expressions that any database handler must
					 * accept in order to filter and otherwise query the collections.
					 */

					class REPO_API_EXPORT Eq
					{
					public:
						Eq(std::string field, const std::vector<repo::lib::RepoVariant>& oneOf) :
							field(field),
							values(oneOf)
						{
						}

						template<typename T>
						Eq(std::string field, const std::vector<T>& oneOf) :
							field(field)
						{
							for (auto& v : oneOf) {
								values.push_back(v);
							}
						}

						template<typename T>
						Eq(std::string field, const std::set<T>& oneOf) :
							field(field)
						{
							for (auto& v : oneOf) {
								values.push_back(v);
							}
						}

						template<typename T>
						Eq(const char* field, const std::vector<T>& oneOf) :
							field(field)
						{
							for (auto& v : oneOf) {
								values.push_back(v);
							}
						}

						template<typename T>
						Eq(const char* field, const std::set<T>& oneOf) :
							field(field)
						{
							for (auto& v : oneOf) {
								values.push_back(v);
							}
						}

						template<typename T>
						Eq(const std::string& field, const T& value)
							:field(field)
						{
							values.push_back(value);
						}

						template<typename T>
						Eq(const char* field, const T& value)
							: field(std::string(field))
						{
							values.push_back(value);
						}

						std::string field;
						std::vector<repo::lib::RepoVariant> values;
					};

					class REPO_API_EXPORT Exists
					{
					public:
						Exists(std::string field, bool exists)
							:field(field),
							exists(exists)
						{
						}

						std::string field;
						bool exists;
					};

					class REPO_API_EXPORT Or
					{
					public:
						template<typename ...Arguments>
						Or(Arguments... anyOf) {
							append(anyOf...); // Will call one of the two overloads depending on the number of Arguments
						}

						template <typename T>
						void append(const T& t) {
							conditions.push_back(t);
						}

						// This call will be recursive using pack expansion, until there is only
						// one parameter left and the single-argument method is called.

						template <typename First, typename ...Rest>
						void append(const First& first, const Rest&... rest) {
							append(first);
							append(rest...);
						}

						std::vector<RepoQuery> conditions;
					};

					/*
					 * Convenience type to help build more complex composite queries in multiple
					 * stages.
					 */
					class REPO_API_EXPORT RepoQueryBuilder
					{
					public:
						template<typename T>
						void append(const T& q) {
							conditions.push_back(q);
						};

						std::vector<RepoQuery> conditions;
					};

					/*
					* Used with a query update call to add new UUIDs to the parents array of an
					* existing document by unique id.
					*/
					class REPO_API_EXPORT AddParent
					{
					public:
						AddParent(const repo::lib::RepoUUID& uniqueId, std::vector<repo::lib::RepoUUID> parentIds) :
							uniqueId(uniqueId),
							parentIds(parentIds.begin(), parentIds.end())
						{
						}

						AddParent(const repo::lib::RepoUUID& uniqueId, std::set<repo::lib::RepoUUID> parentIds) :
							uniqueId(uniqueId),
							parentIds(parentIds)
						{
						}

						AddParent(const repo::lib::RepoUUID& uniqueId, repo::lib::RepoUUID parentId):
							uniqueId(uniqueId),
							parentIds({ parentId })
						{
						}

						repo::lib::RepoUUID uniqueId;
						std::set<repo::lib::RepoUUID> parentIds;
					};

					/*
					 * Convenience type to build a projection in multiple stages.
					 */
					class REPO_API_EXPORT RepoProjectionBuilder
					{
					public:
						
						void includeField(std::string field) {
							includedFields.push_back(field);
						};

						void excludeField(std::string field) {
							excludedFields.push_back(field);
						}

						std::vector<std::string> includedFields;
						std::vector<std::string> excludedFields;
					};
				}

				namespace index
				{
					class REPO_API_EXPORT RepoIndex
					{
					protected:
						friend class repo::core::handler::MongoDatabaseHandler;
						virtual operator model::RepoBSON() const = 0;
					};

					class REPO_API_EXPORT Ascending : public RepoIndex
					{
					public:
						Ascending(std::vector<std::string> fields);

					protected:
						std::vector<std::string> fields;

						virtual operator model::RepoBSON() const;
					};

					class REPO_API_EXPORT Descending : public RepoIndex
					{
					public:
						Descending(std::vector<std::string> fields);

					protected:
						std::vector<std::string> fields;

						virtual operator model::RepoBSON() const;
					};
				}
			}
		}
	}
}
