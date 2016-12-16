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
		class  REPO_API_EXPORT _RepoVector3D
		{
		public:

			_RepoVector3D<T>(const T &x = 0, const T &y = 0, const T &z = 0) : x(x), y(y), z(z) {}
			_RepoVector3D<T>(const std::vector<T > &v)
			{
				x = (v.size() > 0) ? v[0] : 0;
				y = (v.size() > 1) ? v[1] : 0;
				z = (v.size() > 2) ? v[2] : 0;
			}

			_RepoVector3D<T> crossProduct(const _RepoVector3D<T> &vec)
			{
				_RepoVector3D<T> product;
				product.x = (y * vec.z) - (z * vec.y);
				product.y = (z * vec.x) - (x * vec.z);
				product.z = (x * vec.y) - (y * vec.x);
				return product;
			}

			T dotProduct(const _RepoVector3D<T> &vec)
			{
				return x*vec.x + y*vec.y + z*vec.z;
			}


			void normalize()
			{
				T length = std::sqrt(x*x + y*y + z*z);

				if (length > 0)
				{
					x /= length;
					y /= length;
					z /= length;
				}
			}

			inline _RepoVector3D<T>& operator=(const _RepoVector3D<T> &other)
			{
				x = other.x;
				y = other.y;
				z = other.z;
				return *this;
			}

			std::string toString() const
			{
				std::stringstream sstr;
				sstr.precision(17);
				sstr << "[" << std::fixed << x << ", " << std::fixed << y << ", " << std::fixed << z << "]";
				return sstr.str();
			}

			std::vector<T> toStdVector() const
			{
				return{ x, y, z };
			}

			T x, y, z;
		};
	}
}
