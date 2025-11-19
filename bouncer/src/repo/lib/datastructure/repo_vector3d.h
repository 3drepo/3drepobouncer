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
#include <boost/crc.hpp>

#include <vector>
#include <sstream>
#include <cmath>

// Undefine windef.h's weirdness
#undef min
#undef max

namespace repo{
	namespace lib{
		template <class T>
		class  REPO_API_EXPORT _RepoVector3D
		{
		public:

			_RepoVector3D(const T x = 0, const T y = 0, const T z = 0) : x(x), y(y), z(z) {}
			_RepoVector3D(const std::vector<T> &v)
			{
				x = (v.size() > 0) ? v[0] : 0;
				y = (v.size() > 1) ? v[1] : 0;
				z = (v.size() > 2) ? v[2] : 0;
			}

			template<typename From>
			_RepoVector3D(const _RepoVector3D<From>& v)
			{
				x = v.x;
				y = v.y;
				z = v.z;
			}

			unsigned long checkSum() const
			{
				std::stringstream ss;
				ss.precision(17);

				ss << std::fixed << x << std::fixed << y << std::fixed << z;
				auto stringified = ss.str();

				boost::crc_32_type crc32;
				crc32.process_bytes(stringified.c_str(), stringified.size());
				return crc32.checksum();
			}

			_RepoVector3D<T> crossProduct(const _RepoVector3D<T> &vec)
			{
				_RepoVector3D<T> product;
				product.x = (y * vec.z) - (z * vec.y);
				product.y = (z * vec.x) - (x * vec.z);
				product.z = (x * vec.y) - (y * vec.x);
				return product;
			}

			T dotProduct(const _RepoVector3D<T> &vec) const
			{
				return x*vec.x + y*vec.y + z*vec.z;
			}

			static _RepoVector3D<T> min(const _RepoVector3D<T>& a, const _RepoVector3D<T>& b)
			{
				return _RepoVector3D<T>(
					std::min(a.x, b.x),
					std::min(a.y, b.y),
					std::min(a.z, b.z)
				);
			}

			static _RepoVector3D<T> max(const _RepoVector3D<T>& a, const _RepoVector3D<T>& b)
			{
				return _RepoVector3D<T>(
					std::max(a.x, b.x),
					std::max(a.y, b.y),
					std::max(a.z, b.z)
				);
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

			T norm() const
			{
				return std::sqrt(norm2());
			}

			T norm2() const
			{
				return (x * x + y * y + z * z);
			}

			inline _RepoVector3D<T>& operator=(const _RepoVector3D<T> &other)
			{
				x = other.x;
				y = other.y;
				z = other.z;
				return *this;
			}

			inline _RepoVector3D<T> operator+(const _RepoVector3D<T> &other) const
			{
				return _RepoVector3D<T>(x + other.x, y + other.y, z + other.z);
			}

			inline _RepoVector3D<T> operator+(const T& scalar) const
			{
				return _RepoVector3D<T>(x + scalar, y + scalar, z + scalar);
			}

			inline _RepoVector3D<T>& operator+=(const _RepoVector3D<T>& other)
			{
				x += other.x;
				y += other.y;
				z += other.z;
				return *this;
			}

			inline _RepoVector3D<T> operator-(const _RepoVector3D<T> &other) const
			{
				return _RepoVector3D<T>(x - other.x, y - other.y, z - other.z);
			}

			inline _RepoVector3D<T> operator-(const T& scalar) const
			{
				return _RepoVector3D<T>(x - scalar, y - scalar, z - scalar);
			}

			inline _RepoVector3D<T> operator*(const T &scalar) const
			{
				return _RepoVector3D<T>(x * scalar, y * scalar, z * scalar);
			}

			inline _RepoVector3D<T> operator*(const _RepoVector3D<T>& other) const
			{
				return _RepoVector3D<T>(x * other.x, y * other.y, z * other.z);
			}

			inline _RepoVector3D<T> operator/(const T &scalar) const
			{
				if (scalar == 0)
				{
					throw std::invalid_argument("Division by zero");
				}
				return _RepoVector3D<T>(x / scalar, y / scalar, z / scalar);
			}

			inline _RepoVector3D<T> operator/(const _RepoVector3D<T>& other) const
			{
				return _RepoVector3D<T>(x / other.x, y / other.y, z / other.z);
			}

			inline _RepoVector3D<T> operator-() const
			{
				return _RepoVector3D<T>(-x, -y, -z);
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
				return { x, y, z };
			}

			inline operator std::vector<T>() const
			{
				return toStdVector();
			}

			T x, y, z;
		};
	}
}
