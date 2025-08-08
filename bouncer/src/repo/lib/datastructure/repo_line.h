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

#include "repo_vector2d.h"
#include "repo_vector3d.h"

namespace repo{
	namespace lib{
        template<typename T>
        struct REPO_API_EXPORT _RepoLine
        {
            repo::lib::_RepoVector3D<T> start;
            repo::lib::_RepoVector3D<T> end;
            
            _RepoLine() = default;

            _RepoLine(
                const repo::lib::_RepoVector3D<T>& start, 
                const repo::lib::_RepoVector3D<T>& end)
                : start(start), end(end)
            {
            }

            repo::lib::_RepoVector3D<T> d() const {
                return end - start;
            }

            T magnitude() const {
                return (end - start).norm();
            }

            repo::lib::_RepoVector3D<T> center() {
                return (end + start) * 0.5;
            }

            void swap() {
                std::swap(start, end);
            }

            static _RepoLine Max()
            {
				return _RepoLine<T>(repo::lib::_RepoVector3D<T>(-DBL_MAX, -DBL_MAX, -DBL_MAX), repo::lib::_RepoVector3D<T>(DBL_MAX, DBL_MAX, DBL_MAX));
            }
        };

		using RepoLine = _RepoLine<double>;
	}
}
