/**
*  Copyright (C) 2016 3D Repo Ltd
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

#include "repo_vector.h"

namespace repo{
	namespace lib{
		template<typename T>
        struct REPO_API_EXPORT _RepoTriangle
        {
            repo::lib::_RepoVector3D<T> a;
            repo::lib::_RepoVector3D<T> b;
            repo::lib::_RepoVector3D<T> c;

            _RepoTriangle(
                repo::lib::_RepoVector3D<T> a,
                repo::lib::_RepoVector3D<T> b,
                repo::lib::_RepoVector3D<T> c)
				:a(a), b(b), c(c)
            {
            }
        };

		using RepoTriangle = _RepoTriangle<double>;
	}
}