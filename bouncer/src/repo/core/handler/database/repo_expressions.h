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

#include <vector>
#include <string>
#include <memory>

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
					 * Base class for something describing a Query or Filter.Currently this
					 * uses RepoBSONBuilder to build documents that conform to the mongo syntax.
					 * If a better type becomes available, we can change the visitor to
					 * something else.
					 */
					class REPO_API_EXPORT RepoQuery
					{
					public:
						operator model::RepoBSON() const;
						virtual void visit(model::RepoBSONBuilder& builder) const = 0;
					};

					class REPO_API_EXPORT Or : public RepoQuery
					{
					public:
						template<typename ...Arguments>
						Or(Arguments... anyOf) {
							append(anyOf...); // Will call one of the two overloads depending on the number of Arguments
						}

						void visit(model::RepoBSONBuilder& builder) const;

					private:
						// Both Or and RepoQueryBuilder use lists of pointers to copies of their
						// dependents, to avoid object slicing.

						std::vector<std::shared_ptr<RepoQuery>> conditions;

						template <typename T>
						void append(const T& t) {
							conditions.push_back(std::make_shared<T>(t));
						}

						// This call will be recursive using pack expansion, until there is only one
						// parameter left and the single-argument method is called.

						template <typename First, typename ...Rest>
						void append(const First& first, const Rest&... rest) {
							append(first);
							append(rest...);
						}
					};

					template<typename T>
					class REPO_API_EXPORT Eq : public RepoQuery
					{
					public:
						Eq(std::string field, const T& exactly);
						Eq(const char*& field, const T& exactly);
						Eq(std::string field, std::vector<T> oneOf);

						void visit(model::RepoBSONBuilder& builder) const;

					private:
						std::string field;
						std::vector<T> values;
					};

					class REPO_API_EXPORT Exists : public RepoQuery
					{
					public:
						Exists(std::string field, bool exists);

						void visit(model::RepoBSONBuilder& builder) const;

					private:
						std::string field;
						bool exists;
					};

					/*
					 * Convenience type to help build more complex composite queries in
					 * multiple stages.
					 */
					class REPO_API_EXPORT RepoQueryBuilder : public RepoQuery
					{
					public:
						template<typename T>
						void append(const T& q) {
							conditions.push_back(std::make_shared<T>(q));
						}

						virtual void visit(model::RepoBSONBuilder& builder) const;

					private:
						std::vector<std::shared_ptr<RepoQuery>> conditions;
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