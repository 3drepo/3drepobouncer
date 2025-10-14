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

namespace repo {
    namespace lib {
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

            _RepoTriangle& operator+=(const _RepoVector3D<T>& v)
            {
                a += v;
                b += v;
                c += v;
                return *this;
            }

            repo::lib::_RepoVector3D<T> normal() const
            {
                auto ab = b - a;
                auto ac = c - a;
                auto n = ab.crossProduct(ac);
                n.normalize();
                return n;
			}
        };

        using RepoTriangle = _RepoTriangle<double>;

		template<typename N>
        class _RepoMatrix;

		template<typename T, typename N>
        inline _RepoTriangle<T> operator*(const _RepoMatrix<N>& m, const _RepoTriangle<T>& t);
    }
}