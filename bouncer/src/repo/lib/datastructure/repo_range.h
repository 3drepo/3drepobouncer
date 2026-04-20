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

#include "repo/repo_bouncer_global.h"
#include "repo_vector2d.h"

namespace repo {
	namespace lib {
		template<typename T>
		class REPO_API_EXPORT _RepoRange : public repo::lib::_RepoVector2D<T>
		{
			public:
			_RepoRange(const T& min = 0, const T& max = 0) 
				:repo::lib::_RepoVector2D<T>(min, max) 
			{
				if (min > max) {
					std::swap(this->x, this->y);
				}
			}

			_RepoRange(const T& v)
				:repo::lib::_RepoVector2D<T>(v, v)
			{
			}

			_RepoRange(const std::initializer_list<T>& positions)
				:repo::lib::_RepoVector2D<T>(0, 0)
			{
				if (positions.size()) {
					this->x = *positions.begin();
					this->y = *positions.begin();
					for (auto p : positions) {
						this->x = std::min(this->x, p);
						this->y = std::max(this->y, p);
					}
				}
			}

			T& min() { return this->x; }
			const T& min() const { return this->x; }

			T& max() { return this->y; }
			const T& max() const { return this->y; }

			inline _RepoRange<T>& operator=(const _RepoRange<T> &other)
			{
				this->x = other.x;
				this->y = other.y;
				return *this;
			}

			inline _RepoRange<T>& operator=(const T& value)
			{
				this->x = value;
				this->y = value;
				return *this;
			}

			inline bool operator==(const _RepoRange<T>& other)
			{
				return this->x == other.x && this->y == other.y;
			}

			inline _RepoRange<T> operator*(const T& scalar)
			{
				return { this->x * scalar, this->y * scalar };
			}

			inline _RepoRange<T> operator+(const T& scalar)
			{
				return { this->x + scalar, this->y + scalar };
			}

			std::string toString() const
			{
				std::stringstream sstr;
				sstr << "[" << this->x << ", " << this->y << "]";
				return sstr.str();
			}
			
			std::vector<T> toStdVector() const
			{
				return{ this->x, this->y };
			}
			
			T length() const
			{
				return this->y - this->x;
			}

			bool contains(const T& value) const
			{
				return value >= this->x && value <= this->y;
			}

			bool overlaps(const _RepoRange<T>& other) const
			{
				return this->x <= other.y && other.x <= this->y;
			}
		};

		using RepoRange = _RepoRange<double>;
	}
}