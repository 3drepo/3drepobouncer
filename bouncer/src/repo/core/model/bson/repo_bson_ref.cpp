/**
*  Copyright (C) 2019 3D Repo Ltd
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

#include "repo_bson_ref.h"
#include "repo_bson_builder.h"
#include "repo/lib/repo_exception.h"

using namespace repo::core::model;

const std::string RepoRef::REPO_REF_TYPE_FS = "fs";
const std::string RepoRef::REPO_REF_TYPE_UNKNOWN = "unknown";

std::string RepoRef::convertTypeAsString(const RefType &type) {
	switch (type) {
	case RefType::FS:
		return REPO_REF_TYPE_FS;
	default:
		return REPO_REF_TYPE_UNKNOWN;
	}
}

RepoRef::RepoRef(
	const std::string& link,
	const size_t& size,
	const Metadata& metadata
)
	:type(RefType::FS),
	size(size),
	link(link),
	metadata(metadata)
{
}

RepoRef::RepoRef(RepoBSON bson)
{
	type = RefType::UNKNOWN;
	size = bson.getLongField(REPO_REF_LABEL_SIZE);
	link = bson.getStringField(REPO_REF_LABEL_LINK);
	if (bson.hasField(REPO_NODE_LABEL_NAME))
	{
		name = bson.getStringField(REPO_NODE_LABEL_NAME);
	}
	auto typeStr = bson.getStringField(REPO_REF_LABEL_TYPE);
	if (typeStr == REPO_REF_TYPE_FS)
	{
		type = RefType::FS;
	}
}

RepoRef::operator RepoBSON() const
{
	RepoBSONBuilder builder;
	serialise(builder);
	return builder.obj();
}

void RepoRef::serialise(RepoBSONBuilder& builder) const
{
	builder.append(REPO_REF_LABEL_TYPE, RepoRef::convertTypeAsString(type));
	builder.append(REPO_REF_LABEL_LINK, link);
	builder.append(REPO_REF_LABEL_SIZE, (int64_t)size);
	if (!name.empty())
	{
		builder.append(REPO_NODE_LABEL_NAME, name);
	}

	// For Ref documents, each metadata entry is added as a top-level field
	for (const auto& pair : metadata)
	{
		builder.appendRepoVariant(pair.first, pair.second);
	}
}

std::string RepoRef::getFileName() const {
	return name;
}

std::string RepoRef::getRefLink() const {
	return link;
}

size_t RepoRef::getFileSize() const {
	return size;
}

RepoRef::RefType RepoRef::getType() const {
	return type;
}

template<class IdType>
IdType RepoRefT<IdType>::getId() const
{
	return id;
}

/* Explicit specialisations so we use the correct methods from RepoBSON */

template<> RepoRefT<std::string>::RepoRefT(RepoBSON bson)
	: RepoRef(bson)
{
	id = bson.getStringField(REPO_LABEL_ID);
}

template<> RepoRefT<repo::lib::RepoUUID>::RepoRefT(RepoBSON bson)
	: RepoRef(bson)
{
	id = bson.getUUIDField(REPO_LABEL_ID);
}

template<class IdType>
RepoRefT<IdType>::RepoRefT(
	const IdType& id,
	const std::string& link,
	const size_t& size,
	const Metadata& metadata
)
	:RepoRef(link, size, metadata)
{
	this->id = id;
}

/* Declare the specialisations of the two supported types for exported methods */

template RepoRefT<std::string>::RepoRefT(
	const std::string& id,
	const std::string& link,
	const size_t& size,
	const Metadata& metadata);
template std::string RepoRefT<std::string>::getId() const;

template RepoRefT<repo::lib::RepoUUID>::RepoRefT(
	const repo::lib::RepoUUID& id,
	const std::string& link,
	const size_t& size,
	const Metadata& metadata);
template repo::lib::RepoUUID RepoRefT<repo::lib::RepoUUID>::getId() const;

/* Override of serialise to append the Id. This does not require explicit
 * specialisations to be declared because it is only used inside the module.
 */

template<class IdType>
void RepoRefT<IdType>::serialise(RepoBSONBuilder& builder) const
{
	RepoRef::serialise(builder);
	builder.append(REPO_LABEL_ID, id);
}

