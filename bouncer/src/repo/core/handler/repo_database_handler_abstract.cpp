/**
*  Copyright (C) 2015 3D Repo Ltd
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

/**
* Abstract database handler which all database handler needs to inherit from
*/

#include "repo_database_handler_abstract.h"

#include "repo/core/model/bson/repo_bson.h"

const repo::core::model::RepoBSON repo::core::handler::database::Cursor::Iterator::operator*() {
	return impl->operator*();
}

void repo::core::handler::database::Cursor::Iterator::operator++() {
	impl->operator++();
}

bool repo::core::handler::database::Cursor::Iterator::operator!=(const Iterator& other) {
	return impl->operator!=(other.impl);
}

repo::core::handler::database::Cursor::Iterator::Iterator(Impl* impl)
	:impl(impl)
{
}

// This exists so unique_ptr will call any destructors on the base class
repo::core::handler::database::Cursor::~Cursor()
{
}
