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

#include "repo_expressions.h"

#include "repo/core/model/bson/repo_bson.h"
#include "repo/core/model/bson/repo_bson_builder.h"

using namespace repo::core::handler::database;
using namespace repo::core::model;

query::RepoQuery::operator repo::core::model::RepoBSON() const
{
	RepoBSONBuilder builder;
	visit(builder);
	return builder.obj();
}

void query::Or::visit(repo::core::model::RepoBSONBuilder& builder) const
{
	std::vector<RepoBSON> results;
	for (auto& q : conditions)
	{
		results.push_back((RepoBSON)*q);
	}
	builder.appendArray("$or", results);
}

template<typename T>
query::Eq<T>::Eq(std::string field, const T& exactly)
	:field(field), values({exactly})
{
}

template<typename T>
query::Eq<T>::Eq(const char*& field, const T& exactly)
	: values({ exactly })
{
	this->field = field;
}

template<typename T>
query::Eq<T>::Eq(std::string field, std::vector<T> oneOf)
	: field(field), values(oneOf)
{
}

template<typename T>
void query::Eq<T>::visit(repo::core::model::RepoBSONBuilder& builder) const
{
	if (values.size() > 1)
	{
		RepoBSONBuilder setBuilder;
		setBuilder.append("$in", values);
		builder.append(field, setBuilder.obj());
	}
	else if(values.size() == 1)
	{
		builder.append(field, values[0]);
	}
}

query::Exists::Exists(std::string field, bool exists)
	:field(field), exists(exists)
{
}

void query::Exists::visit(model::RepoBSONBuilder& builder) const
{
	RepoBSONBuilder criteria;
	criteria.append("$exists", exists);
	builder.append(field, criteria.obj());
}

void query::RepoQueryBuilder::visit(RepoBSONBuilder& builder) const
{
	for (auto& q : conditions) {
		q->visit(builder);
	}
}

index::Ascending::Ascending(std::vector<std::string> fields)
	:fields(fields)
{
}

index::Ascending::operator repo::core::model::RepoBSON() const
{
	RepoBSONBuilder builder;
	for (auto& f : fields)
	{
		builder.append(f, 1);
	}
	return builder.obj();
}

index::Descending::Descending(std::vector<std::string> fields)
	:fields(fields)
{
}

index::Descending::operator repo::core::model::RepoBSON() const
{
	RepoBSONBuilder builder;
	for (auto& f : fields)
	{
		builder.append(f, -1);
	}
	return builder.obj();
}

// Template instantations for the generic types; these will be ones that the
// database can effectively perform comparisons with during the query, so
// will predominantly be primitives, though more can be added here when
// necessary.

template class query::Eq<repo::lib::RepoUUID>;
template class query::Eq<std::string>;
template class query::Eq<int>;

