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

using namespace repo::core::model;

const std::string RepoRef::REPO_REF_TYPE_FS = "fs";
const std::string RepoRef::REPO_REF_TYPE_GRIDFS = "gridfs";
const std::string RepoRef::REPO_REF_TYPE_UNKNOWN = "unknown";

std::string RepoRef::convertTypeAsString(const RefType &type) {
	switch (type) {
	case RefType::GRIDFS:
		return REPO_REF_TYPE_GRIDFS;
	case RefType::FS:
		return REPO_REF_TYPE_FS;
	default:
		return REPO_REF_TYPE_UNKNOWN;
	}
}

std::string RepoRef::getID() const {
	return getStringField(REPO_LABEL_ID);
}

std::string RepoRef::getFileName() const {
	return getStringField(REPO_NODE_LABEL_NAME);
}

std::string RepoRef::getRefLink() const {
	return getStringField(REPO_REF_LABEL_LINK);
}

std::string RepoRef::getFileExtension() const {
	return getStringField(REPO_NODE_LABEL_EXTENSION);
}

RepoRef::RefType RepoRef::getType() const {
	auto typeStr = getStringField(REPO_REF_LABEL_TYPE);
	auto type = RefType::UNKNOWN;
	if (typeStr == REPO_REF_TYPE_FS)
		type = RefType::FS;
	if (typeStr == REPO_REF_TYPE_GRIDFS)
		type = RefType::GRIDFS;

	return type;
}