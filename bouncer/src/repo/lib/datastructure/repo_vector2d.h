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

#include "../../repo_bouncer_global.h"

#include <vector>
#include <sstream>

namespace repo{
	namespace lib{
		template <class T>
		class  REPO_API_EXPORT _RepoVector2D
		{
		public:

			_RepoVector2D<T>(const T &x = 0, const T &y = 0) : x(x), y(y) {}
			_RepoVector2D<T>(const std::vector<T > &v)
			{
				x = (v.size() > 0) ? v[0] : 0;
				y = (v.size() > 1) ? v[1] : 0;
			}

			inline _RepoVector2D<T>& operator=(const _RepoVector2D<T> &other)
			{
				x = other.x;
				y = other.y;
				return *this;
			}

			std::string toString() const
			{
				std::stringstream sstr;
				sstr << "[" << x << ", " << y << "]";
				return sstr.str();
			}

			std::vector<T> toStdVector() const
			{
				return{ x, y };
			}

			T x, y;
		};

	}
}
