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
#include "repo_data_ref.h"
#include "../../../lib/datastructure/repo_uuid.h"

using namespace repo::core::handler::fileservice;

repo::core::model::RepoBSON DataRef::serialise() const {
	repo::core::model::RepoBSONBuilder builder;
	builder.append(REPO_LABEL_BINARY_START, startPos);
	builder.append(REPO_LABEL_BINARY_SIZE, size);
	builder.append(REPO_LABEL_BINARY_FILENAME, fileName);

	return builder.obj();
}

DataRef DataRef::deserialise(const repo::core::model::RepoBSON &serialisedObj) {
	return DataRef(
		serialisedObj.getStringField(REPO_LABEL_BINARY_FILENAME),
		serialisedObj.getIntField(REPO_LABEL_BINARY_START),
		serialisedObj.getIntField(REPO_LABEL_BINARY_SIZE)
	);
}