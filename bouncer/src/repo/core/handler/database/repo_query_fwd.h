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

#include <variant>

namespace repo {
	namespace core {
		namespace handler {
			namespace database{
				namespace query {
					class Eq;
					class Exists;
					class Or;
					class RepoQueryBuilder;

					using RepoQuery = std::variant<Eq, Exists, Or, RepoQueryBuilder>;
				}
			}
		}
	}
}